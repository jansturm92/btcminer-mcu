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
import random
import struct
import time
from abc import ABC, abstractmethod

import serial
from serial import SerialException


class MiningDevice(ABC):
    """Device interface with abstract methods that must be overridden by the concrete mining device classes"""

    def __init__(
        self, device_type: str, name: str, read_timeout: float, write_timeout: float
    ):
        self.type = device_type
        self.name = name
        self.read_timeout = read_timeout
        self.write_timeout = write_timeout
        self.has_midstate_support = True

    def __str__(self):
        return f"<Device '{self.name}' [type={self.type}]>"

    @abstractmethod
    def connect(self) -> None:
        """
        Establishes a connection to the device. If the device is already connected, this function has no effect.
        :raises DeviceConnectionError
        """
        pass

    @abstractmethod
    def disconnect(self) -> None:
        """Close the connection to the device"""
        pass

    @abstractmethod
    def is_connected(self) -> bool:
        """Checks if device is connected"""
        pass

    @abstractmethod
    def write(self, data: bytes) -> int:
        """
        Sends data of type 'bytes' to the device
        :param data: bytes to be sent
        :raises DeviceConnectionError for any connection issue or if an optional write-timeout is exceeded.
        :return: number of bytes sent
        """
        pass

    @abstractmethod
    def read(self, size: int) -> bytes:
        """
        Reads size bytes from the device. May return fewer bytes than requested if the read_timeout is exceeded.
        :param size:
        :raises DeviceConnectionError for any connection issue
        :return: bytes read from the device
        """
        pass


class DeviceConnectionError(Exception):
    def __init__(self, device: MiningDevice, detail: str):
        self.device = device
        self.detail = detail
        super().__init__(detail)

    def __str__(self):
        return f"Cannot connect to {repr(self.device)}"


class SerialMiningDevice(MiningDevice):
    """
    Mining device that is connected to a serial port.

    :param name: an arbitrary name for the device (shown in logs)
    :param port: the serial port (e.g. /dev/tty* on GNU/Linux)
    :param baudrate: baud rate (e.g. 9600, 115200,...)
    :param read_timeout: read timeout in seconds, i.e. the time available to find a valid share
    :param write_timeout: write timeout in seconds, i.e. the time available to send work to the device
    """

    def __init__(
        self,
        name: str,
        port: str,
        baudrate: int,
        read_timeout: float,
        write_timeout: float,
    ):
        super().__init__("serial", name, read_timeout, write_timeout)
        self.serial = None
        self.port = port
        self.baudrate = baudrate

    def __repr__(self):
        return (
            f"<Device '{self.name}' [type={self.type}, port={self.port}, baudrate={self.baudrate}"
            f", read_timeout={self.read_timeout}s, write_timeout={self.write_timeout}s]>"
        )

    def connect(self) -> None:
        try:
            if self.serial is None:
                self.serial = serial.Serial(
                    port=self.port,
                    baudrate=self.baudrate,
                    timeout=self.read_timeout,
                    write_timeout=self.write_timeout,
                )
            elif not self.serial.isOpen():
                self.serial.open()

        except SerialException as e:
            raise DeviceConnectionError(self, e.strerror)

    def disconnect(self) -> None:
        if self.serial is not None:
            self.serial.close()

    def is_connected(self) -> bool:
        return self.serial and self.serial.isOpen()

    def write(self, data: bytes) -> int:
        if self.serial is None:
            raise DeviceConnectionError(
                self, "device is not connected, call connect() first"
            )
        try:
            return self.serial.write(data)
        except SerialException as e:
            raise DeviceConnectionError(self, e.strerror)

    def read(self, size: int):
        if self.serial is None:
            raise DeviceConnectionError(
                self, "device is not connected, call connect() first"
            )
        try:
            return self.serial.read(size)
        except SerialException as e:
            raise DeviceConnectionError(self, e.strerror)


class SimulatorMiningDevice(MiningDevice):
    def __init__(self, name: str, avg_delay: float, timeout: float):
        super().__init__("simulator", name, timeout, timeout)
        self.avg_delay = avg_delay
        self.data = b""
        self.has_midstate_support = False

    def __repr__(self):
        return (
            f"<Device '{self.name}' [type={self.type}, avg_delay={self.avg_delay}s"
            f", read_timeout={self.read_timeout}s, write_timeout={self.write_timeout}s]>"
        )

    @staticmethod
    def _scanhash(header: bytes) -> int:
        target_hash = bytes.fromhex("00" * 2 + "FF" * 30)
        nonce = int(header[76:][::-1].hex(), 16)
        while nonce < 0xFFFFFFFF:
            # Update the block header with the new 32-bit nonce
            header = header[0:76] + nonce.to_bytes(4, byteorder="little")
            header_hash = hashlib.sha256(hashlib.sha256(header).digest()).digest()[::-1]
            if header_hash <= target_hash:
                return nonce
            nonce += 1
        return nonce

    def connect(self) -> None:
        pass

    def disconnect(self) -> None:
        pass

    def is_connected(self) -> bool:
        return True

    def write(self, data):
        self.data = data

    def read(self, size):
        # add up to +-2 seconds of delay for some randomness
        delay = self.avg_delay + (random.uniform(-2, 2) if self.avg_delay > 2 else 0)
        if self.read_timeout < delay:
            time.sleep(self.read_timeout)
            return b""

        time.sleep(delay)
        return struct.pack(">L", self._scanhash(self.data))


def create_device(config_device: dict) -> MiningDevice:
    if config_device["type"] == "serial":
        return SerialMiningDevice(
            name="STM32F4DISCOVERY",  # TODO read from USB descriptor
            port=config_device["port"],
            baudrate=config_device["baudrate"],
            read_timeout=config_device["read_timeout"],
            write_timeout=config_device["write_timeout"],
        )

    return SimulatorMiningDevice(
        name="STM32 Simulator",
        timeout=config_device["timeout"],
        avg_delay=config_device["avg_delay"],
    )
