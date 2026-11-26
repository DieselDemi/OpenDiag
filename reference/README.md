# Recovery evidence and remaining work

[`inventory.csv`](inventory.csv) maps 906 distinct dialog captions/menu command paths and resource IDs to the reference builds that contain them. A menu entry proves a surface exists, not what an ECU does when selected. The latest build is the breadth baseline; an older-only feature still needs a disposition.

| Reference | Dialog IDs | Menu IDs | Use |
|---|---:|---:|---|
| PTDIAG 1980 CarDAQ | 105 | 33 | Older behavior and adapter-specific differences |
| PTDIAG 2200 McS1 | 118 | 36 | Match the partial Win32 reconstruction and its symbols |
| PTDIAG 2201 MyCANIC | 118 | 36 | Compare nearby revisions across adapter types |
| PTDIAG 2262 MyCANIC | 122 | 37 | Identify changes before FD |
| PTDIAG 2293 MyCANIC FD | 124 | 38 | Primary named-function and UI reference |
| MX32 V337 | 12 | 2 | Separate file conversion GUI/CLI |

The original executables stay private under `exes/`, outside Git. To check a reference set: `shasum -a 256 -c reference/SHA256SUMS`. To regenerate the CSV: `python3 tools/inventory.py > inventory.new.csv` and compare with the checked-in file. The script reads PE resources without executing the files and refuses a hash mismatch.

## Build verification (0.1.0 development)

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
| Get DTCs (40032), OBD-II (40079), data acquisition (40031) | Emissions Mode 03 DTCs and Mode 01 RPM, single shot or sampled; timestamped log export. | Compare bench output; add Ford-specific DTC statuses, freeze frames, PID/DMR definition lists and scaling using real examples. |
| Hex/VBF/binary views (40415/40424/40637); MX32 conversion | Inspect raw BIN, Intel HEX/H32 records and the CRC-checked uncompressed VBF subset. HEX export preserves addresses and data. | Original-to-new golden input/output pairs for each MX32 size/option in scope, including Ford text headers and compressed VBF variants. |
| Init/configuration, VID, KAM | All three original VID offset layouts available for offline inspection. | Collect `PTDIAG.INI`, PID/DMR/OSC examples and corresponding original output; verify layout selection, units, and backup/restore per family. |
| PCM programming (40022), generic programming (40173), block read/write (40332) | Not implemented. No erase, download or reset is sent by this release. | Choose first ECU, match documented prerequisites and image identity, prove preservation/verification and interrupted-session recovery on a spare unit. Then repeat per family. |
| Other networks and tooling | One physical HSCAN PCM profile. SCP/J1850, ISO9141, MS-CAN, secondary HS-CAN, CAN FD, service routines and engineering utilities remain in the inventory. | Establish appropriate provider capability, traces, expected results and independent bench tests per workflow. Use supported OEM authorization where required. |
| Hardware-specific management | Original adapter management/firmware menus are inventoried separately from vehicle functions. | Agree which are superseded by vendor utilities; document any required equivalent. |

## First bench run

1. Record module family, part number, calibration ID, OS/architecture, adapter make/model, driver/API and firmware versions. Use a module known to use 11-bit PCM addressing `7E0/7E8`, 500 kbit/s. Record whether Mac/Linux has a native driver.
2. With a powered bench harness, use the original reference on Windows. Capture **Ping** and the **F188 software ID** result and a read-only trace, including a no-response case. Include the build/hash in the capture notes.
3. With the same module and bus, select the real provider in OpenDiag. Read D100 and F188, save the session log, and compare request, response payload and displayed value. The application log records diagnostic payloads, not individual CAN/ISO-TP frames. Reconnect after unplug and repeat with the second adapter make.
4. Record each result as `OS | architecture | adapter/driver | bus | module/calibration | operation | reference capture | pass/fail`. A simulator pass is not a hardware qualification. A read-only pass is not a programming qualification.

To begin programming qualification, supply a matching known-good calibration, a recoverable spare ECU, independent power, and the documented recovery path. Before general release, verify the post-reset software ID, retained configuration and recovery after a deliberately interrupted **bench** session. Unsupported family/driver combinations stay explicit in the support table.
