# Feature ledger and live-test status

The checked-in resource index covers **903 dialog/menu entries** and 2,269 actionable menu/control locations across the supplied PTDIAG binaries. This is a recovery worklist, not a parity percentage. `inventory.csv` lists dialogs and menu entries; `controls.csv` lists actionable controls; `dialog-procedures.csv` maps resources to recoverable callbacks; `workflows.csv` groups operations. `behavior-evidence.csv` marks only the implemented subset as partial and identifies its acceptance test.

Regenerate inventories without executing the original binaries:

```sh
python3 tools/inventory.py > inventory.new.csv
python3 tools/inventory.py --controls > controls.new.csv
python3 tools/inventory.py --callbacks /path/to/llvm-objdump > procedures.new.csv
python3 tools/ledger.py > build/behavior-ledger.csv
```

The originals remain private under `exes/`. Verify file hashes with `shasum -a 256 -c reference/SHA256SUMS`.

## Original-format evidence

All five original PTDIAG builds loaded and saved the same RPM/vbat configuration and the same two-region KAM definition; their outputs are in [`golden/`](golden/). `acquisition-formats` compares the original output bytes, including line endings. Original MX32 V337 generated the four VID/TRID configuration-block HEX-to-VBF pairs in [`golden/mx32/`](golden/mx32/); `mx32-config-corpus` checks header fields, data, checksums, and a HEX round-trip. These are file-format comparisons; they are not vehicle reads or programming results.

## 0.4 hardware-test scope

The application and all release packages contain no fake J2534 provider, ECU emulator or file-backed programming target. The desktop starts with no provider selected and requires an installed user-selected J2534 provider to connect.

| Area | Implemented for live testing | Still needed for parity |
|---|---|---|
| J2534 connection | Isolated architecture-matched worker, J2534 04.04, provider/driver reporting, explicit physical 11/29-bit ISO 15765 address/rate profile | Test adapter/vehicle support per OS. Implement and validate pin routing, SCP/J1850, ISO 9141, secondary networks and CAN FD. |
| Diagnostics | D100 ping, F188/F190, emissions DTC services, UDS DTC status, Mode 1/2/6/9 supported-ID reads, supported CAN module-list reads | Native reference comparisons across supported modules, snapshots, complete Mode 6 translation, self-tests and service functions. |
| Data acquisition | Original configuration/PID file import, single-rate DID/OBD reads, supported scaling/formulas, trigger/FIFO capture, original configuration round-trip | Live DMR, rapid-rate packet mode, broadcast data/OSC, remaining definitions and measured bus timing. |
| CAN monitor | Raw classic CAN reception, filters, retained capture and CSV export | Validate timestamps, dropped-frame reporting, provider filters and electrical monitor/listen-only behavior per adapter. |
| Offline calibration tools | HEX/H32/VBF/binary views, backup compare, VID/KAM layouts, lookup-table parsing and four original MX32 configuration conversions | Additional MX32 profiles, padding/retention/header modes and compressed/signed VBF formats. |
| PCM/generic programming and block writes | **Not implemented.** There is no erase, download, reset or memory-write operation in the shipped worker/UI. | Reconstruct the actual PTDIAG threads and transport-specific state machines. Implement target selection, authorization, erase, loader/SBL, transfer sizes/checksums, retained data, post-reset identification, and recoverable failures per module family. |
| PTDIAG Vehicle Link Simulator | Excluded from the hardware-only build at the user's request. | Not included in the tester release. |

## Live tester checklist

Use [`live-test.md`](live-test.md). Record OS/architecture, adapter make/model/serial, provider and firmware/API versions, physical connector/pins, CAN IDs/rate, exact vehicle/module/part/software ID, ignition state and the operation. Save the OpenDiag session log and compare each read with the same operation in the original tool where possible. A provider-load success is not an ECU connection; a successful read is not flash qualification.

The previous 0.3 virtual-ECU programmer and its simulated provider have been removed from the current source, build graph, tests, and release package. No physical adapter/vehicle was attached during this implementation. Native Windows GUI startup, macOS arm64 builds and Ubuntu 24.04 Linux builds have software CI checks; these do not substitute for adapter-specific hardware results. Live flash programming remains to be reconstructed from PTDIAG’s target-specific protocol.
