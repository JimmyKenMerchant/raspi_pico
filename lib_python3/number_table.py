#!/usr/bin/env python3

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
def makeTableNormPdf(header, length, halfwidth, mean, variance, height):
    list_pdf = norm.pdf(numpy.linspace(-halfwidth, halfwidth, length), loc=mean, scale=variance);
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
def makeTableLog(header, length, reach, number_log, height): # Make Logarithm
    list_x = numpy.linspace(1, reach, length)
    list_log = []
    for i in range(length):
        list_log.append(math.log(list_x[i], number_log))
    max_log = max(list_log)
    for i in range(length):
        floating_point_value = (list_log[i] / max_log) * height # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
# length: Number of Array
# reach: Number to Reach from 0
# height: Multiplier
def makeTablePower(header, length, reach, height): # Make Power
    list_x = numpy.linspace(0, reach, length)
    list_power = []
    for i in range(length):
        list_power.append(list_x[i] * list_x[i])
    max_power = max(list_power)
    for i in range(length):
        floating_point_value = (list_power[i] / max_power) * height # Floating Point Decimal
        writeFixedDecimal(header, floating_point_value)
        if i != length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

# header: Object to Be Written
# length: Number of Array
# height: Multiplier
def makeTableRightTriangle(header, length, height): # Make Right Triangle
    list_power = numpy.linspace(0, height, length)
    for i in range(length):
        floating_point_value = list_power[i] # Floating Point Decimal
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

