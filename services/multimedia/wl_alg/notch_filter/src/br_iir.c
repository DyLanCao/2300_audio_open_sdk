#include "br_iir.h"
#include "hal_trace.h"

#include <math.h>

#define BR_MAX_COEFS 120
#define PI 3.1415926

/*This is an array of the filter parameters object
defined in the br_iir.h file*/
static struct br_coeffs br_coeff_arr[BR_MAX_COEFS];

/*This initialization function will create the
notch filter coefficients array, you have to specify:
fsfilt = Sampling Frequency
gb     = Gain at cut frequencies
Q      = Q factor, Higher Q gives narrower band
fstep  = Frequency step to increase center frequencies
in the array
fmin   = Minimum frequency for the range of center   frequencies
*/
void br_iir_init(float fsfilt,float gb,float Q,float fstep, short fmin) {
	int i;
      float damp;
      float wo;
      
      damp = (float)sqrt(1 - pow(gb,2))/gb;

	for (i=0;i<BR_MAX_COEFS;i++) {
                //wo = 2*PI*(fstep*i + fmin)/fsfilt;
                wo = 2*(fmin);
		br_coeff_arr[i].e = 1/(1 + damp*(float)tan(wo/(Q*2)));
		br_coeff_arr[i].p = (float)cos(wo);
		br_coeff_arr[i].d[0] = br_coeff_arr[i].e;
		br_coeff_arr[i].d[1] = 2*br_coeff_arr[i].e*br_coeff_arr[i].p;
		br_coeff_arr[i].d[2] = (2*(float)(br_coeff_arr[i].e)-1);
	}
}
float g_hd1 = 1.774647,g_hd2 = 0.806648;
/*This function loads a given set of notch filter coefficients acording to a center frequency index
into a notch filter object
H = filter object
ind = index of the array mapped to a center frequency
*/
void br_iir_setup(struct br_filter * H,int ind) {
	H->e = br_coeff_arr[ind].e;
	H->p = br_coeff_arr[ind].p;
	H->d[0] = br_coeff_arr[ind].d[0];
	H->d[1] = br_coeff_arr[ind].d[1];
	H->d[2] = br_coeff_arr[ind].d[2];
}

/*
This function does the actual notch filter processing
yin = input sample
H   = filter object 
*/


float br_iir_filter(float yin,struct br_filter * H) {
	float yout;

	H->x[0] = H->x[1]; 
	H->x[1] = H->x[2]; 
	H->x[2] = yin;
	
	H->y[0] = H->y[1]; 
	H->y[1] = H->y[2]; 

	//H->y[2] = 0.006;
 	//H->y[2] = H->d[0]*H->x[2] - H->d[1]*H->x[1] + H->d[0]*H->x[0] + H->d[1]*H->y[1] - H->d[2]*H->y[0];
	H->y[2] = H->d[0]*H->x[2] - H->d[1]*H->x[1] + H->d[0]*H->x[0] + g_hd1*H->y[1] - g_hd2*H->y[0];
	//temp_hy111 = temp_hy1*100;
	//temp_hy1 =  (H->d[2]*H->y[1]);
	//H->y[2] = temp_h; 
	
	yout = H->y[2];

	//TRACE("h Hy0:%d Hy1:%d Hy2:%d temp_hy1:%d ",(short)(100*H->y[0]),(short)(100*H->y[1]),(short)(100*H->y[2]),(short)(temp_hy1*1));

	return yout;
}
