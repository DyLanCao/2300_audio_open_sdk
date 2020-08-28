#include "br_iir.h"
#include "autowah.h"
#include "hal_trace.h"

static short center_freq;
static short samp_freq;
static short counter;
static short counter_limit;
static short control;
static short max_freq;
static short min_freq;
static short f_step;
static struct br_filter H;

/*
This is the auto wah effect initialization function.
This initializes the band pass filter and the effect control variables
*/
void AutoWah_init(short effect_rate,short sampling,short maxf,short minf,short Q,float gainfactor,short freq_step) {

	/*Process variables*/
	center_freq = 0;
	samp_freq = sampling;
	counter = effect_rate;
	control = 0;

	/*User Parametters*/
	counter_limit = effect_rate;

	/*Convert frequencies to index ranges*/
	min_freq = 0;
	max_freq = (maxf - minf)/freq_step;

    br_iir_init(sampling,gainfactor,Q,freq_step,minf);

    f_step = freq_step;
}

/*
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
float AutoWah_process(int xin) {
	float yout;


	yout = br_iir_filter(xin,&H);
	#ifdef INPUT_UNSIGNED
		yout += 32767;
	#endif
    //TRACE("xin:%d yout:%d",xin,(short)yout);
    //TRACE("H.e:%d G.d:%d D0:%d D1:%d D2:%d ",(short)(100*H.e),(short)(100*H.p),(short)(100*H.d[0]),(short)(100*H.d[1]),(short)(100*H.d[2]));

	return yout;
}

/*
This function will emulate a LFO that will vary according
to the effect_rate parameter set in the AutoWah_init function.
*/
void AutoWah_sweep(void) {

	if (!--counter) {
		if (!control) {
			br_iir_setup(&H,(center_freq+=f_step));
			if (center_freq > max_freq) {
				control = 1;
			}
		}
		else if (control) {
			br_iir_setup(&H,(center_freq-=f_step));
			if (center_freq == min_freq) {
				control = 0;
			}
		}

		counter = counter_limit;
	}
}

