/* cobra_drv.h */ 

#ifndef COBRA_DRV_H_DEFINED 
#define COBRA_DRV_H_DEFINED 
 
#include "cobra_defs.h" 
 
/*******************************************************************************************************/ 
/* Common-user Macros */ 
/*******************************************************************************************************/ 
#define DRIVER_set_complex(c,i,d)               (((c)->integer=((long)(i))),((c)->divider=((unsigned long)(d)))) 
#define DRIVER_register_bitlen(c)               (Register[(c)].bit_count) 
#define DRIVER_errcount_enable(n,l)             (RegisterWrite(n,CX24130_BERRSSELECT,(l), DEMOD_I2C_IO)) 
#define DRIVER_GetRequestedSearchRangeLimit(n)  (n->lnboffset) 
#define _BCD_adjust(b)                          (_BCD_adjust_improved((b),(0))) 
#define LNB_TUNERLSBA                           ((long)(nim->crystal_freq / 10UL) / 2L) 

/*******************************************************************************************************/ 
/* BCD Macros (3-4-02 moved from cobra_defs.h file) */ 
/*******************************************************************************************************/ 
#define BCD_clear(a)     (BCD_set((a),0L)) 
#define BCD_sign(a)      (BCD_getsign(a)) 
#define BCD_getsign(a)   ((signed int)((a)->sign[0] == '-' ? -1 : 1)) 
#define BCD_setsign(a,b) ((a)->sign[0] = (signed char)((b) < 0 ? '-' : '+')) 
#define _BCD_lsd(a)      ((a)->digits[MAX_BCDNO-1])                        /* least-significant-digit */ 
#define BCD_incr(a)      ((_BCD_lsd(a) = (signed char)(_BCD_lsd(a)+BCD_sign(a))),(_BCD_adjust(a))) 
#define BCD_decr(a)      ((_BCD_lsd(a) = (signed char)(_BCD_lsd(a)-BCD_sign(a))),(_BCD_adjust(a))) 
 
/*******************************************************************************************************/ 
/* Driver-only Enums */ 
/*******************************************************************************************************/ 
 
/*******************************************************************************************************/ 
/* RSERRCNT */ 
/*******************************************************************************************************/ 
typedef enum RsErrCnt{                 /* BERRSSelect Settings*/ 
  RSERRCNT_NONE=0x00,                  /*   No count (counter off) */ 
  RSERRCNT_BLOCK=0x01,                 /*   perform block count */ 
  RSERRCNT_BYTE=0x02,                  /*   perform byte count */ 
  RSERRCNT_BIT=0x03                    /*   perform bit count */ 
}RSERRCNT; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
 
#endif  /* #ifndef COBRA_DRV_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

