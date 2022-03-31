#  Copyright (C) 2022 Jan Sturm
#
#  This program is free software: you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software
#  Foundation, either version 3 of the License, or (at your option) any later
#  version.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along with
#  this program.  If not, see <http://www.gnu.org/licenses/>.

import hashlib
import json
import signal
import struct
import sys
import time
from typing import Optional

import bitcoinlib.encoding
from bitcoinlib.encoding import int_to_varbyteint
from bitcoinlib.keys import deserialize_address
from bitcoinlib.services.authproxy import AuthServiceProxy, JSONRPCException
from bitcoinlib.transactions import Input, Output, Transaction
from bitcoinlib.values import Value

import mining_device
from config_loader import miner_config
from custom_logger import logger


class MinerError(Exception):
    pass


def sha256d(data: bytes) -> bytes:
    """
    Computes the double SHA256 for given data, i.e. SHA256(SHA256(data)).
    :param data: arbitrary length data
    :return: 32 byte double SHA256 hash
    """
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()


class BlockTemplate:
    """
    Representation of a block template received from getblocktemplate RPC.
    See also https://github.com/bitcoin/bips/blob/master/bip-0022.mediawiki

    :param template: result of getblocktemplate RPC
    :param cb_config: user specific coinbase data (message and address)
    """

    def __init__(self, template: dict, cb_config: dict):

        self.template = template
        self.cb_config = cb_config

        # for very low hashrates we can precompute coinbase transaction and Merkle root since we don't need extra-nonce
        self.add_coinbase_tx()
        self.merkle_root = self.merkle_root()

    def __str__(self):
        return f"BlockTemplate for block #{self.template['height']}:\n{json.dumps(self.template, indent=4)}"

    def add_coinbase_tx(self) -> None:
        """
        Creates the coinbase transaction and adds it as the first transaction to the template.

        If at least one witness transaction exists in the block, the commitment needs to be inserted as an additional
        output at the end of this coinbase transaction (see https://bips.xyz/145#block-assembly-with-witness-transactions).
        getblocktemplate already provides the commitment as 'default_witness_commitment'.

        :raises MinerError if coinbase address is invalid
        """
        # see BIP 34 (https://en.bitcoin.it/wiki/BIP_0034) for height of block in coinbase scriptSig
        block_height = self.template["height"]
        height_width = (block_height.bit_length() + 7) // 8
        script = bytes([height_width]) + block_height.to_bytes(
            height_width, byteorder="little"
        )

        msg = self.cb_config["message"]
        if msg.startswith("hex:"):
            script += bytes.fromhex(msg[4:])
        else:
            script += msg[4 * msg.startswith("str:") :].encode()

        try:
            cb_network = deserialize_address(self.cb_config["address"])["network"]
            if not cb_network:
                raise MinerError("Invalid coinbase address")
        except bitcoinlib.encoding.EncodingError as e:
            raise MinerError(e)

        cb_input = Input(
            prev_txid="00" * 32,
            output_n=0xFFFFFFFF,
            unlocking_script=script,
            network=cb_network,
        )
        cb_outputs = [
            Output(
                value=self.template["coinbasevalue"],
                address=self.cb_config["address"],
                network=cb_network,
            )
        ]

        if "default_witness_commitment" in self.template:
            cb_outputs.append(
                Output(
                    value=0,
                    lock_script=self.template["default_witness_commitment"],
                    network=cb_network,
                )
            )

        tx_coinbase = Transaction(
            coinbase=True,
            inputs=[cb_input],
            outputs=cb_outputs,
            network=cb_network,
        )
        logger.debug(f"Coinbase:\n{tx_coinbase.as_json()}")
        self.template["transactions"].insert(
            0,
            dict(
                data=tx_coinbase.raw().hex(),
                # segwit requires a commitment to the 'wtxid' ('hash' in getblocktemplate result).
                # The wtxid of the coinbase tx is 0x0000..0000
                hash="00" * 32,
                txid=sha256d(tx_coinbase.raw())[::-1].hex(),
            ),
        )

    def merkle_root(self) -> bytes:
        """
        Computes the Merkle root of the block (hash of all transaction IDs).

        For segwit each transaction has 2 IDs, txid and wtxid.
        txid  remains the double SHA256 hash of the traditional serialization format
        wtxid (also named 'hash' in getblocktemplate transactions) is the double SHA256 hash
              of the new serialization with witness data.
        See https://bips.xyz/141#transaction-id for more information

        :return: Merkle root in little-endian
        """

        # convert all txid to big-endian bytes
        hashes = [
            bytes.fromhex(tx["txid"])[::-1] for tx in self.template["transactions"]
        ]
        while len(hashes) > 1:
            # Duplicate last hash (will be ignored for even number of hashes)
            hashes.append(hashes[-1])
            hashes = [
                sha256d(left + right) for left, right in zip(*(iter(hashes),) * 2)
            ]
        logger.debug(f"merkle root (big endian) = {hashes[0].hex()}")
        return hashes[0][::-1]

    def block_info(self, nonce: Optional[str] = None) -> str:
        """Returns a printable block info string"""
        if not nonce:
            return f"<Block [height={self.template['height']}]>"
        else:
            return (
                f"<Block [height={self.template['height']}, "
                f"header_hash={self.block_header_hash(nonce).hex()}]"
            )

    def reward_info(self) -> str:
        """Returns a printable reward string"""
        return (
            f"<Reward 'Block#{self.template['height']}' "
            f"[reward={Value(self.template['coinbasevalue'], 'sat').str('auto')}, "
            f"coinbase_address={self.cb_config['address']}, "
            f"txid={self.template['transactions'][0]['txid']}]>"
        )

    def block_header(self, nonce: str) -> bytes:
        """
        Assembles the 80 byte block header in the following format:
        [version (4B) | previous_block_hash (32B) | merkle_root (32B) | timestamp (4B) | bits (4B) | nonce (4B)].

        :param nonce: nonce in hex format (big endian)
        :return: 80 byte block header in little endian
        """
        header = struct.pack("<L", self.template["version"])
        header += bytes.fromhex(self.template["previousblockhash"])[::-1]
        header += self.merkle_root[::-1]
        header += struct.pack("<L", self.template["curtime"])
        header += bytes.fromhex(self.template["bits"])[::-1]
        header += struct.pack("<L", int(nonce, 16) if nonce else 0)
        return header

    def block_header_hash(self, nonce: str) -> bytes:
        """
        Calculates the double SHA256 hash of the block header

        :param nonce: nonce in hex format (big endian)
        :return: 32 byte block header hash in big endian
        """
        return sha256d(self.block_header(nonce))[::-1]

    def target_hash(self) -> bytes:
        """
        Extracts the target hash from block template
        :return: 32 byte target hash in big endian
        """
        return bytes.fromhex(self.template["target"])

    def create_block(self, nonce: str) -> str:
        """
        Creates a block with valid proof of work for submitblock RPC.

        The block contains the block header, transaction counter (VarInt) and the transactions.

        :param nonce: nonce received from miner in hex format (big endian)
        :return: the hex-encoded block data to submit
        """
        block = self.block_header(nonce).hex()
        block += int_to_varbyteint(len(self.template["transactions"])).hex()
        block += "".join(tx["data"] for tx in self.template["transactions"])
        return block


class Miner:
    """
    Class that manages the mining process and handles the communication between the RPC server and the mining device.

    :param config: miner config (see also config.toml)
    """

    def __init__(self, config: dict):
        self.config = config
        self.device = mining_device.create_device(config["device"])
        self.rpc = config["rpc"]
        self.rpc_url = (
            f"http://{self.rpc['username']}:{self.rpc['password']}@{self.rpc['server']}"
        )

    def mine(
        self, block_template: BlockTemplate, nonce_start: Optional[str] = None
    ) -> Optional[str]:
        """
        Starts the mining process on the mining device.

        After reception of a share the block hash needs to be checked against the actual target hash of the block,
        to determine if it is a valid proof of work for the block.
        Make sure a connection to the device was established (see self.connect_device()).

        :param block_template: the block template for the current block to be mined
        :param nonce_start: optional nonce for the mining device to start iterating from (in big endian hex format)
        :raises DeviceConnectionError if there are connection issues with the mining device
        :raises MinerError for some critical error during mining
        :return: nonce if a valid proof of work was found for this block
                 None if the share is invalid for this block (block hash > target hash)
                      if a device timeout was triggered (indicates no valid share was found by the mining device)
        """
        target_hash = block_template.target_hash()
        block_header = block_template.block_header(nonce_start)

        logger.info(f"sending work for {block_template.block_info()} to {self.device}")
        logger.debug(f"\tblock header = {block_header.hex()}")
        self.device.write(block_header)

        logger.info(f"waiting for response from {self.device}")
        response = self.device.read(size=4)
        if len(response):
            nonce = response[::-1].hex()
            logger.info(f"\treceived share (nonce = 0x{nonce}) from {self.device}")

            block_hash = block_template.block_header_hash(nonce)
            logger.info(
                f"\t{block_template.block_info()} block_hash  = {block_hash.hex()}"
            )
            logger.info(
                f"\t{block_template.block_info()} target_hash = {target_hash.hex()}"
            )
            if block_hash <= target_hash:
                logger.info(
                    f"\x1b[33;1m>>> Found a valid hash for {block_template.block_info()}\x1b[0m"
                )
                return nonce

            logger.info("Share invalid (block_hash > target_hash)")
        else:
            logger.info(f"\tresponse timeout")
        return None

    def get_block_template(self) -> BlockTemplate:
        """
        Calls the getblocktemplate JSON-RPC method and instantiates a BlockTemplate object.

        This function will block forever until a successful connection to the RPC server can be established.

        :raises MinerError for some critical error during initialization of BlockTemplate
        :return: a new BlockTemplate object
        """
        while True:
            try:
                rpc = AuthServiceProxy(service_url=self.rpc_url)
                logger.info(f"RPC<{self.rpc['server']}> getblocktemplate()")
                block_template = BlockTemplate(
                    template=rpc.getblocktemplate({"rules": ["segwit"]}),
                    cb_config=self.config["coinbase"],
                )
                logger.debug(block_template)
                return block_template
            except (JSONRPCException, ConnectionError) as e:
                logger.error(f"Cannot connect to {self.rpc['server']}")
                logger.debug(f"\t{e}")
                logger.info("\tTrying again in 5 seconds ...")
                time.sleep(5)
            except OSError as e:
                raise MinerError(e)

    def submit_block(self, block: str) -> bool:
        f"""
        Calls the submitblock JSON-RPC method to submit the newly created block with valid proof of work.

        For connection errors to the server, there will be 10 attempts to submit the data with 5 seconds in between.
        The block will be discarded if all attempts are unsuccessful.

        :param block: the hex-encoded block data to submit
        :return: True if block was accepted by server,
                 False if block was rejected by server or after connection timeout
        """
        attempts = 10
        while True:
            try:
                logger.info(f"RPC<{self.rpc['server']}> submitblock()")
                rpc = AuthServiceProxy(service_url=self.rpc_url)
                response = rpc.submitblock(block)
                if response:
                    logger.error(f"\tRPC response: {response}")
                    return False
                return True
            except ConnectionError as e:
                attempts -= 1
                logger.error(f"Cannot connect to {self.rpc['server']}")
                logger.debug(f"\t{e}")
                if attempts == 0:
                    return False
                logger.info(
                    f"\tTrying again in 5 seconds ({attempts} attempt{'s' if attempts > 1 else ''} left) ..."
                )
                time.sleep(5)

    def connect_device(self) -> None:
        if not self.device.is_connected():
            logger.info(f"Connecting to device {repr(self.device)}")
            self.device.connect()

    def disconnect_device(self) -> None:
        logger.info(f"Disconnecting device {repr(self.device)}")
        self.device.disconnect()

    def start(self) -> None:
        """Starts the mining loop (getblocktemplate -> mining -> submitblock)"""
        logger.info("Starting Miner")
        logger.debug(f"Using config\n{json.dumps(miner_config, indent=4)}")
        while True:
            try:
                self.connect_device()
                block_template = self.get_block_template()
                nonce = self.mine(block_template)
                if nonce:
                    if self.submit_block(block_template.create_block(nonce)):
                        logger.info(
                            f"\x1b[33;1m>>> Successfully mined {block_template.block_info(nonce)}\x1b[0m"
                        )
                        logger.info(
                            f"\x1b[33;1m>>> {block_template.reward_info()}\x1b[0m"
                        )

            except mining_device.DeviceConnectionError as e:
                logger.error(e)
                logger.debug(f"\t{e.detail}")
                time.sleep(3)
            except MinerError as e:
                logger.critical(e)
                break

    def stop(self) -> None:
        """Gracefully stop the mining process."""
        self.disconnect_device()
        logger.info("Shutting down Miner")


def main():
    signal.signal(signal.SIGTERM, lambda x, y: sys.exit(0))
    signal.signal(signal.SIGINT, lambda x, y: sys.exit(0))

    miner = Miner(miner_config)
    try:
        miner.start()
    finally:
        miner.stop()


if __name__ == "__main__":
    main()
