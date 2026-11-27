# Live J2534 test checklist

This 0.4 build performs read-only diagnostics and data acquisition. There are **no ECU write commands** in the live worker. It is safe only as far as the individual diagnostic service is read-only; a bad physical connection can still interrupt or disturb a vehicle network. Follow vehicle/adapter procedures.

## Before the test

- Use a stationary, powered vehicle or a properly fused bench harness. Identify the exact vehicle, model year, module, part number, calibration/software ID, and ignition state.
- Record OS and architecture; adapter model, serial, firmware, driver/provider path and version; and the selected protocol, CAN ID width, TX/RX IDs and rate.
- Confirm the provider is installed by the adapter vendor, appears in the Windows provider list or is selected by its absolute library path, and that its architecture matches the OpenDiag helper selected for it.
- Confirm power/grounds, DLC cable, adapter routing and the exact bus/pins required by the vehicle. The 0.4 custom profile changes CAN IDs and bitrate only. It **does not switch connector pins or networks**.
- Save the vehicle's original PTDIAG configuration and relevant files. Do not create or clear DTCs as a test of this read-only build.

## Read-only sequence

1. Start OpenDiag. Confirm it is disconnected and no provider/device is selected. Choose the installed real J2534 provider.
2. Set the known, **physical** module addresses and bus rate from reliable module documentation. Do not use functional OBD broadcast IDs as a physical ECU address.
3. Connect and save the session log. Confirm the log identifies API, provider/driver and firmware. A DLL-load/API error means the provider did not open; it is not an ECU response.
4. Turn the ignition to the state the module requires and request D100, F188 and, only where supported, F190. Record successful reply, explicit ECU negative response, timeout, disconnect, or provider error independently. Do not infer compatibility from a generic DTC read.
5. Disconnect. If an action returns an unusual result, preserve the original log and repeat only after checking the wiring, ignition state and provider documentation. Compare with the same original-PTDIAG build when available.

## Data acquisition

Use the module's known PID/DMR definitions and its original saved acquisition config, not the generic RPM/vbat format example unless those IDs are confirmed on that module. First read one channel at a low sample rate. Check its response size, scaling, units and displayed value against an independent/reference reading. Only then add channels or enable sampling/capture. Save the configuration and capture with the session report.

## Required report

Record:

```text
OpenDiag version / commit:
OS and architecture:
Vehicle/year or isolated bench:
ECU name / part number / software ID:
Ignition state:
J2534 adapter / serial / firmware:
Provider path / provider version / API version:
CAN protocol / ID width / pin route / TX / RX / bitrate:
Operation and its required ignition/module state:
Result: success | ECU negative response | timeout | provider error | connection lost
Expected result / reference PTDIAG result:
Saved OpenDiag session log:
```

Redact VIN, serial numbers and other identifying data before sharing logs outside the test team.
