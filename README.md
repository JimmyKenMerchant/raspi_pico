# Kenta Ishii's Projects on Raspberry Pi Pico

### Information of this README and comments in this project may be incorrect. This project is not an official document of Raspberry Pi (Trading) Ltd., and other holders of any Intellectual Property (IP).

**Table of Contents**

* [Purpose](#purpose)

* [Installation](#installation)

* [Notes on Projects](#notes-on-projects)
  * [Blinkers](#blinkers)
  * [Twin Dimmers](#twin-dimmers)
  * [Servo](#servo)
  * [Func](#func)
  * [Pedal](#pedal)
    * [pedal_multi](#pedal_multi)
    * [pedal_looper](#pedal_looper)
  * [QSPI Flash](#qspi-flash)

* [Technical Notes](#technical-notes)

* [Tricky XIP](#tricky-xip)

* [Version History](#version-history)

* [Links of References](#links-of-references)

## Purpose

* To Develop Applications of Raspberry Pi Pico Using C Language and Assembler Language

* In view of managing versions, this repository is mainly developing projects in "[Pedal](pedal) (guitar pedals)".

**About Raspberry Pi Pico**

* Raspberry Pi Pico is a single-board microcontroller.

* The chip on Raspberry Pi Pico is RP2040, an original microcontroller By Raspberry Pi (Trading) Ltd.

* On the forum releasing Raspberry Pi 4, a piece of the RP 2040 data sheet is mixed as the Raspberry Pi 4. This chip is long-awaited one for learners and hobbyists.

## Installation

* I'm using Linux-based Ubuntu 20.04 LTS in an AMD64 machine. Also Refer [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

```bash
# Install Packages to Be Needed
# Command "c++" is an alias of "gcc" with implicit "libstdc++" (gcc recognizes files with ".cpp" extension).
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
sudo apt install libstdc++-arm-none-eabi-newlib
sudo apt install minicom
# Several projects use Python-3 scripts to make headers. Add extra Python-3 libraries.
sudo apt install python3-pip libatlas-base-dev
pip3 install numpy scipy
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
# In CMakeLists.txt at the top level, PICO_COPY_TO_RAM is set because I prefer not to use XIP (Execute in place) to make a tricky memory extension for execution.
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

* You can also use OpenOCD. Chapter 5 and 6 of "Getting started with Raspberry Pi Pico" is useful for the installation of OpenOCD and GDB, a debugger. On Raspberry Pi 3B with Raspbian 10:

```bash
# Run through OpenOCD Tested in My Raspberry Pi 3B with USB Powered Pico
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program blinkers/blinkers.elf verify reset exit"
# Or Use with GDB
cd ../
rm -r build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ../
make -j4
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg
# On Another Terminal (File > New Window)
gdb-multiarch blinkers/blinkdrs.elf
# In gdb, Type "target remote localhost:3333", "load", and "monitor reset init"
# "l (list)", "b (break) blinkers_on_pwm_irq_wrap", "c (continue)", "info breakpoints", "delete <Num>", "q (quit)", Ctrl+c (Stop Execution), etc.
# Watchpoints detect changes of values, e.g., "watch blinkers_count", "print blinkers_count", etc.
# Use "display <Variable Name>" for local variables. To unset, use "undisplay <Variable Name>"
# Use "info registers" to know values of registers.
# Write Value e.g., "set {int}0x4003000C = 0x10", 0x4003000C is BUSCTRL: PERFSEL0.
# "x/1xw 0x40030008" to know value in the memory space. 0x40030008 is BUSCTRL: PERFCTR0.
# 0x40030008 is needed to clear by any write to measure counting correctly.
# For Multicore, "info threads", "thread 2", etc.
```

## Notes on Projects

* Projects may output and input strings through USB. To monitor these, use minicom.

### Blinkers

* Outputs: GPIO14 (PMW7 A), GPIO15, GPIO16, and GPIO25 (Embedded LED)

* I tested an output with a 3.0mm red LED and a 1K ohms resistor in series.

### Twin Dimmers

* This project is using ADC. I connected ADC_VREF to 3V3, and AGND to GND. GPIO26 (ADC0) and GPIO27 (ADC1) are used as ADC inputs. Two ADC inputs are converted to digital values with the round robin mode, and these values are used for controlling brightness of two LED outputs from GPIO14 and GPIO15 using PWM.

* I tested this with two B10K ohms potentiometers for ADC inputs.

* PWM emits 1000Hz noise. On the minimum value, PWM doesn't emit 1000Hz because of all ranged low state. On the maximum value, PWM doesn't emit 1000Hz because of all ranged high state.

* On the open state of ADC input, the PWM output shakes. I connected a PWM output to a piezoelectric speaker and a 10K ohms resistor in series. Although we should consider the electric stability on the open state, the shaking of the sound would fit with human factors in view of our awareness because of resembling the sound of a bell.

### Servo

* I applied FEETECH FS90 Servo Motor. I connected its brown wire to GND, its red wire to VBUS, and its orange wire to GPIO2 (PWM0 A). Double-check the actual wiring layout of your servo motor by an official document. I measured approx. 105-110 degrees rotation with 900-2100us pulses which is assumed 120 degrees rotation, and it seems to be just 10% difference. Note that Pico outputs 3.3V signal, even though I connected this signal to FS90 which is driven by 5V from VBUS. 3.3V (OUT) didn't work for FS90.

* In the setting with the 20ms pulse interval, the inner motor of FS90 didn't stop in the 900us pulse that makes the angle of 0 degrees in 120 degrees. By changing the setting to the 12.5ms pulse interval, the inner motor stopped in the 900us pulse.

* "servo_adc" uses ADC0 as an input. I tested this with a B10K ohms potentiometer for the ADC input.

* "servo_console" accepts an input from a console. I only tested this with minicom in Ubuntu via the USB connection.

### Func

* This project outputs a sine wave from GPIO15 (PWM7 B).

* According to the page 147-148 of RP2040 Datasheet, ROM (0x0000_0000) of RP2040 includes utilities for fast floating point as firmware. The actual code is in [mufplib.S written by Mark Owen](https://github.com/raspberrypi/pico-bootrom/blob/master/bootrom/mufplib.S). Note that the binaries seems to be loaded to SRAM, and I can't check these in the disassembling file (func.dis in this case).

### Pedal

* This project is making several types of guitar pedals.

* Caution that this project needs an analogue circuit to receive and output the audio signal, i.e., a DC bias on receiving, low-pass filters on outputting (in case of stereo outputting, this project outputs audio signals from two PWM channels). For reference, [I uploaded a schematic](assets/schematics/pedal_pico.pdf). This schematic applies a difference amplifier referencing [A Deeper Look into Difference Amplifiers by Harry Holt](https://www.analog.com/en/analog-dialogue/articles/deeper-look-into-difference-amplifiers.html). The pins layout of Pico is derived from [RP_Silicon_KiCad](https://github.com/HeadBoffin/RP_Silicon_KiCad). Note that the 3D image for KiCad is [https://github.com/ncarandini/KiCad-RP-Pico](https://github.com/ncarandini/KiCad-RP-Pico). As long as an Op Amp can sink electric current, the output voltage can be negative even if the power source is only positive. However, the single power LM358 makes a sink on 0V (logically) at minimum. On the balanced monaural with a single power Op Amp, the offset of the positive is risen to OFFSET + (OFFSET / 2), and the offset of the negative is fallen to OFFSET - (OFFSET / 2). In my experience on developing, the current consumption with a 9V battery (actual approx. 8.6V) is approx. 35mA.

* Now on testing the durability and searching situations on happening the malfunction with audio input and output. Ripple voltage on power causes the malfunction (VERY NOISY). The ripple occurs from static electricity and so on. Grounding with 1M ohms and ripple filters seem to be effective for the malfunction so far. Especially, in addition to C2 of Pico, a 47uF 6.3V chip, more bypass capacitors to 3V3 are needed for audio input and output (see [EEVblog](https://www.eevblog.com/2016/03/11/eevblog-859-bypass-capacitor-tutorial/)!). I used a 100uF 25V cap in parallel with C2. The gained output from PWM also tends to cause the malfunction. The 3V3 line directly connected with RP2040, and the instability of this line is critical. Besides, the internal environment of RP2040 must be stabilized. "pedal_phaser (in v0.8a)" tends to be malfunctioned. The pedal needs a lot of accesses to the internal bus by the core to communicate with SRAM and peripherals in initial settings of peripherals and the running time. RP2040 spends electric power in accessing the bus, and the change of the status in electricity may make the internal environment become wrong, especially in the time of the transient response of the electric power and the electric status of the chip. By adding the sleep time, "PEDAL_PHASER_TRANSIENT_RESPONSE", after booting and before setting peripherals, the pedal seems to be stabilized so far. The chip may need extensive time to be stabilized for SRAM and peripherals in the manner of the electric status. "PEDAL_PHASER_TRANSIENT_RESPONSE" may differ by the time constant of the additional capacitance with C2, e.g., more capacitance needs more time to pass through the transient response.

* I used "static" state for arrays on headers which is made by Python 3. "static" state saves SRAM space because binaries of arrays aren't stored in SRAM. However, "static" state needs to access to an external flash memory, and the M0+ bus is only one, i.e., the bus is burdened by both instructions and data, causing the malfunction of RP2040. [For more detail, see Tricky XIP](#tricky-xip).

* Pedals decides the middle point of the wave by the moving average. The number of moving average is fixed. The more number is slow to move the middle point. Whereas, the less number is fast, but is effected by the large peak of the wave.

* I use a number table of a normal distribution for the correction to become the natural sound at each pedal. The normal distribution can be aliased as Gaussian distribution, i.e., this correction is well-known in the world of image processing.

* ADC0 inputs an audio signal. Whereas, ADC1 and ADC2 input values of potentiometers to control effects.

* The assignment of GPIO pins is configurable with CMakeLists.txt in lib/util_pedal_pico. GPIO numbers in descriptions as below are the default assignation.

* GPIO9 (Switch-2) and GPIO10 (Switch-1) are inputs to make a three-point switch. Caution that Switch-1 and Switch-2 are internally pulled up (with 50K ohms according to page 633 of RP2040 Datasheet). The external resistor, such as 10K ohms resistor, is preferred to grounding each GPIO. Several pedals have output modes using this switch.

* I set PWM frequency to 28125Hz and the system clock of Pico to 115200Khz. Nyquist frequency with 14062.5Hz and the resolution with 12-bit seem to be insufficient for the good quality of the output sound. However, these pedals are intended to be used in performances, and pedals used to be powered by batteries. In this circumstance, aiming low energy for pedals is very important as well as the sound quality. Besides, reducing 7.84 percents of the system clock improves the stability of the tiny chip in term of electricity. The inner latency in the chip equals 1 divided by 28125, i.e., 36 micro seconds.

* Each pedal has two PWM outputs. One of outputs is inverted for balanced monaural. Applying with balanced monaural is useful for reducing common-mode noise from the power and the digital chip. This helps not only the audio quality, but also the stability of the chip because of no amplification of noise from the chip.

* About jitter noise, Pico is mainly depending on the crystal oscillator and the PLL. I tried not to change the interval to sample sound. I think the noise is reduced well in the chip.

* The extension header, "util_pedal_pico_ex.h", has number tables. Not to meet duplicate declaration in compiling, the header is placed in files which include "main" functions, but not libraries. Note that in the primary "util_pedal_pico.h", number tables are referred with the "extern" keyword.

#### pedal_multi

* Executable, "pedal_multi" has multiselection. You can select 16 effects by GPIO11 (Bit 0), GPIO15 (Bit 1), GPIO12 (Bit 2), and GPIO14 (Bit 3). Note that these pins are pulled up, and detect low states as bit-set.
  * 0: Buffer
  * 1: Sideband
  * 2: Chorus
  * 3: Reverb
  * 4: Room Reverb
  * 5: Tape
  * 6: Phaser
  * 7: Planets
  * 8: Distortion
  * 9: Dist Reverb
  * 10: Dist Planets
  * 11: Fuzz Planets
  * 12: Room Reverb (Reservation)
  * 13: Tape (Reservation)
  * 14: Phaser (Reservation)
  * 15: Planets (Reservation)

* Buffer implements a noise gate with -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF. ADC_VREF is typically 3.3V, and in this case the gate cuts 3.2mVp-p to 48mVp-p. The noise gate has the combination of the hysteresis and the time counting after triggering. I set the hysteresis is the half of the threshold, and the time counting is fixed. Note that the time counting effects the sustain. ADC0 is for the audio input, ADC1 is for the sustain time of the noise gate, and ADC2 is for the threshold of the noise gate. There are output modes. The low state on Switch-1 sets the high attack, and the low state on Switch-2 sets the low attack and the feedback at the sustain.

* Sideband is hinted by a rotating fan which changes your voice. This effect can be described by a Fourier transform, i.e., you add a pulsating wave to produce a sideband like a radio wave. I think this effect is identified as an octave pedal. It's made from the mouse-on-a-wall approach for the chorus effect with my idea. I recommend this effect not just for guitars, but also for basses. Fundamentally, this effect is like the system of synthesizers. To make this function on an analogue circuit, this would be a ring modulator. Outputs are added the pulsating wave. ADC0 is for the audio input, ADC1 is for the speed of the oscillator, and ADC2 is for the threshold to start the oscillator. Frequencies of main pulsating wave and sub pulsating wave are fixed so far. There are output modes. The low state on Switch-1 sets the low gain, and the low state on Switch-2 sets the high gain.

* Chorus is using a delay without feedback. However, to get the spatial expanse, we need an oscillator and another delay, i.e., this effect is simulating a pair of stereo speakers which output the delay alternately with an oscillator, and the function of the speakers is also simulating a rotary speaker too. Besides, a rotary speaker is also simulating sound reflections in a hall too. ADC0 is for the audio input, ADC1 is for the speed of the oscillator, and ADC2 is for the distance between L and R. One of two outputs is L, and another is R which is delayed to simulate the distance of two speakers (another is also inverted for balanced monaural). There are output modes to change the delay time.

* Reverb (reverberation) is using a delay with feedback. ADC0 is for the audio input, ADC1 is for the mixing rate of the dry = current and the wet = delay (Dial 0 is the loudest volume), and ADC2 is for the room size (delay time).

* Room Reverb is the combination of the Reverb and the Chorus. ADC0 is for the audio input, ADC1 is for the delay time of the Reverb, and ADC2 is for the speed of chorus' oscillator. There are output modes to change the reflection and the room size. As a result, the Chorus is like sound reflections of a room. I thought this combination makes natural reverberation in a room. The sound of this effect is like in a tall lobby of a shopping mall, i.e., there is a lot of factors to reflect. Whereas, the sound of the Chorus is like in an open lobby of a hotel.

* Tape is using a delay with feedback. However, to simulate the glitch of the tape double-tracking, the delay time is swung by an oscillator. By the swung delay time, the pitch is swung up and down because the music wave is shrunk and stretched. Note that this type of vibrations with changing the pitch of a note, but not the volume, is not preferred in instrumental ensembles and chorus ensembles because the changed pitch generates a discord. By dialing 10 to the depth, and controlling the speed knob, you can listen the sound like vinyl scratching (but a little wet). ADC0 is for the audio input, ADC1 is for the swing depth, and ADC2 is for the speed of the oscillator.

* Phaser is using an all-pass filter. This sweeps the coefficient of the function to reduce frequencies over a frequency in the sound by phase shifting. ADC0 is for the audio input, ADC1 is for the speed of the oscillator to sweep, and ADC2 is for the swing depth. There are output modes to change the depth of the Phaser (it's the delay time in fact).

* Planets is using a high-pass filter. ADC0 is for the audio input, ADC1 is for the coefficient of the filter, and ADC2 is for the frequency of the filter. Unlike the Phaser, this effect doesn't have any oscillator. There are output modes. The low state on Switch-1 sets low-pass filter mode. The low state on Switch-2 sets band-pass filter mode. The world of wah-wah says a band-pass filter is essential, but in my experience, the high-pass filter is the best for wah-wah. Caution that this effect easily makes resonance by high level input on the high value of the coefficient.

* Distortion is simulating non-linear amplification. ADC0 is for the audio input, ADC1 is for the level of the output. There are output modes. The low state on Switch-1 sets the fuzz mode. The low state on Switch-2 sets the high distortion mode.

* Dist Reverb is the combination of the Distortion and the Reverb (reverberation). ADC0 is for the audio input, ADC1 is for the mixing rate of the dry = current and the wet = delay (Dial 0 is the loudest volume), and ADC2 is for the room size (delay time). There are output modes. The low state on Switch-1 sets fuzz mode. The low state on Switch-2 sets high distortion mode.

* Dist Planets is the combination of the Distortion and the Planets. ADC0 is for the audio input, ADC1 is for the coefficient of the filter, and ADC2 is for the frequency of the filter. There are output modes (low-pass/high-pass/band-pass filter). The mode of the Distortion is fixed to the high distortion mode.

* Fuzz Planets is almost the same as Dist Planets, but the mode of the Distortion is fixed to the fuzz mode.

#### pedal_looper

* Executables, "pedal_looper" is a multi-track recording tool for approx. 29 seconds. ADC0 is for the audio input, ADC1 is for the level of the output and recording. Switch-1 acts as the button-1, and Switch-2 acts as the button-2, i.e., push for the low state, and release for the high state. On the first power-on, the pedal erases data in the region of the external flash memory for recording (blinking GPIO8 during erasing data in default). Hold the button-1 for two seconds also erases all data. After erasing data, the status of the pedal goes on pending. After power-on, the pedal also goes on pending. Push the button-2 to release from pending, and play existing sound tracks. Push the button-2 again to record a new track with existing tracks from the start (turning on GPIO8 during recording in default). To stop recording, push the button-2, then the pedal starts to play tracks that the new track is added. To back to pending, push the button-1 (during recording, the button-1 isn't functioned). By backing to pending, the time to rewind is also reset. Note that the space for 29 seconds in the external flash memory can be allocated by storing the instruction code to SRAM on booting. The space in the flash memory is iteratively rewritten for recording. By attaching a microphone, this pedal would be a multi-track voice memo. Even in 2021, this voice memo can be a local media in a real community.

### QSPI Flash

* This is a test for reading and writing data from/into the external flash chip. The code of procedures to access the chip is included in the ROM of RP2040. The peripheral to access is SSI in XIP block (see the page 586 to 625 of RP2040 Datasheet). Contributed descriptions in RP2040 Datasheet also helps to know the system of XIP.

## Technical Notes

**I/O**

```C
uint32 from_time = time_us_32();
printf("@main 1 - Let's Start!\n");
pedal_buffer_debug_time = time_us_32() - from_time;
printf("@main 2 - pedal_buffer_debug_time %d\n", pedal_buffer_debug_time);
```

* In this code (in the old version of pedal_buffer) as above, the time showed approx. 350 micro seconds with the embedded USB 2.0. However, USB 2.0 becomes a popular middle range communicator. The recommended length of a USB 2.0 cable is up to 5 meters (16 feet), but you can purchase a USB 2.0 cable which has a signal booster, and the length is 10 meters (32 feet). Note that approx. 100 micro seconds with UART.

* When you output the audio signal with PWM on approx. 30518Hz (12-bit resolution per cycle with the 125Mhz clock speed), you are allowed to spend approx. 30 micro seconds for a cycle. The 125Mhz clock speed spends 0.008 micro seconds per clock, and 30 micro seconds includes 3750 clocks. These clocks may be enough on processing with integer, but may not be enough on processing with floating point decimal. To utilize decimal, you can use fixed point decimal, and a number table with expected values that is already calculated by functions such as sine. For example:

```C
uint32 from_time = time_us_32();
pwm_set_chan_level(func_pwm_slice_num, func_pwm_channel, function_generator_pico_sine(function_generator) + FUNC_PWM_OFFSET);
if(function_generator->is_end) {
    function_generator->factor = func_next_factor;
    function_generator->is_end = false;
}
func_debug_time = time_us_32() - from_time;
```

* In this coding as above the time showed 9-10 micro seconds. 2-3 times are allowed to use the sine function in the 30 seconds.

* On outputting PWM with quantized audio signal by ADC, the noise is not only from 30518Hz at PWM cycle, but also from the misalignment of ADC sampling. The noise from the misalignment of the sampling is under 30518Hz, and it could pass your low-pass filter. In PWM IRQ, the timing to start ADC sampling should be stable. Conditional branches before starting ADC sampling cause the misalignment.

* RP2040 has 5 ADC inputs (ADC0-4), and ADC4 is dedicated for a implemented temperature sensor. Pico can be freely used 3 ADC inputs, and ADC3 is connected to the VSYS/3 measurement (see page 7 and page 24 of Raspberry Pi Pico Datasheet). Third-party RP2040 hardware may be able to use 4 ADC inputs. You also see page 579 of RP2040 Datasheet to check the note which describes existence of diodes on ADC inputs. However, for safety, you need a zener diode to GND on the audio input. The input impedance is 100K ohms at minimum (see page 634 of RP2040 Datasheet). This impedance is small as an input from a coil pick up of an electric guitar, and the direct connection apparently reduces the harmonics in my experience.

* Pico executes commands from the external flash memory in default (see page 273 of Raspberry Pi Pico C/C++ SDK). This means the external flash memory is always busy, and it seems to be difficult to access as a data storage like SRAM. For example, data arrays with "static" modifiers don't occupy the space of SRAM. However, in my experience, the modifier seems to cause the malfunction because Pico accesses to the flash memory for both instructions and data on a bus at the same time. The solution is use immediate values embedded in instructions. [For more detail, see Tricky XIP](#tricky-xip).

* See the Atomic Register Access in the page 18 of RP2040 Datasheet, and offsets of addresses for peripherals make atomic accesses, XOR, SET, and CLEAR.

* In making code, I'm approaching with object-oriented thought even in C language. In my view of object-oriented equals standardization, i.e., the unique code in executables should be minimum, and the common code in libraries should be maximum. Watch out the change between v0.8a and v0.9b, and codes of executables in pedal are standardized into libraries. As a result, I integrated pedals into [pedal_multi](#pedal_multi).

**C Language**

* C language needs strict declarations of variable types. Unlike Python, which converts the variable type by functions explicitly, the declaration is implicitly kept through the life of the variable. To change the variables types in C language, we need to make a type casting. For example:

```C
/**
 * Using 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part:
 * In the calculation, we extend the value to 64-bit signed integer because of the overflow from the 32-bit space.
 * In the multiplication to get only the integer part, 32-bit arithmetic shift left is needed at the end because we have had two 16-bit decimal part in each value.
 */
delay_1 = (int32)(int64)(((int64)(delay_1 << 16) * (int64)pedal_chorus_delay_amplitude) >> 32); // Two 16-bit Decimal Parts Need 32-bit Shift after Multiplication
int32 delay_1_l = (int32)(int64)(((int64)(delay_1 << 16) * (int64)abs(fixed_point_value_sine_1)) >> 32);
int32 delay_1_r = (int32)(int64)(((int64)(delay_1 << 16) * (int64)(0x00010000 - abs(fixed_point_value_sine_1))) >> 32);
```

* The variable, "delay_1", is a 32-bit signed integer. However, in a multiplication of the fixed point decimal, we need to cast the variable to 64-bit signed integer. Otherwise, the two's compliment expression for 64-bit is broken. Besides, the next example is dangerous:

```C
uint16* example_array = (uint16*)calloc(5, sizeof(uint16)); // stdlib.h
int16 example = -10;
example_array[1] = example;
if (example_array[1] < 0) printf("It's Negative!");
printf("Value: %d", example_array[1]);
free(example_array); // Don't forget to Release Memory Space
```

* This code doesn't printed out the message because the array points variables with 16-bit unsigned integer. The values is printed as 65526.

## Tricky XIP

* In religious studies, trickster is a tool to cultivate religion among believers. The matter is not on the study, but not on people who used to abandon philosophy and forget the study. Philosophy is a critical thought.

* Several discussions have been already held in the Raspberry Pi forum.

* [XIP a near-useless feature?](https://www.raspberrypi.org/forums/viewtopic.php?f=144&t=300125)

* I think it's a sort of the zenith of the complexity in developing technologies (note that I like city simulators rather than civilization simulators). In fact, Cortex M0+ has von Neumann architecture which doesn't separate memory space between data and instruction (note that renowned MPUs, Intel 8080 and Z80 have this architecture). However, in the high-level usage such as C language, the chip is used as if it has Harvard architecture which separates memory space because modern MPUs, such as AVR, used to have Harvard architecture. This trick came true by XIP (Execute in place) which has instruction cache with an external flash memory. Note that the flash memory is also accessible as a data storage without any restriction because M0+ has only one set of instruction system for SRAM and memory mapped peripherals including XIP. In the recent context, Harvard architecture is described for its performance, but this architecture is also known as having the safe space for instructions, i.e., the instruction code can't easily corrupt the code itself by memory overflow, etc. In assembler, Pico is just a von Neumann. However, in pico-sdk with C language, Pico is a Harvard-like. Just in my experience, reading data from the flash memory on using XIP seems to cause the time delay from the cache miss for instructions. Besides, although I don't have evidence that XIP causes, but the malfunction even occurs at the same program. Caution that this trick causes a critical situation because the plotter of an application doesn't recognize that the von Neumann has memory space for XIP which may destroy the premised system of the application. Unintended time delay on execution is like a weak bolt which pretends specifications of a machine. Besides, the coder of an application doesn't recognize that the chip has von Neumann that may destroy instruction code by memory overflow, etc. I have to say that XIP is not an intended tool in a civilization game. The region for XIP is aliased by caching status (see page 25 and 150 of RP2040 Datasheet). However, if you make a static array in the flash memory with C language, the C ignores aliasing because C considers RAM and ROM as a primary storage, and the flash memory is a secondary storage even if it's fast as a secondary which doesn't have any cache. In fact, the modern technology publishes nonvolatile SRAM. However, XIP is like a virtual memory space using a cache which causes the time delay. I remember a third party software for Windows 95 to make "optimized" virtual memory space. The system of virtual memory, allocating a space to a HDD, is implemented in the multi-task OS as a hedge against memory overflow. We need to know whether virtual memory is optimized or not, it's slow in the manner of unplottable. Cortex M33, the new to low energy, seems to improve this issue and this is certified as a Harvard architecture. However, although I haven't ever touched M33, the instruction set to access memory space for data and instruction is the same as opposed to AVR. This simplification is noticeable in terms of the possible corruption of instruction code. Note that Cortex M23, a new suggestion to low energy and low cost, has von Neumann, and M23 has similar performance to Cortex M0+. The difference of the 23 and the 33 is bigger than the number of 10. XIP has been used even in the middle performance line-ups such as Cortex M4. I think another reason of popularity of XIP is probably scalability of the chip. Silicon dies in chips can be small by reducing the size of SRAM. Low-energy chips are aimed to be used in things such as a part of an automotive. Several modern chips which includes flash memory for instruction use von Neumann.

* You can back to v0.8a and build. Watch how XIP is read in "pedal_phaser.dis":

```
1000042e:	4cb0      	ldr	r4, [pc, #704]	; (100006f0 <pedal_phaser_on_pwm_irq_wrap+0x394>)
10000430:	0089      	lsls	r1, r1, #2
10000432:	5909      	ldr	r1, [r1, r4]
```
* The address 100006f0 stores:

```
100006f0:	10005374 	.word	0x10005374
```

* On 1897th and 1898th lines in "pedal_phaser.elf.map":

```
 .rodata.pedal_phaser_table_sine_1
                0x10005374    0x3b9b0 CMakeFiles/pedal_phaser.dir/pedal_phaser.c.obj
```

* ".rodata" section is for EEPROM and other types of ROM which aren't assumed with any cache except a sort of buffers. In "pedal_phaser_make_header.py" at the source folder:

```python
declare_sine_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"static int32 pedal_phaser_table_sine_1[] = {\n"
]
```

* In this version, I met the "so noisy" malfuction with pedal_phaser. 0x10005374 in RP2040 is in the space for XIP with cacheable and allocating status (see page 150 of RP2040 Datasheet). Caution that XIP is an instruction cache, i.e., reading data from XIP from this status can rewrite instruction code on the cache. The size of the cache is 16KB. I assume the block size is 16-bit and the the number of sets is 8192. If the cache has four ways and 1 bit offset, the index would be Bit[11:1] of an address. Addresses, 0x10000374 and 0x10001374 would be shared with 0x10005374. In "pedal_phaser.dis":

```
1000035c <pedal_phaser_on_pwm_irq_wrap>:
...
10000374:	4acd      	ldr	r2, [pc, #820]	; (100006ac <pedal_phaser_on_pwm_irq_wrap+0x350>)
```

* pedal_phaser_table_sine_1 is an array with 61036 words, 244144 bytes. If XIP tries to cache this amount of data at the same time, all cached instructions are replaced with the data of the array. Although reading this array is in a routine of a PWM IRQ handler, the conflict between the array data and the instruction code possibly occurs. This description as above just tells that cache miss and non-synchronized data occur. We need another evidence to know whether the data would be executed instead of instruction code or not. If each cache block has a tag to know an address, this glitch wouldn't occur. However, we need to beware of the cache is for instruction code which is sequential. Meanwhile, XIP could distinguish between data and instruction and bypasses the cache even if your assigned address is cacheable and allocating, i.e., the fault of pedal_phaser isn't caused from XIP.

* I'm not in favor of the concept of XIP because the time delay affects the performance of pedals, and I pursue low energy consumption. However, the solution may become from an aliased region. The region starting from 0x13000000 bypasses the cache. Offsetting 0x3000000 to the static array may resolve this issue, but I haven't tested it yet. Besides, If we turn off XIP's cache unit, this could be an ordinal flash storage with QSPI (4-bit parallel interface). I use "PICO_COPY_TO_RAM" not to execute code from the flash storage.

* I made a new branch, "bugtrack_1" to evaluate this bug. In the debug code for pedal_phaser, I experienced offsetting 0x3000000 to XIP BASE stops the noise. Comparing to my updated one to utilize SRAM for instruction code which runs the process in the IRQ around 7 to 8 micro seconds (the updated code runs with 115.2Mhz), the debug code delays up to 10 micro seconds (the debug code runs with 125Mhz). However, I should notice that the measured time delay is not significantly changed by the difference of the offset on XIP, and the instruction code stays running after making the noise, i.e., XIP unit seems to be functioned correctly. SO, WHY IT MAKES NOISE?

* I used gdb-multiarch with pedal_phaser.elf to search the phenomenon in the inner bus by a bus performance counter (see page 17-24 of RP2040 Datasheet). In the debug code, XIP is actually contested with other users, and it would made the delay (I counted up xip_main_contested, 0x10). To know how much XIP accesses the bus, you can also count xip_main, 0x11. In the updated code, although APB access is contested as well, XIP is not used and counted 0. See page 15 to 16 of RP2040 Datasheet to know the bus fabric. The bus fabric is just multilayered buses using a crossbar technique (a bus in the chip is just like the bus on the board with Intel 8080). As long as using a crossbar technique, funneling accesses to a block causes collisions. You can also check SRAM blocks are split by 6 blocks and ports, and this structure aims not to funnel accesses to a block. RP2040 has two cores, i.e., the two accesses to XIP at the same time.

* 10 Seconds Run after "load", and "monitor reset init" (by Hand: 10.17) with 0x0000000 XIP Offset (Rate: 0.99720):

```
(gdb) set {int}0x1400000C = 0
(gdb) set {int}0x14000010 = 0
(gdb) x/1dw 0x1400000C
0x1400000c:	0
(gdb) x/1dw 0x14000010
0x14000010:	0
(gdb) c
Continuing.
...
Thread 1 received signal SIGINT, Interrupt.
...
(gdb) x/1dw 0x1400000C
0x1400000c:	195491772
(gdb) x/1dw 0x14000010
0x14000010:	196041286
```

* 10 Seconds Run after "load", and "monitor reset init" (by Hand: 10.10) with 0x3000000 XIP Offset (Rate: 0.99828):

```
(gdb) set {int}0x1400000C = 0
(gdb) set {int}0x14000010 = 0
(gdb) x/1dw 0x1400000C
0x1400000c:	0
(gdb) x/1dw 0x14000010
0x14000010:	0
(gdb) c
Continuing.
...
Thread 1 received signal SIGINT, Interrupt.
...
(gdb) x/1dw 0x1400000C
0x1400000c:	181038688
(gdb) x/1dw 0x14000010
0x14000010:	181350691
```

* 0x1400000C is XIP: CTR_HIT, and 0x14000010 is XIP: CTR_ACCESS. Amazingly, CTR_ACCESS includes noncacheable accesses (see page 154 to 155 of RP2040 Datasheet), and comparing two values makes the rate of cache hit and overall accesses rather than the rate of cache miss. Although you can also check around 200 millions accesses in 10 seconds (Note that the clock is 125Mhz for two cores, and we assume up to 125 millions instruction in 1 seconds on each core), the rate on offsetting 0x300000 is slightly higher than the rate on offsetting 0x0000000. The difference is 0.00108. I tested as follows.

* 20.16 with 0x0000000 Offset (Rate: 0.99743) Started Noise in Testing Term:

```
(gdb) x/1dw 0x1400000C
0x1400000c:	359630899
(gdb) x/1dw 0x14000010
0x14000010:	360556823
```

* 20.17 with 0x3000000 Offset (Rate: 0.99823):

```
(gdb) x/1dw 0x1400000C
0x1400000c:	349198359
(gdb) x/1dw 0x14000010
0x14000010:	349817949
```

* 30.13 with 0x0000000 Offset (Rate: 0.99736) Started Noise in Testing Term:

```
(gdb) x/1dw 0x1400000C
0x1400000c:	520329589
(gdb) x/1dw 0x14000010
0x14000010:	521707721
```

* 30.12 with 0x3000000 Offset (Rate: 0.99823):

```
(gdb) x/1dw 0x1400000C
0x1400000c:	521011101
(gdb) x/1dw 0x14000010
0x14000010:	521934099
```

* The difference is 0.0008 and 0.00087. This result shows increasing 0.08 percents of cache miss by accessing cacheable and allocatable. In 30 seconds, 400000 misses are increased and in a PWM wrapping cycle (30518Hz), 0.43 misses increased. This code has 3 data accesses to XIP region in a PWM wrapping cycle. The procedure, how the cache miss can makes the noise, is not revealed. I searched "execute-in-place" keyword in the Patent Search of [The Unites States Patent and Trademark Office](https://uspto.gov), and confirmed that this technology is outstanding. You can check references of these patents to know what this technology is. Besides, see the page 613 to 614 of RP2040 Datasheet to know the actual bus (APB) phenomenon in the chip. I think the technology of XIP is simple, and depending on the clock speed of the bus for the flash memory. In papers online, the technology of XIP tends to be emphasized as cost-efficient comparing to SRAM. I'm a not a board developer who supplies to factories, but to avoid glances from purchasers of factories, I would consider to cut invisible abilities of the chip. I point out that the code in the chip doesn't exhaust the ability of the MPU in the chip. MPU waits interrupts, and even sleeps. Especially, on synchronizing data, MPU has to wait for completion of transactions of data, i.e., MPU can't finish an instruction for a cycle. This is a reason the issue of the time delay isn't risen so far. XIP seems to be save the cost without tradeoff, but in fact, it has a possible time delay.

* My hypothesis is that the noise is caused by the XIP unit. The cache miss via data access might make a trouble on the distribution system in the instruction cache of the XIP unit. I monitor Pico which sounds the noise using GDB. However, on the PWM IRQ handler, I haven't find out any register and memory space which tells any significant malfunction. You can see the page 11 and 61 to 62 of RP2040 Datasheet and confirm that the serial wire debug (SWD) is directly connected with cores, i.e., the phenomenon under the bus fabric is difficult to monitor. Reading XIP is fast as well as SRAM, but behind the interface of reading XIP, there is the external bus connected to the flash memory. It's like a bottle-neck which is on a branch of a tree, but the branch is actually big as well as its stem.

* The main issue of XIP is caused from its speculative handling against a time delay. In real-time processing, a time delay triggers a malfunction of the system you made. I should say speculativeness in a risk management process is not allowed at all. XIP is a significant selection by the concurrent semiconductor industry, and this selection tells how the industry considers a risk, i.e., just an avoidable matter. We know a risk is an inevitable matter in this real world. I also notice that I reviewed one product, and commonization of this issue is a wild argument. The combination of von Neumann and XIP flash memory is a solution for tough scalability that is customers' order. Customers want the painting of the future comes true, and the semiconductor industry just accepts the order, but it's limited to technologies on our hands. The trickster in this time is a designer which made the painting get close to things in the real world. 

## Version History

* 0.9 Beta (v0.9b) - 05/21/2021
  * Tools and Versions:
    * pico-sdk 1.1.2
    * gcc version 7.3.1 20180622 (release) [ARM/embedded-7-branch revision 261907] (15:7-2018-q2-6)
    * libstdc++-arm-none-eabi-newlib 15:7-2018-q2-5+12
    * GNU assembler version 2.31.1 (arm-none-eabi) using BFD version (2.31.1-11+rpi1+11) 2.31.1
    * binutils-arm-none-eabi (Including GNU assembler) 2.31.1-11+rpi1+11
    * cmake version 3.16.3
    * GNU Make 4.2.1
    * Python 3.7.3
    * Raspbian GNU/Linux 10 (buster)
  * Evaluate Minor Bugs on Software
  * Test with Developing Hardware

* 0.8 Alpha (v0.8a) - 04/20/2021
  * Tools and Versions:
    * pico-sdk 1.1.2
    * gcc version 7.3.1 20180622 (release) [ARM/embedded-7-branch revision 261907] (15:7-2018-q2-6)
    * libstdc++-arm-none-eabi-newlib 15:7-2018-q2-5+12
    * GNU assembler version 2.31.1 (arm-none-eabi) using BFD version (2.31.1-11+rpi1+11) 2.31.1
    * binutils-arm-none-eabi (Including GNU assembler) 2.31.1-11+rpi1+11
    * cmake version 3.16.3
    * GNU Make 4.2.1
    * Python 3.7.3
    * Raspbian GNU/Linux 10 (buster)
  * Evaluate Major Bugs on Software
  * Evaluate Performances of Chip and Board

## Links of References

* [Getting Started with RP2040 – Raspberry Pi](https://www.raspberrypi.org/documentation/rp2040/getting-started/): Downloadable Documentation

* [How to add a reset button to your Raspberry Pi Pico](https://www.raspberrypi.org/blog/how-to-add-a-reset-button-to-your-raspberry-pi-pico/)

* [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

* [TinyUSB](https://github.com/hathach/tinyusb): Pico Sdk implements USB host and USB device libraries of TinyUSB.

* [Bare Metal Examples for Pico by David Welch](https://github.com/dwelch67/raspberrypi-pico)

