#ifndef USB_KEYBOARD_HPP
#define USB_KEYBOARD_HPP

// Wrapper header for EspTinyUSB keyboard functionality
// This provides compatibility with the USB keyboard classes from the EspTinyUSB library

// Include the HID keyboard header from EspTinyUSB
#include <hidkeyboard.h>

// Alias for convenience
using USBKeyboard = HIDkeyboard;

#endif // USB_KEYBOARD_HPP
