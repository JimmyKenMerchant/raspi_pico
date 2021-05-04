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

* You can also use OpenOCD. Chapter 5 and 6 of "Getting started with Raspberry Pi Pico" is useful for the installation of OpenOCD and GDB, a debugger.

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
# For Multicore, "info threads", "thread 2", etc.
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

* This project outputs a sine wave from GPIO15 (PWM7 B).

* According to the page 147-148 of RP2040 Datasheet, ROM (0x0000_0000) of RP2040 includes utilities for fast floating point as firmware. The actual code is in [mufplib.S written by Mark Owen](https://github.com/raspberrypi/pico-bootrom/blob/master/bootrom/mufplib.S). Note that the binaries seems to be loaded to SRAM, and I can't check these in the disassembling file (func.dis in this case).

**Pedal**

* This project is making several types of guitar pedals.

* Caution that this project needs an analogue circuit to receive and output the audio signal, i.e., a DC bias on receiving, low-pass filters on outputting (in case of stereo outputting, this project outputs audio signals from two PWM channels).

* Now on testing the durability and searching situations on happening the malfunction with audio input and output. Ripple voltage on power causes the malfunction (VERY NOISY). The ripple occurs from static electricity and so on. Grounding with 1M ohms and ripple filters seem to be effective for the malfunction so far. Especially, in addition to C2 of Pico, a 47uF 6.3V chip, more bypass capacitors to 3V3 are needed for audio input and output (see [EEVblog](https://www.eevblog.com/2016/03/11/eevblog-859-bypass-capacitor-tutorial/)!). I used a 100uF 25V cap in parallel with C2. The gained output from PWM also tends to cause the malfunction. The 3V3 line directly connected with RP2040, and the instability of this line is critical. Besides, the internal environment of RP2040 must be stabilized. "pedal_phaser" tends to be malfunctioned. The pedal needs a lot of accesses to the internal bus by the core to communicate with SRAM and peripherals in initial settings of peripherals and the running time. RP2040 spends electric power in accessing the bus, and the change of the status in electricity may make the internal environment become wrong, especially in the time of the transient response of the electric power and the electric status of the chip. By adding the sleep time, "PEDAL_PHASER_TRANSIENT_RESPONSE", after booting and before setting peripherals, the pedal seems to be stabilized so far. The chip may need extensive time to be stabilized for SRAM and peripherals in the manner of the electric status. "PEDAL_PHASER_TRANSIENT_RESPONSE" may differ by the time constant of the additional capacitance with C2, e.g., more capacitance needs more time to pass through the transient response.

* I used "static" state for arrays on headers which is made by Python 3. "static" state saves SRAM space because binaries of arrays aren't stored in SRAM. However, "static" state needs to access to an external flash memory, and the M0+ bus is only one, i.e., the bus is burdened by both instructions and data, causing the malfunction of RP2040.

* Pedals decides the middle point of the wave by the moving average. The number of moving average is fixed. The more number is slow to move the middle point. Whereas, the less number is fast, but is effected by the large peak of the wave.

* I use a number table of a normal distribution for the correction to become the natural sound at each pedal. The normal distribution can be aliased as Gaussian distribution, i.e., this correction is well-known in the world of image processing.

* ADC0 inputs an audio signal. Whereas, ADC1 and ADC2 input values of potentiometers to control effects.

* GPIO14 and GPIO15 are inputs to make a three-point switch. Caution that GPIO14 and GPIO15 are internally pulled up (with 50K ohms according to page 633 of RP2040 Datasheet). The external resistor, such as 10K ohms resistor, is preferred to grounding each GPIO. Several pedals have output modes using this switch.

* I set PWM frequency to 28125Hz and the system clock of Pico to 115200Khz. Nyquist frequency with 14062.5Hz and the resolution with 12-bit seem to be insufficient for the good quality of the output sound. However, these pedals are intended to be used in performances, and pedals used to be powered by batteries. In this circumstance, aiming low energy for pedals is very important as well as the sound quality. Besides, reducing 7.84 percents of the system clock improves the stability of the tiny chip in term of electricity. The inner latency in the chip equals 1 divided by 28125, i.e., 36 micro seconds.

* Each pedal has two PWM outputs. One of outputs is inverted for balanced monaural. Applying with balanced monaural is useful for reducing common-mode noise.

* About jitter noise, Pico is mainly depending on the crystal oscillator and the PLL. I tried not to change the interval to sample sound. I think the noise is reduced well in the chip.

* "pedal_buffer" is a just buffer. This also implements a noise gate with -66.22dB (Loss 2047) to -36.39dB (Loss 66) in ADC_VREF. ADC_VREF is typically 3.3V, and in this case the gate cuts 3.2mVp-p to 48mVp-p. The noise gate has the combination of the hysteresis and the time counting after triggering. I set the hysteresis is the half of the threshold, and the time counting is fixed. Note that the time counting effects the sustain. ADC0 is for the audio input, ADC1 is for the sustain time of the noise gate, and ADC2 is for the threshold of the noise gate. There are output modes. The low state on GPIO14 sets the high attack, and the low state on GPIO15 sets the low attack and the feedback at the sustain.

* "pedal_sideband" is hinted by a rotating fan which changes your voice. This effect by a rotating fan can be described by a Fourier transform, i.e., you add a pulsating wave to produce a sideband like a radio wave. In my testing, sine waves have harmonics by this pedal. Note that I renamed this pedal from "pedal_chorus" to "pedal_sideband", and I think this pedal is identified as an octave pedal. It's made from the mouse-on-a-wall approach for the chorus effect with my idea. I recommend this pedal not just for guitars, but also for basses. Fundamentally, this effect is like the system of synthesizers. To make this function on an analogue circuit, this would be a ring modulator. Outputs are added the pulsating wave. ADC0 is for the audio input, ADC1 is for the speed of the oscillator, and ADC2 is for the threshold to start the oscillator. Frequencies of main pulsating wave and sub pulsating wave are fixed so far. There are output modes. The low state on GPIO14 sets the regular gain, and the low state on GPIO15 sets the high gain.

* "pedal_chorus" is using a delay without feedback. However, to get the spatial expanse, we need an oscillator and another delay, i.e., this pedal is simulating a pair of stereo speakers which output the delay alternately with an oscillator, and the function of the speakers is also simulating a rotary speaker too. Besides, a rotary speaker is also simulating sound reflections in a hall too. ADC0 is for the audio input, ADC1 is for the speed of the oscillator, and ADC2 is for the distance between L and R. One of two outputs is L, and another is R which is delayed to simulate the distance of two speakers (another is also inverted for balanced monaural). This pedal uses a lot of processes and it spends 10us with +-5us and up to 20us per sampling cycle (this measurement uses USB output that would use the inner bus).

* "pedal_reverb" is using a delay with feedback. ADC0 is for the audio input, ADC1 is for the mixing rate of the dry = current and the wet = delay (Dial 0 is the loudest volume), and ADC2 is for the room size (delay time).

* "pedal_tape" is using a delay with feedback. However, to simulate the glitch of the tape double-tracking, the delay time is swung by an oscillator. By the swung delay time, the pitch is swung up and down because the music wave is shrunk and stretched. Note that this type of vibrations with changing the pitch of a note, but not the volume, is not preferred in instrumental ensembles and chorus ensembles because the changed pitch generates a discord. By dialing 10 to the depth, and controlling the speed knob, you can listen the sound like vinyl scratching (but a little wet). ADC0 is for the audio input, ADC1 is for the swing depth, and ADC2 is for the speed of the oscillator.

* "pedal_phaser" is using an all-pass filter. This sweeps the coefficient of the function to reduce frequencies over a frequency in the sound by phase shifting. ADC0 is for the audio input, ADC1 is for the speed of the oscillator to sweep, and ADC2 is for the swing depth. There are output modes to change the depth of phaser (it's the delay time in fact).

* "pedal_planets" is using a band-pass filter. ADC0 is for the audio input, ADC1 is for the coefficient of the filter, and ADC2 is for the frequency of the filter. Unlike "pedal_phaser", this pedal doesn't have any oscillator. There are output modes. The low state on GPIO14 sets low-pass filter mode. The low state on GPIO15 sets high-pass filter mode. Note that it may be like a Wah-wah pedal. However, this uses a delay and sounds absolute digital.

* "pedal_distortion" is simulating non-linear amplification. ADC0 is for the audio input, ADC1 is for the level of the output. There are output modes. The low state on GPIO14 sets fuzz mode. The low state on GPIO15 sets high distortion mode.

## Technical Notes

**I/O**

```C
uint32 from_time = time_us_32();
printf("@main 1 - Let's Start!\n");
pedal_buffer_debug_time = time_us_32() - from_time;
printf("@main 2 - pedal_buffer_debug_time %d\n", pedal_buffer_debug_time);
```

* In this coding as above, the time showed approx. 350 micro seconds with the embedded USB 2.0. However, USB 2.0 becomes a popular middle range communicator. The recommended length of a USB 2.0 cable is up to 5 meters (16 feet), but you can purchase a USB 2.0 cable which has a signal booster, and the length is 10 meters (32 feet). Note that approx. 100 micro seconds with UART.

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

* Pico executes commands from the external flash memory in default (see page 273 of Raspberry Pi Pico C/C++ SDK). This means the external flash memory is always busy, and it seems to be difficult to access as a data storage like SRAM. For example, data arrays with "static" modifiers don't occupy the space of SRAM. However, in my experience, the modifier seems to cause the malfunction because Pico accesses to the flash memory for both instructions and data on a bus at the same time. The solution is use immediate values embedded in instructions.

**Tricky XIP**

* Several discussions have been already held in the Raspberry Pi forum.

* [XIP a near-useless feature?](https://www.raspberrypi.org/forums/viewtopic.php?f=144&t=300125)

* I think it's a sort of the zenith of the complexity in developing technologies (note that I like city simulators rather than civilization simulators). In fact, Cortex M0+ has von Neumann architecture which doesn't separate memory space between data and instruction (note that renowned MPUs, Intel 8080 and Z80 have this architecture). However, in the high-level usage such as C language, the chip is used as if it has Harvard architecture which separates memory space because modern MPUs, such as AVR, used to have Harvard architecture. This trick came true by XIP (Execute in place) which has instruction cache with an external flash chip. Note that the flash chip is also accessible as a data storage without any restriction because M0+ has only one set of instruction system for SRAM and memory mapped peripherals including XIP. In the recent context, Harvard architecture is described for its performance, but this architecture is also known as having the safe space for instructions, i.e., the instruction code can't easily corrupt the code itself by memory overflow, etc. In assembler, Pico is just a von Neumann. However, in pico-sdk with C language, Pico is a Harvard-like. Just in my experience, reading data from the flash chip on using XIP seems to cause the time delay from the cache miss for instructions, and even the malfunction. Caution that this trick causes a critical situation because the plotter of an application doesn't recognize that the von Neumann has memory space for XIP which may destroy the premised system of the application. Besides, the coder of an application doesn't recognize that the chip has von Neumann that may destroy instruction code by memory overflow, etc. I have to say that XIP is not an intended tool in a civilization game. The region for XIP is aliased by caching status (see page 25 and 150 of RP2040 Datasheet). However, if you make a static array in the flash memory with C language, the C ignores aliasing because XIP is an external peripheral and C considers of RAM or ROM as a primary storage. In fact, the modern technology publishes nonvolatile SRAM. However, XIP is like a virtual memory space using a cache which causes the time delay. Cortex M23 seems to be improved this issue and this is certified as a Harvard architecture. However, although I haven't ever touched M23, the instruction set to access memory space for data and instruction is the same as opposed to AVR. This simplification is noticeable in terms of the possible corruption of instruction code.

* You can back to v0.8a and build. Watch how XIP is read in "pedal_phaser.dis":

```
1000042e:	4cb0      	ldr	r4, [pc, #704]	; (100006f0 <pedal_phaser_on_pwm_irq_wrap+0x394>)
```
* 100006f0 is stored:

```
100006f0:	10005374 	.word	0x10005374
```

* On the 1897th and 1898th lines in "pedal_phaser.elf.map":

```
 .rodata.pedal_phaser_table_sine_1
                0x10005374    0x3b9b0 CMakeFiles/pedal_phaser.dir/pedal_phaser.c.obj
```

* In "pedal_phaser_make_header.py" at the source folder:

```python
declare_sine_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"static int32 pedal_phaser_table_sine_1[] = {\n"
]
```

* In this version, I met the "so noisy" malfuction. 0x10005374 in RP2040 is in the space for XIP with cacheable and allocating status (see page 150 of RP2040 Datasheet). Caution that XIP is an instruction cache, i.e., reading data from XIP from this status can rewrite instruction code on the cache. The size of the cache is 16KB, i.e., 0x0000 to 0x3FFF. If the cache has four ways and no bit offset, the index would be 12 least significant bits. The address, 0x10000374 and 0x10001374 would be shared with 0x10005374. In "pedal_phaser.dis":

'''
1000035c <pedal_phaser_on_pwm_irq_wrap>:
...
10000374:	4acd      	ldr	r2, [pc, #820]	; (100006ac <pedal_phaser_on_pwm_irq_wrap+0x350>)
'''

* pedal_phaser_table_sine_1 is an array with 61036 words and 244144 bytes. If XIP tries to cache this amount of data at the same time, all cached instructions are replaced with the data of the array. Although reading this array is in a routine of a PWM IRQ handler, the conflict between the array data and the instruction possibly occurs.

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

## Links of References

* [Getting Started with RP2040 – Raspberry Pi](https://www.raspberrypi.org/documentation/rp2040/getting-started/): Downloadable Documentation

* [How to add a reset button to your Raspberry Pi Pico](https://www.raspberrypi.org/blog/how-to-add-a-reset-button-to-your-raspberry-pi-pico/)

* [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

* [TinyUSB](https://github.com/hathach/tinyusb): Pico Sdk implements USB host and USB device libraries of TinyUSB.

* [Bare Metal Examples for Pico by David Welch](https://github.com/dwelch67/raspberrypi-pico)

