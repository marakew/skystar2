/* cobra_cx24113.c */ 
 
#include "cobra.h"                     /* Cobra include files, ordered */ 
 
#if INCLUDE_RATTLER 
 
extern const REGISTER_MAP viper_register_map[]; 
 
 
BOOL  TUNER_CX24113_powerup(NIM *p_nim); 
 
/******************************************************************************************************* 
 * TUNER_CX24128_powerup()  
 * performs tuner power-up reset  
 *******************************************************************************************************/ 
BOOL         
TUNER_CX24113_powerup(NIM  *p_nim)  /* pointer to nim */ 
{ 
	if (TUNER_CX24128_powerup(p_nim) == False) 
	{ 
		return (False); 
	}     
    return(True); 
}/* TUNER_CX24113_powerup() */ 
 
#endif /* INCLUDE_RATTLER */