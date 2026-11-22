# 40 MHz Frequency Meter: ATmega32 + Proteus + CodeVisionAVR + Delphi Android App

This folder holds a cleaned, de-duplicated copy of the project. It keeps only the files you need to **simulate**, **compile the firmware**, **build the Android app** and **install the APK**.

```
FreqMeter_Clean/
├── 1_Proteus_Simulation/        → Proteus 8 schematic + comparator snippet
├── 2_CodeVisionAVR_Firmware/    → ATmega32 firmware (CodeVisionAVR project)
│   ├── Debug/Exe/               → Ready-built firmware (.hex / .cof)
│   └── Driver_Libraries/        → Reusable DIO / LCD / TIMER / INTERRUPT drivers
├── 3_Delphi_Android_App/        → Embarcadero Delphi (FireMonkey) Android app source
├── 4_Android_APK/               → Ready-to-install APK
├── 5_ESP32_Signal_Generator/    → ESP32 10-channel square-wave test generator (1 kHz – 40 MHz)
└── docs/                        → LaTeX project report (FreqMeter_Report.tex) + simulation & test photos
```

---

## How the system works

```
Signal in ──► [LM393 comparator] ──┬──► PB0/T0  (direct count,  < ~3.5 MHz)
                                   └──► 74HC191 (÷16) Q3 ──► PB1/T1 (prescaled count, up to 40 MHz)

ATmega32 @ 11.0592 MHz
  • Timer2 + prescaler 1024 → exact 1.000 s gate window
  • Auto-selects the direct or the ÷16 reading
  • Shows the frequency on a 16x2 LCD (LM016L)
  • Sends the raw value in Hz over UART (PD1/TXD, 9600 8N1) as "123456\r\n"
        │
        ▼
CP2102 USB-UART ──► USB-OTG ──► Android phone running FreqMeter.apk
```

### ATmega32 pin map (from `main.c`)

| Pin             | Function                                                                         |
| --------------- | -------------------------------------------------------------------------------- |
| PA4–PA7         | LCD D4–D7 (4-bit mode)                                                           |
| PB2 / PB3 / PB4 | LCD RS / RW / E                                                                  |
| PB0 (T0)        | Direct signal input                                                              |
| PB1 (T1)        | 74HC191 Q3 (signal ÷ 16)                                                         |
| PD0             | Push-button (active-LOW, internal pull-up): shows the raw Hz value on LCD line 2 |
| PD1 (TXD)       | UART TX → CP2102 RX (9600 baud, 8N1)                                             |
| PD7             | Heartbeat LED (toggles once per measurement)                                     |
| XTAL1/2         | 11.0592 MHz crystal                                                              |

---

## 1. Proteus simulation: `1_Proteus_Simulation/`

| File                 | What it is                                                                                                                                                                                                                              | How to open                                                                          |
| -------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| `freqmeter_c.pdsprj` | **Main schematic.** It contains the ATmega32, 11.0592 MHz crystal, LM016L LCD, 74HC191 ÷16 prescaler, COMPIM virtual serial port (RXD/TXD/RTS/CTS), LED, contrast pot and a signal generator. This is the newest version, saved 17-Nov. | Double-click it, or open **Proteus 8 → File → Open Project**                         |
| `comparator.pdsclip` | Proteus **design snippet** of the LM393 input-conditioning (signal-shaping) stage. It was taken out of the main schematic.                                                                                                              | In Proteus schematic: **File → Import Section… (Design Snippet)** and pick this file |

> [!IMPORTANT]
> **You must point the microcontroller at the CodeVision firmware before you simulate.**
> The saved schematic loads an old temporary file
> (`C:\Users\CYBER\AppData\Local\Temp\VSM Studio\…\Debug.elf`). That file is not your current firmware, and it is lost when Windows clears the Temp folder.
>
> 1. Double-click the **ATmega32 (U1)** in the schematic.
> 2. **Program File** → browse to `..\2_CodeVisionAVR_Firmware\Debug\Exe\freq_m.cof` (or `freq_m.hex`).
>    Pick `.cof` if you want source-level debugging in Proteus.
> 3. Check that **Clock Frequency** = `11.0592MHz` (CKSEL = external crystal). It is already set to 11059200.
> 4. Save the project.
>
> If Proteus asks about the old **"Source Code" (VSM Studio) project** that is embedded in the file, delete that firmware project (Source Code tab → right-click the ATMEGA32 project → Remove). It is an old, broken WinAVR program, and you do not need it.

**COMPIM (virtual serial port):** set it to a real or virtual COM port at **9600 baud, 8N1** if you want to see the UART output on the PC, for example with a serial terminal.

---

## 2. CodeVisionAVR firmware: `2_CodeVisionAVR_Firmware/`

| File                   | What it is                                                                                                                                                                                                                                              |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `freq_m.prj`           | **CodeVisionAVR project.** Chip ATmega32, clock 11.0592 MHz, output folders `Debug\Exe`, `Debug\List`, etc. **Open this file.**                                                                                                                         |
| `main.c`               | **The complete firmware**, in one file. It includes its own register definitions, the UART driver, the 4-bit LCD driver, the 3 timer ISRs (vectors 6/10/12), the 1-second gate, auto-range and display. This is the **only** file the project compiles. |
| `Debug/Exe/freq_m.hex` | Ready-built Intel-HEX firmware (built 17-Nov from this `main.c`, 2004 bytes of code). Use it to **burn a real chip** (USBasp/AVRDUDE, etc.) or load it in Proteus.                                                                                      |
| `Debug/Exe/freq_m.cof` | The same firmware in COFF format, with debug symbols. Use it in **Proteus** for source-level debugging.                                                                                                                                                 |

### How to open and build

1. Start **CodeVisionAVR** (v3.x). Choose **File → Open** and select `freq_m.prj`.
2. Choose **Project → Build All** (Shift+F9).
3. The new `freq_m.hex` / `freq_m.cof` appear in `Debug\Exe\`.

**Fuses for a real chip:** external crystal, high frequency (CKSEL = 1111, CKOPT programmed), JTAG disabled is recommended.

### `Driver_Libraries/` (optional, reusable layered drivers)

> [!NOTE]
> The current `main.c` **does not use these files**. It has its own built-in LCD/UART/timer code. Do **not** add them to `freq_m.prj` together with `main.c`. If you do, you get duplicate `LCD_*`, `Ovf*` and ISR symbols. They are kept here as a clean, working driver library for other projects.

| File(s)                                                                         | Responsibility                                                                     |
| ------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| `STD_TYPES.h`                                                                   | Type definitions (`u8`, `u16`, `u32`, `s8`…), `TRUE/FALSE`, `NULL`                 |
| `BIT_MATH.h`                                                                    | `Set_Bit`, `Clr_Bit`, `Tog_Bit`, `Get_Bit` macros                                  |
| `DIO_register.h` / `DIO_interface.h` / `DIO_program.c`                          | GPIO driver: port/pin direction, read, write, toggle                               |
| `LCD_config.h` / `LCD_interface.h` / `LCD_program.c`                            | 16x2 HD44780 LCD in 4-bit mode (data on PORTA high nibble, RS/RW/E on PB2/PB3/PB4) |
| `TIMER_register.h` / `TIMER_config.h` / `TIMER_interface.h` / `TIMER_program.c` | Timer0 (clk/1024) and Timer1 (external clock on T1), with overflow callbacks       |
| `GLOBAL_INTERRUPT_register.h` / `_interface.h` / `_program.c`                   | Global interrupt enable/disable (SREG I-bit)                                       |

To use them in a new CodeVision project, add every `*_program.c` file to the project (**Project → Configure → Files → Add**). Then `#include` the `*_interface.h` headers you need in your `main.c`.

---

## 3. Delphi Android app: `3_Delphi_Android_App/`

Built with **Embarcadero RAD Studio / Delphi 12 (Studio 23.0)**, FireMonkey, target **Android 64-bit**.

| File                           | What it is                                                                                                                                                                      |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `freq_m.dproj`                 | **Delphi project file. Open this one** in RAD Studio.                                                                                                                           |
| `freq_m.dpr`                   | Program entry point. It creates the main form.                                                                                                                                  |
| `freq_meter.pas`               | **Main form logic.** It finds the USB-serial adapter, asks for USB permission, opens the port at 9600 8N1, reads lines on a background thread and shows them as Hz / kHz / MHz. |
| `freq_meter.fmx`               | Form layout: big frequency label, **Connect** button and the permission-poll timer.                                                                                             |
| `UsbSerialJNI.pas`             | Delphi JNI bridge to Android `UsbManager` and to the Java `usb-serial-for-android` library.                                                                                     |
| `usb-serial.jar`               | The **usb-serial-for-android** Java library (CP21xx, CH34x, FTDI, PL2303 and CDC-ACM drivers). It is linked in the project under _Android 64-bit → Libraries_. **Required.**    |
| `AndroidManifest.template.xml` | Android manifest template. RAD Studio fills it in at build time.                                                                                                                |
| `freq_m.res`                   | Project resource file (required by `{$R *.res}`).                                                                                                                               |
| `build_android.bat`            | Optional one-click command-line build (compile → APK → ADB install).                                                                                                            |

### How to open and build

1. Start **RAD Studio** and choose **File → Open Project** → `freq_m.dproj`.
2. In the **Projects** window, select the target platform **Android 64-bit** and pick your phone as the target device.
3. **Run → Run** (F9) builds the app, installs it and starts it on the phone.
   Or use **Project → Deploy** to make only the APK, which lands in `Android64\Debug\freq_m\bin\freq_m.apk`.
4. Or double-click `build_android.bat` to build from the command line. The compile step was verified and builds with no errors. Packaging the APK from the command line also needs a `freq_m.deployproj` file. RAD Studio creates it once you open **Project → Deployment** with Android 64-bit selected and then choose **File → Save All**. The script tells you if the file is missing.

RAD Studio regenerates the `Android64\` and `__history\` folders and the `.identcache` / `.dproj.local` files on every build. They are not part of the source.

---

## 4. Ready-made APK: `4_Android_APK/`

| File            | What it is                                                                                                                   |
| --------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| `FreqMeter.apk` | Signed debug build of the app (package `com.embarcadero.freq_m`, arm64-v8a). It was checked and is a valid, uncorrupted APK. |

### How to install and use

1. Copy `FreqMeter.apk` to the phone and open it. Allow _"Install unknown apps"_ when Android asks.
   Or install from a PC: `adb install -r FreqMeter.apk`
2. Connect the CP2102 module to the phone through a **USB-OTG adapter**. Wire it as: **CP2102 RX ← ATmega32 PD1 (TXD)**, and **GND ↔ GND**.
3. Open the app and tap **Connect**. Allow USB access when asked. The frequency updates once per second.

---
