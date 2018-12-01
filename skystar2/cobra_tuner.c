/* cobra_tuner.c */ 
 
#include "cobra.h"                     /* Cobra include files, ordered */ 
 
#if INCLUDE_ROSIE 
/*******************************************************************************************************/ 
/* TUNER_install_CX24108() Funct to install CX24108 tuner */ 
/*******************************************************************************************************/ 
BOOL  TUNER_install_CX24108(NIM* p_nim) 
{ 
  if (p_nim == NULL)   
  { 
	return(False); 
  } 
 
  /* initialize the various TUNER functions */ 
  TUNER_Initialize         = _TUNER_CX24108_powerup; 
  TUNER_SetFrequency       = _TUNER_CX24108_SetFrequency; 
  TUNER_SetFilterBandwidth = _TUNER_CX24108_SetFilterBandwidth; 
  TUNER_SetGainSettings    = _TUNER_CX24108_SetGainSettings; 
  TUNER_GetPLLFrequency    = _TUNER_CX24108_GetPLLFrequency; 
  TUNER_GetPLLLockStatus   = _TUNER_CX24108_pll_status; 
  
  /* set the TUNER type */ 
  p_nim->tuner_type = CX24108; 
   
  return(True); 
}  /* TUNER_install_CX24108() */ 
 
#endif /* INCLUDE_ROSIE */ 
 
 
 
#if INCLUDE_VIPER 
/*******************************************************************************************************/ 
/* TUNER_install_CX24128() Funct to install CX24128 tuner */ 
/*******************************************************************************************************/ 
BOOL  TUNER_install_CX24128(NIM *p_nim) 
{ 
  if (p_nim == 0)   
  { 
	return(False); 
  } 
 
  /* initialize the various TUNER functions */ 
  TUNER_Initialize         = TUNER_CX24128_powerup; 
  TUNER_SetFrequency       = TUNER_CX24128_SetFrequency; 
  TUNER_SetFilterBandwidth = TUNER_CX24128_SetFilterBandwidth; 
  TUNER_SetGainSettings    = TUNER_CX24128_SetGainSettings; 
  TUNER_GetPLLFrequency    = TUNER_CX24128_GetPLLFrequency; 
  TUNER_GetPLLLockStatus   = TUNER_CX24128_pll_status; 
 
  /* set the TUNER type */ 
  p_nim->tuner_type = CX24128; 
   
  return(True); 
} 
 
#endif /* INCLUDE_VIPER */ 
 
 
#if INCLUDE_RATTLER 
/*******************************************************************************************************/ 
/* TUNER_install_CX24113() Funct to install CX24113 tuner */ 
/*******************************************************************************************************/ 
BOOL  TUNER_install_CX24113(NIM *p_nim) 
{ 
  if (p_nim == 0)   
  { 
	return(False); 
  } 
 
  /* initialize the various TUNER functions */ 
  TUNER_Initialize         = TUNER_CX24113_powerup; /* Rattler specific */ 
  TUNER_SetFrequency       = TUNER_CX24128_SetFrequency; 
  TUNER_SetFilterBandwidth = TUNER_CX24128_SetFilterBandwidth; 
  TUNER_SetGainSettings    = TUNER_CX24128_SetGainSettings; 
  TUNER_GetPLLFrequency    = TUNER_CX24128_GetPLLFrequency; 
  TUNER_GetPLLLockStatus   = TUNER_CX24128_pll_status; 
 
  /* set the TUNER type */ 
  p_nim->tuner_type = CX24113; 
   
  return(True); 
} 
 
#endif /* INCLUDE_RATTLER */ 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/* CR 9509 : Add an extra newline */

