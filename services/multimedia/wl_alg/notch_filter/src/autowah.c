#include "br_iir.h"
#include "autowah.h"
#include "hal_trace.h"


typedef struct 
{
    /* data */
    double iir_coef1[6];
    double iir_coef2[6];
}iir_coef_t;

double iir_buffer1[5];
double iir_buffer2[5];

const iir_coef_t iir_coef_cfg = {
    // .iir_coef1 = {1.0000,-1.9773,1.0000,1.0000,-1.8384,0.8595}, //freq 384
    // .iir_coef2 = {1.0000,-1.7981,1.0000,1.0000,-1.4612,0.6253}, //freq 1154
    .iir_coef1 = {1.0000,-1.9966,1.0000,1.0000,-1.9402,0.9435}, //freq 148
    .iir_coef2 = {1.0000,-1.9856,1.0000,1.0000,-1.8729,0.8856}, //freq 306
    
};


double iir_buffer1_sec[5];
double iir_buffer2_sec[5];
const iir_coef_t iir_coef_cfg_sec = {
	// .iir_coef1 = {1.0000,-1.9199,1.0000,1.0000,-1.6798,0.7499}, //freq 723
    // .iir_coef2 = {1.0000,-1.3220,1.0000,1.0000,-0.9106,0.3776}, //freq 2161
    .iir_coef1 = {1.0000,-1.9686,1.0000,1.0000,-1.8077,0.8366}, //freq 452
    .iir_coef2 = {1.0000,-1.9455,1.0000,1.0000,-1.7408,0.7896}, //freq 596
};


double iir_buffer1_thr[5];
double iir_buffer2_thr[5];
const iir_coef_t iir_coef_cfg_thr = {
	// .iir_coef1 = {1.0000,-1.9923,1.0000,1.0000,-1.9087,0.9161}, //freq 148
    // .iir_coef2 = {1.0000,-1.9966,1.0000,1.0000,-1.9402,0.9435}, //freq 223
    .iir_coef1 = {1.0000,-1.9139,1.0000,1.0000,-1.6667,0.7417}, //freq 750
    .iir_coef2 = {1.0000,-1.8324,1.0000,1.0000,-1.5154,0.6541}, //freq 1050
};



double iir_buffer1_four[5];
double iir_buffer2_four[5];
const iir_coef_t iir_coef_cfg_four = {
	// .iir_coef1 = {1.0000,-1.9923,1.0000,1.0000,-1.9087,0.9161}, //freq 1246
    // .iir_coef2 = {1.0000,-1.9966,1.0000,1.0000,-1.9402,0.9435}, //freq 6694
    .iir_coef1 = {1.0000,-1.8324,1.0000,1.0000,-1.5154,0.6541}, //freq 1050
    .iir_coef2 = {1.0000,-1.7820,1.0000,1.0000,-1.4370,0.6128}, //freq 1200
};


double iir_buffer1_five[5];
double iir_buffer2_five[5];
const iir_coef_t iir_coef_cfg_five = {
	// .iir_coef1 = {1.0000,-1.8939,1.0000,1.0000,-1.6257,0.7167}, //freq 1246
    // .iir_coef2 = {1.0000,1.1144,1.0000,1.0000,0.3876,-0.3044}, //freq 6694
    .iir_coef1 = {1.0000,-1.5946,1.0000,1.0000,-1.1938,0.4972}, //freq 1650
    .iir_coef2 = {1.0000,-1.2989,1.0000,1.0000,-0.8890,0.3689}, //freq 2200
};

double iir_buffer1_six[5];
double iir_buffer2_six[5];
const iir_coef_t iir_coef_cfg_six = {
	// .iir_coef1 = {1.0000,-1.8939,1.0000,1.0000,-1.6257,0.7167}, //freq 833
    // .iir_coef2 = {1.0000,1.1144,1.0000,1.0000,0.3876,-0.3044}, //freq 5505
    .iir_coef1 = {1.0000,-1.5946,1.0000,1.0000,-1.1938,0.4972}, //freq 1650
    .iir_coef2 = {1.0000,-1.5208,1.0000,1.0000,-1.1110,0.4610}, //freq 1800
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
		iir_buffer2_sec[icnt] = 0;
        iir_buffer1_sec[icnt] = 0;
		iir_buffer2_thr[icnt] = 0;
        iir_buffer1_thr[icnt] = 0;

		iir_buffer2_four[icnt] = 0;
        iir_buffer1_four[icnt] = 0;


		iir_buffer2_five[icnt] = 0;
        iir_buffer1_five[icnt] = 0;

        iir_buffer2_six[icnt] = 0;
        iir_buffer1_six[icnt] = 0;
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
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
double notch_filter_process_sec(double data)
{

    // first filter process
    iir_buffer1_sec[2] = iir_buffer1_sec[1];
    iir_buffer1_sec[1] = iir_buffer1_sec[0];
    iir_buffer1_sec[0] = data;


    data = iir_buffer1_sec[0]*iir_coef_cfg_sec.iir_coef1[0] + iir_buffer1_sec[1]*iir_coef_cfg_sec.iir_coef1[1] + iir_buffer1_sec[2]*iir_coef_cfg_sec.iir_coef1[2] - (iir_buffer1_sec[3]*iir_coef_cfg_sec.iir_coef1[4] + iir_buffer1_sec[4]*iir_coef_cfg_sec.iir_coef1[5]);
    //printf("data:%f buff1:%f \n\t",data,iir_buffer1[1]);

    iir_buffer1_sec[4] = iir_buffer1_sec[3];
    iir_buffer1_sec[3] = data;

    // second filter process
    iir_buffer2_sec[2] = iir_buffer2_sec[1];
    iir_buffer2_sec[1] = iir_buffer2_sec[0];
    iir_buffer2_sec[0] = data;

    data = iir_buffer2_sec[0]*iir_coef_cfg_sec.iir_coef2[0] + iir_buffer2_sec[1]*iir_coef_cfg_sec.iir_coef2[1] + iir_buffer2_sec[2]*iir_coef_cfg_sec.iir_coef2[2] - (iir_buffer2_sec[3]*iir_coef_cfg_sec.iir_coef2[4] + iir_buffer2_sec[4]*iir_coef_cfg_sec.iir_coef2[5]);

    iir_buffer2_sec[4] = iir_buffer2_sec[3];
    iir_buffer2_sec[3] = data;

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
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
double notch_filter_process_thr(double data)
{

    // first filter process
    iir_buffer1_thr[2] = iir_buffer1_thr[1];
    iir_buffer1_thr[1] = iir_buffer1_thr[0];
    iir_buffer1_thr[0] = data;


    data = iir_buffer1_thr[0]*iir_coef_cfg_thr.iir_coef1[0] + iir_buffer1_thr[1]*iir_coef_cfg_thr.iir_coef1[1] + iir_buffer1_thr[2]*iir_coef_cfg_thr.iir_coef1[2] - (iir_buffer1_thr[3]*iir_coef_cfg_thr.iir_coef1[4] + iir_buffer1_thr[4]*iir_coef_cfg_thr.iir_coef1[5]);
    //printf("data:%f buff1:%f \n\t",data,iir_buffer1[1]);

    iir_buffer1_thr[4] = iir_buffer1_thr[3];
    iir_buffer1_thr[3] = data;

    // second filter process
    iir_buffer2_thr[2] = iir_buffer2_thr[1];
    iir_buffer2_thr[1] = iir_buffer2_thr[0];
    iir_buffer2_thr[0] = data;

    data = iir_buffer2_thr[0]*iir_coef_cfg_thr.iir_coef2[0] + iir_buffer2_thr[1]*iir_coef_cfg_thr.iir_coef2[1] + iir_buffer2_thr[2]*iir_coef_cfg_thr.iir_coef2[2] - (iir_buffer2_thr[3]*iir_coef_cfg_thr.iir_coef2[4] + iir_buffer2_thr[4]*iir_coef_cfg_thr.iir_coef2[5]);

    iir_buffer2_thr[4] = iir_buffer2_thr[3];
    iir_buffer2_thr[3] = data;

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
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
double notch_filter_process_four(double data)
{

    // first filter process
    iir_buffer1_four[2] = iir_buffer1_four[1];
    iir_buffer1_four[1] = iir_buffer1_four[0];
    iir_buffer1_four[0] = data;


    data = iir_buffer1_four[0]*iir_coef_cfg_four.iir_coef1[0] + iir_buffer1_four[1]*iir_coef_cfg_four.iir_coef1[1] + iir_buffer1_four[2]*iir_coef_cfg_four.iir_coef1[2] - (iir_buffer1_four[3]*iir_coef_cfg_four.iir_coef1[4] + iir_buffer1_four[4]*iir_coef_cfg_four.iir_coef1[5]);
    //printf("data:%f buff1:%f \n\t",data,iir_buffer1[1]);

    iir_buffer1_four[4] = iir_buffer1_four[3];
    iir_buffer1_four[3] = data;

    // second filter process
    iir_buffer2_four[2] = iir_buffer2_four[1];
    iir_buffer2_four[1] = iir_buffer2_four[0];
    iir_buffer2_four[0] = data;

    data = iir_buffer2_four[0]*iir_coef_cfg_four.iir_coef2[0] + iir_buffer2_four[1]*iir_coef_cfg_four.iir_coef2[1] + iir_buffer2_four[2]*iir_coef_cfg_four.iir_coef2[2] - (iir_buffer2_four[3]*iir_coef_cfg_four.iir_coef2[4] + iir_buffer2_four[4]*iir_coef_cfg_four.iir_coef2[5]);

    iir_buffer2_four[4] = iir_buffer2_four[3];
    iir_buffer2_four[3] = data;

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
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
double notch_filter_process_five(double data)
{

    // first filter process
    iir_buffer1_five[2] = iir_buffer1_five[1];
    iir_buffer1_five[1] = iir_buffer1_five[0];
    iir_buffer1_five[0] = data;


    data = iir_buffer1_five[0]*iir_coef_cfg_five.iir_coef1[0] + iir_buffer1_five[1]*iir_coef_cfg_five.iir_coef1[1] + iir_buffer1_five[2]*iir_coef_cfg_five.iir_coef1[2] - (iir_buffer1_five[3]*iir_coef_cfg_five.iir_coef1[4] + iir_buffer1_five[4]*iir_coef_cfg_five.iir_coef1[5]);
    //printf("data:%f buff1:%f \n\t",data,iir_buffer1[1]);

    iir_buffer1_five[4] = iir_buffer1_five[3];
    iir_buffer1_five[3] = data;

    // second filter process
    iir_buffer2_five[2] = iir_buffer2_five[1];
    iir_buffer2_five[1] = iir_buffer2_five[0];
    iir_buffer2_five[0] = data;

    data = iir_buffer2_five[0]*iir_coef_cfg_five.iir_coef2[0] + iir_buffer2_five[1]*iir_coef_cfg_five.iir_coef2[1] + iir_buffer2_five[2]*iir_coef_cfg_five.iir_coef2[2] - (iir_buffer2_five[3]*iir_coef_cfg_five.iir_coef2[4] + iir_buffer2_five[4]*iir_coef_cfg_five.iir_coef2[5]);

    iir_buffer2_five[4] = iir_buffer2_five[3];
    iir_buffer2_five[3] = data;

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
This function generates the current output value
Note that if the input and output signal are integer
unsigned types, we need to add a half scale offset
*/
double notch_filter_process_six(double data)
{

    // first filter process
    iir_buffer1_six[2] = iir_buffer1_six[1];
    iir_buffer1_six[1] = iir_buffer1_six[0];
    iir_buffer1_six[0] = data;


    data = iir_buffer1_six[0]*iir_coef_cfg_six.iir_coef1[0] + iir_buffer1_six[1]*iir_coef_cfg_six.iir_coef1[1] + iir_buffer1_six[2]*iir_coef_cfg_six.iir_coef1[2] - (iir_buffer1_six[3]*iir_coef_cfg_six.iir_coef1[4] + iir_buffer1_six[4]*iir_coef_cfg_six.iir_coef1[5]);
    //printf("data:%f buff1:%f \n\t",data,iir_buffer1[1]);

    iir_buffer1_six[4] = iir_buffer1_six[3];
    iir_buffer1_six[3] = data;

    // second filter process
    iir_buffer2_six[2] = iir_buffer2_six[1];
    iir_buffer2_six[1] = iir_buffer2_six[0];
    iir_buffer2_six[0] = data;

    data = iir_buffer2_six[0]*iir_coef_cfg_six.iir_coef2[0] + iir_buffer2_six[1]*iir_coef_cfg_six.iir_coef2[1] + iir_buffer2_six[2]*iir_coef_cfg_six.iir_coef2[2] - (iir_buffer2_six[3]*iir_coef_cfg_six.iir_coef2[4] + iir_buffer2_six[4]*iir_coef_cfg_six.iir_coef2[5]);

    iir_buffer2_six[4] = iir_buffer2_six[3];
    iir_buffer2_six[3] = data;

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