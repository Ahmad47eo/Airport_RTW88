**Native-style AirPort Wi-Fi support for selected Realtek RTL88xx PCIe adapters on macOS.**

AirPort_RTW88 brings Realtek wireless hardware into macOS through the native AirPort/IO80211 networking architecture. The goal is to make compatible adapters usable through the system's Wi-Fi interface rather than presenting them as conventional Ethernet devices or requiring a separate application to manage connections.

Part of the [Realtek AirPort Family](https://github.com/xnoah222/Realtek-Airport-Family).

> **Realtek Wi-Fi. From Ventura to Tahoe.**

## Overview

AirPort_RTW88 is a macOS kernel-extension project based on the Linux `rtw88` wireless driver and related macOS driver-development work.

The project focuses on selected Realtek PCIe adapters and their integration with macOS's native wireless networking interface. Compatibility depends on the adapter, macOS version, and required networking components.

AirPort_RTW88 is independent of Realtek's official driver software. It is intended for compatible macOS installations, including Intel-based Hackintosh systems using OpenCore.

## Compatibility

### Supported macOS versions

| macOS version | Compatibility |
|---|---|
| Ventura 13.7.7–13.7.8 | Supported |
| Sonoma 14.4 and later | Supported |
| Sequoia 15.x | Supported |
| Tahoe 26.x | Supported |
| Monterey 12.x | Not supported |

Support for a macOS release does not guarantee that every compatible chipset has been tested on that release.

### Supported hardware

The project supports the following Realtek PCIe wireless adapters:

| Chipset | Interface | Status |
|---|---|---|
| RTL8822BE | PCIe | Supported |
| RTL8822CE | PCIe | Supported |
| RTL8821CE | PCIe | Supported |

**USB and SDIO variants are not supported.** Other Realtek chipsets should not be considered compatible unless they are explicitly added and tested.

Bluetooth functionality on combo wireless cards is separate from this Wi-Fi driver and is not provided by AirPort_RTW88.

## Features

AirPort_RTW88 is designed to provide native-style wireless networking through macOS.

The project's functionality includes:

- Integration with the macOS AirPort/IO80211 networking architecture.
- Wi-Fi network discovery and scanning.
- Connection to compatible wireless networks.
- Access to the system Wi-Fi interface for network management.
- Support for compatible 2.4 GHz and 5 GHz wireless networks.
- WPA2 networking.
- Partial WPA3 implementation.
- Integration with the macOS networking stack instead of a standalone Ethernet-style interface.

Actual functionality depends on the adapter, macOS version, access point, and system configuration.

### WPA3 status

WPA3 support is **partial**. This does not mean that all WPA3 networks, authentication methods, or transition configurations are supported.

If a network cannot be joined, check its security configuration and test with a compatible WPA2 network before concluding that the adapter itself is not working.

### AWDL and Apple wireless features

AWDL is a separate Apple wireless networking mechanism used by features such as AirDrop and other peer-to-peer services.

AirPort_RTW88 does not currently provide complete AWDL support. Consequently, ordinary Wi-Fi connectivity does not imply that AirDrop, peer-to-peer Continuity, or other AWDL-dependent features will work.

## Architecture

AirPort_RTW88 uses the Linux `rtw88` driver as a hardware-driver reference and adapts the required functionality to macOS.

Its implementation is intended to integrate with macOS's native AirPort/IO80211 networking architecture rather than expose the wireless adapter as an ordinary Ethernet interface.

### macOS networking components

Requirements vary by macOS release.

**Ventura 13.7.7–13.7.8**

Use the driver configuration intended for the supported Ventura releases. Do not assume that the configuration for newer macOS versions can be copied unchanged.

**Sonoma 14.4 and later, Sequoia, and Tahoe**

These releases may require a compatible legacy wireless networking stack, including:

- `IO80211FamilyLegacy.kext`
- `IOSkywalkFamily.kext`
- `AMFIPass.kext`

The native `IOSkywalkFamily` may need to be blocked through OpenCore so the intended compatible version can load. Follow the installation instructions for the specific release and driver build you are using.

These components must be mutually compatible with the macOS version. Installing arbitrary versions or copying another machine's configuration can prevent the system from booting correctly or stop Wi-Fi from working.

## Installation

Installation depends on the macOS release and the OpenCore configuration.

Before making changes:

1. Confirm the exact wireless chipset and PCI device ID.
2. Verify that your macOS version is supported.
3. Back up your working EFI folder and keep a bootable recovery option available.
4. Review the required kext dependencies and their loading order.
5. Follow the instructions supplied with the driver release.
6. Test network scanning and connection stability before relying on the adapter for essential connectivity.

For Sonoma 14.4 and later, verify the requirements for `IO80211FamilyLegacy`, `IOSkywalkFamily`, and `AMFIPass` in the applicable installation documentation.

Do not install Realtek88LegacyAirport or AirPort_RTW89 as substitutes for AirPort_RTW88 simply because they belong to the same family. Each project has its own supported hardware and operating-system range.

## Building from source

Build requirements and commands may vary between releases.

The general development environment includes:

- Xcode Command Line Tools.
- A compatible macOS SDK and kernel-development environment.
- The source dependencies required by the repository.
- A supported build environment for the target architecture.

Consult the repository's build configuration and source documentation for the commands and dependencies associated with the version you are building. Do not assume that build instructions for another Realtek driver project apply here.

## Troubleshooting

### The Wi-Fi interface does not appear

- Confirm that the correct driver is installed.
- Check that your adapter is one of the explicitly supported chipsets.
- Verify the macOS compatibility range.
- Check OpenCore's kext configuration and dependency order.
- Confirm that any required legacy networking components are loading.

### The adapter is detected, but no networks appear

- Verify that the appropriate firmware is available to the driver.
- Check whether the installed build includes support for your exact chipset.
- Review relevant kernel logs for initialization or scanning errors.
- Test with a known working access point in a supported frequency band.

### The network appears, but connection fails

Check the network's authentication and encryption settings. WPA3 support is partial, so testing with a compatible WPA2 configuration can help distinguish an authentication limitation from an adapter-initialization issue.

### Wi-Fi works, but AirDrop does not

AirPort_RTW88's ordinary Wi-Fi connectivity does not provide complete AWDL support. AirDrop and other AWDL-dependent features should not be expected to work.

### Wi-Fi stops working after a macOS update

Verify that the new macOS build is within the supported range and that the required networking components remain compatible. Review the relevant release notes before changing kernel extensions or OpenCore settings.

When reporting a problem, include the macOS version, adapter chipset, PCI device ID, driver version, and relevant diagnostic logs. Remove personal identifiers and other sensitive information before sharing logs or configuration files publicly.

## Known limitations

- macOS Monterey 12 is unsupported.
- USB and SDIO wireless adapters are unsupported.
- Only the explicitly listed RTL88xx PCIe adapters are covered by the current hardware compatibility statement.
- WPA3 support is partial.
- AWDL, AirDrop, and related Apple peer-to-peer wireless features are not fully implemented.
- Behavior can vary across macOS releases, hardware revisions, and wireless access points.
- Support for ordinary Wi-Fi does not imply support for every Apple wireless service.

## Credits

AirPort_RTW88 builds on upstream research, code, and development resources.

- **Linux `rtw88`** — principal reference for Realtek wireless hardware-driver functionality.
- **OpenIntelWireless (`itlwm` and `AirportItlwm`)** — technical reference for native-style macOS AirPort/IO80211 integration.
- **Feixiao** — upstream macOS Realtek driver material used during development.
- **MacKernelSDK** — interfaces and resources for macOS kernel-extension development.

These acknowledgements recognize technical contributions and references. They do not imply that upstream projects maintain, endorse, or officially support AirPort_RTW88.

## External dependencies

The project may rely on third-party components for compatibility with newer macOS networking stacks.

| Component or source | Purpose |
|---|---|
| `sXmpwn/atheros-ke` | Source of `HS80211Family.kext` used by the separate legacy driver project |
| `Edwardwich/BCM-WIFI-Sequoia` | Distribution source for legacy `IO80211` and `IOSkywalk` networking components |
| `kaoskinkae/AMFIPass` | Distribution source for `AMFIPass.kext` |
| OpenCore | Bootloader and kernel-extension configuration |
| Linux `rtw88` | Upstream hardware-driver reference |

`HS80211Family.kext` belongs to the legacy networking implementation and is not a general requirement for AirPort_RTW88.

Third-party components remain under their respective owners' licenses. Consult their original repositories for current installation instructions and redistribution terms.

## Licensing

AirPort_RTW88 uses a mixed-license model to distinguish original project work from third-party components.

### Realtek AirPort Family Project License 1.0

Original work covered by the **Realtek AirPort Family Project License 1.0** may be used, modified, redistributed, and commercially distributed under the terms of that license, where legally applicable.

Modified distributions that rely on covered original work must comply with the license's project-identity requirements and must not misrepresent themselves as official releases of the original project.

The custom license applies only to work it can legally cover. It does not automatically relicense third-party code or override upstream licensing terms.

### Third-party code and assets

Linux `rtw88`, Feixiao-derived material, OpenIntelWireless references, MacKernelSDK, firmware, and other third-party components remain subject to their applicable licenses and ownership.

Redistributors must preserve the required notices and comply with the terms applicable to each component. The family license does not replace those obligations.

Consult the license files accompanying the repository and the relevant upstream projects for the exact terms governing the code being used or redistributed.

## Disclaimer

AirPort_RTW88 is an independent community project. It is not affiliated with, endorsed by, or supported by Apple or Realtek.

macOS, third-party frameworks, firmware, and upstream projects remain the property of their respective owners.

Compatibility and stability depend on the adapter, macOS release, firmware, and system configuration. Use the driver at your own risk, and keep a recoverable EFI configuration before modifying kernel extensions or networking components.

## Related projects

AirPort_RTW88 is part of the Realtek AirPort Family, which maintains separate drivers for different generations of macOS and Realtek hardware.

- **Realtek AirPort Family:** https://github.com/xnoah222/Realtek-Airport-Family
- **Realtek88LegacyAirport:** See the Realtek AirPort Family repository for the legacy driver.
- **AirPort_RTW89:** https://github.com/FranciumOS-org/Airport_RTW89

## Repository

- **Source code:** https://github.com/xnoah222/Airport_RTW88
- **Releases:** https://github.com/xnoah222/Airport_RTW88/releases
- **Issues:** https://github.com/xnoah222/Airport_RTW88/issues

For bug reports and compatibility questions, use the repository's issue tracker and provide enough technical information to reproduce the problem without exposing private system information.

