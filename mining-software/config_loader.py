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

"""Loads config file 'config.toml', merges it with CLI values and provides a global config object"""

import argparse
from pathlib import Path

import toml

miner_config = toml.load(Path(__file__).with_name("config.toml"))

parser = argparse.ArgumentParser(
    argument_default=argparse.SUPPRESS, usage="%(prog)s [options]"
)

# [rpc]
parser.add_argument(
    "-o, --server",
    metavar="<ip:port>",
    dest="rpc.server",
    help=f"ip address and port for bitcoin json-rpc server (default '{miner_config['rpc']['server']}')",
)
parser.add_argument(
    "-u, --user",
    metavar="<username>",
    dest="rpc.username",
    help=f"username for bitcoin json-rpc server (default '{miner_config['rpc']['username']}')",
)
parser.add_argument(
    "-p, --pass",
    metavar="<password>",
    dest="rpc.password",
    help=f"password for bitcoin json-rpc server (default '{miner_config['rpc']['password']}')",
)

# [coinbase]
parser.add_argument(
    "--coinbase-message",
    metavar="<[(hex|str):]message>",
    dest="coinbase.message",
    help=f"Set coinbase message. The prefix 'str:' is used for text, prefix 'hex:' for a hexadecimal encoded message. "
    f"Omitting the prefix defaults to 'str:'  (default '{miner_config['coinbase']['message']}')",
)
parser.add_argument(
    "--coinbase-addr",
    metavar="<addr>",
    dest="coinbase.address",
    help=f"Set coinbase payout address for solo mining (default '{miner_config['coinbase']['address']}')",
)

# [logging]
parser.add_argument(
    "-D, --debug",
    dest="logging.level",
    action="store_const",
    const="DEBUG",
    help=f"Enable debug output",
)
parser.add_argument(
    "-q, --quiet",
    dest="logging.enabled",
    action="store_false",
    help=f"Disable logging output",
)

# overwrite config file values with argparse values
for dest, value in vars(parser.parse_args()).items():
    group, key = dest.split(".")
    miner_config[group].update({key: value})
