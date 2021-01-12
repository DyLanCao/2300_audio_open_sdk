
#ifndef TDOA_H_
#define TDOA_H_

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "arm_math.h"
#include "speech_memory.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif                                                                                        
#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif

#define FRAME_LENS 512

int UpperPowerOfTwo(int n) {
    return (int)pow(2, ceil(log(n) / log(2))); 
}


//num is 320 hamming windows data
const float hamming_win[FRAME_LENS] = {
//#include "hamm_win_320.txt"
#include "hamm_win_512.txt"
};
// Apply hamming window on data in place
void Hamming(float *data, int num_point) {
    for (int i = 0; i < num_point; i++) {
        data[i] = data[i] * hamming_win[i];  
    }
}


//Fast Fourier Transform
//=======================

// #define M_PI 3.141592653589793238462643383279502
static void make_sintbl(int n, float* sintbl)
{
	int i, n2, n4, n8;
	float c, s, dc, ds, t;

	n2 = n / 2;  n4 = n / 4;  n8 = n / 8;
	t = (float)sin(M_PI / n);
	dc = 2 * t * t;  ds = (float)sqrt(dc * (2 - dc));
	t = 2 * dc;  c = sintbl[n4] = 1;  s = sintbl[0] = 0;
	for (i = 1; i < n8; i++) {
		c -= dc;  dc += t * c;
		s += ds;  ds -= t * s;
		sintbl[i] = s;  sintbl[n4 - i] = c;
	}
	if (n8 != 0) sintbl[n8] = (float)sqrt(0.5);
	for (i = 0; i < n4; i++)
		sintbl[n2 - i] = sintbl[i];
	for (i = 0; i < n2 + n4; i++)
		sintbl[i + n2] = -sintbl[i];
}

static void make_bitrev(int n, int* bitrev)
{
	int i, j, k, n2;

	n2 = n / 2;  i = j = 0;
	for (;;) {
		bitrev[i] = j;
		if (++i >= n) break;
		k = n2;
		while (k <= j) { j -= k; k /= 2; }
		j += k;
	}
}

//x:实部  y:虚部  n：fft长度
int fft(float* x, float* y, int n)
{
	static int    last_n = 0;     /* previous n */
	static int    *bitrev = NULL; /* bit reversal table */
	static float *sintbl = NULL; /* trigonometric function table */
	int i, j, k, ik, h, d, k2, n4, inverse;
	float t, s, c, dx, dy;

	/* preparation */
	if (n < 0) {
		n = -n;  inverse = 1; /* inverse transform */
	}
	else {
		inverse = 0;
	}
	n4 = n / 4;
	if (n != last_n || n == 0) {
		last_n = n;
		if (sintbl != NULL) speech_free(sintbl);
		if (bitrev != NULL) speech_free(bitrev);
		if (n == 0) return 0;             /* free the memory */
		sintbl = (float*)speech_malloc((n + n4) * sizeof(float));
		bitrev = (int*)speech_malloc(n * sizeof(int));
		if (sintbl == NULL || bitrev == NULL) {
			//Error("%s in %f(%d): out of memory\n", __func__, __FILE__, __LINE__);
			return 1;
		}
		make_sintbl(n, sintbl);
		make_bitrev(n, bitrev);
	}

	/* bit reversal */
	for (i = 0; i < n; i++) {
		j = bitrev[i];
		if (i < j) {
			t = x[i];  x[i] = x[j];  x[j] = t;
			t = y[i];  y[i] = y[j];  y[j] = t;
		}
	}

	/* transformation */
	for (k = 1; k < n; k = k2) {
		h = 0;  k2 = k + k;  d = n / k2;
		for (j = 0; j < k; j++) {
			c = sintbl[h + n4];
			if (inverse)
				s = -sintbl[h];
			else
				s = sintbl[h];
			for (i = j; i < n; i += k2) {
				ik = i + k;
				dx = s * y[ik] + c * x[ik];
				dy = c * y[ik] - s * x[ik];
				x[ik] = x[i] - dx;  x[i] += dx;
				y[ik] = y[i] - dy;  y[i] += dy;
			}
			h += d;
		}
	}
	if (inverse) {
		/* divide by n in case of the inverse transformation */
		for (i = 0; i < n; i++)
		{
			x[i] /= n;
			y[i] /= n;
		}
	}
	return 0;  /* finished successfully */
}


// Delay & Sum
// @params data : in format channel0, channel1
void DelayAndSum(const float *data, int num_channel, int num_sample,
                 int *tdoa, float *out) {
    for (int i = 0; i < num_sample; i++) {
        int count = 0;
        float sum = 0.0;
        for (int j = 0; j < num_channel; j++) {
            if (i + tdoa[j] >= 0 && i + tdoa[j] < num_sample) {
                sum += data[j * num_sample + i + tdoa[j]];
                count++;
            }
        }
        assert(count > 0);
        out[i] = sum / count;
    }
}


// Reference:
// 1. Microphone Array Signal Processing(chappter 9: Direction-of-Arrival and Time-Difference-of-Arrival Estimation)

// Calc tdoa(time delay of arrival
// using GCC-PHAT(Gerneral Cross Correlation - Phase Transform)
// @params data : in format channel0, channel1
// @params ref : reference_channel
// @params margin: margin [-tao, tao]
void GccPhatTdoa(const float *data, int num_channel, int num_sample, int ref, int margin, int *tdoa) 
{
    assert(data != NULL);
    assert(ref >= 0 && ref < num_channel);
    assert(margin <= num_sample / 2);
    // constrait the number data points to 2^n
    int num_points = UpperPowerOfTwo(num_sample);
    int half = num_points / 2;
	float win_data[num_points*num_channel];

    // copy data and apply window
    for (int i = 0; i < num_channel; i++) {
        // memcpy(win_data + i * num_points, data + i * num_sample, 
        //        sizeof(double) * num_sample);
		for(int icnt = 0; icnt < num_sample; icnt++)
		{
			win_data[i*num_points + icnt] = data[i*num_sample + icnt]; 
		}

        Hamming(win_data, num_sample);
    }

    float fft_real[num_points*num_channel];
    float fft_img[num_points*num_channel];
	#if 1
    // do fft
    for (int i = 0; i < num_channel; i++) {
        // memcpy(fft_real + i * num_points, win_data + i * num_points, 
        //         sizeof(float) * num_points);
		for(int inum = 0; inum < num_points; inum++)
		{
			fft_real[i*num_points + inum ]  = win_data[i*num_points + inum];
		}

        fft(fft_real + i * num_points, fft_img + i * num_points, num_points);  
    }
	#endif

    float corr_real[num_points * num_channel];
    float corr_img[num_points * num_channel];
    // do gcc-phat
    for (int i = 0; i < num_channel; i++) {
        if (i != ref) {
            // * do fft cross correlation, fft(i) fft(ref)*
            // (a + bj) (c + dj)* = (a + bj) (c - dj) = (ac + bd) + (bc - ad)j
            for (int j = 0; j < num_points; j++) {
                int m = ref * num_points + j, n = i * num_points + j;
				

                corr_real[n] = fft_real[n] * fft_real[m] + fft_img[n] * fft_img[m];
                corr_img[n] = fft_img[n] * fft_real[m] - fft_real[n] * fft_img[m];
                float var = corr_real[n] * corr_real[n] + corr_img[n] * corr_img[n];
  				float length = (float)sqrt(var);
                corr_real[n] /= length;
                corr_img[n] /= length;

		
            }

			#if 1
            // * do inserse fft
            fft(corr_real + i * num_points, corr_img + i * num_points, -num_points);
            // * rearange idft index(fftshift), make num_points / 2 as the center
            for (int j = 0; j < half; j++) {
                float t = corr_real[i * num_points + j];
                corr_real[i * num_points + j] = corr_real[i * num_points + j + half];
                corr_real[i * num_points + j + half] = t;
            }
            // * select max
            int max_j = half - margin;
            //assert(max_j >= 0);
            float max = corr_real[i * num_points + max_j]; 
            for (int j = half - margin; j < half + margin; j++) {
                if (corr_real[i * num_points + j] > max) {
                    max = corr_real[i * num_points + j];
                    max_j = j;
                }
            }
            tdoa[i] = max_j - half;
			#endif

        }
        else {
            tdoa[i] = 0;
        }
    }
}


void intToFloat( int16_t *input, float *output, int length )
{
    int i;
 
    for ( i = 0; i < length; i++ ) {
        output[i] = (float)input[i];
    }
}
 
void floatToInt( float *input, int16_t *output, int length )
{
    int i;
 
    for ( i = 0; i < length; i++ ) {
        if ( input[i] > (float)32767.0 ) {
            input[i] = (float)32767.0;
        } else if ( input[i] < (float)(-32768.0) ) {
            input[i] = (float)(-32768.0);
        }
        // convert
        output[i] = (int16_t)input[i];
    }
}


#endif
