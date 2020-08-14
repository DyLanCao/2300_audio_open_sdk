
#ifdef PROGRAMMER

int bt_iqimb_ini ()
{
    return 0;
}

int bt_iqimb_test_ex (int mismatch_type)
{
    return 0;
}

#else

#include <math.h>
#include <stdio.h>

#include "hal_dma.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "iqcorrect.h"
#include "string.h"
#include "heap_api.h"
#include "hal_chipid.h"

typedef short int16;
typedef unsigned short uint16;
typedef int   int32;
typedef unsigned int   uint32;
#define SearchMethod

#define NTbl (1024)
#define fftSize 1024
#define bitshift 10
#define pi (3.1415926535898)
#define min(a,b)    (((a) < (b)) ? (a) : (b))

#ifndef uint32_t
typedef unsigned long uint32_t;
#endif

#ifndef uint8
typedef unsigned char  uint8;
#endif

#define READ_REG(b,a) *(volatile uint32_t *)(b+a)
#define WRITE_REG(v,b,a) *(volatile uint32_t *)(b+a) = v

#define   BUF_SIZE   (512)

#define MAX_COUNT 5
volatile int iqimb_dma_status = 0;
//short M0data[BUF_SIZE];

#define MED_MEM_POOL_SIZE (10*1024)
static uint8_t *g_medMemPool = NULL;

uint32_t  hal_sys_timer_get(void);

extern void *rt_malloc(unsigned int size);
extern void rt_free(void *rmem);

typedef struct ComplexInt_
{
    int re;
    int im;
} ComplexInt;

typedef struct ComplexShort_
{
    short re;
    short im;
} ComplexShort;

static void Tblgen(ComplexShort*w0, ComplexShort*w1, ComplexShort*w2,int len)
{
    TRACE("Tblgen ini");
    for (int i=0; i<len; i++)
    {
        w0[i].re = (short)(cos(2*pi*i/(256.0/4))*32767);
        w0[i].im = (short)(sin(2*pi*i/(256.0/4))*32767);
        //w1[i].re = (short)(cos(2*pi*i/(256.0/5))*32767);
        //w1[i].im = (short)(sin(2*pi*i/(256.0/5))*32767);
        //w2[i].re = (short)(cos(2*pi*i/(256.0/3))*32767);
        //w2[i].im = (short)(sin(2*pi*i/(256.0/3))*32767);

        /*
        w0[i].re = (short)(cos(2*pi*i/(2048.0/34))*32767);
        w0[i].im = (short)(sin(2*pi*i/(2048.0/34))*32767);
        w1[i].re = (short)(cos(2*pi*i/(2048.0/33))*32767);
        w1[i].im = (short)(sin(2*pi*i/(2048.0/33))*32767);
        w2[i].re = (short)(cos(2*pi*i/(2048.0/35))*32767);
        w2[i].im = (short)(sin(2*pi*i/(2048.0/35))*32767);
        */

        /*      w0[i].re = (short)(cos(2*pi*i/(2048.0/32))*32767);
                w0[i].im = (short)(sin(2*pi*i/(2048.0/32))*32767);
                w1[i].re = (short)(cos(2*pi*i/(2048.0/31))*32767);
                w1[i].im = (short)(sin(2*pi*i/(2048.0/31))*32767);
                w2[i].re = (short)(cos(2*pi*i/(2048.0/33))*32767);
                w2[i].im = (short)(sin(2*pi*i/(2048.0/33))*32767);
        */
        /*
        w0[i].re = (short)(cos(2*pi*i/(3072.0/48))*32767);
        w0[i].im = (short)(sin(2*pi*i/(3072.0/48))*32767);
        //w0[i].re = (short)(cos(2*pi*i/29)*32767);
        //w0[i].im = (short)(sin(2*pi*i/29)*32767);
        w1[i].re = (short)(cos(2*pi*i/(3072.0/47))*32767);
        w1[i].im = (short)(sin(2*pi*i/(3072.0/47))*32767);
        w2[i].re = (short)(cos(2*pi*i/(3072.0/49))*32767);
        w2[i].im = (short)(sin(2*pi*i/(3072.0/49))*32767);*/
    }
    TRACE("Tblgen ok");
}
static void Tblgen_iq(ComplexShort*w0, ComplexShort*w1, ComplexShort*w2,int len)
{
    for (int i=0; i<len; i++)
    {
        w0[i].re = (short)(cos(2*pi*i/(256.0/8))*32767);
        w0[i].im = (short)(sin(2*pi*i/(256.0/8))*32767);
        //w1[i].re = (short)(cos(2*pi*i/(256.0/9))*32767);
        //w1[i].im = (short)(sin(2*pi*i/(256.0/9))*32767);
        //w2[i].re = (short)(cos(2*pi*i/(256.0/7))*32767);
        //w2[i].im = (short)(sin(2*pi*i/(256.0/7))*32767);

        /*
        w0[i].re = (short)(cos(2*pi*i/(2048.0/66))*32767);
        w0[i].im = (short)(sin(2*pi*i/(2048.0/66))*32767);
        w1[i].re = (short)(cos(2*pi*i/(2048.0/65))*32767);
        w1[i].im = (short)(sin(2*pi*i/(2048.0/65))*32767);
        w2[i].re = (short)(cos(2*pi*i/(2048.0/67))*32767);
        w2[i].im = (short)(sin(2*pi*i/(2048.0/67))*32767);
        */
        /*
         w0[i].re = (short)(cos(2*pi*i/(2048.0/64))*32767);
        w0[i].im = (short)(sin(2*pi*i/(2048.0/64))*32767);
        w1[i].re = (short)(cos(2*pi*i/(2048.0/63))*32767);
        w1[i].im = (short)(sin(2*pi*i/(2048.0/63))*32767);
        w2[i].re = (short)(cos(2*pi*i/(2048.0/65))*32767);
        w2[i].im = (short)(sin(2*pi*i/(2048.0/65))*32767);
        */
        /*
        w0[i].re = (short)(cos(2*pi*i/(3072.0/96))*32767);
        w0[i].im = (short)(sin(2*pi*i/(3072.0/96))*32767);
        //w0[i].re = (short)(cos(2*pi*i/29)*32767);
        //w0[i].im = (short)(sin(2*pi*i/29)*32767);
        w1[i].re = (short)(cos(2*pi*i/(3072.0/95))*32767);
        w1[i].im = (short)(sin(2*pi*i/(3072.0/95))*32767);
        w2[i].re = (short)(cos(2*pi*i/(3072.0/97))*32767);
        w2[i].im = (short)(sin(2*pi*i/(3072.0/97))*32767);
        */
    }
}

typedef struct IQMismatchPreprocessState_
{
    short *M0data;
    ComplexShort *Table0;
    ComplexShort *Table1;
    ComplexShort *Table2;
} IQMismatchPreprocessState;

IQMismatchPreprocessState *IQMismatchPreprocessState_init(int fft_size)
{
    TRACE("malloc ini");
    IQMismatchPreprocessState *st = (IQMismatchPreprocessState *)med_calloc(1, sizeof(IQMismatchPreprocessState));
    st->M0data = (short*)med_calloc(fft_size, sizeof(short));
    st->Table0 = (ComplexShort*)med_calloc(fft_size, sizeof(ComplexShort));
    st->Table1 = (ComplexShort*)med_calloc(fft_size, sizeof(ComplexShort));
    st->Table2 = (ComplexShort*)med_calloc(fft_size, sizeof(ComplexShort));
    TRACE("st:%x, st->M0data:%x, st->Table0:%x, st->Table1:%x, st->Table2:%x", st, st->M0data, st->Table0, st->Table1, st->Table2);
    TRACE("malloc ok");
    return st;
}

int32_t IQMismatchPreprocessState_destroy(IQMismatchPreprocessState *st)
{
    TRACE("st:%x, st->M0data:%x, st->Table0:%x, st->Table1:%x, st->Table2:%x", st, st->M0data, st->Table0, st->Table1, st->Table2);
    med_free(st->M0data);
    med_free(st->Table0);
    med_free(st->Table1);
    med_free(st->Table2);
    med_free(st);
    return 0;
}
int IQMismatchParameterCalc_ex(short *M0data,
                               short Cohcnt, ComplexShort *Table0, ComplexShort *Table1, ComplexShort *Table2,int fftsize)
{
    int i,j,k;
    int M0 = 0;
    ComplexInt tmp0;
    //ComplexInt tmp1;
    //ComplexInt tmp2;
    for (j = 0; j < Cohcnt; j++)
    {
        tmp0.re = 0;
        tmp0.im = 0;
        //  tmp1.re = 0;
        //tmp1.im = 0;
        //  tmp2.re = 0;
        //tmp2.im = 0;
        k = 0;
        for (i=0; i<fftsize; i++)
        {
            if(k>=fftsize)
                k = 0;
            tmp0.re = tmp0.re + (((int)M0data[i] *(int)Table0[k].re) >> 15) ;
            tmp0.im = tmp0.im + (((int)M0data[i] * (int)Table0[k].im) >> 15) ;
            //tmp1.re = tmp1.re + (((int)M0data[i] *(int)Table1[k].re) >> 15) ;
            //tmp1.im = tmp1.im + (((int)M0data[i] * (int)Table1[k].im) >> 15) ;
            //tmp2.re = tmp2.re + (((int)M0data[i] *(int)Table2[k].re) >> 15) ;
            //tmp2.im = tmp2.im + (((int)M0data[i] * (int)Table2[k].im) >> 15) ;
            k = k + 1;
        }
        tmp0.re = tmp0.re/ fftsize;// >> bitshift;
        tmp0.im = tmp0.im/ fftsize;// >>bitshift;
        // tmp1.re = tmp1.re/ fftsize;// >> bitshift;
        // tmp1.im = tmp1.im/ fftsize;// >>bitshift;
        //  tmp2.re = tmp2.re/ fftsize;// >> bitshift;
        //  tmp2.im = tmp2.im/ fftsize;// >>bitshift;
        M0 = M0 + tmp0.re*tmp0.re + tmp0.im*tmp0.im;
        //  M0 = M0 + tmp1.re*tmp1.re + tmp1.im*tmp1.im;
        //  M0 = M0 + tmp2.re*tmp2.re + tmp2.im*tmp2.im;
    }
    return (int)(M0/Cohcnt);
}

void caculate_energy_main_test(IQMismatchPreprocessState *st,int* Energy,int* Energy1,int fftsize,short Cohcnt)
{
    *Energy1 = IQMismatchParameterCalc_ex(st->M0data, Cohcnt, st->Table0, st->Table1, st->Table2,fftsize);
}


static struct HAL_DMA_DESC_T iqimb_dma_desc[1];


static void iqimb_dma_dout_handler(uint8_t chan, uint32_t remains, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    uint32_t val;
    //config dpd rx
    val = READ_REG(0x40150000,0x30);
    val &= 0xfffffffe; //dma ctrl, bit0
    val |= (0<<0);
    WRITE_REG(val,0x40150000,0x30);
    val = READ_REG(0x40150000,0);
    val &= 0xfffffffe; //device en, bit0
    val |= (0<<0);
    WRITE_REG(val,0x40150000,0);
    iqimb_dma_status = 0;
    return;
}

int bt_iqimb_dma_enable ( short * dma_dst_data, uint16 size)
{
    struct HAL_DMA_CH_CFG_T iqimb_dma_cfg;
    uint32_t val;
    iqimb_dma_status = 1;
    memset(&iqimb_dma_cfg, 0, sizeof(iqimb_dma_cfg));
    iqimb_dma_cfg.ch = hal_audma_get_chan(HAL_AUDMA_BTDUMP,HAL_DMA_HIGH_PRIO);
    iqimb_dma_cfg.dst_bsize = HAL_DMA_BSIZE_16;
    iqimb_dma_cfg.dst_periph = (enum HAL_DMA_PERIPH_T)0; //useless
    iqimb_dma_cfg.dst_width = HAL_DMA_WIDTH_WORD;
    iqimb_dma_cfg.handler = iqimb_dma_dout_handler;
    iqimb_dma_cfg.src = 0; // useless
    iqimb_dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
    iqimb_dma_cfg.src_periph = HAL_AUDMA_BTDUMP;//(enum HAL_DMA_PERIPH_T)0; //useless HAL_AUDMA_BT_DUMP;
    iqimb_dma_cfg.src_tsize = size;//1600; //1600*2/26=123us
    iqimb_dma_cfg.src_width = HAL_DMA_WIDTH_WORD;
    iqimb_dma_cfg.try_burst = 1;
    iqimb_dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;
    iqimb_dma_cfg.dst = (uint32_t)(dma_dst_data);
    hal_audma_init_desc(&iqimb_dma_desc[0], &iqimb_dma_cfg, 0, 1);
    hal_audma_sg_start(&iqimb_dma_desc[0], &iqimb_dma_cfg);
    //configed after mismatch parameter done, or apb clock muxed.
    //wait
    for(volatile int i=0; i<5000; i++);
    //config dpd rx
    val = READ_REG(0x40150000,0x30);
    val &= 0xfffffffe; //dma ctrl, bit0
    val |= (1<<0);
    WRITE_REG(val,0x40150000,0x30);
    val = READ_REG(0x40150000,0);
    val &= 0xfffffffe; //device en, bit0
    val |= (1<<0);
    val &= 0xffffffdf; //fm_psd, bit5
    val |= (1<<5);

    WRITE_REG(val,0x40150000,0);
    while(1)
    {

        if(iqimb_dma_status==0)
        {
            hal_audma_free_chan(iqimb_dma_cfg.ch);
            break;
        }
        hal_dma_get_sg_remain_size(iqimb_dma_cfg.ch);
    }
    return 1;

}

int bt_iqimb_ini ()
{
    uint32_t val;

    TRACE("bt_iqimb_ini :");

#define CMU_BASE_ADDR 0x40000000

    //TRACE("%s start", __func__);
    //SET_MARK(0x1001);
    // cmu clock
    val = READ_REG(CMU_BASE_ADDR, 0x48);
    val &= 0x7fffffff;
    val |= (1<<31);
    val &= 0xbfffffff;
    val |= (1<<30);
    WRITE_REG(val,CMU_BASE_ADDR,0x48);

    val = READ_REG(CMU_BASE_ADDR, 0x60);
    val &= 0xfffffbff;
    val |= (1<<11);//en_clk_top_oscx2
    WRITE_REG(val,CMU_BASE_ADDR,0x60);

    val = READ_REG(CMU_BASE_ADDR, 0x10);
    val &= 0xbfffffff;
    val |= (1<<30);//en clk bt all
    WRITE_REG(val,CMU_BASE_ADDR,0x10);

    //config gpadc 26m
    val = READ_REG(0xd0350208,0x0);
    val &= 0xffefffff; //bit20
    val |= (1<<20); //reg_gclk_en(BTM_CLK_26RXNFMI)
    val &= 0xffbfffff; //bit22
    val |= (1<<22); //reg_gclk_en(BTM_CLK_26RXDUMP)
    val &= 0xfdffffff; //bit25
    val |= (1<<25); //reg_gclk_en(BTM_CLK_26RXADC)
    WRITE_REG(val,0xd0350208,0x0);
    // TRACE("0xd0350208 : 0x%08x",val);

    val = READ_REG(0xd0350228,0x0);
    val &= 0xffefffff; //bit20
    val |= (1<<20); //reg_gclk_mode(BTM_CLK_26RXNFMI)
    val &= 0xffbfffff; //bit22
    val |= (1<<22); //reg_gclk_mode(BTM_CLK_26RXDUMP)
    val &= 0xfdffffff; //bit25
    val |= (1<<25); //reg_gclk_mode(BTM_CLK_26RXADC)
    val |= (1<<28); //clock enable by lxw
    WRITE_REG(val,0xd0350228,0x0);
    // TRACE("0xd0350228 : 0x%08x",val);
    return 1;
}

int bt_Txdc_cal_set(int ch_num, int dc_add)
{
    uint32_t val;
    uint32_t tmp = (uint32_t)dc_add;
    //sel apb clock
    val = READ_REG(0xd0350348,0x0);
    if(ch_num==0)
    {
        val &= 0x7ffffc00; //bit31 1 int_en_mismatch
        val |= tmp & 0x3ff;
    }
    else
    {
        // TRACE("bt_Txdc_cal_set, dcadd : 0x%08x",tmp);
        val &= 0x7c00ffff; //bit31 1 int_en_mismatch
        val |= (tmp & 0x3ff) <<16;
    }
    WRITE_REG(val,0xd0350348,0x0);
    return 1;
}

//g/p mismatch base addr 0xd0310000
//dc_i: 0xd0350348 9:0
//dc_q: 0xd0350348 25:16
int bt_iqimb_add_mismatch(int ch_num, int gain_mis, int phase_mis, int dc_i, int dc_q, uint32_t addr)
{
    uint32_t val;
    //sel apb clock
    val = READ_REG(0xd0350348,0x0);
    val &= 0x7fffffff; //bit31 1 int_en_mismatch
    val |= (0x0<<31);
    WRITE_REG(val,0xd0350348,0x0);
    val = (gain_mis << 16) | (phase_mis & 0x0000ffff);
    WRITE_REG(val,addr,ch_num*4);
    WRITE_REG(0x20,0xd0350220,0);
    WRITE_REG(0x20,0xd0350224,0);
    //sel 26m clock
    val = READ_REG(0xd0350348,0x0);
    val &= 0x7fffffff; //bit31 1 int_en_mismatch
    val |= (0x1<<31);
    WRITE_REG(val,0xd0350348,0x0);
    WRITE_REG(0x20,0xd0350220,0);
    WRITE_REG(0x20,0xd0350224,0);
    return 1;
}


#define CACULATE_TIMES  1
void show_btdump_data(void* data, int len)
{
    short* share_mem = (short*)data;
    // TRACE("check_mem_data :share_mem= 0x%08x, 0x%08x,0x%08x",share_mem,share_mem[0],share_mem[1]);
    uint8_t i =0;
    uint8_t m =0;
    int remain = len;
    for(m=0; m<1; m++)
    {
        for(i=0; i<1; i++)
        {

            if (remain >16)
            {
                DUMP16("%04x ",share_mem,16);
                share_mem +=16;
                remain -= 16;
            }
            else
            {
                DUMP16("%04x ",share_mem,remain);

                remain =0;
                return;
            }

        }
        hal_sys_timer_delay(MS_TO_TICKS(200));
    }
}


void DC_correction(IQMismatchPreprocessState *st,int *dc_i_r,int *dc_q_r,int fftsize,int Cohcnt)
{

    int Energy,Energy1;
    int dc_i;
    int dc_q;
    int dc_i_base;
    int dc_q_base;

#ifdef SearchMethod
    if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
        dc_i_base = 41;
        dc_q_base = 41;
    } else {
        dc_i_base = -24;
        dc_q_base = -24;
    }
    int step = 8;
    int ki,kj;
    int indi=dc_i_base;
    int indq=dc_q_base;
    int maxv = 0x7FFFFFFF;
    dc_i = dc_i_base;
    dc_q = dc_q_base;
    for (ki = 0; ki<6; ki++)
    {
        dc_i = dc_i + step;
        dc_q = dc_q_base;
        for(kj = 0; kj<6; kj++)
        {
            dc_q = dc_q + step;
            bt_Txdc_cal_set(0,dc_i);
            bt_Txdc_cal_set(1,dc_q);
            bt_iqimb_dma_enable(st->M0data,fftsize);
            // show_btdump_data((uint8_t *)st->M0data, 10);
            caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
            //TRACE("dc cal done!!!    dc_i = %d ,dc_q = %d,Energy1 = %d",dc_i,dc_q,Energy1);
            if(Energy1<maxv)
            {
                maxv= Energy1;
                indi = dc_i;
                indq = dc_q;
            }
        }
    }
    dc_i_base = indi;
    dc_q_base = indq;
    dc_i = dc_i_base - 4;
    for (ki = 0; ki<=5; ki++)
    {
        dc_q = dc_q_base-4;
        for(kj=0; kj<=5; kj++)
        {
            bt_Txdc_cal_set(0,dc_i);
            bt_Txdc_cal_set(1,dc_q);
            bt_iqimb_dma_enable(st->M0data,fftsize);
            caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
            if(Energy1<maxv)
            {
                maxv= Energy1;
                indi = dc_i;
                indq = dc_q;
            }
            dc_q = dc_q + 2;
        }
        dc_i = dc_i+2;
    }
    /*for (dc_i = indi-4;dc_i<=indi+4;dc_i+=8)
        {
          dc_i = indi;
          dc_q = indq;
          for(dc_q=indq-4;dc_q<=indq+4;dc_q+=8)
          {
             bt_Txdc_cal_set(0,dc_i);
             bt_Txdc_cal_set(1,dc_q);
             bt_iqimb_dma_enable(st->M0data,fftsize);
             caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
             if(Energy1<maxv)
              {
                 maxv= Energy1;
                 indi = dc_i;
                 indq = dc_q;
              }
          }
        }
        */
    *dc_i_r = indi;
    *dc_q_r = indq;
#else
    int P0,PIplus,PIneg,PQplus,PQneg,PIPQ;
    int CI,CQ,tmp_D;
    int dc_iters = 2;
    int dc_step = 4;
    int tmp;
    uint8 k;
    dc_i_base =63;
    dc_q_base = 63;
    for(k=0; k<dc_iters; k++)
    {
        dc_i = dc_i_base;
        dc_q = dc_q_base;
        bt_Txdc_cal_set(0,dc_i);
        bt_Txdc_cal_set(1,dc_q);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        // show_btdump_data((uint8_t *)M0data, 1024);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        P0 = Energy1;
        dc_i = dc_i_base + dc_step;
        dc_q = dc_q_base;
        bt_Txdc_cal_set(0,dc_i);
        bt_Txdc_cal_set(1,dc_q);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        PIplus = Energy1;
        dc_i = dc_i_base - dc_step;
        dc_q = dc_q_base;
        bt_Txdc_cal_set(0,dc_i);
        bt_Txdc_cal_set(1,dc_q);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        PIneg = Energy1;
        dc_i = dc_i_base;
        dc_q = dc_q_base + dc_step;
        bt_Txdc_cal_set(0,dc_i);
        bt_Txdc_cal_set(1,dc_q);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        PQplus = Energy1;
        dc_i = dc_i_base;
        dc_q = dc_q_base - dc_step;
        bt_Txdc_cal_set(0,dc_i);
        bt_Txdc_cal_set(1,dc_q);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        PQneg = Energy1;
        dc_i = dc_i_base + dc_step;
        dc_q = dc_q_base + dc_step;
        bt_Txdc_cal_set(0,dc_i);
        bt_Txdc_cal_set(1,dc_q);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        PIPQ = Energy1;
        tmp = 2*PIplus*PIplus + 2*PQplus*PQplus + 2*PIPQ*PIPQ;
        tmp = tmp - 6*P0*P0;
        tmp = tmp + 4*P0*(PIneg+PQneg+PIPQ);
        tmp = tmp + 2*(PIplus*(PQplus - PQneg)-PIneg*(PQplus+PQneg));
        tmp_D = tmp - 4*PIPQ*(PIplus+PQplus);
        tmp = P0*(2*(PIplus - PIneg)+PQplus -PQneg);
        tmp = tmp - PIPQ*(PQneg-PQplus)+PIneg*(PQplus+PQneg);
        tmp = tmp + PQplus*(PQneg-PQplus-2*PIplus);
        CI = -dc_step*tmp;
        tmp = P0*(2*(PQplus - PQneg)+PIplus-PIneg);
        tmp = tmp - PIPQ*(PIneg-PIplus)+PQneg*(PIplus+PIneg);
        tmp = tmp + PIplus*(PIneg-PIplus-2*PQplus);
        CQ = -dc_step*tmp;
        dc_i_base = dc_i_base + CI/tmp_D;
        dc_q_base = dc_q_base + CQ/tmp_D;
        dc_step = dc_step/2;
    }
    *dc_i_r = dc_i_base;
    *dc_q_r = dc_q_base;
#endif

}
int IQ_GAIN_Mismatch_Correction(IQMismatchPreprocessState *st,int phase_mis_base,int fftsize,uint32_t addr,int Cohcnt)
{   
	  int gain_mis_base = 0;
    int phase_mis_tmp =0;
    int gain_mis_tmp =0;
    int Energy,Energy1 ;
#ifdef SearchMethod
    int step = 4;
    int maxv = 0xfffffff;
    gain_mis_base = -100;
    for (gain_mis_tmp = -100; gain_mis_tmp<=100; gain_mis_tmp+=step)
    {
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr); //no mismatch
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        //TRACE("gain_mis_tmp = %d ,phase_mis_tmp = %d,Energy1 = %d",gain_mis_tmp,phase_mis_tmp,Energy1);
        if(maxv>Energy1)
        {
            maxv = Energy1;
            gain_mis_base = gain_mis_tmp;
        }
    }
#else
    uint8 k = 0;
    int tmp = 0;
    int energy_ret0_last = 1048576;
    int energy_ret0 = 0;
    int energy_ret1 = 0;
    int energy_ret2 = 0;
    int gainstep =8;
    int iters = 4;
    phase_mis_tmp = phase_mis_base;
    energy_ret0_last = 1048576;
    for(k=0; k<iters+2; k++)
    {
        gain_mis_tmp = gain_mis_base;
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        energy_ret0 = Energy1;
        if(energy_ret0<50)
            break;
        if(k>0)
        {
            if(energy_ret0_last < energy_ret0)
            {
                gain_mis_base = gain_mis_base + tmp;
                gain_mis_tmp = gain_mis_base;
                bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr);
                bt_iqimb_dma_enable(st->M0data,fftsize);
                caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
                energy_ret0 = Energy1;
            }
        }
        //TRACE("IQ gain correct energy_ret0 = %d",energy_ret0);
        gainstep = 4;///////////////////
        gain_mis_tmp = gain_mis_base + gainstep;
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        energy_ret2 = Energy1;
        gain_mis_tmp = gain_mis_base - gainstep;
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        energy_ret1 = Energy1;
        tmp = energy_ret2 -2*energy_ret0 + energy_ret1;
        //TRACE("IQ gain correct energy_ret2 -2*energy_ret0 + energy_ret1 = %d",tmp);
        if(tmp>0)
        {
            tmp = (energy_ret2-energy_ret1)*gainstep/tmp/2;
            tmp = min(tmp,4*gainstep);
            gainstep = gainstep/2;
        }
        else
        {
            tmp = 0;
            iters = iters+1;
        }
        if((gain_mis_base - tmp>60)||(gain_mis_base - tmp<-60))
            tmp = 0;
        //TRACE("IQ gain correct gain_mis = %d ,gain_adj = %d",gain_mis_base,tmp);
        gain_mis_base = gain_mis_base - tmp;
        energy_ret0_last = energy_ret0;
    }
#endif
    return gain_mis_base;
}
int IQ_Phase_Mismatch_Correction(IQMismatchPreprocessState *st,int gain_mis_base,int fftsize,uint32_t addr,int Cohcnt)
{
    int phase_mis_base = 0;
    int phase_mis_tmp =0;
    int gain_mis_tmp =0;
    int Energy,Energy1 ;
#ifdef SearchMethod
    int step = 4;
    int maxv = 0xfffffff;
    phase_mis_base = -100;
    for (phase_mis_tmp = -100; phase_mis_tmp<=100; phase_mis_tmp+=step)
    {
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr); //no mismatch
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        //TRACE("gain_mis_tmp = %d ,phase_mis_tmp = %d,Energy1 = %d",gain_mis_tmp,phase_mis_tmp,Energy1);
        if(maxv>Energy1)
        {
            maxv = Energy1;
            phase_mis_base = phase_mis_tmp;
        }
    }
#else
    uint8 k = 0;
    int iters = 4;
    int tmp = 0;
    int energy_tmp1 = 0;
    int energy_ret0_last = 1048576;
    int energy_ret0 = 0;
    int energy_ret1 = 0;
    int energy_ret2 = 0;
    int phasestep =4;
    tmp = 0;
    energy_ret0_last = 1048576;
    gain_mis_tmp = gain_mis_base;
    for(k=0; k<iters; k++)
    {
        phase_mis_tmp = phase_mis_base;
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr); //no mismatch
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        energy_ret0 = Energy1;
        if(k>0)
        {
            if(energy_ret0_last < energy_ret0)
            {
                phase_mis_base = phase_mis_base + tmp;
                phase_mis_tmp = phase_mis_base;
                bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr); //no mismatch
                bt_iqimb_dma_enable(st->M0data,fftsize);
                caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
                energy_ret0 = Energy1;
            }
        }
        if(energy_ret0<50)
            break;
        // TRACE("IQ phase correct energy_ret0 = %d",energy_ret0);
        phasestep = 4;//////////////////////////
        phase_mis_tmp = phase_mis_base + phasestep;
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr); //no mismatch
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        energy_ret2 = Energy1;
        // TRACE("IQ phase correct energy_ret2 = %d",energy_ret2);
        phase_mis_tmp = phase_mis_base - phasestep;
        bt_iqimb_add_mismatch(0,gain_mis_tmp,phase_mis_tmp,0,0,addr); //no mismatch
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        energy_ret1 = Energy1;
        //TRACE("IQ phase correct energy_ret1 = %d",energy_ret1);
        tmp = energy_ret2 -2*energy_ret0 + energy_ret1;
        // TRACE("IQ phase correct energy_ret2 -2*energy_ret0 + energy_ret1 = %d",tmp);
        if(tmp>0)
        {
            tmp = (energy_ret2-energy_ret1)*phasestep/tmp/2;
            tmp = min(tmp,4*phasestep);
            phasestep = phasestep/2;
        }
        else
        {
            tmp = 0;
            iters = iters + 1;
        }
        if((phase_mis_base - tmp>60)||(phase_mis_base - tmp<-60))
            tmp = 0;
        // TRACE("IQ phase correct phase_mis = %d ,phase_adj = %d",phase_mis_base,tmp);
        phase_mis_base = phase_mis_base - tmp;
        energy_ret0_last = energy_ret0;

    }
#endif
    return phase_mis_base;
}

int bt_IQ_DC_Mismatch_Correction_Release(void)
{
    int phase_mis_base;
    int gain_mis_base;
    int Energy,Energy1;
    int dc_i_base,dc_q_base;
    int Cohcnt;
    uint32_t time_start = hal_sys_timer_get();
    int fftsize;
    uint32_t addr = 0xd0310000;
    syspool_init();
    syspool_get_buff(&g_medMemPool, MED_MEM_POOL_SIZE);
    med_heap_init(&g_medMemPool[0], MED_MEM_POOL_SIZE);
    //config gpadc 26m
#ifdef SearchMethod
    fftsize = 512;
    Cohcnt = 1;
    IQMismatchPreprocessState *st;
    st = IQMismatchPreprocessState_init(fftsize);
    Tblgen(st->Table0,st->Table1,st->Table2,fftsize);
    DC_correction(st,&dc_i_base,&dc_q_base,fftsize,Cohcnt);
    bt_Txdc_cal_set(0,dc_i_base); //set cal DC I
    bt_Txdc_cal_set(1,dc_q_base);
    //val = READ_REG(0xd0350348,0x0);
    //TRACE("0xd0350348 = 0x%08x",val);
    bt_iqimb_dma_enable(st->M0data,fftsize);
    caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
    TRACE("dc cal done!!!	 dc_i = %d ,dc_q = %d,Energy1 = %d",dc_i_base,dc_q_base,Energy1);
    fftsize = 512;
    Tblgen_iq(st->Table0,st->Table1,st->Table2,fftsize);
    addr = 0xd03100ac;
    gain_mis_base = 0;
    phase_mis_base = IQ_Phase_Mismatch_Correction(st,gain_mis_base,fftsize,addr,Cohcnt);
    gain_mis_base = IQ_GAIN_Mismatch_Correction(st,phase_mis_base,fftsize,addr,Cohcnt);
// phase_mis_base = -81;
    bt_iqimb_add_mismatch(0,gain_mis_base,phase_mis_base,0,0,addr);
    bt_iqimb_dma_enable(st->M0data,fftsize);
    caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
    TRACE("IQ phase correct addr = 0x%08x,phase_mis = %d ,gain_mis = %d,Energy = %d",addr,phase_mis_base,gain_mis_base,Energy1);
    for(addr=  0xd0310000; addr<= 0xd0310138; addr+=4)
        bt_iqimb_add_mismatch(0,gain_mis_base,phase_mis_base,0,0,addr); //no mismatch
    //IQMismatchPreprocessState_destroy(st);
#else
    int chs = 27;
    fftsize = 1024;
    st = IQMismatchPreprocessState_init(fftsize);
    Tblgen(st->Table0,st->Table1,st->Table2,fftsize);
    DC_correction(st,&dc_i_base,&dc_q_base,fftsize,Cohcnt);
    bt_Txdc_cal_set(0,dc_i_base); //set cal DC I
    bt_Txdc_cal_set(1,dc_q_base);
    //val = READ_REG(0xd0350348,0x0);
    //TRACE("0xd0350348 = 0x%08x",val);
    bt_iqimb_dma_enable(st->M0data,fftsize);
    caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
    TRACE("dc cal done!!!	 dc_i = %d ,dc_q = %d,Energy1 = %d",dc_i_base,dc_q_base,Energy1);
    if(Energy1>200)
    {
        DC_correction(st,&dc_i_base,&dc_q_base,fftsize,Cohcnt);
        bt_Txdc_cal_set(0,dc_i_base); //set cal DC I
        bt_Txdc_cal_set(1,dc_q_base);
        //val = READ_REG(0xd0350348,0x0);
        //TRACE("0xd0350348 = 0x%08x",val);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
        TRACE("dc cal done!!!	 dc_i = %d ,dc_q = %d,Energy1 = %d",dc_i_base,dc_q_base,Energy1);
    }
    if(Energy1>200)
    {
        DC_correction(st,&dc_i_base,&dc_q_base,fftsize,Cohcnt);
        bt_Txdc_cal_set(0,dc_i_base); //set cal DC I
        bt_Txdc_cal_set(1,dc_q_base);
        //val = READ_REG(0xd0350348,0x0);
        //TRACE("0xd0350348 = 0x%08x",val);
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
    }
// TRACE("use time: %d ticks", (hal_sys_timer_get()-time_start));
    TRACE("dc cal done!!!	 dc_i = %d ,dc_q = %d,Energy1 = %d",dc_i_base,dc_q_base,Energy1);
#if 1
    fftsize = 1024;
    Tblgen_iq(st->Table0,st->Table1,st->Table2,fftsize);
    for (int k = 0; k<3; k++)
    {
        if(k==0)
        {
            // WRITE_REG(0x0,0xD02201E4,0x0);
            // WRITE_REG(0x000A000D,0xD02201E4,0x0);
            //osDelay(1000);
            addr = 0xd0310000;// 0xd0310034;
        }
        else if(k==1)
        {
            WRITE_REG(0x0,0xD02201E4,0x0);
            WRITE_REG(0x000A0027,0xD02201E4,0x0);
            addr = 0xd031009c;
        }
        else
        {
            WRITE_REG(0x0,0xD02201E4,0x0);
            WRITE_REG(0x000A0041,0xD02201E4,0x0);
            addr = 0xd0310104;
        }
        gain_mis_base =  IQ_GAIN_Mismatch_Correction(st,0,fftsize,addr,Cohcnt);
        phase_mis_base = IQ_Phase_Mismatch_Correction(st,gain_mis_base,fftsize,addr,Cohcnt);
        bt_iqimb_add_mismatch(0,gain_mis_base,phase_mis_base,0,0,addr); //no mismatch
        bt_iqimb_dma_enable(st->M0data,fftsize);
        caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
//  TRACE("IQ phase correct addr = 0x%08x,phase_mis = %d ,gain_mis = %d,Energy = %d",addr,phase_mis_base,gain_mis_base,Energy1);
        if(Energy1>500)
        {
            if(ABS(gain_mis_base)>ABS(phase_mis_base))
            {
                gain_mis_base =  IQ_GAIN_Mismatch_Correction(st,0,fftsize,addr,Cohcnt);
                phase_mis_base = IQ_Phase_Mismatch_Correction(st,gain_mis_base,fftsize,addr,Cohcnt);
            }
            else
            {
                phase_mis_base = IQ_Phase_Mismatch_Correction(st,0,fftsize,addr,Cohcnt);
                gain_mis_base =  IQ_GAIN_Mismatch_Correction(st,phase_mis_base,fftsize,addr,Cohcnt);
            }

            bt_iqimb_add_mismatch(0,gain_mis_base,phase_mis_base,0,0,addr); //no mismatch
// TRACE("use time: %d ticks", (hal_sys_timer_get()-time_start));
            bt_iqimb_dma_enable(st->M0data,fftsize);
            caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
//  TRACE("IQ phase correct addr = 0x%08x,phase_mis = %d ,gain_mis = %d,Energy = %d",addr,phase_mis_base,gain_mis_base,Energy1);
        }

        if(Energy1>500)
        {
            if(ABS(gain_mis_base)>ABS(phase_mis_base))
            {
                gain_mis_base =  IQ_GAIN_Mismatch_Correction(st,0,fftsize,addr,Cohcnt);
                phase_mis_base = IQ_Phase_Mismatch_Correction(st,gain_mis_base,fftsize,addr,Cohcnt);
            }
            else
            {
                phase_mis_base = IQ_Phase_Mismatch_Correction(st,0,fftsize,addr,Cohcnt);
                gain_mis_base =  IQ_GAIN_Mismatch_Correction(st,phase_mis_base,fftsize,addr,Cohcnt);
            }
            // TRACE("use time: %d ticks", (hal_sys_timer_get()-time_start));
            bt_iqimb_dma_enable(st->M0data,fftsize);
            caculate_energy_main_test(st,&Energy,&Energy1,fftsize,Cohcnt);
            // TRACE("IQ phase correct addr = 0x%08x,phase_mis = %d ,gain_mis = %d,Energy = %d",addr,phase_mis_base,gain_mis_base,Energy1);
        }
        if(k==0)
        {
            chs= 27;
            addr = addr-13*4;
        }
        else if(k==1)
        {
            chs = 26;
            addr = addr - 12*4;
        }
        else
        {
            chs = 26;
            addr = addr -12*4;
        }
        for (int ch=0; ch<chs; ch++)
        {
            bt_iqimb_add_mismatch(0,gain_mis_base,phase_mis_base,0,0,addr); //no mismatch
            // osDelay(1);
            // TRACE("IQ phase correct addr = 0x%08x,phase_mis = %d ,gain_mis = %d,Energy = %d",addr,phase_mis_base,gain_mis_base,Energy1);
            addr = addr +4;
        }
        //TRACE("IQ phase correct addr = 0x%08x,phase_mis = %d ,gain_mis = %d,Energy = %d",addr-4,phase_mis_base,gain_mis_base,Energy1);
    }
#endif
#endif
    TRACE("use time: %d ticks", (hal_sys_timer_get()-time_start));
    return 0;
}

int bt_iqimb_test_ex (int mismatch_type)
{
    bt_IQ_DC_Mismatch_Correction_Release();
    return 1;
}

#endif

