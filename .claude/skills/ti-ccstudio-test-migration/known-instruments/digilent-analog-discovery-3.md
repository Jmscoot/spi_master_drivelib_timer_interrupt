# Digilent Analog Discovery 3

Aliases: AD3, Digilent AD3, Analog Discovery 3, Digilent Analog Discovery 3, Analog Discovery3
Instrument class: mixed-signal

## Software prerequisites

| Component | Purpose | License | Detect | Install source | Install method | Auto-installable? |
|---|---|---|---|---|---|---|
| WaveForms Runtime (`dwf` shared library) | The C library every binding calls. The Windows installer ships `dwf.dll` builds for x86, x86_64 and arm64. On macOS it is `dwf.framework`. On Linux it is `libdwf.so`. | Proprietary, free of charge. Digilent says "WaveForms is FREE to download and use"; the files are NI-copyrighted. The full EULA text is not confirmed by any fetched source. | Windows: any of `C:\Program Files\Digilent\WaveForms3\dwf.dll` (64-bit installer), `C:\Windows\System32\dwf.dll`, or `C:\Program Files (x86)\Digilent\WaveFormsSDK` (32-bit installs) exists. Check the paths first: Python may not be installed, and `python` can be a Microsoft Store alias that exits with code 49. If Python is present, `python -c "import ctypes; ctypes.cdll.dwf"` loading without error also confirms it. macOS: `/Library/Frameworks/dwf.framework/dwf` exists. Linux: `python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdwf.so')"` succeeds, or `/usr/share/digilent/waveforms` exists. | https://digilent.com/shop/waveforms/ (download needs a Digilent account at cloud.digilent.com > My Products > WaveForms > Documents & Downloads) | Windows: run `digilent.waveforms_v#.#.#_64bit.exe` with the "WaveForms Runtime" section selected. It asks for admin rights unless launched from an admin prompt; `/S` makes it silent. macOS: open the `.dmg` and copy `dwf.framework` to `/Library/Frameworks`; you may need to click "Allow anyway" under Security & Privacy. Linux: `sudo apt install ./digilent.waveforms_*.deb` (an `.rpm` is also offered). | no. The download needs a login, and the installer needs admin/sudo rights. |
| Adept Runtime (USB drivers and libraries) | Needed to communicate with the device. On Windows it is a section of the WaveForms installer. On Linux it is a separate package and provides the FTDI udev rules. | Proprietary, free of charge (Digilent/NI). The license text is not confirmed by any fetched source. | Windows: `C:\Program Files\Digilent\Runtime\UsbDriver` exists (installed together with the WaveForms Runtime). macOS: installed together with the WaveForms Runtime. Linux: `dpkg -s digilent.adept.runtime`. The package name is taken from the `.deb` file name in the manual's install command. | https://digilent.com/shop/waveforms/ (Adept Runtime download, same account) | Windows: the "Adept Runtime" section of the WaveForms installer. The driver prompt can be pre-approved with `CertUtil -addstore TrustedPublisher nic.cer`. Linux: `sudo apt install ./digilent.adept.runtime_*.deb ./digilent.waveforms_*.deb`. | no. It needs admin/sudo rights and a login-gated download. |
| WaveForms SDK (header, libs, samples, `dwfconstants.py`) | Provides the C header and the Python constants module (`dwfconstants`) used by the ctypes approach, plus the `dwfcmd` command-line sample. | Proprietary, free of charge. Code on files.digilent.com is covered by the NI Sample Code License (ni.com/samplecodelicense). | Windows: `C:\Program Files\Digilent\WaveFormsSDK\samples\py\dwfconstants.py` (64-bit installer) or `C:\Program Files (x86)\Digilent\WaveFormsSDK\samples\py\dwfconstants.py` (32-bit installs) exists. The header is `inc\dwf.h` under the same `WaveFormsSDK` folder. macOS: `/Applications/WaveForms.app/Contents/Resources/SDK/samples/py/dwfconstants.py`. Linux: `/usr/share/digilent/waveforms/samples/py/dwfconstants.py`. | Bundled with the WaveForms installer | Windows: the "WaveForms SDK" section of the installer. macOS: inside `WaveForms.app/Contents/Resources/SDK`. Linux: installed by the waveforms package under `/usr/share/digilent/waveforms/`. | no (comes with the vendor installer) |
| Python 3 + `pydwf` (optional) | Object-oriented Python wrapper over `dwf`. Built on ctypes. | MIT (the pydwf wrapper only; the runtime it wraps is proprietary) | `python -m pydwf version`, or `pip show pydwf` | https://pydwf.readthedocs.io/en/latest/ | `pip install pydwf` (needs Python >= 3.6 and numpy) | yes |
| Python `numpy` (with pydwf) | pydwf dependency | not confirmed by any fetched source | `pip show numpy` | PyPI | `pip install numpy` (pulled in by `pip install pydwf`) | yes |
| Node.js + `koffi` | FFI from Node.js to the `dwf` shared library | MIT (koffi only; the runtime it wraps is proprietary) | `npm ls koffi` in the test project | https://www.npmjs.com/package/koffi | `npm i koffi`. Needs Node.js 16 or later. Prebuilt binaries exist for Windows/Linux/macOS x86_64 and ARM64, so no C++ compiler is needed. | yes |

## Language bindings

| Language | Approach (native / FFI / bridge) | Package | Install | License | Notes |
|---|---|---|---|---|---|
| Python | FFI (ctypes) to `dwf`, following Digilent's official samples | stdlib `ctypes` + `dwfconstants` from the WaveForms SDK samples. Digilent's reference code is WaveForms-SDK-Getting-Started-PY. | Nothing beyond the WaveForms Runtime + SDK. Load the library with `ctypes.cdll.dwf` (Windows), `cdll.LoadLibrary("/Library/Frameworks/dwf.framework/dwf")` (macOS) or `cdll.LoadLibrary("libdwf.so")` (Linux). Put the SDK `samples/py` folder on `sys.path` and `import dwfconstants`. | ctypes: Python license. Digilent sample repo: MIT. Runtime: proprietary, free of charge. | **Recommended.** Digilent-maintained pattern, and it follows the current runtime version. Every `FDwf*` function returns 1 on success and 0 on error; read the error with `FDwfGetLastError` / `FDwfGetLastErrorMsg`. |
| Python | Native wrapper | `pydwf` 1.1.19 | `pip install pydwf` | MIT | Alternative. Development was suspended in October 2023, the repo is private, and the author advises against using pydwf for new projects. It is based on libdwf 3.20.1. AD3 is listed as supported but was not tested by the author. Channel indices start at 0. |
| JavaScript (Node.js) | FFI to `dwf` via koffi | `koffi` | `npm i koffi` | MIT | **Recommended.** It avoids a second interpreter, and the C API is documented. Load with `koffi.load('dwf.dll')` (Windows), `koffi.load('/Library/Frameworks/dwf.framework/dwf')` (macOS) or `koffi.load('libdwf.so')` (Linux). Declare functions with prototype strings such as `lib.func('int FDwfDeviceOpen(int idevice, _Out_ int *phdwf)')` and pass `[null]` for scalar out-params. Pass a `Float64Array` / `Buffer` for sample buffers. Declare `FDwfDigitalInStatusData` as `int FDwfDigitalInStatusData(int hdwf, void *rgData, int countOfDataBytes)` (as in `dwf.h`) or with `uint16_t *rgData`. Pass a `new Uint16Array(n)` when the sample format is 16, and pass `2 * n` as the byte count. On Windows x64 the default koffi declaration (no calling-convention keyword) works with `dwf.dll`, and `HDWF` is `int`. Constants are not exported by `dwf.dll`; copy them from `dwfconstants.py` or `dwf.h` (see "DWF constant values" below). |
| JavaScript (Node.js) | Subprocess bridge to a Python script | Python script using ctypes + `dwfconstants` (or `pydwf`), launched with `child_process` | Python + the WaveForms SDK (+ `pip install pydwf` if pydwf is used) | See the Python rows | Alternative, for when koffi cannot load `dwf`. The script opens the device, runs one verb, prints JSON and closes the device. Only one process may hold the device at a time (see Pitfalls). |
| Any | Vendor CLI | `dwfcmd` (sample command-line application in the WaveForms SDK) | Comes with the WaveForms SDK. On Linux the examples are in `/usr/share/digilent/waveforms/dwfcmd/`. | Proprietary, free of charge | The manual says it "can be used for control the device generate and capture signals". Its command syntax is not confirmed by any fetched source. |

Other languages: The WaveForms SDK provides a C header plus libraries, so it can be used from C/C++ directly. Digilent says the SDK supports "C, Python, and more", and third-party toolkits exist for NI LabVIEW and MATLAB. An official C#/.NET binding is not confirmed by any fetched source. A third-party Rust crate (`dwf` 0.2.2, Apache-2.0) is a bindgen of `dwf.h`. A third-party Python package `dwfpy` exists; its license is not confirmed by any fetched source.

## Channels

| Function | Channel names | Notes |
|---|---|---|
| Oscilloscope (capture-analog) | `1+`/`1-` (Scope channel 1), `2+`/`2-` (Scope channel 2) | Differential inputs. SDK channel index is 0-based: channel 1 is index 0 and channel 2 is index 1. Input impedance 1 MOhm \|\| 24 pF. Bandwidth 10 MHz (0.5 dB), 14-bit, 125 MS/s. |
| Waveform generator (generate-signal) | `W1`, `W2` | SDK channel index 0 = W1, 1 = W2. The two wavegen channels are separate device resources. |
| Digital I/O, logic analyzer, pattern generator (capture-logic, drive-digital) | `DIO 0` ... `DIO 15` (pin labels `0`...`15`) | 16 lines. Logic analyzer buffer is 16Ki samples in the default device configuration (see "Device configurations and buffer sizes" below). Read samples with `FDwfDigitalInSampleFormatSet(16)` as uint16 words, where bit N is DIO N. |
| Trigger I/O | `T1`, `T2` | 2 external trigger lines, LVCMOS 3.3 V |
| Power supplies | `V+` (0.5...5 V), `V-` (-0.5...-5 V) | 0.8 A per channel. Not used by any verb. |

## Wire colours

The 2x15 flywire uses four colours for the 16 DIO pins, so every DIO colour
appears on four wires: two solid and two with a white stripe. Tell them
apart by the pin label printed on the AD3 case next to the connector (`0`
... `15`, `T1`, `W1`, and so on). DIO 0-7 are solid, and DIO 8-15 are the
same colours with a white stripe. Every black wire is ground.

| Pin | Wire colour | Notes |
|---|---|---|
| `1+` | orange | Scope channel 1 positive |
| `1-` | orange with white stripe | Scope channel 1 negative |
| `2+` | blue | Scope channel 2 positive |
| `2-` | blue with white stripe | Scope channel 2 negative |
| `W1` | yellow | |
| `W2` | yellow with white stripe | |
| `V+` | red | |
| `V-` | white-striped; base colour not confirmed by any fetched source | |
| `T1` | grey | |
| `T2` | grey with white stripe | |
| `GND` | black | Several black wires; all are ground. |
| `DIO 0` | pink | Same colour as `DIO 4`; use the wire at label `0`. |
| `DIO 1` | green | Same colour as `DIO 5`; label `1`. |
| `DIO 2` | purple | Same colour as `DIO 6`; label `2`. |
| `DIO 3` | brown | Same colour as `DIO 7`; label `3`. |
| `DIO 4` | pink | Same colour as `DIO 0`; label `4`. |
| `DIO 5` | green | Same colour as `DIO 1`; label `5`. |
| `DIO 6` | purple | Same colour as `DIO 2`; label `6`. |
| `DIO 7` | brown | Same colour as `DIO 3`; label `7`. |
| `DIO 8` | pink with white stripe | Same colour as `DIO 12`; label `8`. |
| `DIO 9` | green with white stripe | Same colour as `DIO 13`; label `9`. |
| `DIO 10` | purple with white stripe | Same colour as `DIO 14`; label `10`. |
| `DIO 11` | brown with white stripe | Same colour as `DIO 15`; label `11`. |
| `DIO 12` | pink with white stripe | Same colour as `DIO 8`; label `12`. |
| `DIO 13` | green with white stripe | Same colour as `DIO 9`; label `13`. |
| `DIO 14` | purple with white stripe | Same colour as `DIO 10`; label `14`. |
| `DIO 15` | brown with white stripe | Same colour as `DIO 11`; label `15`. |

## Output limits

| Output | Range | Logic level configurable? | Notes |
|---|---|---|---|
| Wavegen `W1`, `W2` | -5 V ... +5 V (max 10 Vpk-pk), at least 30 mA, 0 Ohm output. Absolute rating ±10 V / 40 mA. | n/a (analog; amplitude and offset are configurable) | Feasible into any target IO voltage if the code clamps each request so that offset ± amplitude stays within 0 V ... Target IO voltage. A closed or disabled channel outputs close to 0 V; it is **not** high impedance. |
| DIO 0-15, T1, T2 | 0 V / 3.3 V (LVCMOS3V3). Drive 4/8/12/16 mA, slow or fast slew, applied the same to all pins. Protected by a 220 Ohm series PTC plus ESD diodes, rated -20...+20 V. | **No.** The output high level is fixed at 3.3 V. Only the input threshold is selectable (1.4 V default, or 0.6 V for low-voltage signals); per the manual, "the digital output is still 3.3 V". | drive-digital is feasible only when Target IO voltage >= 3.3 V. It is **not feasible** for 1.8 V / 2.5 V targets. With the 0.6 V threshold, drive, slew and pull adjustments are unavailable. A common pull option (~200 uA) offers pull-up, pull-down, keeper or none. |

## Verbs

| Verb | Feasible | Python | JavaScript | Notes |
|---|---|---|---|---|
| `capture-logic(channel)` | yes | ctypes: `FDwfDigitalInInternalClockInfo`, `FDwfDigitalInDividerSet(h, int(clk/fs))`, `FDwfDigitalInSampleFormatSet(h, 16)`, `FDwfDigitalInBufferSizeSet`, optionally `FDwfDigitalInTriggerSourceSet(trigsrcDetectorDigitalIn)` / `FDwfDigitalInTriggerSet` / `FDwfDigitalInTriggerPositionSet` / `FDwfDigitalInTriggerAutoTimeoutSet`, then `FDwfDigitalInConfigure(h, False, True)`, poll `FDwfDigitalInStatus(h, True, &sts)` until `stsDone`, then `FDwfDigitalInStatusData(h, uint16buf, 2*n)`, and finally `FDwfDigitalInReset`. pydwf: `device.digitalIn` (method names not confirmed by any fetched source). | koffi, same C calls: `FDwfDigitalInDividerSet`, `FDwfDigitalInSampleFormatSet`, `FDwfDigitalInBufferSizeSet`, `FDwfDigitalInConfigure`, `FDwfDigitalInStatus`, `FDwfDigitalInStatusData` (into a `Uint16Array`), `FDwfDigitalInReset` | Buffer is 16Ki samples in the default configuration and 32Ki with `FDwfDeviceConfigOpen` configuration 3 or 4. `FDwfDigitalInBufferSizeSet` silently caps larger requests. After the Set call, read the actual size with `FDwfDigitalInBufferSizeGet`, and use that size as the sample count (see Pitfalls). Choose the divider so the capture window fits that size. Input threshold is 1.4 V, or 0.6 V for low-voltage signals. |
| `capture-analog(channel)` | yes | ctypes: `FDwfAnalogInChannelEnableSet(h, ch, True)`, `FDwfAnalogInChannelOffsetSet`, `FDwfAnalogInChannelRangeSet`, `FDwfAnalogInBufferSizeSet`, `FDwfAnalogInFrequencySet`, `FDwfAnalogInChannelFilterSet(filterDecimate)`, optionally a trigger with `FDwfAnalogInTriggerSourceSet(trigsrcDetectorAnalogIn)` / `FDwfAnalogInTriggerTypeSet(trigtypeEdge)` / `FDwfAnalogInTriggerLevelSet` / `FDwfAnalogInTriggerConditionSet` / `FDwfAnalogInTriggerAutoTimeoutSet`, then `FDwfAnalogInConfigure(h, False, True)`, poll `FDwfAnalogInStatus(h, True, &sts)` until `DwfStateDone`, then `FDwfAnalogInStatusData(h, ch, buf, n)`, and finally `FDwfAnalogInReset`. For a single value use `FDwfAnalogInStatusSample`. pydwf: `device.analogIn.reset()`, `.configure(reconfigure, start)`, `.status(True)`, `.statusData(ch, n)`. | koffi, same C calls: `FDwfAnalogInChannelEnableSet`, `FDwfAnalogInChannelRangeSet`, `FDwfAnalogInFrequencySet`, `FDwfAnalogInBufferSizeSet`, `FDwfAnalogInConfigure`, `FDwfAnalogInStatus`, `FDwfAnalogInStatusData` (into a `Float64Array`), `FDwfAnalogInReset` | Up to 125 MS/s. Buffer is 16Ki samples per channel in the default configuration and 32Ki with `FDwfDeviceConfigOpen` configuration 1. Read the actual size with `FDwfAnalogInBufferSizeGet` after setting it. Low range (<= 500 mV/div): ±5 V differential, -20...+5 V common-mode. High range: ±25 V. |
| `measure-frequency(channel)` | yes (derived) | Capture with `capture-analog` (or `capture-logic`), then compute the frequency from edge crossings / period in the returned samples | Same, computed in JS from the `FDwfAnalogInStatusData` / `FDwfDigitalInStatusData` buffer | The dwf.h API has `FDwfAnalogInCounterSet(HDWF, double sec)` and `FDwfAnalogInCounterStatus(HDWF, double *pcnt, double *pfreq, int *ptick)`. Their required acquisition mode and AD3 support are not confirmed by any fetched source, so derive the frequency from a capture. Pick a sample rate of at least several times the signal frequency, and a buffer that holds several periods. |
| `generate-signal(freq, amp, offset)` | yes (clamped) | ctypes: `FDwfAnalogOutNodeEnableSet(h, ch, AnalogOutNodeCarrier, True)`, `FDwfAnalogOutNodeFunctionSet(h, ch, AnalogOutNodeCarrier, funcSine)`, `FDwfAnalogOutNodeFrequencySet`, `FDwfAnalogOutNodeAmplitudeSet`, `FDwfAnalogOutNodeOffsetSet`, optionally `FDwfAnalogOutNodeSymmetrySet` / `FDwfAnalogOutRunSet` / `FDwfAnalogOutWaitSet` / `FDwfAnalogOutRepeatSet`, then `FDwfAnalogOutConfigure(h, ch, True)`. Stop with `FDwfAnalogOutReset`. pydwf: `device.analogOut.reset(ch)`, `.nodeEnableSet(ch, DwfAnalogOutNode.Carrier, True)`, `.nodeFunctionSet(..., DwfAnalogOutFunction.Sine)`, `.nodeFrequencySet(...)`, `.configure(ch, True)`. pydwf amplitude/offset method names are not confirmed by any fetched source. | koffi, same C calls: `FDwfAnalogOutNodeEnableSet`, `FDwfAnalogOutNodeFunctionSet`, `FDwfAnalogOutNodeFrequencySet`, `FDwfAnalogOutNodeAmplitudeSet`, `FDwfAnalogOutNodeOffsetSet`, `FDwfAnalogOutConfigure`, `FDwfAnalogOutReset` | Output range is ±5 V, so clamp offset ± amplitude to 0 V ... Target IO voltage. Other waveforms include `funcDC` and `funcSquare`. Bandwidth 4 MHz (0.5 dB). Carrier buffer 16Ki samples in the default configuration. |
| `drive-digital(pin, level \| pattern)` | yes, only when Target IO voltage >= 3.3 V | Level: `FDwfDigitalIOOutputEnableSet(h, mask)`, `FDwfDigitalIOOutputSet(h, mask)`, `FDwfDigitalIOStatus`, `FDwfDigitalIOInputStatus`, `FDwfDigitalIOReset`. pydwf: `device.digitalIO.reset()`, `.outputEnableSet(mask)`, `.outputSet(mask)`, `.configure()`, `.status()`, `.inputStatus()`. Pattern: `FDwfDigitalOutEnableSet(h, pin, 1)`, `FDwfDigitalOutTypeSet(DwfDigitalOutTypePulse / Custom)`, `FDwfDigitalOutDividerSet`, `FDwfDigitalOutCounterSet(h, pin, low, high)` or `FDwfDigitalOutDataSet(h, pin, bits, n)`, `FDwfDigitalOutIdleSet`, optionally `FDwfDigitalOutRunSet` / `FDwfDigitalOutRepeatSet`, then `FDwfDigitalOutConfigure(h, True)`. Stop with `FDwfDigitalOutReset`. | koffi, same C calls: `FDwfDigitalIOOutputEnableSet`, `FDwfDigitalIOOutputSet`, `FDwfDigitalIOReset`; for patterns `FDwfDigitalOutEnableSet`, `FDwfDigitalOutTypeSet`, `FDwfDigitalOutDividerSet`, `FDwfDigitalOutCounterSet` / `FDwfDigitalOutDataSet`, `FDwfDigitalOutConfigure`, `FDwfDigitalOutReset` | **Not feasible for targets below 3.3 V**, because the output high is fixed at 3.3 V LVCMOS. The C function that applies DigitalIO settings (behind pydwf `digitalIO.configure()`) is not confirmed by any fetched source for the ctypes/koffi path. Pin precedence: DigitalIO output-enable = 1 means DigitalIO drives the pin; enable = 0 with output = 1 means Z; both 0 hands the pin to DigitalOut. Drive/pull settings go through `FDwfAnalogIOChannelNodeSet` on the "VDD" channel; Digilent's samples mark this UNTESTED. |

Device open/close (all verbs): `FDwfEnum(enumfilterAll, &count)` (the function is `FDwfEnum`; there is no `FDwfDeviceEnum`, and koffi fails with "Cannot find function" for a wrong name), then `FDwfDeviceOpen(-1, &hdwf)` (first available device) or `FDwfDeviceConfigOpen(idx, cfg, &hdwf)`. Close with `FDwfDeviceClose(hdwf)`. Get errors with `FDwfGetLastError` / `FDwfGetLastErrorMsg(buf512)`. pydwf: `with openDwfDevice(DwfLibrary()) as device:`.

### Device configurations and buffer sizes

The AD3 has six device configurations, and each one divides the on-board
memory differently. `FDwfDeviceOpen` opens configuration 0.
`FDwfDeviceConfigOpen(-1, cfg, &hdwf)` opens a specific configuration. Buffer
sizes are in samples, per channel:

| Config | AnalogIn (scope) | AnalogOut (wavegen) | DigitalIn (logic) | DigitalOut (pattern) |
|---|---|---|---|---|
| 0 (default) | 16384 | 16384 | 16384 | 2048 |
| 1 | 32768 | 4096 | 4096 | 2048 |
| 2 | 8192 | 32768 | 2048 | 2048 |
| 3 | 16384 | 4096 | 32768 | 2048 |
| 4 | 4096 | 4096 | 32768 | 32768 |
| 5 | 8192 | 16384 | 2048 | 2048 |

Read the table at runtime with `FDwfEnumConfig(idxDevice, &count)` and
`FDwfEnumConfigInfo(idxConfig, info, &value)` (`info`: 7 = AnalogIn buffer,
8 = AnalogOut buffer, 9 = DigitalIn buffer, 10 = DigitalOut buffer). After
opening, `FDwfDigitalInBufferSizeInfo` / `FDwfAnalogInBufferSizeInfo` return
the maximum for the open configuration.

### DWF constant values

These are the values in the SDK's `dwfconstants.py`. Copy them from this table
or from that file. Do not write them from memory: several `DwfState` values
are out of order, and a wrong `DwfStateDone` makes a status poll loop forever.

| Constant | Value | Constant | Value |
|---|---|---|---|
| `hdwfNone` | 0 | `enumfilterAll` | 0 |
| `DwfStateReady` | 0 | `DwfStateArmed` | 1 |
| `DwfStateDone` | **2** | `DwfStateRunning` / `DwfStateTriggered` | 3 |
| `DwfStateConfig` | 4 | `DwfStatePrefill` | 5 |
| `DwfStateNotDone` | 6 | `DwfStateWait` | 7 |
| `trigsrcNone` | 0 | `trigsrcDetectorAnalogIn` | 2 |
| `trigsrcDetectorDigitalIn` | 3 | `trigtypeEdge` | 0 |
| `DwfTriggerSlopeRise` | 0 | `DwfTriggerSlopeFall` | 1 |
| `filterDecimate` | 0 | `filterAverage` | 1 |
| `funcDC` | 0 | `funcSine` | 1 |
| `funcSquare` | 2 | `AnalogOutNodeCarrier` | 0 |
| `DwfDigitalOutTypePulse` | 0 | `DwfDigitalOutTypeCustom` | 1 |
| `DwfDigitalOutIdleInit` | 0 | `DwfDigitalOutIdleLow` | 1 |
| `DwfDigitalOutIdleHigh` | 2 | `DwfDigitalOutIdleZet` | 3 |

## Pitfalls

- **Device-open exclusivity:** only one process can have the device open at a time. If the WaveForms GUI or another process holds it, `FDwfDeviceOpen` returns 0 and `FDwfGetLastErrorMsg` returns "Devices are busy, used by other applications". Put that message in the `InstrumentUnavailable` error. Close the WaveForms GUI before running tests, and always call `FDwfDeviceClose` in a finally/teardown block.
- **The holder can be an earlier test run:** a previous run of the test suite that did not exit, or a diagnostic script, holds the device in the same way as the WaveForms GUI. A teardown hook that throws before calling `FDwfDeviceClose` also leaves the device open until that process exits. Call `FDwfDeviceClose` even when other teardown steps fail. Configure the test runner to exit when the tests finish (for example, mocha `"exit": true`).
- **Finding the busy-device holder:** after `FDwfEnum`, `FDwfEnumDeviceIsOpened(idx, &isUsed)` returns 1 when another process holds the device, without trying to open it. The error message does not name the holder. On Windows, list the Node.js processes and their command lines with `powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='node.exe'\" | Select-Object ProcessId,CommandLine | Format-List"`, and look for a test-runner or diagnostic script that loads `dwf`. The WaveForms GUI is `WaveForms.exe`. Report the PID and command line so the user can close that process. Do not stop a process that you did not start.
- **Recovery after a killed holder:** force-killing the process that holds the device releases it right away, and the next `FDwfDeviceOpen` succeeds without an unplug. If `FDwfDeviceOpen` still reports busy, first look for another holder (see above). Unplug and replug the AD3 USB cable only when `FDwfEnumDeviceIsOpened` reports 0 and opening still fails.
- **Unconnected DIO inputs read 0:** a DIO pin with nothing connected reads as 0, and `FDwfDigitalInStatusData` still returns 1 (success). A capture where the wired channel is 0 for the whole window usually means a wiring problem, such as the wrong pin, a missing ground, or a target output that needs a pull-up it does not have. It is rarely a firmware problem. Before a long capture, check the level of the wired pin: `FDwfDigitalIOStatus(h)` then `FDwfDigitalIOInputStatus(h, &word)` returns all 16 DIO levels immediately. You can also run a short capture that covers at least one expected signal period. If the pin never changes, check the wiring before you change firmware or test code.
- **Resource model:** only one instrument instance can control a given device resource at a time. The two wavegen channels are separate resources.
- **Warm-up:** Digilent recommends letting the device heat up for 5-10 minutes to reach operating temperature. Use this for accuracy-sensitive analog measurements.
- **Outputs are not high-Z when off:** a closed or disabled wavegen channel outputs close to 0 V. In the GUI, the "On Close: Continue" setting keeps outputs active after the application exits. Reset outputs (`FDwfAnalogOutReset`, `FDwfDigitalOutReset`, `FDwfDigitalIOReset`) before closing.
- **Buffer limits depend on the device configuration:** `FDwfDeviceOpen` opens configuration 0. In that configuration the scope, wavegen and logic analyzer buffers are 16Ki samples each, and the pattern generator buffer is 2Ki. The larger 32Ki buffers need `FDwfDeviceConfigOpen` with a different configuration (see "Device configurations and buffer sizes"). For longer captures, stream or record over USB at up to about 10-12 MS/s.
- **Oversized buffer requests are silently capped:** `FDwfDigitalInBufferSizeSet(h, 32760)` in configuration 0 returns 1, but `FDwfDigitalInBufferSizeGet` then reports 16384. If the code reads the requested count, the samples past 16384 are stale data from an earlier capture. This produces false edges at sample 16384 and a wrong frequency (about 14% high in one observed case). Do not hard-code a buffer size such as `MAX_BUFFER = 32768`. Do not compute the sample count from the requested size either. Call `...BufferSizeGet` after `...BufferSizeSet`, and use the returned value to size the read buffer, the `FDwfDigitalInStatusData` byte count, and the time base. Choose the divider (sample rate) so that the capture window fits in that many samples.
- **Sample rate:** the scope ADC runs at 125 MS/s. The system clock is adjustable from 50 to 125 MHz, which affects the available divider-based rates.
- **Power:** "Communication with device failed" usually means a power problem. The AD3 draws about 3.33 W idle, and the USB/AUX eFuse limits at 3.1 A. Use a capable USB port or the 5 V auxiliary supply.
- **Linux udev rules:** OS updates can delete the FTDI udev rules for AD1/2/3. If the device stops enumerating, reinstall the Adept Runtime.
- **Acquisition state machine:** AnalogIn / DigitalIn move through Ready -> Config -> Prefill -> Armed -> Running -> Done. Poll `...Status(h, True, &sts)` until `sts == DwfStateDone` (value **2**) before reading data. The numeric values are not in state order (see "DWF constant values"). Give the poll loop a deadline, so that a wrong constant or a missing trigger fails the test instead of hanging.
- **Busy devices during enumeration:** Digilent's samples try the next device index when `FDwfDeviceConfigOpen` fails because a device is busy.
- **pydwf is frozen** at 1.1.19 (libdwf 3.20.1) and its author advises against new projects. Prefer direct ctypes / koffi calls.
- **The runtime download requires a Digilent account login**, so the runtime cannot be fetched automatically.
- **Python may be absent on Windows test hosts:** the JavaScript/koffi path needs no Python. On Windows, `python` can be a Microsoft Store alias that exits with code 49, so detect `dwf` by file path. Write diagnostic scripts in the same language as the test framework.

## Sources

- https://files.digilent.com/manuals/WaveForms/3.24.3/start10.html
- https://files.digilent.com/manuals/WaveForms/3.24.3/main.html
- https://files.digilent.com/
- https://digilent.com/shop/waveforms/
- https://support.digilent.com/hc/en-us/articles/16470375224475-How-to-download-WaveForms
- https://github.com/Digilent/WaveForms-SDK-Getting-Started-PY
- https://raw.githubusercontent.com/Digilent/WaveForms-SDK-Getting-Started-PY/main/WF_SDK/device.py
- https://raw.githubusercontent.com/Digilent/WaveForms-SDK-Getting-Started-PY/main/WF_SDK/scope.py
- https://raw.githubusercontent.com/Digilent/WaveForms-SDK-Getting-Started-PY/main/WF_SDK/wavegen.py
- https://raw.githubusercontent.com/Digilent/WaveForms-SDK-Getting-Started-PY/main/WF_SDK/logic.py
- https://raw.githubusercontent.com/Digilent/WaveForms-SDK-Getting-Started-PY/main/WF_SDK/pattern.py
- https://raw.githubusercontent.com/Digilent/WaveForms-SDK-Getting-Started-PY/main/WF_SDK/static.py
- https://pydwf.readthedocs.io/en/latest/
- https://pydwf.readthedocs.io/en/latest/welcome.html
- https://pydwf.readthedocs.io/en/latest/pydwf_api/pydwf_overview.html
- https://pydwf.readthedocs.io/en/latest/pydwf_api/DwfDevice.analogIn.html
- https://pydwf.readthedocs.io/en/latest/pydwf_api/DwfDevice.digitalIn.html
- https://pydwf.readthedocs.io/en/latest/pydwf_api/DwfDevice.digitalIO.html
- https://pydwf.readthedocs.io/en/latest/background/C_Library.html
- https://github.com/abyszuk/pydwf
- https://raw.githubusercontent.com/abyszuk/pydwf/master/source/pydwf/core/api/analog_in.py
- https://docs.rs/dwf/latest/dwf/all.html
- https://docs.rs/dwf/latest/dwf/dwf/fn.FDwfAnalogInCounterSet.html
- https://docs.rs/dwf/latest/dwf/dwf/fn.FDwfAnalogInCounterStatus.html
- https://koffi.dev/
- https://koffi.dev/output
- https://www.npmjs.com/package/koffi
- https://www.scribd.com/doc/148471171/Analog-Discovery-Pin-Out
- https://www.hackster.io/jcclarke/inductive-sensor-prototyping-w-ldc1000-analog-discovery-2-c7f477
