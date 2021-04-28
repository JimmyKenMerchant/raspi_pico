#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import sys
sys.path.append("../../lib_python3")
import number_table as nt

headername = "pedal_distortion.h"

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
"#ifndef _PEDAL_DISTORTION_H\n",
"#define _PEDAL_DISTORTION_H 1\n",
"\n",
"#ifdef __cplusplus\n",
"extern \"C\" {\n",
"#endif\n",
"\n"
]

declare_pdf_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_distortion_table_pdf_1[] = {\n"
]

pdf_length_1 = 2047 # Number of Array
pdf_halfwidth_1 = 1.0 # Center to Side
pdf_scale_1 = 1.0 # Variance
pdf_height_1 = 2.0 # Maximum Height

declare_log_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_distortion_table_log_1[] = {\n"
]

log_length_1 = 2047 # Number of Array
log_reach_1 = 2.0 # Number to Reach from 1
log_number_log_1 = 2.0 # Base Number
log_height_1 = 6.0 # Maximum Height

declare_log_2 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_distortion_table_log_2[] = {\n"
]

log_length_2 = 2047 # Number of Array
log_reach_2 = 2.0 # Number to Reach from 1
log_number_log_2 = 2.0 # Base Number
log_height_2 = 10.0 # Maximum Height

declare_power_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_distortion_table_power_1[] = {\n"
]

power_length_1 = 2047 # Number of Array
power_reach_1 = 1.0 # Number to Reach from 0
power_height_1 = 1.0 # Variance

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

# Table PDF 1
header.writelines(declare_pdf_1)
nt.makeTablePdf(header, pdf_length_1, pdf_halfwidth_1, pdf_scale_1, pdf_height_1)

# Table Logarithm 1
header.writelines(declare_log_1)
nt.makeTableLog(header, log_length_1, log_reach_1, log_number_log_1, log_height_1)

# Table Logarithm 2
header.writelines(declare_log_2)
nt.makeTableLog(header, log_length_2, log_reach_2, log_number_log_2, log_height_2)

# Table Power 1
header.writelines(declare_power_1)
nt.makeTablePower(header, power_length_1, power_reach_1, power_height_1)

header.writelines(postfix)
header.close()

