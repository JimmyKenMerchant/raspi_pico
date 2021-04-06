#!/usr/bin/python3

# Author: Kenta Ishii
# SPDX short identifier: BSD-3-Clause

import sys
import math

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
"#ifndef _PEDAL_CHORUS_H\n",
"#define _PEDAL_CHORUS_H 1\n",
"\n",
"#ifdef __cplusplus\n",
"extern \"C\" {\n",
"#endif\n",
"\n",
"// Fixed Point Decimal = Bit[31:16] Integer Part, Bit[15:0] Decimal Part\n",
"float32 pedal_chorus_table_sine[] = {\n"
]

postfix = [
"};\n",
"\n",
"#ifdef __cplusplus\n",
"}\n",
"#endif\n",
"\n",
"#endif\n"
]

headername = "pedal_chorus.h"

print (sys.version)
header = open(headername, "w") # Write Only With UTF-8
header.writelines(prefix)

for i in range(30518):
    header.write("    ")
    header.write(str(math.sin((i/30518) * 2 * math.pi)))
    header.write("f")
    if i != 30518 - 1:
        header.write(",")
    header.write("\n")

header.writelines(postfix)
header.close()

