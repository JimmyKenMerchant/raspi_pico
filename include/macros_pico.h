/**
 * Written Codes:
 * Copyright 2021 Kenta Ishii
 * License: 3-Clause BSD License
 * SPDX Short Identifier: BSD-3-Clause
 *
 * Raspberry Pi Pico SDK:
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MACROS_PICO_H
#define _MACROS_PICO_H 1

#ifdef __cplusplus
extern "C" {
#endif

/********************************
 * Unique Definitions on General
 ********************************/

/* Constants */

#define _RAP(...) __VA_ARGS__

#define _1_BIG(x) x
#define _2_BIG(x) x x
#define _3_BIG(x) x x x
#define _4_BIG(x) x x x x
#define _5_BIG(x) x x x x x
#define _6_BIG(x) x x x x x x
#define _7_BIG(x) x x x x x x x
#define _8_BIG(x) x x x x x x x x
#define _9_BIG(x) x x x x x x x x x
#define _10_BIG(x) x x x x x x x x x x
#define _11_BIG(x) x x x x x x x x x x x
#define _12_BIG(x) x x x x x x x x x x x x
#define _13_BIG(x) x x x x x x x x x x x x x
#define _14_BIG(x) x x x x x x x x x x x x x x
#define _15_BIG(x) x x x x x x x x x x x x x x x
#define _16_BIG(x) x x x x x x x x x x x x x x x x
#define _17_BIG(x) x x x x x x x x x x x x x x x x x
#define _18_BIG(x) x x x x x x x x x x x x x x x x x x
#define _19_BIG(x) x x x x x x x x x x x x x x x x x x x
#define _20_BIG(x) x x x x x x x x x x x x x x x x x x x x
#define _21_BIG(x) x x x x x x x x x x x x x x x x x x x x x
#define _22_BIG(x) x x x x x x x x x x x x x x x x x x x x x x
#define _23_BIG(x) x x x x x x x x x x x x x x x x x x x x x x x
#define _24_BIG(x) x x x x x x x x x x x x x x x x x x x x x x x x

#define _1(x) x,
#define _2(x) x,x,
#define _3(x) x,x,x,
#define _4(x) x,x,x,x,
#define _5(x) x,x,x,x,x,
#define _6(x) x,x,x,x,x,x,
#define _7(x) x,x,x,x,x,x,x,
#define _8(x) x,x,x,x,x,x,x,x,
#define _9(x) x,x,x,x,x,x,x,x,x,
#define _10(x) x,x,x,x,x,x,x,x,x,x,
#define _11(x) x,x,x,x,x,x,x,x,x,x,x,
#define _12(x) x,x,x,x,x,x,x,x,x,x,x,x,
#define _13(x) x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _14(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _15(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _16(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _17(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _18(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _19(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _20(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _21(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _22(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _23(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _24(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _25(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _26(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _27(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _28(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _29(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _30(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _31(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _32(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _33(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _34(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _35(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _36(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _37(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _38(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _39(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _40(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _41(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _42(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _43(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _44(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _45(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _46(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _47(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _48(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _49(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _50(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _51(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _52(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _53(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _54(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _55(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _56(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _57(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _58(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _59(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _60(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _61(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _62(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _63(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,
#define _64(x) x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,

#define _70(x) _60(x) _10(x)
#define _80(x) _70(x) _10(x)
#define _90(x) _80(x) _10(x)
#define _100(x) _90(x) _10(x)
#define _200(x) _100(x) _100(x)
#define _300(x) _200(x) _100(x)
#define _400(x) _300(x) _100(x)
#define _500(x) _400(x) _100(x)
#define _600(x) _500(x) _100(x)
#define _700(x) _600(x) _100(x)
#define _800(x) _700(x) _100(x)
#define _900(x) _800(x) _100(x)
#define _1000(x) _900(x) _100(x)

#define uchar8 unsigned char
#define uint16 unsigned short int
#define uint32 unsigned long int
#define uint64 unsigned long long int
#define char8 char // Use as Pointer Too, Signed/Unsigned is Typically Unknown
#define int16 short int
#define int32 long int // Use as Pointer Too, Signed/Unsigned is Typically Unknown
#define int64 long long int
#define float32 float
#define float64 double

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef exit_success
#define exit_success 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef exit_failure
#define exit_failure 1
#endif
#ifndef bool
#define bool unsigned char
#endif
#ifndef true
#define true ((bool)1)
#endif
#ifndef false
#define false ((bool)0)
#endif
#ifndef True
#define True ((bool)1)
#endif
#ifndef False
#define False ((bool)0)
#endif
#ifndef TRUE
#define TRUE ((bool)1)
#endif
#ifndef FALSE
#define FALSE ((bool)0)
#endif
#ifndef null
#define null 0
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef Null
#define Null 0
#endif

#define _wordsizeof(x) ( sizeof(x) + 3 ) / 4

#ifdef __cplusplus
}
#endif

#endif
