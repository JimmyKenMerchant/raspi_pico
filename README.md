# Kenta Ishii's Projects on Raspberry Pi Pico

### Information of this README and comments in this project may be incorrect. This project is not an official document of Raspberry Pi (Trading) Ltd., and other holders of any Intellectual Property (IP).

## Purpose

* To Develop Applications of Raspberry Pi Pico Using C Language and Assembler Language

**About Raspberry Pi Pico**

* Raspberry Pi Pico is a single-board microcontroller.

* The chip on Raspberry Pi Pico is RP2040, an original microcontroller By Raspberry Pi (Trading) Ltd.

* On the forum releasing Raspberry Pi 4, a piece of the RP 2040 data sheet is mixed as the Raspberry Pi 4. This chip is long-awaited one for learners and hobbyists.

## Installation

* I'm using Linux-based Ubuntu 20.04 LTS in an AMD64 machine. Also Refer [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

```bash
# Install Packages to Be Needed
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
sudo apt install libstdc++-arm-none-eabi-newlib
sudo apt install minicom
# Make Directory
mkdir ~/pico_projects
cd ~/pico_projects
# Install Projects
git clone -b master https://github.com/raspberrypi/pico-sdk.git
git clone -b main https://github.com/JimmyKenMerchant/raspi_pico.git
# Install TinyUSB as A Submodule of pico-sdk
cd pico-sdk
git submodule update --init
cd ../
# Import CMAKE File in PICO-SDK to My Projects
cp pico-sdk/external/pico_sdk_import.cmake raspi_pico/
# Build
cd raspi_pico
mkdir build
cd build
cmake ../
# You can also make in each individual folder of each project.
make -j4
# Connect Pico and Your PC through USB2.0
# Push and Hold "BOOTSEL" Button on Connecting to Your PC
# Copy, Paste, and Run!
cp blinkers/blinkers.uf2 /media/$USER/RPI-RP2/
# Monitor "printf" Messages from Console
sudo minicom -b 115200 -o -D /dev/ttyACM0
```

## Notes on Projects

* Projects may output and input strings through USB. To monitor these, use minicom.

**Blinkers**

* Outputs: GPIO14 (PMW7 A), GPIO15, GPIO16, and GPIO25 (Embedded LED)

* I tested an output with a 3.0mm red LED and a 1K ohms resistor in series.

**Twin Dimmers**

* This project is using ADC. I connected ADC_VREF to 3V3, and AGND to GND. GPIO26 (ADC0) and GPIO27 (ADC1) are used as ADC inputs. Two ADC inputs are converted to digital values with the round robin mode, and these values are used for controlling brightness of two LED outputs from GPIO14 and GPIO15 using PWM.

* I tested this with two B10K ohms potentiometers for ADC inputs.

* PWM emits 1000Hz noise. On the minimum value, PWM doesn't emit 1000Hz because of all ranged low state. On the maximum value, PWM doesn't emit 1000Hz because of all ranged high state.

* On the open state of ADC input, the PWM output shakes. I connected a PWM output to a piezoelectric speaker and a 10K ohms resistor in series. Although we should consider the electric stability on the open state, the shaking of the sound would fit with human factors in view of our awareness because of resembling the sound of a bell.

**Servo**

* I applied FEETECH FS90 Servo Motor. I connected its brown wire to GND, its red wire to VBUS, and its orange wire to GPIO2 (PWM0 A). Double-check the actual wiring layout of your servo motor by an official document. I measured approx. 105-110 degrees rotation with 900-2100us pulses which is assumed 120 degrees rotation, and it seems to be just 10% difference. Note that Pico outputs 3.3V signal, even though I connected this signal to FS90 which is driven by 5V from VBUS. 3.3V (OUT) didn't work for FS90.

* In the setting with the 20ms pulse interval, the inner motor of FS90 didn't stop in the 900us pulse that makes the angle of 0 degrees in 120 degrees. By changing the setting to the 12.5ms pulse interval, the inner motor stopped in the 900us pulse.

* "servo_adc" uses ADC0 as an input. I tested this with a B10K ohms potentiometer for the ADC input.

* "servo_console" accepts an input from a console. I only tested this with minicom in Ubuntu via the USB connection.

**Func**

* Now Under Debugging: This project outputs a sine wave from GPIO15 (PWM7 B).

* According to the page 147-148 of RP2040 Datasheet, ROM (0x0000_0000) of RP2040 includes utilities for fast floating point as firmware. The actual code is in [mufplib.S written by Mark Owen](https://github.com/raspberrypi/pico-bootrom/blob/master/bootrom/mufplib.S). Note that the binaries seems to be loaded to SRAM, and I can't check these in the disassembling file (func.dis in this case).

## Links of References

* [Getting Started with RP2040 – Raspberry Pi](https://www.raspberrypi.org/documentation/rp2040/getting-started/): Downloadable Documentation

* [How to add a reset button to your Raspberry Pi Pico](https://www.raspberrypi.org/blog/how-to-add-a-reset-button-to-your-raspberry-pi-pico/)

* [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

* [TinyUSB](https://github.com/hathach/tinyusb): Pico Sdk implements USB host and USB device libraries of TinyUSB.

* [Bare Metal Examples for Pico by David Welch](https://github.com/dwelch67/raspberrypi-pico)

