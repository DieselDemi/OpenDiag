# OpenDiag

OpenDiag is a native Qt 6 reconstruction of the Ford PTDIAG diagnostics and programming suite. The current release is a **read-only PCM diagnostic and offline image tool**. ECU programming and full PTDIAG/MX32 parity are tracked in [the recovery matrix](reference/README.md).

## Try it without a vehicle

Requirements: a C++17 compiler, CMake 3.22+, Ninja, Qt 6.4+ Widgets and Test, and Python 3 for the tests. The reference EXEs are not needed to build or run OpenDiag.

```sh
cmake --preset native
cmake --build --preset native
ctest --preset native
```

On macOS, `open build/native/OpenDiag.app`. On Linux, run `build/native/OpenDiag`. On Windows, use a Visual Studio x64 developer shell with Qt on `CMAKE_PREFIX_PATH`; run `build\native\OpenDiag.exe`. If Qt is in a nonstandard location, pass `-DCMAKE_PREFIX_PATH=/path/to/Qt` to the configure step.

Select **Simulator (no vehicle) → Connect → Ping module → Read**. Expect `GGDS (D100 = 0x01)`. Select **Read module software ID → Read** to get `SIM-PCM-001`. VIN, stored emissions DTCs, current engine RPM, and 1 Hz RPM sampling are also available. **Save session log** exports the displayed session, including driver/firmware versions and diagnostic request/response payloads. The display keeps the most recent 10,000 lines.

`opendiag-driver --replay OpenDiag.log` decodes an exported session again without loading a provider or transmitting. It rejects incomplete transactions and mismatched replies.

## Connect a J2534 adapter

Install the adapter's own provider. Select it in the provider list (Windows) or browse to its **absolute** library path. This version uses J2534-1 **04.04**, ISO 15765 at 500 kbit/s, physical 11-bit PCM request `7E0` / response `7E8`. The driver handles ISO-TP segmentation and flow control. The simulator uses the same dynamic loading, filters, and message calls as a real provider.

| Target | Provider support |
|---|---|
| Windows x64 | Discovers `PassThruSupport.04.04` in both registry views. PE32 x86 DLLs go to the x86 worker; x64 DLLs to the x64 worker. The GUI remains 64-bit. |
| Linux x86-64 | Select a native `.so` exposing the 04.04 API with **32-bit J2534 scalar fields**. The user must have permission to open the adapter. |
| macOS arm64 | Select an arm64 or universal `.dylib` with the same ABI. A Windows DLL or Intel-only dylib is not a native Apple-silicon driver. |

OpenDiag reports provider load, API version, transport, ECU negative response, and timeout errors separately. **No physical adapter/ECU pair has been qualified yet.** See [first bench run](reference/README.md#first-bench-run) for the evidence needed to add a support-table row.

For a local Windows build that needs x86 drivers, also build the Qt-free host in a separate shell/build tree and place `opendiag-driver-x86.exe` beside the GUI. CI includes it in the Windows installer.

```powershell
cmake -S . -B build32 -G "Visual Studio 17 2022" -A Win32 -DOPENDIAG_WORKER_ONLY=ON
cmake --build build32 --config Release
ctest --test-dir build32 -C Release --output-on-failure
Copy-Item build32/Release/opendiag-driver-x86.exe build/native/
```

## Offline files

**Inspect image** or `opendiag-image inspect FILE` reports format, byte count, ranges and addresses. For a raw `.bin`, displayed addresses are offsets from zero. The CLI also exports data as Intel HEX and inspects raw VID block offsets:

```sh
opendiag-image inspect calibration.vbf
opendiag-image to-hex calibration.vbf new.hex
opendiag-image to-hex block.bin new.hex 10000  # explicit hexadecimal base address
opendiag-image vid vid-block.bin 2            # specify original layout 0, 1 or 2
```

The CLI is beside the driver helper (inside `OpenDiag.app/Contents/MacOS` on Mac). HEX export requires a new output file. Intel HEX checks record length, checksum, EOF, extended addresses and overlaps; type 03/05 entry records survive conversion. `.h32` is accepted only when it actually contains Intel HEX records. The VBF reader covers uncompressed 2.x files with big-endian address/length blocks and CRC-16/CCITT-FALSE per block. It does not validate a whole-file signature or copy VBF header metadata into HEX. Input limit: 64 MiB. The VID command reports raw bytes at the recovered offsets, not inferred engineering values.

## Build layout and packages

- `native/diagnostics.cpp`: allowed read operations and reply decoding. `native/worker.cpp`: J2534 session in a separate process, bounded waits and cleanup. `native/window.cpp`: Qt client. Original Win32 sources are still available; `OPENDIAG_LEGACY_WIN32` is an optional historical target.
- `native/image.cpp`: shared desktop/CLI file parser. `native/vid_fields.inc`: the same 105 VID offsets used by the legacy and native builds.
- [CI](.github/workflows/native.yml) builds and tests Windows x64 (plus x86 worker), macOS arm64, and Ubuntu 24.04 x86-64. Packages are a Windows NSIS installer, macOS DMG, and Ubuntu DEB. Vendor drivers are installed separately.
- To package: configure with `-DOPENDIAG_DEPLOY_QT=ON` on Windows/macOS, build, then run `cpack --config build/native/CPackConfig.cmake -C Release`. On Windows set `OPENDIAG_X86_WORKER` to the absolute path to the separately built helper; NSIS and `windeployqt` must be on PATH. On Linux, install distro Qt, `dpkg-dev` and `file`. On macOS the development package is ad-hoc signed; a public release requires Developer ID signing and notarization.
