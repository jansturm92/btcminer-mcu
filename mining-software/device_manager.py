import asyncio
import uuid

import serial.tools.list_ports

from custom_logger import logger
from mining_device import MiningDevice, SerialMiningDevice, SimulatorMiningDevice


class DeviceManager:
    """
    Manages all configured mining devices and provides auto-detection of serial mining devices.

    :param config: miner config (see also config.toml)
    """

    def __init__(self, config: dict):
        self.config = config
        self.__devices: list[MiningDevice] = []
        self.__add_devices_config()
        if config.get("autodetect", False):
            self.__autodetect_serial_devices()
        if not self.__devices:
            raise Exception(
                "No mining devices configured or found. Please specify at least one mining device and/or use auto-detection"
            )

    def devices(self) -> list[MiningDevice]:
        """Returns a list of all managed mining devices."""
        return self.__devices

    def serial_devices(self) -> list[SerialMiningDevice]:
        """Returns a list of all managed serial mining devices"""
        return [d for d in self.__devices if isinstance(d, SerialMiningDevice)]

    @staticmethod
    def __get_serial_device_name(config_device: dict) -> str:
        """
        Returns either the given name in config, the name from the USB device descriptor or a random name.

        :param config_device: device configuration (see [[devices]] in config.toml)
        :return: name of the device
        """
        if "name" in config_device:
            return config_device["name"]

        for port, desc, _ in sorted(serial.tools.list_ports.comports()):
            if port == config_device["port"]:
                return desc
        return f"Serial_{uuid.uuid4()}"

    def create_device(self, config_device: dict) -> MiningDevice:
        """
        Creates a specific MiningDevice instance for the given device config.

        :param config_device: device configuration (see [[devices]] in config.toml)
        :return: MiningDevice instance
        """
        try:
            device_type = config_device["type"]
            if device_type == "serial":
                return SerialMiningDevice(
                    name=self.__get_serial_device_name(config_device),
                    port=config_device["port"],
                    baudrate=config_device["baudrate"],
                    write_timeout=config_device.get("write_timeout", 3),
                )

            if device_type == "simulator":
                return SimulatorMiningDevice(
                    name=config_device.get("name", f"Simulator_{uuid.uuid4()}"),
                    avg_delay=config_device.get("avg_delay", 5),
                )
            raise Exception(f"Unknown device type '{device_type}'")

        except KeyError as e:
            raise Exception(f"Please specify mandatory device parameter {e}")

    def __log_info(self, msg) -> None:
        logger.info(f"@{self.__class__.__name__}: {msg}")

    def __log_debug(self, msg) -> None:
        logger.debug(f"@{self.__class__.__name__}: {msg}")

    def __add_devices_config(self) -> None:
        """Adds all devices that are specified in the config under a separate [[devices]] section"""
        for dev_conf in self.config.get("devices", []):
            device = self.create_device(dev_conf)
            self.__log_info(f"Adding from config {repr(device)}")
            self.__devices.append(device)

    def __autodetect_serial_devices(self) -> None:
        """
        Probes all connected serial devices for mining capabilities.

        The following baudrates will be tested: 115200, 57600, 38400, 19200, 9600.
        If a mining device was discovered, it will be added iff a device with the same port was not already added.
        """
        self.__log_info("Auto-detecting serial mining devices ...")
        for port, desc, _ in sorted(serial.tools.list_ports.comports()):
            self.__log_info(f"\t[port={port}] Found device '{desc}' {_}")
            # check if already added as mining device
            if any(d.port == port for d in self.serial_devices()):
                self.__log_info(
                    f"\t[port={port}] '{desc}' already added as mining device"
                )
                continue

            # check if device has (midstate) mining capabilities
            self.__log_info(f"\t[port={port}] Testing device ...")
            for baudrate in [115200, 57600, 38400, 19200, 9600]:
                self.__log_debug(f"\t[port={port}]\tTrying baudrate {baudrate}")
                dut = self.create_device(
                    dict(
                        name=desc,
                        type="serial",
                        port=port,
                        baudrate=baudrate,
                        write_timeout=3,
                    )
                )
                success = asyncio.run(dut.is_mining_device())
                if success:
                    self.__log_info(f"\t==> Found {repr(dut)}")
                    self.__devices.append(dut)
                    break
            if not success:
                self.__log_info(f"\t[port={port}] Not a mining device")
