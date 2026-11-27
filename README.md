# OpenDiag

OpenDiag is a native Qt 6 rebuild of Ford PTDIAG. **The 0.4 tester build uses real J2534 providers only.** It starts disconnected; the application, tests and release packages contain no mock pass-thru provider, ECU emulator or file-backed ECU.

This build supports live **read-only diagnostics and data acquisition**, plus offline file tools. It does **not** erase, program, reset or write a connected ECU. The original PTDIAG flash and memory-write workflows remain open parity work in the [feature ledger](reference/README.md). Offline lookup and file comparison are not programming operations.

## Build

Requirements: C++17, CMake 3.22+, Ninja, Qt 6.4+ Widgets/Test, and Python 3.

```sh
cmake --preset native
cmake --build --preset native
ctest --preset native
```

Launch `build/native/OpenDiag.app` (macOS), `build/native/OpenDiag` (Linux), or `build/native/OpenDiag.exe` (Windows). No provider is preselected and Connect remains disabled until you select a J2534 device. Windows discovers installed J2534 04.04 providers in both registry views. Browse to your vendor's native provider where discovery is unavailable. The x64 application selects its separate x86 worker for a PE32 vendor DLL.

## First live test

1. Install the adapter manufacturer's driver and connect the adapter to a correctly powered vehicle or test fixture. Select the provider and documented physical module address/profile. OpenDiag selects the CAN bitrate and IDs, **not DLC pin routing**.
2. Use the correct ignition and vehicle state. Connect; confirm the session log reports the selected provider, API, driver/firmware, bus and address. Try Ping (D100), software ID (F188), VIN (F190) if supported, and the appropriate read-only DTC service.
3. Load the actual module-specific PTDIAG acquisition/PID definition rather than assuming the packaged example applies. Save the session log and capture after each test. Use the [live J2534 checklist](reference/live-test.md) to record the exact vehicle, module, adapter and result.

The read-only surface covers emissions DTCs, UDS DTC status, Mode 1/2/6/9 supported-ID scans, single-rate DID/PID acquisition, triggers/captures, and raw classic CAN receive. Network and PID support is vehicle-specific. Mode 6 currently reports raw values. CAN receive is **not electrical silent mode**; adapter ACK behavior is hardware-dependent.

The log captures provider details and ISO 15765 request/response PDUs, not every ISO-TP segment or raw CAN frame. DTC and PID reads are read-only; no erase/write/reset request is offered.

## Offline file tools

OpenDiag retains HEX/H32/VBF/binary page views, translated tables, VID-layout viewing, PTDIAG KAM layout file interchange, lookup matching and calibration-versus-backup comparison. It can convert the four MX32 VID/TRID whole-block profiles checked against original MX32 V337 outputs. Those operations inspect or create **files only**; they do not validate or write a calibration to a vehicle.

```sh
opendiag-image inspect calibration.vbf
opendiag-image to-hex calibration.vbf new.hex
opendiag-image vid vid-block.bin 2
opendiag-image lookup generic lookup.txt 8C3A-14C204-AF
opendiag-image compare backup.bin 10000 calibration.vbf
opendiag-image table backup.bin 10000 8 4 s16 motorola csv 10000
opendiag-image mx32-config vid128 input.h32 new-block.vbf
```

Intel HEX validates lengths, checksums, EOF and address overlaps. Supported uncompressed VBF 2.x verifies segment CRC-16 and `file_checksum` CRC-32; this does not validate an OEM digital signature. Input limit: 64 MiB. Golden outputs from all five supplied PTDIAG binaries and the four supported MX32 conversions are in [`reference/golden/`](reference/golden/).

## Hardware providers

| Platform | J2534 provider |
|---|---|
| Windows x64 | Registered providers are discovered in both registry views. PE32 providers use the x86 worker; PE32+ providers use the x64 worker. |
| Linux x86-64 | Browse to a native J2534 04.04 `.so`; install/permit the vendor driver as documented by its manufacturer. |
| macOS Apple silicon | Browse to a native arm64 or universal J2534 04.04 `.dylib`; Windows DLLs and Intel-only drivers cannot be loaded as native providers. |

The current live network path is ISO 15765 over explicit 11/29-bit CAN IDs. Rate selection does not switch pins. SCP/J1850, ISO 9141, secondary-network routing and CAN FD are not in this live-test build.

## Programming scope

PTDIAG programming requires target-specific entry, authorization, erase regions/sectors, SBL handling, transfer/checksum rules, VID/TRID/EPATS preservation, reset and recovery behavior. These write operations are **not currently implemented**, so there is no live programmer to test yet. No device writes will be substituted with a fake or file-backed ECU. See the [feature ledger](reference/README.md); a real programming path must be recovered per module family.

## Builds and releases

CI builds Windows x64 plus the x86 J2534 worker, macOS arm64 and Ubuntu x86-64. Release packages contain the app, driver host, file tools and example configuration files only—not an adapter driver, pass-thru emulator or virtual ECU. Windows requires the vendor provider. macOS developer packages are ad-hoc signed; public distribution requires Developer ID signing/notarization. The [live test checklist](reference/live-test.md) defines the data to record for tester results.
