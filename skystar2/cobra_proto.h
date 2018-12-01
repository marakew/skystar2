/* cobra_proto.h */ 
 
#ifndef COBRA_PROTO_H_DEFINED 
#define COBRA_PROTO_H_DEFINED 
 
/*******************************************************************************************************/ 
/* Include Cobra internal */ 
/*******************************************************************************************************/ 
#include "cobra_regs.h"                /* Cobra register header file */ 
 
#ifdef __cplusplus 
extern "C" { 
#endif  /* __cplusplus */ 
 
/*******************************************************************************************************/ 
/* DRIVER prototypes */ 
/*******************************************************************************************************/ 
#if INCLUDE_DEBUG 
void   DRIVER_SetError(NIM *nim,APIERRNO err,char *filename,int lineno); 
char   *DRIVER_GetError(APIERRNO err); 
#endif /* INCLUDE_DEBUG */ 
 
void   DRIVER_preinit(void); 
BOOL   DRIVER_ValidateNim(NIM *nim); 
BOOL   DRIVER_ValidateNimIq(NIM *nim); 
BOOL   DRIVER_ValidNim(NIM *nim); 
BOOL   _DRIVER_wait(NIM *nim,int mscount); 
unsigned long  API_OS_Time(void); 
BOOL   API_OS_Wait(NIM *nim,int waitms); 
long   DRIVER_symbolrate_in(unsigned long symbolrate,unsigned long sampleratehz); 
long   DRIVER_symbolrate_out(unsigned long symbolrate, unsigned long sampleratehz); 
BOOL   DRIVER_div_zero(NIM *nim,unsigned long l);
unsigned long  DRIVER_compute_demod_pll_mult(NIM *nim,unsigned long Fs, unsigned long Fc); 
unsigned long  DRIVER_compute_fs(unsigned long pllmult,unsigned long Fc); 

/*******************************************************************************************************/ 
/* BCD Prototypes */ 
/*******************************************************************************************************/ 
void   BCD_set(BCDNO *bcdno, unsigned long newval); 
void   BCD_add(BCDNO *bcd, unsigned long add); 
void   BCD_add_bcd(BCDNO *bcd, BCDNO *bcdtoadd); 
void   BCD_mult(BCDNO *bcdtodiv, unsigned long multby); 
void   BCD_mult_bcd(BCDNO *bcd, BCDNO *bcdmultby); 
void   BCD_div_bcd(BCDNO *bcdtodiv, BCDNO *bcd); 
void   BCD_div(BCDNO *bcd, unsigned long divby); 
unsigned long  BCD_out(BCDNO *bcd); 
BOOL   BCD_zero(BCDNO *bcd); 
void   _BCD_adjust_improved(BCDNO *bcd,int ledge); 
void   BCD_move_bcd(BCDNO *bcd, BCDNO *bcdsource); 
void   _BCD_div_ten(BCDNO *bcd); 
void   _BCD_mult_ten(BCDNO *bcd); 
void   _BCD_mult_bcd_ones(BCDNO *bcd, char digit); 
void   _BCD_div_bcd_ones(BCDNO *bcd, char digit); 
int    BCD_compare(BCDNO *bcd, BCDNO *bcd2); 
void   BCD_subt_bcd(BCDNO *bcd, BCDNO *subt); 
void   BCD_subt(BCDNO *bcd, unsigned long subt); 
BCDNO  *BCD_abs(BCDNO *bcd); 
BOOL   BCD_highdigit(BCDNO *bcd, int idx); 
void   _BCD_neg(BCDNO *bcd, int idx); 
BOOL   BCD_leading_low(BCDNO *bcdno); 
void   BCD_test(void); 
void   BCD_print(BCDNO *bcd); 
 
BOOL   DRIVER_SetPNSequence(NIM *nim,BOOL pnflag); 
BOOL   DRIVER_GetPNSequence(NIM *nim,BOOL *pnflag); 
BOOL   DRIVER_error_measurements_off(NIM *nim); 
void   DRIVER_BBB_errinfo(NIM *nim,long *last_count,CMPLXNO *cpx,unsigned long errwindow,unsigned long errmult,MSTATUS *mstat); 
BOOL   DRIVER_BBB_error_off(NIM *nim); 
BOOL   DRIVER_PNBER_error_off(NIM *nim); 
BOOL   DRIVER_errcount_disable(NIM *nim); 
BOOL   DRIVER_Reset(NIM *nim); 
BOOL   DRIVER_HardReset(NIM *nim); 
BOOL   DRIVER_Preset(NIM *nim); 
BOOL   DRIVER_CxType(NIM *nim,DEMOD *demod,char **demod_str); 
BOOL   DRIVER_AcqSetViterbiSearchList(NIM *nim,VITLIST*); 
BOOL   DRIVER_AcqGetViterbiSearchList(NIM *nim,VITLIST*); 
BOOL   DRIVER_SetViterbiRate(NIM *nim,CODERATE coderate); 
BOOL   DRIVER_GetViterbiRate(NIM *nim,CODERATE *coderate); 
 
BOOL   DRIVER_Default(NIM *nim); 
long   DRIVER_convert_twos(unsigned long numeric,int bitslen); 
unsigned long  DRIVER_convert_twos_saturate(long,int); 
BOOL   DRIVER_SetSmoothClock(NIM *nim,CLOCKSMOOTHSEL cs,BOOL fromCC); 
BOOL   DRIVER_SetSmoothClockEn(NIM *nim,CLOCKSMOOTHSEL cs); 
BOOL   DRIVER_SWAssistInit(NIM *nim);
BOOL   DRIVER_SWAssistExit(NIM *nim);
BOOL   DRIVER_SWAssistAcq(NIM *nim);
BOOL   DRIVER_SWAssistAcq_CR1DIV2(NIM *nim,unsigned int vrates);
BOOL   DRIVER_SWAssistAcqBinning(NIM *nim,long freq,unsigned long *locked);
BOOL   DRIVER_SWAssistTuner(NIM *nim);      /* (CR 6243) */
BOOL   _DRIVER_SWAssistTunerPass(NIM *nim,unsigned long freq,BOOL *hit);  /* (CR 6243) */
BOOL   DRIVER_SetLNBMode(NIM *nim,LNBMODE *lnbmode); 
BOOL   DRIVER_SendDiseqc(NIM *nim,unsigned char *msg,unsigned char msg_len,BOOL msg_long, BOOL last_msg); 
BOOL   DRIVER_SetTunerFrequency(NIM *nim,unsigned long freq); 
BOOL   DRIVER_ValidTunerFreqRange(NIM *nim,unsigned long freq); 

#ifdef OPTIMAL_FS_CODE
BOOL   DRIVER_Opt_Fs_buildtable(NIM *nim);
BOOL   DRIVER_Opt_Fs_calcPLLMult(NIM *nim);
unsigned long  DRIVER_Opt_Fs_Hbelow(NIM *nim,unsigned long Fa,unsigned long clk);
unsigned long  DRIVER_Opt_Fs_Hfastclock(NIM *nim,unsigned int harmcnt,long *harmtable,unsigned long fastclock);
BOOL   DRIVER_Opt_Fs_optimizeFs(NIM *nim,unsigned long Fa,unsigned int *bestpllmult);
#endif  /* #ifdef OPTIMAL_FS_CODE */

BOOL   DRIVER_SetSoftDecisionThreshold(NIM *nim); 
BOOL   DRIVER_SetTunerFilterBWVoltage(NIM *nim,unsigned long mV); 
BOOL   DRIVER_SetRSCntlPin(NIM *nim,REGIDX regIdx,RS_CNTLPIN_SEL RSCntlPinSel); 
BOOL   DRIVER_SetViterbiLockThresh(NIM *nim,CODERATE coderate); 
BOOL   DRIVER_Cobra(NIM *nim);

/*******************************************************************************************************/ 
/* Camaric code */
/*******************************************************************************************************/ 
#ifdef CAMARIC_FEATURES
BOOL  DRIVER_Camaric(NIM *nim); 
BOOL  DRIVER_Default_Camaric(NIM *nim); 
BOOL  DRIVER_Cobra_compat(NIM *nim);
TRANSPEC  DRIVER_Tspec_Get_Auto(NIM *nim);
unsigned long  DRIVER_Set_Pdmfout(NIM *nim,unsigned long symbolrate, unsigned long samplerate); 
long  DRIVER_calc_pdmf_closer(long Fsym1,long fsym2,unsigned long samplerate,long pdin); 
long  DRIVER_calc_closest_pdmfout(unsigned long symbolrate, unsigned long samplerate, unsigned long bondary); 
unsigned long  DRIVER_log2(unsigned long x); 
BOOL  DRIVER_SetCTLTrackBW(NIM *nim,unsigned long symbolrateksps); 
BOOL  DRIVER_DCIISetACQDmdWin(NIM *nim,CODERATE coderate);
#endif  /* #ifdef CAMARIC_FEATURES */

/*******************************************************************************************************/ 
/* [cobra_iq.c] IQ constellation prototypes */ 
/*******************************************************************************************************/ 
#ifdef INCLUDE_CONSTELLATION 
BOOL  IQ_Sample(NIM *nim); 
BOOL  IQ_SampleGetIq(NIM *nim,unsigned char*,unsigned char*); 
int   IQ_ConstCount(NIM *nim); 
#endif /* #ifdef INCLUDE_CONSTELLATION */ 
 
/*******************************************************************************************************/ 
/* [cobra_regs.c] low-level register i/o functions */ 
/*******************************************************************************************************/ 
BOOL RegisterWrite(NIM* p_nim, unsigned short reg_field_index, unsigned long value, IO_METHOD io_method); 
BOOL RegisterRead(NIM* p_nim, unsigned short reg_field_index, unsigned long* p_value, IO_METHOD io_method); 
 
unsigned long RegisterHDWRMask(REGIDX); 
int   Register_bitlength(REGIDX regidx); 
BOOL  RegisterWritePLLMult(NIM *nim, unsigned long ulRegVal); 
#ifdef CAMARIC_FEATURES
BOOL  RegisterReadCentralFreq(NIM *nim, unsigned long *pFreq); 
BOOL  RegisterWriteCentralFreq(NIM *nim, unsigned long ulFreq); 
#endif  /* #ifdef CAMARIC_FEATURES */
BOOL  RegisterWriteClkSmoothDiv(NIM *nim, unsigned long ulClkSmoothDiv); 
 
/*******************************************************************************************************/ 
/* [cobra_regs.c] Register map verify functions.  exec'd with InitEnv() to verify backbone of system */ 
/*******************************************************************************************************/ 
#if INCLUDE_DEBUG 
BOOL RegMapTest (NIM *nim); 
#endif /* INCLUDE_DEBUG */ 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#ifdef __cplusplus 
} 
#endif 
 
#endif  /* #ifndef COBRA_PROTO_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

