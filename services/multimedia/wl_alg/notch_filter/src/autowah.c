#include "br_iir.h"
#include "autowah.h"
#include "hal_trace.h"


double iir_data1[6] = {1.0000,-1.9773,1.0000,1.0000,-1.8384,0.8595};
double iir_data2[6] = {1.0000,-1.7981,1.0000,1.0000,-1.4612,0.6253};
typedef struct 
{
    /* data */
    double iir_coef1[6];
    double iir_coef2[6];
}iir_coef_t;

double iir_buffer1[5];
double iir_buffer2[5];

const iir_coef_t iir_coef_cfg = {
    .iir_coef1 = {1.0000,-1.9773,1.0000,1.0000,-1.8384,0.8595}, //freq 384
    .iir_coef2 = {1.0000,-1.7981,1.0000,1.0000,-1.4612,0.6253}, //freq 1154
};


/*
This is the auto wah effect initialization function.
This initializes the band pass filter and the effect control variables
*/

void notch_filter_init(void)
{
    for(uint32_t icnt = 0; icnt < 5; icnt++)
    {
        iir_buffer2[icnt] = 0;
        iir_buffer1[icnt] = 0;
    }

}


/*
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
double notch_filter_process(double data)
{

    // first filter process
    iir_buffer1[2] = iir_buffer1[1];
    iir_buffer1[1] = iir_buffer1[0];
    iir_buffer1[0] = data;


    data = iir_buffer1[0]*iir_coef_cfg.iir_coef1[0] + iir_buffer1[1]*iir_coef_cfg.iir_coef1[1] + iir_buffer1[2]*iir_coef_cfg.iir_coef1[2] - (iir_buffer1[3]*iir_coef_cfg.iir_coef1[4] + iir_buffer1[4]*iir_coef_cfg.iir_coef1[5]);
    //printf("data:%f buff1:%f \n\t",data,iir_buffer1[1]);

    iir_buffer1[4] = iir_buffer1[3];
    iir_buffer1[3] = data;

    // second filter process
    iir_buffer2[2] = iir_buffer2[1];
    iir_buffer2[1] = iir_buffer2[0];
    iir_buffer2[0] = data;

    data = iir_buffer2[0]*iir_coef_cfg.iir_coef2[0] + iir_buffer2[1]*iir_coef_cfg.iir_coef2[1] + iir_buffer2[2]*iir_coef_cfg.iir_coef2[2] - (iir_buffer2[3]*iir_coef_cfg.iir_coef2[4] + iir_buffer2[4]*iir_coef_cfg.iir_coef2[5]);

    iir_buffer2[4] = iir_buffer2[3];
    iir_buffer2[3] = data;

    if(data > (double)32767.0)
    {
        data = 32767.0;
    }

    if(data < (double)-32768.0)
    {
        data = -32768.0;
    }

    return data;

}

/*
This function will emulate a LFO that will vary according
to the effect_rate parameter set in the AutoWah_init function.
*/


void notch_filter_exit(void)
{
	// todo process
}