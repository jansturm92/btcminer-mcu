# Copyright (C) 2022 Jan Sturm
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.

.ONESHELL:

PYTHON = python3

BITCOIN_CORE_VERSION = 22.0
BITCOIN_CORE_ARCHIVE = bitcoin-${BITCOIN_CORE_VERSION}-x86_64-linux-gnu.tar.gz
BITCOIN_CORE_ARCHIVE_URL = https://bitcoincore.org/bin/bitcoin-core-${BITCOIN_CORE_VERSION}/$(BITCOIN_CORE_ARCHIVE)
BITCOIN_CORE_DIR = ./bitcoin-$(BITCOIN_CORE_VERSION)
BITCOIND_EXE = $(BITCOIN_CORE_DIR)/bin/bitcoind
BITCOIND_DATA_DIR = /tmp/bitcoind
BITCOIND_CONF_FILE = $(PWD)/bitcoin.conf

############################   BITCOIN-CORE   ############################

$(BITCOIN_CORE_ARCHIVE):
	wget $(BITCOIN_CORE_ARCHIVE_URL)

$(BITCOIN_CORE_DIR): | $(BITCOIN_CORE_ARCHIVE)
	tar -xzf $(BITCOIN_CORE_ARCHIVE)

$(BITCOIND_DATA_DIR):
	mkdir $(BITCOIND_DATA_DIR)
	ln -s $(BITCOIND_CONF_FILE) $(BITCOIND_DATA_DIR)/bitcoin.conf

run-regtest: | $(BITCOIN_CORE_DIR) $(BITCOIND_DATA_DIR)
	$(BITCOIND_EXE) -datadir=$(BITCOIND_DATA_DIR)

reset-regtest:
	$(RM) -R $(BITCOIND_DATA_DIR)

############################   MINING-SOFTWARE   ##########################

init-regtest:
	cd mining-software
	$(PYTHON) init_regtest.py

start-mining:
	$(PYTHON) mining-software/miner.py

test-mining:
	cd mining-software
	$(PYTHON) -m tests.test_miner -q

############################   MINING-FIRMWARE   ##########################

PIO_ENV ?= disco_f407vg
MONITOR_PORT ?= /dev/ttyUSB0

show-pio-envs:
	cd mining-firmware
	pio project config

flash-firmware:
	cd mining-firmware
	pio run -e $(PIO_ENV) -t upload

test-firmware:
	cd mining-firmware
	pio test -e $(PIO_ENV)

monitor-firmware:
	cd mining-firmware
	pio device monitor --port $(MONITOR_PORT)

check-firmware:
	cd mining-firmware
	pio check --fail-on-defect=medium --fail-on-defect=high
