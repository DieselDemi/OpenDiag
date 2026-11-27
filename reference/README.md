# Recovery evidence and remaining work

[`inventory.csv`](inventory.csv) maps 903 distinct dialog/menu entries to reference builds (800 PTDIAG entries). Four PE resource-name offsets in the earlier inventory resolve to the same `SELECTBOX` dialog. [`controls.csv`](controls.csv) adds controls, classes, captions and menu ownership. The latest build is the breadth baseline; older-only features still need a disposition.

| Reference | Dialog IDs | Menu IDs | Use |
|---|---:|---:|---|
| PTDIAG 1980 CarDAQ | 105 | 33 | Older behavior and adapter-specific differences |
| PTDIAG 2200 McS1 | 118 | 36 | Match the partial Win32 reconstruction and its symbols |
| PTDIAG 2201 MyCANIC | 118 | 36 | Compare nearby revisions across adapter types |
| PTDIAG 2262 MyCANIC | 122 | 37 | Identify changes before FD |
| PTDIAG 2293 MyCANIC FD | 124 | 38 | Primary named-function and UI reference |
| MX32 V337 | 12 | 2 | Separate file conversion GUI/CLI |

The originals stay private in `exes/`. Verify them with `shasum -a 256 -c reference/SHA256SUMS`. [`dialog-procedures.csv`](dialog-procedures.csv) records 482 bindings, including 122 in 2293, from direct dialog creation calls with literal template and callback arguments. A binding identifies a dialog handler, not the behavior of every branch.

```sh
python3 tools/inventory.py > inventory.new.csv
python3 tools/inventory.py --controls > controls.new.csv
python3 tools/inventory.py --callbacks /path/to/llvm-objdump > procedures.new.csv
python3 tools/ledger.py > build/behavior-ledger.csv
python3 tools/ledger.py --workflow acquisition
python3 tools/ledger.py --older-only
```

The ledger joins [`workflows.csv`](workflows.csv), [`behavior-evidence.csv`](behavior-evidence.csv), menu ownership and recovered procedures. It has **2,269 PTDIAG locations** (676 menu paths and 1,593 control variants, including notifying static controls). Keys are scoped to the resource; duplicate build entries are reconciled. Each row has a prerequisite, expected result and qualification gate. Partial means only the stated subset is implemented; virtual-only means that the operation runs on a file-backed target, not a physical ECU. Other rows are unverified. Dynamic choices and ECU variants can add work without changing these counts.

## Original application fixtures

All five original PTDIAG EXEs were run separately under CrossOver. Using **Data Acquisition → File → Load Configuration File → Save Configuration File**, each wrote a [golden configuration](golden/) from the same [RPM/vbat input](examples/RPM-VBAT.cfg). The files retain their CRLF bytes and original scratch-directory annotation. `acquisition-formats` decodes each and checks byte-identical saves without edits. The 1980 output lacks the newer headers and units column. The running 2293 capture status window reported a **200,000**-set default FIFO, despite its help describing 100,000. These are original file outputs, not vehicle traces.

To repeat on native Windows with a private reference set, build with `BUILD_TESTING=ON`, then run in a desktop session:

```powershell
.\tools\reference-windows.ps1 -ReferenceExe .\exes\PTDIAG_2293_MyCANIC_FD.EXE `
    -Probe .\build\tests\Release\opendiag-reference-probe.exe -OutputDirectory C:\Temp\OpenDiagRef2293
```

Choose a **new** output directory. The probe uses the acquisition file menus and checks the resulting file. Use `-Kam` for the KAM tool's **Load Config File → Save Config File** round trip. All five originals accepted OpenDiag's exported two-region layout and independently wrote the [KAM configuration fixtures](golden/); OpenDiag uses each original output to reproduce the same addressed virtual-bank backup. These are file-format results, not vehicle memory reads. The probe selects no Connect or vehicle programming action. Native Windows CI separately exercises the replacement's x64 GUI, x86 helper and installed driver with the simulator, without requiring the private EXEs.

## Build verification

The four `mx32-config` configuration-region profiles have independent [MX32 V337 golden input/output pairs](golden/mx32/). The probe selected the original file-size menu and **Run → Make VBF File** with default metadata and address checking enabled. All samples are deterministic synthetic byte sequences. To reproduce, use `reference-windows.ps1` with `-ReferenceExe .\exes\MX32_V337.EXE -MxConfig vid128` (or `trid128`, `trid256`, `vidtrid384`) and the same probe/new-directory parameters as above. The output date/comment is not deterministic. `mx32-config-corpus` checks the non-comment header, binary body, both checksums and a data round trip.

For **0.3.0**, all nine suites passed on Apple silicon and Ubuntu 24.04 amd64. The recovery suite kills an actual programmer process, removes its original media, restarts it and checks the resulting bank byte-for-byte. It also checks restoration without the loader/candidate, rejected corrupted backups, interrupted restoration, and F188 read-back through a separately loaded simulator DLL/library. The mounted ad-hoc-signed DMG passed the programming and MX32 tests; the DEB was installed and its installed tools exercised. The Windows x64 desktop workflow and Windows programming CLI/recovery suite ran under Wine on Linux. These compatibility-layer runs are not native Windows acceptance; the branch has not been published to run hosted CI. No physical ECU programming has been implemented or qualified.

The locally built Windows NSIS installer also returned zero in a clean CrossOver bottle. Its installed payload was checked against all 68 staged files; the installed application launch/close, programming/recovery and MX32 conversions were exercised with Wine on Linux. A repeat silent-install wrapper timed out after deploying matching files, so unattended upgrades on native Windows remain unverified.

For **0.2.0**, all seven CTest suites passed on Apple silicon and Ubuntu 24.04 amd64, including the original-file corpus, acquisition GUI workflow, replay, error paths and optional private DTC dictionary test. The mounted DMG's signature, bundled examples, driver and image CLI were checked; the DEB was installed and its installed worker tested. The Windows x64 application and x86 driver host cross-compiled. Both worker architectures read voltage, acquisition data, the software lookup ID and Mode 9 data with matching simulator DLLs under CrossOver. The GUI test and packaged Windows application faulted in QtGui under CrossOver. No native Windows runner or physical adapter/ECU was used for these 0.2.0 results.

Earlier **0.1.0** checks:

| Build | Evidence |
|---|---|
| macOS arm64, Qt 6.12 | All four CTest suites passed. Mounted the DMG, verified the ad-hoc bundle signature, and ran simulator/replay and image tests directly from the mounted copy. |
| Ubuntu 24.04 amd64, distro Qt 6.4 | All four suites passed in an x86-64 Linux container. Installed the generated DEB and ran the session test against its installed driver and simulator. |
| Windows x64 app and x86 helper, MinGW | Both compiled. The x64 and x86 hosts each pinged and read the simulated PCM through a matching Windows DLL under CrossOver. The Windows Qt GUI smoke test faulted in QtGui under CrossOver; native Windows GUI and installer validation are outstanding. |

These are software tests. There are no qualified hardware combinations or validated programming workflows in this release.

## Feature matrix

| Workflow / reference evidence | Current implementation | What closes the row |
|---|---|---|
| Connect and Ping, `BoxConnect`/`PingModule` | Generic 04.04 worker, simulator, PCM D100 ping. In 2293 `PingModule` asks `lptGetPIDValue` for D100; values 01/02/03 report GGDS, 81/85/87 GDS. OpenDiag distinguishes reply, rejection and timeout. | Compare successful and unsupported/rejected cases on a known module and two adapter makes. Compare the original's fallback status separately. |
| Module identification, F188/F190 in 2293 | Read application software ID and VIN. Hex display for non-text software IDs. | Match original and native results on the same module. |
| Get DTCs (40032), OBD-II (40079) | Mode 03/07/0A, ISO 14229 DTCs and masks, multi-module read for standard `CAN` addresses, Mode 1/2/6/9 supported-ID scans. Optional local import of 10,170 KWP and 13,309 UDS descriptions. | Compare real results across networks; add snapshots, complete translations and self-tests. |
| Data acquisition (40031) | Config/PID list import and selection, scaling and arithmetic formulas, single-rate monitoring, min/max, FIFO, high/post triggers, numeric CSV save/reopen and raw session replay. | Compare with original bench samples; recover rapid mode, advanced formulas, broadcast, OSC and live DMR. |
| Hex/VBF/binary views (40415/40424/40637); MX32 conversion | Paged sparse ranges, recognized Ford/KAM header text, translated tables, HEX export, VBF body CRC-32 and all four MX32 VID/TRID full-block profiles compared to original V337 output. | Other MX32 size/option profiles, padding modes, compressed and signed VBF variants. |
| Init/configuration, VID, KAM | Acquisition INI import, offline DMR snapshots and VID layouts, translated table reader, PTDIAG KAM region file import/export and addressed backup/restore on virtual flash. | Compare wider INI, DMR and OSC inputs and qualify live backup/restore per family. |
| PCM programming (40022), generic programming (40173), block read/write (40332) | [Virtual target](virtual-programming.md): image and lookup checks, RAM loader staging, sector erase/transfer, bank verification, boot ID read-back through simulator, portable backup and interrupted-job recovery. Real J2534 operations remain read-only. | Select a recoverable ECU and known-good image; implement its documented loader, authorization, voltage and timing requirements and qualify on real hardware. |
| Other networks and tooling | Physical 11/29-bit ISO 15765 and raw classic CAN receive/filter and CSV export. Setting bitrate does not switch connector pins. | Qualify routing, legacy buses, FD, service and engineering functions with appropriate traces, authorization and bench hardware. |
| Hardware-specific management | Original adapter management/firmware menus are inventoried separately from vehicle functions. | Agree which are superseded by vendor utilities; document any required equivalent. |

## First bench run

1. Record module family, part number, calibration ID, OS/architecture, adapter make/model, driver/API and firmware versions. Use a module known to use 11-bit PCM addressing `7E0/7E8`, 500 kbit/s. Record whether Mac/Linux has a native driver.
2. With a powered bench harness, use the original reference on Windows. Capture **Ping** and the **F188 software ID** result and a read-only trace, including a no-response case. Include the build/hash in the capture notes.
3. With the same module and bus, select the real provider in OpenDiag. Read D100 and F188, save the session log, and compare request, response payload and displayed value. The application log records diagnostic payloads, not individual CAN/ISO-TP frames. Reconnect after unplug and repeat with the second adapter make.
4. Record each result as `OS | architecture | adapter/driver | bus | module/calibration | operation | reference capture | pass/fail`. A simulator pass is not a hardware qualification. A read-only pass is not a programming qualification.

To begin programming qualification, supply a matching known-good calibration, a recoverable spare ECU, independent power, and the documented recovery path. Before general release, verify the post-reset software ID, retained configuration and recovery after a deliberately interrupted **bench** session. Unsupported family/driver combinations stay explicit in the support table.
