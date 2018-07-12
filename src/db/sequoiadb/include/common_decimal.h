/*    Copyright 2012 SequoiaDB Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
 
/*
 * Copyright 2009-2012 10gen, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BSON_COMMON_DECIMAL_H_
#define BSON_COMMON_DECIMAL_H_

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(__GNUC__) || defined(__xlC__)
    #define SDB_EXPORT
#else
    #ifdef SDB_STATIC_BUILD
        #define SDB_EXPORT
    #elif defined(SDB_DLL_BUILD)
        #define SDB_EXPORT __declspec(dllexport)
    #else
        #define SDB_EXPORT __declspec(dllimport)
    #endif
#endif

#if defined (__linux__) || defined (_AIX)
#include <sys/types.h>
#include <unistd.h>
#elif defined (_WIN32)
#include <Windows.h>
#include <WinBase.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
#define SDB_EXTERN_C_START extern "C" {
#define SDB_EXTERN_C_END }
#else
#define SDB_EXTERN_C_START
#define SDB_EXTERN_C_END
#endif

SDB_EXTERN_C_START

#define DECIMAL_SIGN_MASK           0xC000
#define SDB_DECIMAL_POS             0x0000
#define SDB_DECIMAL_NEG             0x4000
#define SDB_DECIMAL_SPECIAL_SIGN    0xC000

#define SDB_DECIMAL_SPECIAL_NAN     0x0000
#define SDB_DECIMAL_SPECIAL_MIN     0x0001
#define SDB_DECIMAL_SPECIAL_MAX     0x0002

#define DECIMAL_DSCALE_MASK         0x3FFF


#define SDB_DECIMAL_DBL_DIG        ( DBL_DIG )

//sign + dscale
//   sign  = dscale & 0xC000
//   scale = dscale & 0x3FFF


/*
 * Hardcoded precision limit - arbitrary, but must be small enough that
 * dscale values will fit in 14 bits.
 */
#define DECIMAL_MAX_PRECISION       1000
#define DECIMAL_MAX_DISPLAY_SCALE   DECIMAL_MAX_PRECISION
#define DECIMAL_MIN_DISPLAY_SCALE   0
#define DECIMAL_MIN_SIG_DIGITS      16

#define DECIMAL_NBASE               10000
#define DECIMAL_HALF_NBASE          5000
#define DECIMAL_DEC_DIGITS          4     /* decimal digits per NBASE digit */
#define DECIMAL_MUL_GUARD_DIGITS    2     /* these are measured in NBASE digits */
#define DECIMAL_DIV_GUARD_DIGITS    4

typedef struct {
   int typemod;    /* precision & scale define:  
                         precision = (typmod >> 16) & 0xffff
                         scale     = typmod & 0xffff */
   int ndigits;    /* length of digits */
   int sign;       /* the decimal's sign */
   int dscale;     /* display scale */
   int weight;     /* weight of first digit */
   int isOwn;      /* is digits allocated self */
   short *buff ;   /* start of palloc'd space for digits[] */
   short *digits;  /* real decimal data */
} bson_decimal ;

//storage detail define in bson.h  (BSON_DECIMAL)
#define DECIMAL_HEADER_SIZE  12  /*size + typemod + dscale + weight*/

#pragma pack(1)
typedef struct 
{
  int    size;    //total size of this value

  int    typemod; //precision + scale
                  //   precision = (typmod >> 16) & 0xffff
                  //   scale     = typmod & 0xffff

  short  dscale;  //sign + dscale
                  //   sign  = dscale & 0xC000
                  //   scale = dscale & 0x3FFF

  short  weight;  //weight of this decimal (NBASE=10000)

  //short  digitis[0]; //real data
} __decimal ;

#pragma pack()


SDB_EXPORT void decimal_init( bson_decimal *decimal );
SDB_EXPORT int decimal_init1( bson_decimal *decimal, int precision, 
                              int scale ) ;

SDB_EXPORT void decimal_free( bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_zero( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_zero( const bson_decimal *decimal ) ;

SDB_EXPORT int decimal_is_special( const bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_nan( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_nan( const bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_min( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_min( const bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_max( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_max( const bson_decimal *decimal ) ;

SDB_EXPORT int decimal_round( bson_decimal *decimal, int rscale ) ;

SDB_EXPORT int decimal_to_int( const bson_decimal *decimal ) ;
SDB_EXPORT double decimal_to_double( const bson_decimal *decimal ) ;
SDB_EXPORT int64_t decimal_to_long( const bson_decimal *decimal ) ;

SDB_EXPORT int decimal_to_str_get_len( const bson_decimal *decimal, 
                                       int *size ) ;
SDB_EXPORT int decimal_to_str( const bson_decimal *decimal, char *value, 
                               int value_size ) ;

// the caller is responsible for freeing this decimal( decimal_free )
SDB_EXPORT int decimal_from_int( int value, bson_decimal *decimal ) ;

// the caller is responsible for freeing this decimal( decimal_free )
SDB_EXPORT int decimal_from_long( int64_t value, bson_decimal *decimal) ;

// the caller is responsible for freeing this decimal( decimal_free )
SDB_EXPORT int decimal_from_double( double value, bson_decimal *decimal ) ;

// the caller is responsible for freeing this decimal( decimal_free )
SDB_EXPORT int decimal_from_str( const char *value, bson_decimal *decimal ) ;

SDB_EXPORT int decimal_get_typemod( const bson_decimal *decimal, int *precision, 
                                    int *scale ) ;
SDB_EXPORT int decimal_get_typemod2( const bson_decimal *decimal ) ;
SDB_EXPORT int decimal_copy( const bson_decimal *source, 
                             bson_decimal *target ) ;

int decimal_from_bsonvalue( const char *value, bson_decimal *decimal ) ;

int decimal_to_jsonstr( const bson_decimal *decimal, char *value, 
                        int value_size ) ;

int decimal_to_jsonstr_len( int sign, int weight, int dscale, 
                            int typemod, int *size ) ;

SDB_EXPORT int decimal_cmp( const bson_decimal *left, 
                            const bson_decimal *right ) ;

SDB_EXPORT int decimal_add( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_sub( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_mul( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_div( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_abs( bson_decimal *decimal ) ;

SDB_EXPORT int decimal_ceil( const bson_decimal *decimal, 
                             bson_decimal *result ) ;

SDB_EXPORT int decimal_floor( const bson_decimal *decimal, 
                              bson_decimal *result ) ;

SDB_EXPORT int decimal_mod( const bson_decimal *left, const bson_decimal *right, 
                            bson_decimal *result ) ;

int decimal_update_typemod( bson_decimal *decimal, int typemod ) ;

int decimal_is_out_of_precision( bson_decimal *decimal, int typemod ) ;

SDB_EXTERN_C_END


#endif


