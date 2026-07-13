# Ninebot MAX G3 VESC bridge — discovery build

This is a MAX G3-specific starting point for replacing the stock motor
controller with a VESC while retaining the scooter dashboard. It is based on
the supplied MAX G2 ESP32/I2C design, but it does **not** assume that the newer
MAX G3 dashboard speaks the same protocol.

The default build is deliberately bench-safe:

- dashboard UART frames are captured and checksum-checked;
- the VESC can poll the ESP32 over I2C address `0x45`;
- throttle and brake sent to the VESC are always zero;
- the included LispBM script continuously disables motor output.

## Why this is not ride-ready

The MAX G3 uses a 46.8 V nominal battery (54.6 V maximum charge), an 850 W
nominal / 2,000 W peak motor, an 11-inch wheel, and a newer full-colour
dashboard. Public specifications do not document its dashboard connector,
logic voltage, UART framing, BMS protocol, Hall order, motor parameters, or
safe current limits. Copying the MAX G2 values would be unsafe.

## Hardware assumed

- Arduino Nano ESP32, as used by the supplied G2 prototype
- a VESC whose absolute voltage rating has suitable margin above 54.6 V
- a **bidirectional logic-level interface** appropriate for measured dashboard
  voltages (do not connect an unknown scooter signal directly to the ESP32)
- fused bench supply/current limiting for initial tests

The original G2 wiring photo is not a MAX G3 pinout. Do not use it as one.

## First capture

1. Disconnect the traction motor or raise the driven wheel securely.
2. Measure every candidate dashboard wire relative to ground with the stock
   controller installed. Record off, idle, button, throttle, and brake states.
3. Identify communication direction and logic voltage with an oscilloscope or
   logic analyser. Do not infer a 3.3 V signal from wire colour.
4. Flash `firmware/G3VescBridge` to the Nano ESP32.
5. Through the correct level shifter, connect only Nano `D4` (RX) to one data
   direction while the stock controller remains connected. The discovery
   firmware leaves its TX pin disabled to avoid bus contention. Use a logic
   analyser to capture both directions simultaneously, or move RX and label
   each capture clearly.
6. Capture the USB serial output at 115200 baud while performing, separately:
   idle, full throttle, full brake, each ride mode, lights, indicators, horn,
   lock/unlock, and power button presses.
7. Summarise a saved capture:

   ```powershell
   .\tools\Decode-G3Capture.ps1 -Path .\captures\g3-idle.txt
   ```

## VESC bench check

Load `vesc/G3VescBench.lisp` with VESC Tool. The live Lisp variables
`vt-good-rx`, `vt-bad-rx`, `vt-throttle-raw`, and `vt-brake-raw` verify the I2C
link. The script cannot drive the motor.

## Information needed to finish the ride port

- exact scooter region/model code and model year
- VESC make/model and firmware version
- stock or replacement battery, BMS, and its continuous/peak current ratings
- stock or replacement motor
- clear connector photos and measured pin voltages
- UART/logic-analyser captures for the actions listed above
- VESC motor-detection export, Hall table, and temperature-sensor type

Only after those values are verified should `kG3ProtocolVerified` and
`kAllowDriveOutput` be considered. They are separate gates on purpose.

## References

- [Official Segway MAX G3 product page](https://www.segway.com/ekickscooter/products/max-g3.html)
- [Official MAX G3 user manual (PDF)](https://store.segway.com/media/wysiwyg/warranty/Max_G3_User_Manual.pdf)
- [Documented legacy Ninebot UART framing](https://github-wiki-see.page/m/etransport/ninebot-docs/wiki/protocol) — useful background only, not proof of MAX G3 compatibility
