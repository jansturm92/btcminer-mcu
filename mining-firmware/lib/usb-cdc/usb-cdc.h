// Copyright (C) 2022 Jan Sturm
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MINING_FIRMWARE_LIB_USB_CDC_H
#define MINING_FIRMWARE_LIB_USB_CDC_H

// clang-format off
#include <stdint.h> // include first because of missing stdint.h header in <libopencm3/usb/cdc.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbd.h>
// clang-format on

usbd_device *usbd_setup(const struct usb_device_descriptor *dev,
                        usbd_endpoint_callback rx_cb, const char *const *usb_strings);

#endif // MINING_FIRMWARE_LIB_USB_CDC_H
