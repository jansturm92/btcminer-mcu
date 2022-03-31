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

"""
Performs some initialization tasks on a running bitcoind regtest instance

- wallet creation
- generation of several random blocks
- generation of some transactions
- generation of address to be used for coinbase transaction
"""

from bitcoinlib.services.authproxy import AuthServiceProxy, JSONRPCException

from config_loader import miner_config
from custom_logger import logger

wallet_name = "stm32wallet"
initial_blocks = 1000
random_transactions = 5
tx_fee = 0.1

config = miner_config["rpc"]
url = f"http://{config['username']}:{config['password']}@{config['server']}"
rpc = AuthServiceProxy(service_url=url)
logger.info(f"Setting up regtest on {config['server']}")

try:
    try:
        rpc.createwallet(wallet_name)
        logger.info(f"  Created wallet '{wallet_name}'")
    except JSONRPCException as e:
        if "already exists" not in e.message:
            raise e

    try:
        rpc.loadwallet(wallet_name)
        logger.info(f"  Loaded wallet '{wallet_name}'")
    except JSONRPCException as e:
        if "already loaded" not in e.message:
            raise e

    rpc = AuthServiceProxy(service_url=f"{url}/wallet/{wallet_name}")
    logger.info(f"  Using wallet '{wallet_name}'")

    addr1 = rpc.getnewaddress()
    logger.info(f"  Generating {initial_blocks} blocks to address '{addr1}' ...")
    rpc.generatetoaddress(initial_blocks, addr1)
    logger.info(f"  ... done")

    logger.info(f"  Generating {random_transactions} transactions")
    logger.info(f"    > Setting transaction fee to {tx_fee} BTC/kB")
    rpc.settxfee(tx_fee)
    for i in range(random_transactions):
        addr = rpc.getnewaddress()
        rpc.sendtoaddress(addr, 1)
        logger.info(f"    > TX#{i}: Sent 1 BTC to '{addr}'")

    logger.info("")
    logger.info("@" * 90)
    logger.info(
        f"\x1b[31;1m==> Use address '{rpc.getnewaddress('mining-rewards', 'p2sh-segwit')}' as coinbase-addr for mining-software"
    )
    logger.info("@" * 90)

except JSONRPCException as e:
    logger.error(e)
except ConnectionError as e:
    logger.error(e)
    logger.error(
        f"Make sure an instance of bitcoind is running on '{config['server']}'"
    )
