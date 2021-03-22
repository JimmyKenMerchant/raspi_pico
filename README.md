# Kenta Ishii's Projects on Raspberry Pi Pico

### Information of this README and comments in this project may be incorrect. This project is not an official document of Raspberry Pi (Trading) Ltd., and other holders of any Intellectual Property (IP).

## Purpose

* To Develop Applications of Raspberry Pi Pico Using C Language and Assembler Language

**About Raspberry Pi Pico**

* Raspberry Pi Pico is a single-board microcontroller.

* The chip on Raspberry Pi Pico is RP2040, an original microcontroller By Raspberry Pi (Trading) Ltd.

* On the forum releasing Raspberry Pi 4, a piece of the RP 2040 data sheet is mixed as the Raspberry Pi 4. This chip is long-awaited one for learners and hobbyists.

## Installation

* I'm using Linux-based Ubuntu in an AMD64 machine. Also Refer [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

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
make -j4
# Connect Pico and Your PC through USB2.0
# Push and Hold "BOOTSEL" Button on Connecting to Your PC
# Copy, Paste, and Run!
cd blinker
cp blinker.uf2 /media/$USER/RPI-RP2/
# Monitor "printf" Messages
sudo minicom -b 115200 -o -D /dev/ttyACM0
```

## Links of References

* [Getting Started with RP2040 – Raspberry Pi](https://www.raspberrypi.org/documentation/rp2040/getting-started/): Downloadable Documentation

* [How to add a reset button to your Raspberry Pi Pico](https://www.raspberrypi.org/blog/how-to-add-a-reset-button-to-your-raspberry-pi-pico/)

* [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

* [TinyUSB](https://github.com/hathach/tinyusb)

