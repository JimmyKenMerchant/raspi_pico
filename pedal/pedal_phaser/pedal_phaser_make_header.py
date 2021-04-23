#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import sys
sys.path.append("../../lib_python3")
import number_table as nt

headername = "pedal_phaser.h"

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
"#ifndef _PEDAL_PHASER_H\n",
"#define _PEDAL_PHASER_H 1\n",
"\n",
"#ifdef __cplusplus\n",
"extern \"C\" {\n",
"#endif\n",
"\n"
]

declare_sine_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"static int32 pedal_phaser_table_sine_1[] = {\n"
]

number_sine_1 = 61036

declare_pdf_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"static int32 pedal_phaser_table_pdf_1[] = {\n"
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

print (sys.version)
header = open(headername, "w") # Write Only With UTF-8
header.writelines(prefix)

# Table Sine 1
header.writelines(declare_sine_1)
nt.makeTableSineHalf(header, number_sine_1)

# Table PDF 1
header.writelines(declare_pdf_1)
nt.makeTablePdf(header, number_pdf_length_1, number_pdf_halfwidth_1, number_pdf_scale_1)

header.writelines(postfix)
header.close()
