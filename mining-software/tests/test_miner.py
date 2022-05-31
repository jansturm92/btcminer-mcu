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
import asyncio
import copy
import json
import unittest
from pathlib import Path

import toml

from config_loader import miner_config
from miner import BlockTemplate, Miner
from sha256d_ms import calculate_midstate


def bits_to_target(bits: bytes) -> str:
    """
    Converts the compact target format (32 bit) into the target hash threshold (256 bit)

    for a detailed explanation see https://developer.bitcoin.org/reference/block_chain.html#target-nbits
    :param bits: compressed target
    :return: string hex representation of target hash
    """
    if not bits:
        return "0" * 64
    coeff = int.from_bytes(b"\x00" + bits[1:], byteorder="big")
    return hex(coeff * 256 ** (bits[0] - 3))[2:].zfill(64)


def template_from_block(block: dict, cb_config: dict) -> BlockTemplate:
    """
    Converts a given (mined) block , e.g. from 'bitcoin-cli getblock', into a block template
    similar to the result of getblocktemplate.

    :param block: a json object with valid proof of work obtained from 'bitcoin-cli getblock'
    :param cb_config: extra information about the corresponding coinbase transaction that is
                      not directly extractable from the given block (i.e. message, address, value)
    :return: BlockTemplate object that corresponds to the given block
    """
    template = dict(
        version=block["version"],
        previousblockhash=block["previousblockhash"],
        transactions=[dict(data=None, txid=tx) for tx in block["tx"][1:]],
        height=block["height"],
        bits=block["bits"],
        curtime=block["time"],
        target=bits_to_target(bytes.fromhex(block["bits"])),
        coinbasevalue=cb_config["value"],
    )
    if "witness_commitment" in cb_config:
        template["default_witness_commitment"] = cb_config["witness_commitment"]
    return BlockTemplate(template=template, cb_config=cb_config)


class TestMiner(unittest.TestCase):
    data = []
    test_config = {}

    @classmethod
    def setUpClass(cls):
        """
        Merges test config with base config, and preloads block and template data
        """
        cls.test_config = {
            **miner_config,
            **toml.load(Path(__file__).with_name("test_config.toml")),
        }
        for block_conf in cls.test_config["blocks"]:
            with open(Path(__file__).with_name(block_conf["file"])) as f:
                block = json.load(f)
            cls.data.append(
                dict(
                    block=block,
                    template=template_from_block(block, block_conf["coinbase"]),
                )
            )

    def setUp(self):
        print("")

    def test_mining_valid_share(self):
        miner = Miner(config=self.test_config)
        for test in self.data:
            with self.subTest(msg=f"BTC Block #{test['block']['height']}"):
                nonce_expected = test["block"]["nonce"]
                # only a few nonce iterations until correct nonce is found
                nonce = asyncio.run(
                    miner.mine(test["template"], hex(nonce_expected - 100))
                )
                self.assertIsNotNone(nonce)
                self.assertEqual(nonce_expected, int(nonce, 16))

    def test_mining_timeout(self):
        timeout_config = copy.deepcopy(self.test_config)
        # force instant timeout
        timeout_config["timeout"] = 0.001

        miner = Miner(config=timeout_config)
        for test in self.data:
            with self.subTest(msg=f"BTC Block #{test['block']['height']}"):
                nonce_expected = test["block"]["nonce"]
                # timeout should hit instantly, no nonce is returned
                nonce = asyncio.run(
                    miner.mine(test["template"], hex(nonce_expected - 100))
                )
                self.assertIsNone(nonce)

    def test_merkle_root(self):
        for test in self.data:
            with self.subTest(msg=f"BTC Block #{test['block']['height']}"):
                self.assertEqual(
                    bytes.fromhex(test["block"]["merkleroot"]),
                    test["template"].merkle_root,
                )


class TestSha256(unittest.TestCase):
    def setUp(self):
        print("")
        self.header_22222 = (
            "02000000426f46ed1c52cf2fff79f2812628701d2a1a7817f4aa89a50402000000000000"
            "eaedc86055f8961836c6b72dcce28ca55587998c9f16bb05c8a803b36316b914e2212551"
            "5c98041ab8686264"
        )
        self.midstate_22222 = (
            "167ff5ad63ab786ce8fcb09136fff458ea016749b643beff9b0f750b51975651"
        )

    def test_midstate(self):
        midstate = calculate_midstate(bytes.fromhex(self.header_22222))
        self.assertEqual(bytes.fromhex(self.midstate_22222), midstate)


if __name__ == "__main__":
    runner = unittest.TextTestRunner(verbosity=2)
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestMiner))
    suite.addTest(unittest.makeSuite(TestSha256))
    result = runner.run(suite)
