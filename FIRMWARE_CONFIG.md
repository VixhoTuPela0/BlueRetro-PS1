# BlueRetro-PS1 Firmware Configuration

This firmware is optimised for **HW1 + PlayStation 1 / PlayStation 2** use.

## Hardware
- **Supported:** HW1 only.
- HW2 support has been removed from the build configuration.

## Console / System
- **Supported:** PSX / PS2 (`CONFIG_BLUERETRO_SYSTEM_PSX_PS2=y`).

## Supported Bluetooth Controllers
Only the following controller profiles are compiled in:

| Controller | Profile |
|---|---|
| Any Bluetooth HID gamepad | Generic HID |
| DualShock 4 (PS4) | PS (`Wireless Controller`) |
| DualSense (PS5) | PS (`DualSense Wireless Controller`) |

Other profiles (PS3, Wii, Switch, Switch 2) are **not compiled** and will not be recognised.

## Maximum Simultaneous Controllers
Up to **2 controllers** can be connected at the same time.

This is enforced at the Bluetooth controller level:
- `CONFIG_BTDM_CTRL_BLE_MAX_CONN=2`
- `CONFIG_BTDM_CTRL_BR_EDR_MAX_ACL_CONN=2`

## Build
Use the `configs/hw1/playstation` preset:

```bash
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;configs/hw1/playstation" build
```
