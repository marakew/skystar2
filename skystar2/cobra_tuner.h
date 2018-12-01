/* cobra_tuner.h */ 
 
#ifndef COBRA_TUNER_H_DEFINED 
#define COBRA_TUNER_H_DEFINED 
 
//#include "cobra.h"
 
#ifdef __cplusplus 
extern "C" { 
#else  
#endif  /* __cplusplus */  
 
/*******************************************************************************************************/ 
/* TUNER_install_CX24108() -- Tuner functions able to be installed/controlled by the driver */ 
/*******************************************************************************************************/ 
#if INCLUDE_ROSIE 
BOOL  TUNER_install_CX24108(NIM *nim); 
#endif /* INCLUDE_ROSIE */ 
#if INCLUDE_VIPER 
BOOL  TUNER_install_CX24128(NIM *nim); 
#endif /* INCLUDE_VIPER */ 
#if INCLUDE_RATTLER 
BOOL  TUNER_install_CX24113(NIM *nim); 
#endif /* INCLUDE_RATTLER */ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
 
#ifdef __cplusplus 
} 
#endif 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
 
#endif  /* #ifndef COBRA_TUNER_H_DEFINED */ 
/* CR 9509 : Add an extra newline */
