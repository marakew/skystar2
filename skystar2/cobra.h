/* cobra.h */ 
 
#ifndef COBRA_H_DEFINED 
#define COBRA_H_DEFINED 
 
/*******************************************************************************************************/ 
/* standard definitions */ 
/*******************************************************************************************************/ 
#define ON			1 
#define INCLUDE_DEBUG		1 

/* Tuners - at least one tuner should be enabled */ 
//kir++
#define INCLUDE_ROSIE		0
#define INCLUDE_VIPER		1
#define INCLUDE_RATTLER		1

#define INCLUDE_CONSTELLATION          /* Include Constellation code at compile-time if defined */ 
//#define STRIP_REGNAMES                 /* If defined, register str name column is discarded in the register map */ 
#define INCLUDE_DISEQC2                /* Include Diseqc 2.x code at compile-time if defined */ 
#define INCLUDE_BANDWIDTHADJ	1      /* Include Anti-alias bandwidth adjust */ 

#if INCLUDE_ROSIE 
#define INCLUDE_REPEATER	0 
#endif 
 
#if INCLUDE_RATTLER 
#define INCLUDE_REPEATER	1 
#endif 
 
//kir++
#define INCLUDE_VIPER_BTI	0
#define CAMARIC_FEATURES
#define OPTIMAL_FS_CODE
#define DCII_DEFAULT_SMOOTCLK

/*******************************************************************************************************/ 
/* standard inclusions */ 
/*******************************************************************************************************/ 
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/ctype.h>
 
#include "cobra_ver.h"                 /* version info */ 
#include "cobra_enum.h" 
#include "cobra_defs.h" 
#include "cobra_str.h" 
#include "cobra_api.h" 
#include "cobra_tuner.h" 
#include "cobra_gbls.h" 
//#include "cobra_bcd.h"  
#include "cobra_drv.h"                  
#include "cobra_proto.h" 
 

#if INCLUDE_ROSIE 
#include "cobra_cx24108.h" 
#endif	/* #if INCLUDE_ROSIE */
#if INCLUDE_VIPER 
#include "cobra_cx24128.h" 
#endif  /* #if INCLUDE_VIPER */ 
#if INCLUDE_RATTLER 
#include "cobra_cx24113.h" 
#endif  /* #if INCLUDE_RATTLER */ 

/*******************************************************************************************************/ 
/* hardware-specific definitions */ 
/*******************************************************************************************************/ 
 
/*******************************************************************************************************/ 
/* Macros */ 
/*******************************************************************************************************/ 
#if INCLUDE_DEBUG 
#define  DRIVER_SET_ERROR(a,b)		(DRIVER_SetError((a),(b),(__FILE__),(__LINE__))) 
 
#define  DRIVER_SET_ERROR_MSG(a,b,m,i)	(DRIVER_SetError((a),(b),(m),(i))) 
 
#define  TUNER_CX24128_VALIDATE(n)	if (TUNER_CX24128_validate(n) == False)		\
						return(False); 
 
#define  TUNER_CX24108_VALIDATE(n)	if (TUNER_CX24108_validate(n) == False)	\
						return(False); 
 
#define  DRIVER_VALIDATE_NIM(a)		if (DRIVER_ValidateNim(a) == False)		\
					{						\
						DRIVER_SET_ERROR (a, API_BAD_PARM);	\
						return(False);				\
					} 
 
#define  REGMAP_TEST(n)			if (RegMapTest(n) == False)	\
						return(False);
#else  /* INCLUDE_DEBUG */ 
#define  DRIVER_SET_ERROR(a,b)		{}/* do nothing */ 
#define  DRIVER_SET_ERROR_MSG(a,b,m,i)	{}/* do nothing */ 
#define  TUNER_CX24128_VALIDATE(n)	{}/* do nothing */ 
#define  TUNER_CX24108_VALIDATE(n)	{}/* do nothing */ 
#define  DRIVER_VALIDATE_NIM(a)		{}/* do nothing */  
#define  REGMAP_TEST(n)			{}/* do nothing */  
#endif /* INCLUDE_DEBUG */ 
 
#define  API_ClearPendingInterrupts(a)  (_API_ClearPendingInterrupts((a),((INTEROPTS)INTR_LNB_REPLY_READY|(INTEROPTS)(INTR_ACQ_SYNC|INTR_ACQ_FAILURE|INTR_VITERBI_LOSS|INTR_VITERBI_SYNC|INTR_DEMOD_LOSS|INTR_DEMOD_SYNC)))) 
 
#ifndef  CMPLXMAC 
#define  CMPLX_set(c,i,d)        (((c)->integer=((long)i)),((c)->divider=((unsigned long)d))) 
#endif /* #ifndef  CMPLXMAC */ 
 
/*******************************************************************************************************/ 
#ifndef  min 
#define  min(a,b)                ((a) < (b) ? (a) : (b)) 
#endif 
 
/*******************************************************************************************************/ 
#ifndef  max 
#define  max(a,b)                ((a) > (b) ? (a) : (b)) 
#endif 
 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#endif  /* #ifndef COBRA_H_DEFINED */ 
/* CR 9509 : Add an extra newline */
