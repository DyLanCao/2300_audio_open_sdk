
/*
 *  Copyright (c) 2020 The wl project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifdef WL_FIR_FILTER

/*
 *  Copyright (c) 
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */



#include "hal_trace.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>  // size_t
#include <stdlib.h>
#include <string.h>


  
// maximum number of inputs that can be handled
// in one function call
#define MAX_INPUT_LEN   160
// maximum length of filter than can be handled
#define MAX_FLT_LEN     64
// buffer to hold all of the input samples
#define BUFFER_LEN      (MAX_FLT_LEN - 1 + MAX_INPUT_LEN)
 
// array to hold input samples
double insamp[BUFFER_LEN];


 
// FIR init
void firFloatInit( void )
{
    memset( insamp, 0, sizeof( insamp ) );
}
 
// the FIR filter function
void firFloat( double *coeffs, double *input, double *output,int length, int filterLength )
{
    double acc;     // accumulator for MACs
    double *coeffp; // pointer to coefficients
    double *inputp; // pointer to input samples
    int n;
    int k;
 
    // put the new samples at the high end of the buffer
    memcpy( &insamp[filterLength - 1], input,
            length * sizeof(double) );
 
    // apply the filter to each input sample
    for ( n = 0; n < length; n++ ) {
        // calculate output n
        coeffp = coeffs;
        inputp = &insamp[filterLength - 1 + n];
        acc = 0;
        for ( k = 0; k < filterLength; k++ ) {
            acc += (*coeffp++) * (*inputp--);
        }
        output[n] = acc;
    }
    // shift input samples back in time for next time
    memmove( &insamp[0], &insamp[length],
            (filterLength - 1) * sizeof(double) );
 
}

void intToFloat( int16_t *input, double *output, int length )
{
    int i;
 
    for ( i = 0; i < length; i++ ) {
        output[i] = (double)input[i];
    }
}
 
void floatToInt( double *input, int16_t *output, int length )
{
    int i;
 
    for ( i = 0; i < length; i++ ) {
        if ( input[i] > (double)32767.0 ) {
            input[i] = (double)32767.0;
        } else if ( input[i] < (double)(-32768.0) ) {
            input[i] = (double)(-32768.0);
        }
        // convert
        output[i] = (int16_t)input[i];
    }
}


#endif

