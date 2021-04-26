#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import sys
sys.path.append("../../lib_python3")
import number_table as nt

headername = "pedal_buffer.h"

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
"#ifndef _PEDAL_BUFFER_H\n",
"#define _PEDAL_BUFFER_H 1\n",
"\n",
"#ifdef __cplusplus\n",
"extern \"C\" {\n",
"#endif\n",
"\n"
]

declare_pdf_1 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_buffer_table_pdf_1[] = {\n"
]

pdf_length_1 = 2047 # Number of Array
pdf_halfwidth_1 = 1.0 # Center to Side
pdf_scale_1 = 1.0 # Variance
pdf_height_1 = 0.5 # Maximum Height

declare_pdf_2 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_buffer_table_pdf_2[] = {\n"
]

pdf_length_2 = 2047 # Number of Array
pdf_halfwidth_2 = 1.0 # Center to Side
pdf_scale_2 = 1.0 # Variance
pdf_height_2 = 1.0 # Maximum Height

declare_pdf_3 = [
"// 32-bit Signed (Two's Compliment) Fixed Decimal, Bit[31] +/-, Bit[30:16] Integer Part, Bit[15:0] Decimal Part\n",
"int32 pedal_buffer_table_pdf_3[] = {\n"
]

pdf_length_3 = 2047 # Number of Array
pdf_halfwidth_3 = 1.0 # Center to Side
pdf_scale_3 = 1.5 # Variance
pdf_height_3 = 2.0 # Maximum Height

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

# Table PDF 2
header.writelines(declare_pdf_2)
nt.makeTablePdf(header, pdf_length_2, pdf_halfwidth_2, pdf_scale_2, pdf_height_2)

# Table PDF 3
header.writelines(declare_pdf_3)
nt.makeTablePdf(header, pdf_length_3, pdf_halfwidth_3, pdf_scale_3, pdf_height_3)

header.writelines(postfix)
header.close()
