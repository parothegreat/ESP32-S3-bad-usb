#ifndef USB_KEYBOARD_HPP
#define USB_KEYBOARD_HPP

// Wrapper header for EspTinyUSB keyboard functionality
// This provides compatibility with the USB keyboard classes from the EspTinyUSB library

// Include the main EspTinyUSB header
#include <EspTinyUSB.h>

// Alias for commonly used HID keyboard constants and classes
using USBKeyboard = esptinyusb::USBkeyboard;

#endif // USB_KEYBOARD_HPP
