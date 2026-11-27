# OpenDiag

OpenDiag is a native Qt 6 reconstruction of the Ford PTDIAG diagnostics and programming suite. Version 0.3 adds **programming and interruption recovery on a persistent virtual ECU**. The J2534 connection to a real vehicle is still read-only. The [behavior ledger](reference/README.md) distinguishes file compatibility, simulated operations, and hardware qualification.

## Try it without a vehicle

Requirements: a C++17 compiler, CMake 3.22+, Ninja, Qt 6.4+ Widgets and Test, and Python 3 for the tests. The reference EXEs are not needed to build or run OpenDiag.

```sh
cmake --preset native
cmake --build --preset native
ctest --preset native
```

On macOS, `open build/native/OpenDiag.app`. On Linux, run `build/native/OpenDiag`. On Windows, use a Visual Studio x64 developer shell with Qt on `CMAKE_PREFIX_PATH`; run `build\native\OpenDiag.exe`. If Qt is in a nonstandard location, pass `-DCMAKE_PREFIX_PATH=/path/to/Qt` to the configure step.

Select **Simulator (no vehicle) → Connect → Ping module → Read**. Expect `GGDS (D100 = 0x01)`. Select **Read module software ID → Read** to get `SIM-PCM-001`. **File → Save session log** exports the most recent 10,000 log lines, including driver/firmware versions, request/response payloads, and acquisition boundaries.

`opendiag-driver --replay OpenDiag.log` decodes an exported session again without loading a provider or transmitting. It rejects incomplete transactions and mismatched replies.

### Try programming and recovery

1. Open **Programming → Program or recover virtual ECU → Try PCM example**. Choose a **new directory**. The example starts at software ID `SIM-A`; the job selects `SIM-B` using a PTDIAG-style lookup table. The example IDs, voltages and memory layout are synthetic.
2. Use **Single step** to enter programming and erase one sector. Close the dialog. Reopen that workspace: the bank is now unbootable, and **Run / resume** continues from the saved checkpoint. OpenDiag verifies the complete bank before reset, including bytes outside the update and the retained VID/TRID fields.
3. Select **Select this virtual ECU in Diagnostics → Connect → Read**. The simulated J2534 provider reads `SIM-B` from the verified bank. Disconnect to release the workspace lock. Reopen the programmer and **Restore pre-job backup**; Diagnostics will now read `SIM-A` again.

PCM, generic (including a mapped RAM loader image), block and configuration jobs are also available from the CLI. A recovery kit reconstructs a workspace without the original input files. See [programming profiles, recipes and commands](reference/virtual-programming.md).

### Try a complete PTDIAG acquisition

1. **Setup → Import PTDIAG.INI**: choose [`reference/examples/PTDIAG.INI`](reference/examples/PTDIAG.INI). It loads the documented RPM/vbat configuration and a PID selection list using paths relative to the INI. The packaged examples are in `OpenDiag.app/Contents/Resources/examples`, the Windows installation's `examples`, or `/usr/share/opendiag/examples`.
2. Connect the **Simulator** and open **Data acquisition**. On the RPM row set **High trigger** to `820` and **Post samples** to `2`. Click **Start monitor**. Expect four complete sets: RPM `800, 820, 840, 860`; vbat `14` for each. Inspect min/max and **View capture**.
3. **Save capture** writes a numeric millisecond CSV. **Open capture** reads that data back. **File → Replay acquisition from session log** instead re-decodes the request/response bytes of a complete matching acquisition, with its original timestamps and FIFO capacity. Save the capture itself for long runs; the displayed log is bounded.

Colon-record configurations support diagnostic DIDs (`p`), one-based packet fields (`k`), Mode 1 (`1`) and freeze frame 0 (`2`). Signed/unsigned, bit and four-byte float translations, binary point, scalar, offset, and arithmetic formulas using `x`, `+`, `-`, `*`, `/` and parentheses are supported. The monitor reads each distinct request once per sweep, records only complete sweeps, and supports stop-on-full or a rolling FIFO (default 200,000 sets) and per-channel high triggers. `k` is a field within a DID response, **not** module-programmed rapid packet mode. DMR (`d`) configurations can be decoded against a saved image in **Tools → Read DMRs from image**; live DMR, rapid mode, broadcast definitions and OSC are not qualified. The INI importer applies acquisition settings and reports unapplied keys. See the [actual files saved by all five originals](reference/golden/).

### Diagnostics and CAN monitoring

- **Diagnostic → Read DTCs from module list** loads the original three-column [module format](reference/examples/modules.cfg). The implemented scan reads ISO 14229 status records for `CAN` entries in the standard physical `7E0`–`7E7` range at 500 kbit/s; other protocols/routes are explicitly reported per module. The single-target reader also accepts a custom physical address pair.
- Read stored, pending and permanent emissions codes, UDS status records, VIN, calibration IDs and CVNs. Mode 1/2/6/9 scans follow the ECU's supported-ID bitmaps. Mode 6 results are raw; a complete definition/units translator is still outstanding. **View → Ford trouble code definitions** reads the KWP2000 and ISO 14229 dictionaries from the hash-verified local 2293 executable as data; no original binary is executed or bundled by OpenDiag.
- **Tools → Monitor CAN data** opens a separate raw classic CAN channel when diagnostics is disconnected. Filter 7xx, 6xx/7xx or a specific ID and save/reopen timestamped CAN CSV. The view shows the latest 1,000 matching frames; the capture retains the latest 100,000 unfiltered frames. It sends no application frames; this does not imply the adapter is electrically silent or disables bus ACKs.

## Connect a J2534 adapter

Install the adapter's own provider. Select it in the provider list (Windows) or browse to its **absolute** library path. The default is J2534-1 **04.04**, physical ISO 15765 at 500 kbit/s, PCM request `7E0` / response `7E8`. Choose a module preset or **Setup → General configuration** for another physical 11/29-bit address pair and bitrate. Changing bitrate does not switch connector pins. The driver handles ISO-TP segmentation and flow control. The simulator uses the same dynamic loading, filters, and message calls as a real provider.

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

**Utility → View HEX, VBF or binary file** shows sparse ranges, 256-byte hex/ASCII pages, supported header text and HEX export. **Tools → Read translated table from image** displays signed, unsigned or float cells in either byte order, with CSV or fixed-width export. `opendiag-image inspect FILE` reports format, byte count, ranges and addresses. For a raw `.bin`, displayed addresses are offsets from zero unless a base is supplied. **Programming** also supports offline lookup matching and candidate/backup comparison.

```sh
opendiag-image inspect calibration.vbf
opendiag-image to-hex calibration.vbf new.hex
opendiag-image to-hex block.bin new.hex 10000  # explicit hexadecimal base address
opendiag-image vid vid-block.bin 2            # specify original layout 0, 1 or 2
opendiag-image lookup generic lookup.txt 8C3A-14C204-AF
opendiag-image compare backup.bin 10000 calibration.vbf
opendiag-image table backup.bin 10000 8 4 s16 motorola csv 10000
opendiag-image mx32-config --list
opendiag-image mx32-config vid128 reference/golden/mx32/vid128/Sample.h32 new-config.vbf
```

The CLI is beside the driver helper (inside `OpenDiag.app/Contents/MacOS` on Mac). HEX export requires a new output file. Intel HEX checks record length, checksum, EOF, extended addresses and overlaps; type 03/05 entry records survive conversion. Recognized PTDIAG KAM and Ford text preambles are shown separately from the data. `.h32` is accepted only when it contains Intel HEX records. The VBF reader covers uncompressed 2.x files with big-endian address/length blocks and CRC-16/CCITT-FALSE per block. It verifies the CRC-32 of the binary body when `file_checksum` is present. This is not signature verification. Input limit: 64 MiB. The VID command reports raw bytes at the recovered offsets, not inferred engineering values.

**Utility → MX32 configuration block to VBF** converts complete VID 128-byte, TRID 128/256-byte and combined VID+TRID 384-byte profiles to VBF 2.4. Machine-readable header fields and the entire binary body are compared with [outputs from MX32 V337](reference/golden/mx32/); producer/date comments differ. Gaps, extra data, and the wrong address/size are rejected. This does not implement the other MX32 calibration profiles.

## Build layout and packages

- `native/diagnostics.cpp`: read operations and reply decoding. `native/worker.cpp`: J2534 session in a separate process, bounded waits and cleanup. `native/window.cpp`: Qt client; `native/acquisition.cpp` and `native/monitor.cpp`: file compatibility, sampling, triggers and replay. Original Win32 sources are still available; `OPENDIAG_LEGACY_WIN32` is an optional historical target.
- `native/image.cpp`: shared desktop/CLI file parser. `native/vid_fields.inc`: the same 105 VID offsets used by the legacy and native builds.
- [CI](.github/workflows/native.yml) builds and tests Windows x64 (plus x86 worker), macOS arm64, and Ubuntu 24.04 x86-64. Packages are a Windows NSIS installer, macOS DMG, and Ubuntu DEB. Vendor drivers are installed separately.
- To package: configure with `-DOPENDIAG_DEPLOY_QT=ON` on Windows/macOS, build, then run `cpack --config build/native/CPackConfig.cmake -C Release`. On Windows set `OPENDIAG_X86_WORKER` to the absolute path to the separately built helper; NSIS and `windeployqt` must be on PATH. On Linux, install distro Qt, `dpkg-dev` and `file`. On macOS the development package is ad-hoc signed; a public release requires Developer ID signing and notarization.
