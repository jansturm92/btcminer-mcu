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

"""Provides global logging object, with custom formatting and coloring"""

import logging

from config_loader import miner_config


class CustomFormatter(logging.Formatter):
    COLORS = {
        logging.DEBUG: "\x1b[37;20m",  # grey
        logging.INFO: "\x1b[32;20m",  # green
        logging.WARNING: "\x1b[33;20m",  # yellow
        logging.ERROR: "\x1b[31;20m",  # red
        logging.CRITICAL: "\x1b[31;1m",  # bold red
    }

    def format(self, record):
        formatter = logging.Formatter(
            f"{self.COLORS.get(record.levelno)}[%(asctime)s] [%(levelname)8s] --- %(message)s\x1b[0m"
        )
        return formatter.format(record)


level = miner_config["logging"]["level"]
logger = logging.getLogger()
logger.disabled = not miner_config["logging"]["enabled"]
logger.setLevel(level)

ch = logging.StreamHandler()
ch.setLevel(level)
ch.setFormatter(CustomFormatter())

logger.addHandler(ch)
