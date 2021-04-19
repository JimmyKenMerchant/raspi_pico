#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import sys
import math
import numpy
from scipy.stats import norm

headername = "pedal_planets.h"

prefix = [
"/**\n",
" * Written Codes:\n",
" * Copyright 2021 Kenta Ishii\n",
" * License: 3-Clause BSD License\n",
" * SPDX Short Identifier: BSD-3-Clause\n",
" *\n",
" * Raspberry Pi Pico SDK:\n",
" * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.\n",
" *\n",
" * SPDX-License-Identifier: BSD-3-Clause\n",
" */\n",
"\n",
"/* THIS IS AN AUTOMATICALLY GENERATED HEADER FILE */\n",
"#ifndef _PEDAL_PLANETS_H\n",
"#define _PEDAL_PLANETS_H 1\n",
"\n",
"#ifdef __cplusplus\n",
"extern \"C\" {\n",
"#endif\n",
"\n"
]

declare_sine_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"static int32 pedal_planets_table_sine_1[] = {\n"
]

number_sine_1 = 61036

declare_pdf_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"static int32 pedal_planets_table_pdf_1[] = {\n"
]

number_pdf_length_1 = 2047 # Number of Array
number_pdf_halfwidth_1 = 1.0 # Center to Side
number_pdf_scale_1 = 1.0 # Variance

postfix = [
"#ifdef __cplusplus\n",
"}\n",
"#endif\n",
"\n",
"#endif\n"
]

def makeTableSine(number_sine): # Make Sine Values in 0-360 Degrees
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

def makeTablePdf(number_pdf_length, number_pdf_halfwidth, number_pdf_scale):
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

print (sys.version)
header = open(headername, "w") # Write Only With UTF-8
header.writelines(prefix)

# Table Sine 1
header.writelines(declare_sine_1)
makeTableSine(number_sine_1)

# Table PDF 1
header.writelines(declare_pdf_1)
makeTablePdf(number_pdf_length_1, number_pdf_halfwidth_1, number_pdf_scale_1)

header.writelines(postfix)
header.close()

