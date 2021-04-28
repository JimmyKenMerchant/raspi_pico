#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import math
import numpy
from scipy.stats import norm

# header: Object to Be Written
# length: Number of Array
# halfwidth: Center to Side
# scale: Variance
# height: Maximum Height
def makeTablePdf(header, length, halfwidth, scale, height):
    list_pdf = norm.pdf(numpy.linspace(-halfwidth, halfwidth, length), scale=scale);
    max_pdf = max(list_pdf)
    for i in range(length):
        floating_point_value = (list_pdf[i] / max_pdf) * height # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
# length: Number of Array
def makeTableSine(header, length): # Make Sine Values in 0-360 Degrees
    for i in range(length):
        floating_point_value = math.sin((i/length) * 2 * math.pi) # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
# length: Number of Array
def makeTableSineHalf(header, length): # Make Sine Values in 0-180 Degrees
    for i in range(length):
        floating_point_value = math.sin((i/length) * math.pi) # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
# length: Number of Array
# reach: Number to Reach from 1
# number_log: Base Number
# height: Multiplier
def makeTableLog(header, length, reach, number_log, height): # Make Natural Log
    list_log = numpy.linspace(1, reach, length)
    for i in range(length):
        floating_point_value = math.log(list_log[i], number_log) * height # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
# length: Number of Array
# reach: Number to Reach from 0
# height: Multiplier
def makeTablePower(header, length, reach, height): # Make Natural Log
    list_power = numpy.linspace(0, reach, length)
    for i in range(length):
        floating_point_value = list_power[i] * list_power[i] * height # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
def writeFixedDecimal(header, floating_point_value):
    if floating_point_value < 0:
        is_negative = True
        floating_point_value = abs(floating_point_value);
    else:
        is_negative = False;
    floating_point_value_int = math.floor(floating_point_value) # Round Down
    floating_point_value_decimal = floating_point_value - floating_point_value_int
    fixed_point_value = 0
    for j in range (4):
        fixed_point_value |= (int(floating_point_value_int % 16) << 16) << (j * 4)
        floating_point_value_int /= 16;
    for j in range (4):
        floating_point_value_decimal *= 16
        floating_point_value_decimal_floored = math.floor(floating_point_value_decimal)
        fixed_point_value |= int(floating_point_value_decimal_floored << 16) >> ((j + 1) * 4)
        floating_point_value_decimal -= floating_point_value_decimal_floored
    if is_negative == True:
        fixed_point_value = -fixed_point_value;
    header.write("    ")
    header.write(str(hex(fixed_point_value & 0xFFFFFFFF)))

