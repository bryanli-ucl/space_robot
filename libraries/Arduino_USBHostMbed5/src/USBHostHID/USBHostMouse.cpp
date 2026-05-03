/* mbed USBHost Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "USBHostMouse.h"

#if USBHOST_MOUSE

USBHostMouse::USBHostMouse()
{
    init();
}

void USBHostMouse::init()
{
    dev = NULL;
    int_in = NULL;
    onUpdate = NULL;
    onButtonUpdate = NULL;
    onXUpdate = NULL;
    onYUpdate = NULL;
    onZUpdate = NULL;
    report_id = 0;
    dev_connected = false;
    mouse_device_found = false;
    mouse_intf = -1;

    buttons = 0;
    x = 0;
    y = 0;
    z = 0;
}

bool USBHostMouse::connected()
{
    return dev_connected;
}

bool USBHostMouse::connect()
{
    int len_listen;

    if (dev_connected) {
        return true;
    }
    host = USBHost::getHostInst();

    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {

            if(host->enumerate(dev, this)) {
                break;
            }
            if (mouse_device_found) {
                {
                    /* As this is done in a specific thread
                     * this lock is taken to avoid to process the device
                     * disconnect in usb process during the device registering */
                    USBHost::Lock  Lock(host);
                    int_in = dev->getEndpoint(mouse_intf, INTERRUPT_ENDPOINT, IN);
                    if (!int_in) {
                        break;
                    }

                    printf("[Mouse] New device: VID:%04x PID:%04x [dev: %p - intf: %d - ep_size: %d]\n",
                           dev->getVid(), dev->getPid(), dev, mouse_intf, int_in->getSize());
                    dev->setName("Mouse", mouse_intf);
                    host->registerDriver(dev, mouse_intf, this, &USBHostMouse::init);

                    int_in->attach(this, &USBHostMouse::rxHandler);
                    len_listen = int_in->getSize();
                    if (len_listen > sizeof(report)) {
                        len_listen = sizeof(report);
                    }
                }
                int ret=host->interruptRead(dev, int_in, report, len_listen, false);
                MBED_ASSERT((ret==USB_TYPE_OK) || (ret ==USB_TYPE_PROCESSING) || (ret == USB_TYPE_FREE));
                if ((ret==USB_TYPE_OK) || (ret ==USB_TYPE_PROCESSING)) {
                    dev_connected = true;
                }
                if (ret == USB_TYPE_FREE) {
                    dev_connected = false;
                }
                return true;
            }
        }
    }
    init();
    return false;
}

void USBHostMouse::rxHandler()
{
    int len = int_in->getLengthTransferred();
    if (len == 0) {
        // resubmit
        int sz = int_in->getSize();
        if (sz > sizeof(report)) sz = sizeof(report);
        if (dev) host->interruptRead(dev, int_in, report, sz, false);
        return;
    }

    uint8_t new_buttons = buttons;
    int16_t new_x = x;
    int16_t new_y = y;
    int16_t new_z = z;

    if (len == 7) {
        // 2.4G wireless receiver: buttons + int16 X + int16 Y + int16 Wheel
        new_buttons = report[0] & 0x07;
        new_x = (int16_t)(report[1] | (report[2] << 8));
        new_y = (int16_t)(report[3] | (report[4] << 8));
        new_z = (int16_t)(report[5] | (report[6] << 8));
    } else if (len >= 8) {
        // Wired mouse with int16: buttons + padding + int16 X + int16 Y + int16 Wheel
        new_buttons = report[0] & 0x07;
        new_x = (int16_t)(report[2] | (report[3] << 8));
        new_y = (int16_t)(report[4] | (report[5] << 8));
        new_z = (int16_t)(report[6] | (report[7] << 8));
    } else if (len >= 4) {
        // Standard boot mouse (int8)
        new_buttons = report[0] & 0x07;
        new_x = (int8_t)report[1];
        new_y = (int8_t)report[2];
        new_z = (int8_t)report[3];
    }

    if (onUpdate) {
        (*onUpdate)(new_buttons, (int8_t)new_x, (int8_t)new_y, (int8_t)new_z);
    }

    if (onButtonUpdate && (buttons != new_buttons)) {
        (*onButtonUpdate)(new_buttons);
    }

    if (onXUpdate && (x != (int8_t)new_x)) {
        (*onXUpdate)((int8_t)new_x);
    }

    if (onYUpdate && (y != (int8_t)new_y)) {
        (*onYUpdate)((int8_t)new_y);
    }

    if (onZUpdate && (z != (int8_t)new_z)) {
        (*onZUpdate)((int8_t)new_z);
    }

    // Debug: print parsed values every ~200 ms to avoid flooding
    static uint32_t last_dbg = 0;
    uint32_t now = millis();
    if (now - last_dbg > 200) {
        printf("[Mouse] btn=%d x=%d y=%d z=%d\n", new_buttons, (int)new_x, (int)new_y, (int)new_z);
        last_dbg = now;
    }

    buttons = new_buttons;
    x = (int8_t)new_x;
    y = (int8_t)new_y;
    z = (int8_t)new_z;

    // resubmit
    int sz = int_in->getSize();
    if (sz > sizeof(report)) sz = sizeof(report);
    if (dev) host->interruptRead(dev, int_in, report, sz, false);
}

/*virtual*/ void USBHostMouse::setVidPid(uint16_t vid, uint16_t pid)
{
    // we don't check VID/PID for mouse driver
}

/*virtual*/ bool USBHostMouse::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
    if ((mouse_intf == -1) &&
            (intf_class == HID_CLASS) &&
            (intf_subclass == 0x01) &&
            (intf_protocol == 0x02)) {
        mouse_intf = intf_nb;
        return true;
    }
    return false;
}

/*virtual*/ bool USBHostMouse::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
    if (intf_nb == mouse_intf) {
        if (type == INTERRUPT_ENDPOINT && dir == IN) {
            mouse_device_found = true;
            return true;
        }
    }
    return false;
}

#endif
