/* cobra_gbls.h */ 
 
#ifndef COBRA_GBLS_H_DEFINED 
#define COBRA_GBLS_H_DEFINED 
 
#ifndef COBRAEXT 
extern  NIM_LIST  nim_list; 
extern  unsigned int      *tuners_supported; 
extern  char     **_tuners_supported; 
#if INCLUDE_VIPER 
/* Global flags. */ 
extern  BOOL FWindowEnable; 
#endif 
 
/* --> */ 
extern  BOOL (*TUNER_SetType)(NIM *nim); 
extern  BOOL (*TUNER_Initialize)(NIM *nim); 
extern  BOOL (*TUNER_SetFilterBandwidth)(NIM *nim,unsigned long bandwidthkhz); 
extern  BOOL (*TUNER_SetGainSettings)(NIM *nim,unsigned long symbolrateksps); 
extern  BOOL (*TUNER_GetPLLLockStatus)(NIM *nim,BOOL *locked); 
extern  BOOL (*TUNER_GetPLLFrequency)(NIM *nim,unsigned long *pllfreq); 
extern  BOOL (*TUNER_SetFrequency)(NIM *nim,unsigned long freq); 
 
#else 
 
NIM_LIST  nim_list;                    /* list of active NIMs */ 
unsigned int *tuners_supported;        /* list of supported tuners */ 
char  **_tuners_supported;      /* names of supported tuners */ 
#if INCLUDE_VIPER 
/* Global flags. */ 
BOOL FWindowEnable; 
#endif 
BOOL (*TUNER_SetType)(NIM *nim); 
BOOL (*TUNER_Initialize)(NIM *nim); 
BOOL (*TUNER_SetFilterBandwidth)(NIM *nim,unsigned long bandwidthkhz); 
BOOL (*TUNER_SetGainSettings)(NIM *nim,unsigned long symbolrateksps); 
BOOL (*TUNER_GetPLLLockStatus)(NIM *nim,BOOL *locked); 
BOOL (*TUNER_GetPLLFrequency)(NIM *nim,unsigned long *pllfreq); 
BOOL (*TUNER_SetFrequency)(NIM *nim,unsigned long freq); 
 
#endif 
 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
 
#endif  /* #ifndef COBRA_GBLS_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

