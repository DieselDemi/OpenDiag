# File-backed programming target

The target is a directory, not a CAN address. Programming changes the bytes in its `flash.bin`. Provision it from an explicit geometry and baseline, then use it to test PC-side image selection, retention, sector programming, interruption, verification, reset and recovery. There is no executable loader, security unlock, production Ford memory map, programming voltage control, or J2534 flashing transport in this target. A digest checks the supplied bytes; it is not an OEM signature.

## Run the example

From a build directory with `opendiag-image` on PATH (on macOS it is also in `OpenDiag.app/Contents/MacOS`):

```sh
# Run from the repository root. bench.odecu and export destinations must not exist yet.
opendiag-image ecu create bench.odecu reference/examples/programming/device.json
opendiag-image ecu prepare bench.odecu reference/examples/programming/generic.json
opendiag-image ecu step bench.odecu     # enter programming; the bank cannot boot
opendiag-image ecu step bench.odecu     # erase one changed sector
opendiag-image ecu status bench.odecu
opendiag-image ecu recovery-kit bench.odecu recovery-copy
opendiag-image ecu run bench.odecu      # resume, transfer, verify, reset and re-identify
opendiag-image ecu export bench.odecu installed.bin
opendiag-image ecu restore bench.odecu  # reinstall the pre-job image and original ID
```

Expected result after `run`: `phase: complete`, `bootable: true`, `reportedSoftware: SIM-B`, two changed sectors and twelve incoming bytes overridden by retention. After `restore`, the reported ID is `SIM-A`. `create another.odecu recovery-copy/device.json` (after `opendiag-image ecu`) creates an independent copy of the recovered unit. The kit contains the pre-job bank and its profile; on a never-programmed workspace it contains the initial bank.

## Device and job inputs

- See [`device.json`](examples/programming/device.json). `base` and range addresses are hexadecimal strings; sizes are decimal bytes. The full baseline must be contiguous and cover the entire bank. Its `sha256` is over the **source file bytes**, including line endings in a HEX file. The bank is limited to 64 MiB and 65,536 sectors. Sector size is an aligned power of two between 16 and 65,536 bytes.
- `identityField` is an ASCII software ID inside flash, optionally padded on the right with NUL or space. The baseline must report the declared software. Each candidate must actually contain the requested new ID after overlays and retention. Changing a label in a recipe cannot make a different bank pass identification.
- `retain` is an explicit array of named ranges (possibly empty) or `{"file":"KAM.cfg","sha256":"…"}`. The latter imports the PTDIAG KAM layout: a colon in column one, a hexadecimal address and a decimal length. Imported names are `KAM-01`, `KAM-02`, etc. All ranges must fit the bank, not overlap, and not cover the ID field.
- Jobs specify `part`, exact `from`/`to` software, `minMv`, `maxMv` and an image digest. All referenced files must be ordinary local files beside the recipe. Use the [PCM](examples/programming/pcm.json) and [generic](examples/programming/generic.json) examples as templates. Supported addressed inputs are Intel HEX/H32 and the image reader's VBF subset. Optional `lookup` is a digest-pinned original-format table. Its whole-ID match must select the same software, primary filename and (if applicable) loader filename as the recipe. The table's legacy Windows directory is not used as a local destination.
- PCM and generic jobs preserve unspecified bytes and all retained ranges, even where incoming image data would overwrite them. Generic jobs can also declare a secondary loader: it is validated, mapped into the profile's separate RAM range, and staged again from the local job snapshot on resume. Loader bytes are neither executed nor installed into flash.
- `mode: "block"` changes a sparse region while keeping the same ID (`from == to`). For raw BIN input the `primary` object must also include a hexadecimal `address`; an addressed input must not specify an override. `mode: "configuration"` restores only **complete** named retained ranges from an addressed backup with unchanged software ID. Partial ranges or extra addresses fail before erasure. The pre-job backup covers the whole bank for all four modes.

## Saved checkpoints and backups

`prepared → erasing → programming → verifying → resetting → complete`. A step enters the phase, processes one sector, verifies the bank, or resets it. Change of phase and the next-sector cursor are committed atomically to `state.json`. Each sector is flushed and read back before advancing. Repeating an uncommitted erase or transfer is idempotent. The simulated flash enforces erase-before-setting-bits. The full expected digest is checked at verification **and again at reset** before the installed software ID changes.

`job-<UUID>` holds verified pre-job and projected images plus optional loader data. A new invocation verifies these before resuming, so the original input media need not be present. A failed check stops at the checkpoint. `restore` needs only the verified pre-job bank, independent of candidate/loader corruption. Restoration itself is restartable. A workspace is locked across an operation, an open programming dialog or an attached diagnostic session. Close it before moving or copying its directory. Previous job directories retain their snapshots; the interactive restore command addresses the current job.

Additional commands:

```sh
opendiag-image ecu voltage bench.odecu 13600           # virtual mV; restore the job's required supply to continue
opendiag-image ecu backup bench.odecu pre-job.bin      # raw verified pre-job bank
opendiag-image ecu read-block bench.odecu 1010 8 vid.bin
opendiag-image ecu save-config bench.odecu fields.hex # all retained ranges; append a name to select just one
opendiag-image ecu kam-layout bench.odecu regions.cfg
opendiag-image ecu save-kam bench.odecu regions.cfg KAM-backup.hex
opendiag-driver /absolute/path/to/opendiag-sim --virtual /absolute/path/to/bench.odecu
# At the worker's stdin: module, voltage, then quit (one command per line).
```

Raw and retained exports require a new destination. Incomplete banks cannot be opened for diagnostics or exported as installed firmware. A diagnostic connection to a bootable workspace holds a lock and reports the software ID read from the flash bytes and the modeled supply voltage. The fixed-data simulator and physical-provider selections remain separate.
