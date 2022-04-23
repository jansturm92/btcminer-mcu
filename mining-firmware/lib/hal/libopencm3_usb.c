// Original work Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
// Modified work Copyright (C) 2022 Jan Sturm
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

#ifdef IO_USB_CDC

#include "libopencm3_usb.h"
#include "libopencm3_util.h"
#include <string.h>

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x82,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    }};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
    .header =
        {
            .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
            .bcdCDC = 0x0110,
        },
    .call_mgmt =
        {
            .bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
            .bmCapabilities = 0,
            .bDataInterface = 1,
        },
    .acm =
        {
            .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
            .bDescriptorType = CS_INTERFACE,
            .bDescriptorSubtype = USB_CDC_TYPE_ACM,
            .bmCapabilities = 0,
        },
    .cdc_union = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1,
    }};

static const struct usb_interface_descriptor comm_iface[] = {
    {.bLength = USB_DT_INTERFACE_SIZE,
     .bDescriptorType = USB_DT_INTERFACE,
     .bInterfaceNumber = 0,
     .bAlternateSetting = 0,
     .bNumEndpoints = 1,
     .bInterfaceClass = USB_CLASS_CDC,
     .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
     .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
     .iInterface = 0,

     .endpoint = comm_endp,

     .extra = &cdcacm_functional_descriptors,
     .extralen = sizeof(cdcacm_functional_descriptors)}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
                                                  .num_altsetting = 1,
                                                  .altsetting = comm_iface,
                                              },
                                              {
                                                  .num_altsetting = 1,
                                                  .altsetting = data_iface,
                                              }};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 0x32,

    .interface = ifaces,
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes cdcacm_control_request(
    usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
    void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
    (void)complete;
    (void)buf;
    (void)usbd_dev;

    switch (req->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
        /*
         * This Linux cdc_acm driver requires this to be implemented
         * even though it's optional in the CDC spec, and we don't
         * advertise it in the ACM functional descriptor.
         */
        return USBD_REQ_HANDLED;
    }
    case USB_CDC_REQ_SET_LINE_CODING:
        if (*len < sizeof(struct usb_cdc_line_coding)) {
            return USBD_REQ_NOTSUPP;
        }

        return USBD_REQ_HANDLED;
    }
    return USBD_REQ_NOTSUPP;
}

uint8_t rx_buf[48];
/**
 * @brief RX callback for CDC device, that reads data from mining software.
 * @details Writes 48 bytes = [midstate (32B) | timestamp (4B) | bits (4B) | nonce (4B)]
 *          into #rx_buf buffer and executes post-processing callback.
 * @param usbd_dev the usb device handle returned from usbd_setup()
 * @param ep unused
 */
static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
    (void)ep;

    if (usbd_ep_read_packet(usbd_dev, 0x01, rx_buf, 48) != 48)
        return;
    board_read_data((const char *)rx_buf, sizeof(rx_buf));
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue) {
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_register_control_callback(usbd_dev, USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                   USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                   cdcacm_control_request);
}

char i_serial_number[32];
/// USB string descriptors
static const char *usb_strings[] = {
    BOARD_NAME,              // iManufacturer
    USB_DESCRIPTOR_IPRODUCT, // iProduct
    i_serial_number,         // iSerialNumber
};

static usbd_device *usbd_dev;

static const struct usb_device_descriptor dev_descriptor = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = USB_VENDOR_ID,
    .idProduct = USB_PRODUCT_ID,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/// USB interrupt handler
// cppcheck-suppress unusedFunction
void otg_fs_isr(void) { usbd_poll(usbd_dev); }

int board_send_data(const uint8_t *data, const int len) {
    while (usbd_ep_write_packet(usbd_dev, 0x82, data, len) == 0)
        ;
    return len;
}

/**
 * @brief Initializes a virtual serial device via USB.
 * @param serial_number[in] Pointer to serial number buffer
 * @param sn_len Length of serial number buffer in bytes
 */
void usbd_setup(const char *serial_number, size_t sn_len) {

    if (serial_number != NULL)
        memcpy(i_serial_number, serial_number, MIN(sizeof(i_serial_number), sn_len));
    usbd_dev = usbd_init(&otgfs_usb_driver, &dev_descriptor, &config, usb_strings, 3,
                         usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
}
#endif
