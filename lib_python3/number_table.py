#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import math
import numpy
from scipy.stats import norm

def makeTablePdf(header, number_pdf_length, number_pdf_halfwidth, number_pdf_scale):
    list_pdf = norm.pdf(numpy.linspace(-number_pdf_halfwidth, number_pdf_halfwidth, number_pdf_length), scale=number_pdf_scale);
    max_pdf = max(list_pdf)
    for i in range(number_pdf_length):
        header.write("    ")
        floating_point_value = list_pdf[i] / max_pdf # Floating Point Decimal
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
        header.write(str(hex(fixed_point_value & 0xFFFFFFFF)))
        if i != number_pdf_length - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")
    
def makeTableSine(header, number_sine): # Make Sine Values in 0-360 Degrees
    for i in range(number_sine):
        header.write("    ")
        floating_point_value = math.sin((i/number_sine) * 2 * math.pi) # Floating Point Decimal
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
        header.write(str(hex(fixed_point_value & 0xFFFFFFFF)))
        if i != number_sine - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

def makeTableSineHalf(header, number_sine): # Make Sine Values in 0-180 Degrees
    for i in range(number_sine):
        header.write("    ")
        floating_point_value = math.sin((i/number_sine) * math.pi) # Floating Point Decimal
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
        header.write(str(hex(fixed_point_value & 0xFFFFFFFF)))
        if i != number_sine - 1:
            header.write(",")
        header.write("\n")
    header.write("};\n\n")

