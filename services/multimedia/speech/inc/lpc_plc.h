#ifndef LPC_PLC_H
#define LPC_PLC_H

#include "typedef.h"

/* G722 PLC basic parameters */
#define  SF		   16		/* input Sampling Frequency (in kHz) */
#define  LPCO	   8		/* LPC predictor Order */
#define  DECF	   8 		/* DECimation Factor for coarse pitch period search */
#define  DFO     60    /* Decimation Filter Order (for coarse pitch extraction) */
#define  FRSZ    120   /* FRame SiZe (in 16 kHz samples) */
#define  WINSZ	 120   /* lpc analysis WINdow SiZe (in 16 kHz samples) */
#define  PWSZ	   240   /* Pitch analysis Window SiZe (in 16 kHz samples) */
#define  WML     160   /* pitch refinement Waveform-Matching window Length */
#define  MAXPP	 265	 /* MAXimum Pitch Period (in 16 kHz samples) */
#define  MINPP   40    /* MINimum Pitch Period (in 16 kHz samples) */

#define  GATTST	  2     /* frame index into erasure to start gain atten */
#define  GATTEND	6     /* frame index into erasure to stop gain atten */
#define	 OLAL	   20    /* OLA Length for 1st bad frame in erasure */
#define  OLALG	 60    /* OLA Length for 1st Good frame after erase */
#define  SOLAL	 8     /* Short OverLap-Add window Length, first good frame */
#define  PPHL    5     /* Pitch Period History buffer Length (frames) */
#define  MLO     20    /* figure of Merit LOw threshold */
#define	 MHI     28    /* figure of Merit HIgh threshold */
#define  MAXOS   80    /* MAXimum # of samples in waveform OffSet for time warping */

/* derived parameters and constants */
#define  FRSZD    (FRSZ/DECF)       /* FRame SiZe in Decimated domain */
#define  MAXPPD   34				      /* ceil(MAXPP/DECF); MAX PP in Decimated domain */
#define  MAXPPD1  (MAXPPD+1)        /* MAXPPD + 1 */
#define  MINPPD   5                 /* floor(MINPP/DECF); MIN PP in Decimated domain */
#define  PWSZD    (PWSZ/DECF)	      /* Pitch analysis Window SiZe in Decimated domain */
#define  XQOFF    (WML+MAXPP+1)	   /* xq[] offset before current frame */
#define  LXQ      (XQOFF+FRSZ)	   /* Length of xq[ ] buffer */
#define  LXD      (MAXPPD+1+PWSZD)  /* Length of xwd[ ] (input X[ ] weighted and Decimated) */
#define  XDOFF    (LXD-FRSZD)       /* XwD[ ] array OFFset for current frame */

/* the following are used in coarptch */
#define cpp_Qvalue  3
#define cpp_scale   (1<<cpp_Qvalue)
#define HMAXPPD (MAXPPD/2)
#define M1  (MINPPD-1)
#define M2  MAXPPD1
#define HDECF (DECF/2)
#define TH1       23921    /* first threshold for cor*cor/energy   */
#define TH2       13107    /* second threshold for cor*cor/energy  */
#define LPTH1     25559    /* Last Pitch cor*cor/energy THreshold 1 */
#define LPTH2     14090    /* Last Pitch cor*cor/energy THreshold 2 */
#define MPDTH     1966     /* Multiple Pitch Deviation THreshold */
#define SMDTH     3113     /* Sub-Multiple pitch Deviation THreshold  0.125 */
#define MPTH4     9830
#define MAX_NPEAKS  7      /* MAXimum Number of PEAKS in coarse pitch search */

#define	UPBOUND  16384   /* UPper BOUND of scaling factor for plc periodic extrapolation Q14 */
#define	DWNBOUND -16384  /* LOwer BOUND of scaling factor for plc periodic extrapolation Q14 */

#define AHP           31785 /* 0.97 in Q15 */
#define W_NBH_TRCK		31785 /* 0.97 in Q15 */
#define W_NBH_TRCK_M1	  983 /* 0.03 in Q15 */
#define W_NBH_CHNG		32512 /* 127/128 in Q15 */
#define W_NBH_CHNG_M1	  256 /* 1/128 in Q15 */
#define W_NBL_TRCK		31785 /* 0.97 in Q15 */
#define W_NBL_TRCK_M1	  983 /* 0.03 in Q15 */
#define W_NBL_CHNG		32512 /* 127/128 in Q15 */
#define W_NBL_CHNG_M1	  256 /* 1/128 in Q15 */
#define LBO 12  /* Low band Offset in 16kHz domain */
#define	xqoff	(XQOFF+FRSZ)

struct WB_PLC_State {
	Word32   energymax32;
	Word32   cormax;
	Word16   wsz;
	Word16   scaled_flag;
	Word16   xq[LXQ + 24 + MAXOS];
	Word16   stsyml[LPCO];
	Word16   al[1 + LPCO];
	Word16   alast[1 + LPCO];
	Word16   ppt;
	Word16   stwpml[LPCO];
	Word16   xwd[XDOFF];
	Word16   xwd_exp;
	Word16   dfm[DFO];
	Word16   scaler;
	Word16   merit;
	Word16   ptfe;
	Word16   ppf;
	Word16   ppinc;
	Word16   pweflag;
	Word16   cpplast;
	Word16   pph[PPHL];
	Word16   pp;
	Word16   cfecount;
	Word16   ngfae;
	Word16   nfle;
	Word16   avm;
	Word16   lag;
};

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
 * Function: Reset_WB_PLC()
 *
 * Description: reset the plc state variables
 *
 * Inputs:  *plc  - pointer to plc state memory
 *
 * Outputs: *plc  - values reset
 *---------------------------------------------------------------------------*/
void Reset_WB_PLC(struct WB_PLC_State *plc);

/*-----------------------------------------------------------------------------
 * Function: WB_PLC()
 *
 * Description: PLC function called in good frames
 *
 * Inputs:  *plc  - pointer to plc state memory
 *          *out  - pointer to output buffer
 *          *inbuf- pointer to the good frame input
 *
 * Outputs: *out  - potentially modified output buffer
 *---------------------------------------------------------------------------*/
void	WB_PLC(struct 	WB_PLC_State *plc, Word16 	*out,Word16 	*inbuf);

/*-----------------------------------------------------------------------------
 * Function: WB_PLC_erasure()
 *
 * Description: PLC function called in bad frames
 *
 * Inputs:  *plc  - pointer to plc state memory
 *          *out  - pointer to output buffer
 * Outputs: *qdb  - extrapolated samples beyond the output buffer required
 *                  for qmf memory, ringing, and rephasing
 *---------------------------------------------------------------------------*/
void WB_PLC_erasure(struct  WB_PLC_State *plc, Word16 *out, Word16 *encbuf, Word16 *qdb);

#ifdef __cplusplus
}
#endif

#endif