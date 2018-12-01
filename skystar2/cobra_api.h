/* cobra_api.h */ 
 
#ifndef COBRA_API_H_DEFINED 
#define COBRA_API_H_DEFINED 
 
#ifdef __cplusplus 
extern "C" { 
#endif  /* __cplusplus */  
 
/*******************************************************************************************************/ 
/* API prototypes */ 
/*******************************************************************************************************/ 
BOOL  API_InitEnvironment(NIM *nim, unsigned long demodhandle, WRITE_SB SBwrite, READ_SB SBread,BOOL (*TUNER_install)(NIM *nim),unsigned long crystalfreq,VCOINIT vcoinit,MPEG_OUT *mpeg,LNBMODE *lnbmode,BOOL (*waitfunct)(NIM *nim,int mscount), void *ptuner); 
BOOL  API_ChangeChannel(NIM *nim,CHANOBJ *chanobj); 
BOOL  API_Monitor(NIM *nim,ACQSTATE *acqstate,LOCKIND *lockind); 
BOOL  API_NIMGetChipInfo(NIM *nim,char **demod_string ,char **Tuner_string ,int *demod_type,int *tuner_type,int *board_type); 
BOOL  API_GetDriverVersion(NIM *nim,VERDRV *verdrv); 
BOOL  API_ReleaseEnvironment(NIM *nim); 
BOOL  API_SetTunerFrequency(NIM *nim,unsigned long freq); 
BOOL  API_GetTunerFrequency(NIM *nim,unsigned long *freq); 
BOOL  API_SetTunerBW(NIM *nim, unsigned long tunerBW, long *sigmadelta); 
BOOL  API_SetOutputOptions(NIM *nim,MPEG_OUT *mpeg_out); 
BOOL  API_SetInterruptOptions(NIM *nim,INTEROPTS interopts); 
BOOL  API_SetSearchRangeLimit(NIM *nim,unsigned long lnboffset,unsigned long *actual); 
BOOL  API_GetSearchRangeLimit(NIM *nim,unsigned long *lnboffset); 
BOOL  API_SetModulation(NIM *nim,MODTYPE modtype); 
BOOL  API_SetSampleFrequency(NIM *nim, SAMPFRQ sampfrq); 
BOOL  __API_SetSampleFrequency(NIM *nim, unsigned long sampratehz); 
BOOL  API_GetSampleFrequency(NIM *nim,unsigned long *samplerate); 
BOOL  API_SetTransportSpec(NIM *nim,TRANSPEC transpec); 
BOOL  API_GetTransportSpec(NIM *nim,TRANSPEC *transpec); 
BOOL  API_SetSymbolRate(NIM *nim,SYMBRATE symbolrate); 
BOOL  API_GetSymbolRate(NIM *nim,SYMBRATE *symbolrate); 
BOOL  API_GetMinSymbolRate(NIM *nim,unsigned long *minsymbolrate); 
BOOL  API_GetMaxSymbolRate(NIM *nim,unsigned long *maxsymbolrate); 
BOOL  API_SetViterbiRate(NIM *nim,CODERATE coderate); 
BOOL  API_GetViterbiRate(NIM *nim,CODERATE *coderate); 
BOOL  API_SetSpectralInversion(NIM *nim,SPECINV specinv); 
BOOL  API_GetSpectralInversion(NIM *nim,SPECINV *specinv); 
BOOL  API_AcqBegin(NIM *nim); 
BOOL  API_AcqContinue(NIM *nim,ACQSTATE *acqstate); 
BOOL  API_AcqSetViterbiCodeRates(NIM *nim,unsigned int vcr); 
BOOL  API_AcqGetViterbiCodeRates(NIM *nim,unsigned int *vcr); 
BOOL  API_GetPendingInterrupts(NIM *nim,INTEROPTS *interopts); 
BOOL  _API_ClearPendingInterrupts(NIM *nim,INTEROPTS intropts); 
BOOL  API_GetLockIndicators(NIM *nim,LOCKIND *lockind); 
BOOL  API_SetDemodErrorMode(NIM *nim,ERRORMODE errmode); 
BOOL  API_GetDemodErrorMode(NIM *nim,ERRORMODE *errmode); 
BOOL  API_GetEffectiveFrequency(NIM *p_nim, unsigned long *p_effect_frequency); 
BOOL  API_DetectTuner(unsigned long  demod_handle,  WRITE_SB SBwrite, READ_SB SBread, TUNER* p_tuner_type); 
 
BOOL  API_SetTunerGainThreshold(NIM* p_nim, signed char threshold_dBm); 
BOOL  API_GetTunerGainThreshold(NIM* p_nim, signed char* p_threshold_dBm); 
 
#if INCLUDE_DEBUG 
char* API_GetErrorMessage(NIM *nim,APIERRNO __errno); 
int   API_GetLastError(NIM *nim); 
char* API_GetErrorFilename(NIM *nim); 
unsigned long API_GetErrorLineNumber(NIM *nim); 
#endif /* INCLUDE_DEBUG */ 
 
BOOL  API_GetChannelEsNo(NIM *nim,ESNOMODE emode,CMPLXNO *esno,MSTATUS *mstat); 
BOOL  API_GetPNBER(NIM *nim,PNBER errwindow,CMPLXNO *pnber,MSTATUS *mstat); 
BOOL  API_GetBER(NIM *nim,unsigned long errwindow,CMPLXNO *ber,MSTATUS *mstat); 
BOOL  API_GetByteErrors(NIM *nim,unsigned long errwindow,CMPLXNO *byteerr,MSTATUS *mstat); 
BOOL  API_GetBlockErrors(NIM *nim,unsigned long errwindow,CMPLXNO *blockerr,MSTATUS *mstat); 
BOOL  API_GetNormCount(NIM *nim,unsigned char *normcounter); 
BOOL  API_GetFrequencyOffset(NIM *nim,long *freqoffset); 
BOOL  API_GetAcquisitionOffset(NIM *nim,long *lnboffset); 
BOOL  API_SetCentralFreq(NIM *nim,long centralfreq); 
BOOL  API_GetCentralFreq(NIM *nim,long *centralfreq); 
BOOL  API_GetCTL(NIM *nim,long *ctl); 
BOOL  API_EnableRSCorrection(NIM *nim,BOOL opt); 
BOOL  API_GetAGCAcc(NIM *nim,AGCACC *agcacc); 
BOOL  API_GetBTL(NIM *nim,long *btl); 
BOOL  API_SetLNBDC(NIM *nim,LNBPOL lnbpol); 
BOOL  API_SetLNBMode(NIM *nim,LNBMODE *lnbmode); 
BOOL  API_SetLNBTone(NIM *nim,LNBTONE lnbtone); 
BOOL  API_SendDiseqcMessage(NIM *nim,unsigned char *message,unsigned char message_length,BOOL last_message,LNBBURST bursttype); 
BOOL  API_ReadReg(NIM *nim,int reg,unsigned char *data); 
BOOL  API_WriteReg(NIM *nim,int reg,unsigned char *data); 
BOOL  API_SetSleepMode(NIM *nim,BOOL sleep); 
BOOL  API_GetTunerStructure (NIM *nim, ACTIVE_TUNER* tuner); 
 
#ifdef INCLUDE_CONSTELLATION 
BOOL  API_ConstOn(NIM *nim,IQPAK *iqpak); 
BOOL  API_ConstOff(NIM *nim); 
int   API_ConstCount(NIM *nim); 
BOOL  API_ConstSetBusy(NIM *nim,BOOL busy_state); 
BOOL  API_ConstGetBusy(NIM *nim); 
BOOL  API_ConstGetPoints(NIM *nim,unsigned char *ivals,unsigned char *qvals,int iqcount); 
BOOL  API_ConstGetIQSample(NIM *nim,unsigned char *I,unsigned char *Q); 
BOOL  API_ConstGetUnbufferredIQSample(NIM *nim,signed char *I,signed char *Q); 
#endif  /* #ifdef INCLUDE_CONSTELLATION */ 
 
BOOL  API_SetDriverWait(NIM *nim,BOOL (*waitfunct)(NIM *nim,int mscount)); 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#ifdef CAMARIC_FEATURES
#ifdef INCLUDE_DISEQC2 
BOOL  API_DiseqcReceiveMessage(NIM *nim, unsigned char *buffer, int buffer_len, int *received_len, RXMODE rxmode, int *parityerrors); 
BOOL  API_DiseqcSetRxMode(NIM *nim, RXMODE rxmode); 
BOOL  API_DiseqcGetRxMode(NIM *nim, RXMODE *rxmode); 
BOOL  API_Diseqc22KHzSetAmplitude(NIM *nim, int amplitude); 
BOOL  API_Diseqc22KHzGetAmplitude(NIM *nim, int *amplitude); 
BOOL  API_DiseqcSetVersion(NIM *nim, DISEQC_VER dv); 
BOOL  API_DiseqcGetVersion(NIM *nim, DISEQC_VER *dv); 
BOOL  API_SetDiseqcInterrupt(NIM *nim,BOOL on); 
BOOL  API_GetDiseqcInterrupt(NIM *nim,INTEROPTS *interopts); 
BOOL  API_ClearDiseqcInterrupt(NIM *nim); 
#endif /* #ifdef INCLUDE_DISEQC2 */ 

BOOL  API_LNBSetDrain(NIM *nim, LNBDRAIN  lnbdrain); 
BOOL  API_LNBGetDrain(NIM *nim, LNBDRAIN  *lnbdrain); 
//...
#endif 
BOOL  API_GetPLLFrequency(NIM *nim,unsigned long *pllfreq); 
BOOL  API_GetPowerEstimation(NIM* p_nim, signed char* p_power); 
 
#if INCLUDE_ROSIE 
BOOL  API_FindVCOEdges(NIM *nim,RDIVVAL rdiv); 
#endif 
 
BOOL   API_GetLNBDC(NIM    *nim, LNBPOL *lnbpol);  
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
 
#ifdef __cplusplus 
} 
#endif 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
 
#endif  /* #ifndef COBRA_API_H_DEFINED */  
/* CR 9509 : Add an extra newline */

