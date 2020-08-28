/******************/
#ifndef __BR_IIR_H__
#define __BR_IIR_H__

/*
Notch filter coefficients object
*/
struct br_coeffs {
	float e;
	float p;
	float d[3];
};

/*
Notch filter object
*/
struct br_filter {
	float e;
	float p;
	float d[3];
	float x[3];
	float y[3];
};

//typedef struct br_filter h_filter;

extern void br_iir_init(float fsfilt,float gb,float Q,float fstep, short fmin);
extern void br_iir_setup(struct br_filter * H,int index);
extern float br_iir_filter(float yin,struct br_filter * H);

#endif

