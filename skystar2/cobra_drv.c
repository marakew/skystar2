/* cobra_drv.c */ 
 
#define COBRAEXT                       /* forces globals to be static to this function */ 
 
#include "cobra.h"                     /* Cobra include files, ordered */ 
#include "cobra_regs.h"                /* Cobra Internal */ 
 
/*******************************************************************************************************/ 
/* static arrays visible to this file only */ 
/*******************************************************************************************************/ 
static int       dvb_cr_equate[] = {1,2,3,4,5,6,7,0};
static CODERATE  dvb_cr_list[] =  {CODERATE_1DIV2,CODERATE_2DIV3,CODERATE_3DIV4,
                                   CODERATE_4DIV5,CODERATE_5DIV6,CODERATE_6DIV7,
                                   CODERATE_7DIV8,CODERATE_NONE};
static int       dss_cr_equate[] = {1,2,6,0};
static CODERATE  dss_cr_list[] =  {CODERATE_1DIV2,CODERATE_2DIV3,CODERATE_6DIV7,
                                   CODERATE_NONE};
static int       dcii_cr_equate[] = {0,1,2,3,4,5,6,7,0};
static CODERATE  dcii_cr_list[] = {CODERATE_5DIV11,CODERATE_1DIV2,CODERATE_3DIV5,
                                   CODERATE_2DIV3,CODERATE_3DIV4,
                                   CODERATE_4DIV5,CODERATE_5DIV6,
                                   CODERATE_7DIV8,CODERATE_NONE};

#if INCLUDE_DEBUG 
/*******************************************************************************************************/ 
/* DRIVER_SetError() */ 
/*******************************************************************************************************/ 
void       DRIVER_SetError(            /* function to record error number into nim */ 
NIM        *nim,                       /* nim pointer */ 
APIERRNO   err,                        /* API error number */ 
char       *filename,                  /* filename (or useful info) (or NULL) where error occurred */ 
int        lineno)                     /* line number where error occurred */ 
{ 
  /* usage:  It is important to place this error-recording function at or near */ 
  /* point of error, in order to track and find program and line of error      */ 
  /* see:  Macro -> DRIVER_SET_ERROR */ 
 
  /* validate nim, if invalid nim, nothing to do, so bail */ 
  if (nim == NULL)
	return;

  /*
   if (DRIVER_ValidNim(nim) == False)
	return;
     No need to validate */ 
 
  /* record only the first error, not the last one encountered */ 
  if (nim->__errno == API_NOERR) 
  { 
    nim->__errno = err; 
    nim->errfname = filename; 
    nim->errline = lineno; 
  } 
  return; 
 
}  /* DRIVER_SetError() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_GetError() */ 
/*******************************************************************************************************/ 
char  *DRIVER_GetError(                /* function to retrieve  string info concerning error no in nim */ 
APIERRNO   err)                        /* error number */ 
{ 
  int  i; 
  static NIM_ERROR  nimerr[] =                      /* static list of errors (add to list as needed) */ 
  { 
    {API_NOERR,       "(No error)"},                /* No error encountered */ 
    {API_BADPTR,      "Bad pointer"},               /* bad pointer passed to API function */ 
    {API_INVALID_NIM, "Bad NIM pointer"},           /* bad nim pointer passed to API */ 
    {API_NIM_OPENED,  "NIM already opened"},        /* bad nim pointer passed to API */ 
    {API_SBIO_NULL,   "User SBIO functs are NULL"}, /* user-supplied SBRead or SBWrite funct is NULL */ 
    {API_TIMER_NULL,  "Invalid Timer() function"},  /* timer specified at init is not valid */ 
    {API_NIM_NULL,    "NIM ptr is NULL"},           /* nim pointer passed is null */ 
    {API_IO_READERR,  "I/O Err: read"},             /* error encountered at read demod */ 
    {API_IO_WRITERR,  "I/O Err: write"},            /* error encountered at write demod */ 
    {API_INVALID_TUNER,"tuner type is not valid"},  /* tuner-type invalid or not supported */ 
    {API_TUNER_PARMS, "tunerparms pointer is NULL"},/* tuner-parms pointer is NULL */ 
    {API_CONST_IQBUF, "NULL at ConstGetPoints()"},  /* NULL pointer pased at ConstGetPoints() */ 
    {API_CONST_IQLOW, "Too few ConstGetPoints()"},  /* too few constellation pts, reduce count or try later */ 
    {API_BAD_PARM,    "API: Bad parm passed"},      /* bad parameter passed by caller -- see fil/line to determine error */ 
    {API_SETILLEGAL,  "Warn: API illegal setting"}, /* requested setting is illegal and ignored */ 
    {API_BAD_RTNVAL,  "API: Bad return value"},     /* hardware returned a n invalid result */ 
    {API_PARM_RANGE,  "API: Parm bounds error"},    /* parm passed was out of valid range */ 
    {API_LOCKIND_ERR,     "API: Error reading lockind"},/* unable to read a lockind register */ 
    {API_REG_MATCH_IDX,   "Register index corrupt"},    /* Register array built in cobra_reg.h is corrupt */ 
    {API_REG_MATCH_TRX,   "Register translate error"},  /* unable to translate from raw to enum */ 
    {API_REG_MATCH_DTRX,  "Register detranslate error"},/* unable to translate from raw to enum */ 
    {API_REG_VERFY_IDX,   "Reg.Map: Idx != rec no"},    /* register map index does not match linear position */ 
    {API_REG_VERFY_ADDR,  "Reg.Map: Addr range err"},   /* reg.map addr variable out of range */ 
    {API_REG_VERFY_REGRW, "Reg.Map: regrw field err"},  /* reg.map regrw field contains invalid data */ 
    {API_REG_VERFY_REGFLT,"Reg.Map: bad filter val"},   /* reg.map regfilter field contains invalid data */ 
    {API_REG_VERFY_REGDTP,"Reg.Map: bad data type"},    /* reg.map regdaattype field contains invalid data */ 
    {API_REG_VERFY_DFLT,  "Reg.Map: Default bound err"},/* reg.map default value is out of bounds */ 
    {API_REG_VERFY_BCNT,  "Reg.Map: Bit cnt/len err"},  /* reg.map bit count/length are questionable */ 
    {API_REG_VERFY_DTLEN, "Reg.Map: data type/len err"},/* reg.map regdattype inconsistent with length */ 
    {API_REG_HDWR_REWTERR,"Read/Mask/Wt err at write"}, /* I/O err at SBWrite() at read/write/mask */ 
    {API_REG_HDWR_REGRDO, "Write attempt to r/o reg"},  /* I/O error: write attempt to RO register */ 
    {API_REG_HDWR_REGWTO, "Read attempt to w/o reg"},   /* I/O error: read attempt to WO register */ 
    {API_RANGE,       "Warn:Write Bounds Error"},   /* Register written with bounds error data */ 
    {API_INIT_XTAL,   "Init: xtal bounds error"},   /* crystal freq is out-of-bounds */ 
    {API_INIT_VCO,    "Init: vcoinit neither T/F"}, /* vcoinit is neither True nor False */ 
    {API_INIT_MPEG,   "Init: Default MPEG is NULL"},/* Mpeg (default settings) struct is NULL */ 
    {API_INIT_TUNER,  "Init: Tuner parm bad"},      /* tuner parm passed is invalid */ 
    {API_DEMOD_ERR,   "Demod has invalid setting"}, /* demod register read has invalid setting */ 
    {API_VITSETTING,  "Demod viterbi setting null"},/* demod viterbi search list is set to zero */ 
    {API_ERRBYTE,     "Demod not set for BYTEerr"}, /* demod is not set to return BYTE error counts */ 
    {API_NOTSUPPORT,  "Feature not supported"},     /* asked-for feature not supported by driver */ 
    {API_INVALID_VCONO,       "Tuner: Invalid vco no"},     /* invalid vco number selected in driver */ 
    {API_BAD_BP,      "Tuner: Invalid BP pct"},     /* invalid breakpoint% */ 
    {API_BAD_CXCTL,   "Tuner: Invalid ctl bits"},   /* invalid setting for tuner control bits (b20,19) */ 
    {API_BAD_CXDATA,  "Tuner: data range error"},   /* data to be sent to tuner fails range check */ 
    {API_BAD_CXMETH,  "Tuner: io method setting"},  /* tuner i/o method is invalid see TUNER_io_method setting */ 
    {API_TUNERERR,    "Tuner: default setting err"},/* unable to set tuner to default i/o settings */ 
    {API_TUNERIO,     "Tuner: unable perform I/O"}, /* unable to write data to tuner */ 
    {API_BAD_DIV,     "API: Averted div by zero"},  /* trapped a div by zero err, results are undef */ 
    {API_BAD_TUNERPARMS,  "Tuner: Bad Parms passed"},   /* tunerparms passed by caller contains invalid settings */ 
    {API_TUNEREDGE,   "Tuner: Can't find VCO edge"},/* unable to find a tuner edge */ 
    {API_VITSET,      "Viterbi coderate settings"}, /* viterbi code rate settings are wrong/bad */ 
    {API_IQ_IO,       "Demod: Err reading IQ pair"},/* demod: unable to read I, Q paired data */ 
    {API_CXTYPE,      "Demod: Unable to ID demod"}, /* unable to determine demod type (CxType) */ 
    {API_BADCXDATABND,"Tuner: band range error"},   /* band data sent to tuner fails data range test */ 
    {API_BADCXDATAVGA,"Tuner: VGA range error"},    /* vga data sent to tuner fails data range test */ 
    {API_BADCXDATAVCA,"Tuner: VCA range error"},    /* vca data sent to tuner fails data range test */ 
    {API_BADCXDATAPLL,"Tuner: data range error"},   /* pll data sent to tuner fails data range test */ 
    {API_TUNERREF,    "Tuner: Invalif Ref Divider"},/* invalid refernce divider passed as parm to funct */ 
    {API_TUNERVCO,    "No valid VCO exists for the selected frequency: VCO  edge detection required."}, 
                                                    /* ^^ No valid VCO exists for the selected frequency (detect VCO edges) */ 
    {API_LNB_MSGLEN,  "LNB message too short"},     /* unable to send a short LNB message */ 
    {API_LNB_STALLED, "LNB message stalled"},       /* unable to send LNB message in time allocated */ 
    {API_LNB_BUSY,    "LNB message BUSY"},          /* LNB message busy flag is not set (should be set -> 0x01 indicates NOT busy) */ 
    {API_DEMOD_REVB_SINGLE,"Invalid use of RevB SW"},/*   (invalid use of Rev B SW) */ 
    {API_DISEQC_RXLEN,"Diseqc: Rx buffer too short"},/* passed-in rx buffer len was <= 0 */ 
    {API_DISEQC_VERSION,"Diseqc: No Rx HW on demod"},/* demod does not contain this capability */ 
    {API_DISEQC_TIMEOUT,"Diseqc: Rx Timeout"},      /*  demod took to long to rx diseqc message */ 
    {API_PDMFOUT,      "Error setting pdmfout reg"},/*  error setting pdmfout register */ 
                                                    /* end-of-list of errors -- must be last record */ 
    {API_IQ_NULL,     "IQPAK pointer is NULL"},     /* IQPAK constellation pointer is NULL */ 
    {API_SCE_DEADLOCK,"SCE Deadlock"},              /* Symbol Clock Estimator deadlock */ 
    {API_DEMOD_UNSUPPORTED,"Demod: Not supported by driver"}, /* demod is not supported by the driver */ 
    {API_EOERR,       "(end of err list)"}          /* timer specified at init is not valid */ 
  }; 
 
  /* find the error string, report it back to collar */ 
  for (i = 1 ; nimerr[i].__errno != API_EOERR ; i++) 
  { 
    if (nimerr[i].__errno == err) 
    { 
      return(nimerr[i].errstr); 
    } 
  } 
 
  return(nimerr[0].errstr); 
 
}  /* DRIVER_GetError() */ 
#endif /* INCLUDE_DEBUG */ 
 
/*******************************************************************************************************/ 
/* DRIVER_preinit() */ 
/*******************************************************************************************************/ 
void DRIVER_preinit()                  /* function to pre-init tuner/demod strings (data must be available */ 
{                                      /* prior to successful InitEnv() for GUI access) */ 
  static  int driver_inited = False; 
   
  static unsigned int ts_list[] = /* tuners-supported list */ 
      { 
#if INCLUDE_ROSIE 
	(unsigned int)CX24108,     /* tuner enum (rosie) */ 
#endif 
#if INCLUDE_VIPER 
	(unsigned int)CX24128,     /* tuner enum (viper) */ 
#endif 
#if INCLUDE_RATTLER 
	(unsigned int)CX24113,     /* tuner enum (Rattler) */ 
#endif 
      0};                         /* zero marks end-of-list */ 
   
  static char  *ts_names[MAX_TUNERSUPPORT] = 
      { 
#if INCLUDE_ROSIE 
       ROSIE_TYPE_STRING,                 /* rosie */ 
#endif 
#if INCLUDE_VIPER 
       VIPER_TYPE_STRING,                 /* Viper */ 
#endif 
#if INCLUDE_RATTLER 
       RATTLER_TYPE_STRING,               /* Rattler */ 
#endif 
      NULL};                      /* NULL marks end-of-list */ 
 
  if (driver_inited == False) 
  { 
    tuners_supported = ts_list;     /* list of supported tuners */ 
    _tuners_supported = &ts_names[0]; 
  } 
 
  driver_inited = True; 
  return; 
 
}  /* DRIVER_preinit() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_ValidateNim() */ 
/*******************************************************************************************************/ 
BOOL DRIVER_ValidateNim(               /* function to test nim for NULL and inclusion in NIM_LIST */ 
NIM  *nim)                             /* pointer to nim */ 
{ 
  DRIVER_preinit(); 
 
  /* test for NULL nim */ 
  if (nim == NULL) 
  { 
    /* invalid nim or already allocated */ 
    DRIVER_SET_ERROR(nim,API_NIM_NULL); 
  } 
  else 
  { 
    /* nim was not NULL, so test other validity properties */ 
    return(DRIVER_ValidNim(nim)); 
  } 
 
  return(False); 
 
}  /* DRIVER_ValidNim() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_ValidateNimIq() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_ValidateNimIq(            /* function to validate nim IQ pointer */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
   
  if (nim->iqpak == NULL) 
  { 
    /* invalid nim or already allocated */ 
    DRIVER_SET_ERROR(nim,API_IQ_NULL); 
    return(False); 
  } 
 
  return(True); 
 
}  /* DRIVER_ValidateNimIq() */ 
 
 
 
/*******************************************************************************************************/ 
/* DRIVER_ValidNim() */ 
/*******************************************************************************************************/ 
BOOL DRIVER_ValidNim(                  /* function to validate a nim pointer */ 
NIM  *nim)                             /* pointer to nim */ 
{ 
  int  i; 
 
  DRIVER_preinit(); 
 
  /* test nims saved via init env for validity */ 
  for (i = 0 ; i < MAX_NIMS ; i++) 
  { 
    if (nim_list.nim[i] == nim)
	return(True); 
  } 
  return(False); 
 
}  /* DRIVER_ValidNim() */ 
 
 
/*******************************************************************************************************/ 
/* _DRIVER_wait() */ 
/*******************************************************************************************************/ 
BOOL  _DRIVER_wait(                     /* function to wait a specified number of ms */ 
NIM  *nim,                             /* pointer to nim */ 
int  mscount)                          /* ms to wait */ 
{ 
   unsigned int current_tick, stop_tick; 
 
   if ((nim == NULL) || (mscount < 0)) 
   { 
      return (False); 
   } 
 
   /* determine the stop tick */ 
   if ((stop_tick = API_OS_Time()) == (unsigned int)-1) 
   { 
      return (False); 
   } 
   stop_tick += mscount; 
 
   /* countinue testing time until ms count is achieved */ 
   while ((current_tick = API_OS_Time()) != (unsigned int)-1) 
   { 
      if (current_tick > stop_tick) 
      { 
         return (True); 
      } 

      if (mscount > 1)
      {
		//sleep(nim->ptuner, mscount * 1000);
      }
   } 

   return (False); 
}  /* _DRIVER_wait() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_symbolrate_in() */ 
/*******************************************************************************************************/ 
long           DRIVER_symbolrate_in(   /* function to perform common symbol rate computation */ 
unsigned long  symbolrate,             /* symbol rate (in) */ 
unsigned long  sampleratekhz)          /* sample rate in hz (in) */ 
{ 
  static BCDNO  bcd; 
  unsigned long  computed_symbolrate; 
   
  /* compute symbolrate programmed to the demod: (symbolrate * 2^23) / rounded(samplerate/1000) */ 
  BCD_set(&bcd,(symbolrate*1000UL)+500UL); 
  BCD_mult(&bcd,(long)(0x01L<<23L)); 
  BCD_div(&bcd,(sampleratekhz)); 
 
  /* symbol rate  */ 
  computed_symbolrate = BCD_out(&bcd); 
 
  return((long)computed_symbolrate); 
 
}  /* DRIVER_symbolrate_in() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_symbolrate_out() */ 
/*******************************************************************************************************/ 
long           DRIVER_symbolrate_out(  /* function to reverse symbolrate_in() function */ 
unsigned long  symbolrate,             /* symbol rate (in) */ 
unsigned long  sampleratekhz)          /* sample rate (in) */ 
{ 
  static BCDNO  bcd; 
  unsigned long  computed_symbolrate = 0UL; 
 
  BCD_set(&bcd,(long)symbolrate); 
  BCD_mult(&bcd,100000L); 
  BCD_div(&bcd,(0x01L<<23L)); 
  BCD_mult(&bcd,sampleratekhz); 
  BCD_div(&bcd,100000L); 
 
  computed_symbolrate = BCD_out(&bcd); 
 
  return((long)computed_symbolrate); 
 
}  /* DRIVER_symbolrate_out() */ 

 
/*******************************************************************************************************/
/* DRIVER_div_zero() */
/*******************************************************************************************************/
BOOL           DRIVER_div_zero(        /* attempt to pre-trap divide-by-zero */
NIM            *nim,                   /* pointer to nim */
unsigned long  denom)                      /* (potential) denom */
{
  /* trap div-by-zero errors */
  if (denom == 0UL)
  {
    /* trap div by zero errors */
    if (nim != NULL)
    {
      DRIVER_SET_ERROR(nim,API_BAD_DIV);
    }
    return(False);
  }
  
  return(True);

}  /* DRIVER_div_zero() */

 
/*******************************************************************************************************/ 
/* DRIVER_compute_demod_pll_mult() */ 
/*******************************************************************************************************/ 
unsigned long  DRIVER_compute_demod_pll_mult(  /* function to compute pll freq. */ 
NIM            *nim,                           /* pointer to nim */ 
unsigned long  Fs,                             /* sample frequency */ 
unsigned long  Fc)                             /* crystal frequency */ 
{ 
	unsigned long   pll_mult; 
	unsigned short  remainder; 
 
	if (Fs == 0UL || Fc == 0UL) 
	{ 
		/* don't pass any div by zero possibilities */ 
		if (nim != NULL)   
		{ 
			DRIVER_SET_ERROR(nim,API_BAD_DIV); 
		} 
		return(0UL); 
	} 
 
	/* compute pll_mult = (Fs * 6L) / Fc */ 
    /* make sure the result has the one extra digit from the fraction */ 
	pll_mult = ((Fs * 6UL) + 5UL) / (Fc/10); /* CR 9760 */ 
 
	remainder = (unsigned short)(pll_mult % 10); 
 
	pll_mult = pll_mult/10; /* loose the extra digit */ 
 
	if (remainder > 5) 
	{ 
		/* round the pll multiplier value */ 
		pll_mult++; 
	}			    
	return(pll_mult); 
 
}  /* DRIVER_compute_demod_pll_mult() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_compute_fs() */ 
/*******************************************************************************************************/ 
unsigned long  DRIVER_compute_fs(      /* function to compute the sample frequency */ 
unsigned long  pllmult,                /* demod pll multiplier value */ 
unsigned long  Fc)                     /* crystal frequency */ 
{ 
  unsigned long   rtn; 
 
  /* compute Fs = (Fc * pllmult) / 6 ; -> rounded -> (Fc * pllmult + 5) / 60 */ 
  rtn = ((Fc * pllmult)+5UL)/6UL; 
   
  return(rtn); 
 
}  /* DRIVER_compute_fs() */ 
 
/*******************************************************************************************************/
/* BCD Functions ***************************************************************************************/
/*******************************************************************************************************/
 
/*******************************************************************************************************/ 
/* BCD_add_bcd() */ 
/*******************************************************************************************************/ 
void   BCD_add_bcd(                    /* bcd math function used when LONG might be saturated */ 
BCDNO  *bcd,                           /* bcd struct */ 
BCDNO  *bcdtoadd)                      /* bcd struct to be used to add to bcd */ 
{ 
  int  idx; 
  int  result; 
 
  /* add two bcd numbers */ 
  if (BCD_getsign(bcd) != BCD_getsign(bcdtoadd)) 
  { 
    if (BCD_compare(bcd,bcdtoadd) < 0) 
    { 
      BCD_subt_bcd(bcdtoadd,BCD_abs(bcd)); 
      BCD_move_bcd(bcd,bcdtoadd); 
    } 
    else  BCD_subt_bcd(bcd,bcdtoadd);
    return; 
  } 
 
  for (idx = MAX_BCDNO-1 ; idx > 0 ; idx--) 
  { 
    result = (bcd->digits[idx] + bcdtoadd->digits[idx]); 
    bcd->digits[idx] = (signed char)result; 
  } 
 
  /* (CR 6805) */ 
  _BCD_adjust(bcd); 
 
  return; 
 
}  /* BCD_add_bcd() */ 
 
 
/*******************************************************************************************************/ 
/* _BCD_adjust_improved() */ 
/*******************************************************************************************************/ 
void   _BCD_adjust_improved(           /* internal bcd function to adjust bcd struct after computation */ 
BCDNO  *bcd,                           /* pointer to bcd struct */ 
int    ledge)                          /* left edge (highest digit) last melded into bcd (add/subt) */ 
{ 
  register int  idx; 
 
  for (idx = MAX_BCDNO-1 ; idx > ledge ; ) 
  { 
    if (bcd->digits[idx] >= 0 && bcd->digits[idx] <= 9)  idx--; 
    else 
    { 
      if (bcd->digits[idx] > 9) 
      { 
        bcd->digits[idx-1]++; 
        bcd->digits[idx] -= 10; 
      } 
      else 
      { 
        bcd->digits[idx-1]--; 
        bcd->digits[idx] += 10; 
      } 
    } 
  } 
   
  return; 
 
}  /* _BCD_adjust_improved() */   
 
 
/*******************************************************************************************************/ 
/* BCD_set() */ 
/*******************************************************************************************************/ 
void           BCD_set(                /* function to set a value into a BCDNO struct */ 
BCDNO          *bcd,                   /* pointer to bcd struct */ 
unsigned long  newval)                 /* no. to convert into a BCDNO */ 
{ 
  int    idx; 
  unsigned long  ulTemp = M*MM; 
 
  /* clear the bcd storage */   
  bcd->sign[0] = CNULL; 
  for (idx = 0 ; idx < MAX_BCDNO ; idx++)  bcd->digits[idx] = CNULL; 
 
  idx = MAX_BCDNO-10; 
  while (ulTemp > 0UL && newval != 0UL) 
  { 
    if (newval >= ulTemp) 
    { 
      bcd->digits[idx]++; 
      newval -= ulTemp; 
    } 
    else 
    { 
      idx++; 
      ulTemp /= 10UL; 
    } 
  } 
 
}  /* BCD_set() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_add() */ 
/*******************************************************************************************************/ 
void           BCD_add(                /* function to add a binary number value into a BCDNO number */ 
BCDNO          *bcd,                   /* pointer to bcd struct */ 
unsigned long  add)                    /* binary numbr to add into BCDNO */ 
{ 
  BCDNO  temp_bcd; 
 
  /* convert number to add to a bcd, then proceed */ 
  BCD_set(&temp_bcd,add); 
  BCD_add_bcd(bcd,&temp_bcd); 
  return; 
 
}  /* BCD_add() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_subt() */ 
/*******************************************************************************************************/ 
void           BCD_subt(               /* function to subtract a binary number value into a BCDNO number */ 
BCDNO          *bcd,                   /* pointer to bcd struct */ 
unsigned long  subt)                   /* binary numbr to add into BCDNO */ 
{ 
  BCDNO  temp_bcd; 
 
  /* convert number to subtract to a bcd, then proceed */ 
  BCD_set(&temp_bcd,subt); 
  BCD_subt_bcd(bcd,&temp_bcd); 
  return; 
 
}  /* BCD_subt() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_subt_bcd() */ 
/*******************************************************************************************************/ 
void   BCD_subt_bcd(                   /* function to subtract a BCDNO from a BCDNO */ 
BCDNO  *bcd,                           /* pointer to bcd struct */ 
BCDNO  *subt)                          /* pointer to bcd struct */ 
{ 
  int  idx; 
  int  bstart; 
  int  result; 
 
  /* determine the range of bcd digits that must be addressed */ 
  for ( bstart = 0 ; bstart <= MAX_BCDNO-1 ; bstart++) 
  { 
    if (subt->digits[bstart] != 0) 
    { 
      if (bstart > 0)  bstart -= 1; 
     
      /* subtract from bcd */ 
      for (idx = MAX_BCDNO-1 ; idx > bstart  ; idx--) 
      { 
        result = (bcd->digits[idx] - subt->digits[idx]); 
        bcd->digits[idx] = (signed char)result; 
      } 
      _BCD_adjust_improved(bcd,bstart); 
      return; 
    } 
  } 
   
  return; 
 
}  /* BCD_subt_bcd() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_mult() */ 
/*******************************************************************************************************/ 
void           BCD_mult(               /* function to multiply a BCDNO by a binary number */ 
BCDNO          *bcd,                   /* pointer to bcd struct */ 
unsigned long  multby)                 /* binary number to multiply by */ 
{ 
  static  BCDNO  temp_bcd; 
 
  /* convert multby to bcd, then proceed */ 
  BCD_set(&temp_bcd,multby); 
  BCD_mult_bcd(bcd,&temp_bcd); 
  return; 
 
}  /* BCD_mult() */ 
 
 
/*******************************************************************************************************/ 
/* _BCD_mult_bcd_ones() */ 
/*******************************************************************************************************/ 
void   _BCD_mult_bcd_ones(             /* function to multiply BCDNO one digit at a time */ 
BCDNO  *bcd,                           /* pointer to bcd struct */ 
char   digit)                          /* digit to multiply by */ 
{ 
  int    idx; 
 
  /* loop through each temp_bcd low digit until temp_bcd == zero */ 
  for (idx = MAX_BCDNO-1 ; idx > 0 ; idx--) 
  { 
    char  result; 
       
    /* compute result */ 
    result = (signed char)(bcd->digits[idx] * digit); 
    bcd->digits[idx] = result; 
  } 
  _BCD_adjust(bcd); 
 
  return; 
 
}  /* _BCD_mult_bcd_ones() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_mult_bcd() */ 
/*******************************************************************************************************/ 
void   BCD_mult_bcd(                   /* function to multiply BCDNO by a BCDNO */ 
BCDNO  *bcd,                           /* pointer to bcd struct */ 
BCDNO  *bcdmultby)                     /* pointer to bcd struct */ 
{ 
  int  i; 
  int  cnt; 
 
  signed char   digit; 
  signed int    sign; 
 
  BCDNO  temp_bcd; 
  BCDNO  result; 
  BCDNO  result2; 
 
  sign = -1; 
  if (bcd->sign[0] == bcdmultby->sign[0])  sign = 0; 
 
  /* test for mult by zero */ 
  if (BCD_zero(bcd) == True)  return; 
  if (BCD_zero(bcdmultby) == True) 
  { 
    BCD_clear(bcd); 
    return; 
  } 
   
  /* mult two bcd numbers */ 
  BCD_move_bcd(&temp_bcd,bcdmultby); 
  BCD_clear(&result2); 
 
  cnt = 0; 
  while(BCD_zero(&temp_bcd) == False) 
  { 
    digit = temp_bcd.digits[MAX_BCDNO-1]; 
    _BCD_div_ten(&temp_bcd); 
 
    if (digit != 0) 
    { 
      BCD_move_bcd(&result,bcd); 
      _BCD_mult_bcd_ones(&result,digit); 
      if (cnt != 0)  for (i = 0 ; i < cnt ; i++)  _BCD_mult_ten(&result); 
      BCD_add_bcd(&result2,&result); 
    } 
    cnt++; 
  } 
 
  /* place computed results and sign back into caller storage */ 
  BCD_move_bcd(bcd,&result2); 
  bcd->sign[0] = (signed char)(sign == -1 ? '-' : ' '); 
 
  return; 
 
}  /* BCD_mult_bcd() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_div_bcd() */ 
/*******************************************************************************************************/ 
void   BCD_div_bcd(                    /* function to divide a BCDNO by a BCDNO */ 
BCDNO  *numer,                         /* pointer to bcd struct */ 
BCDNO  *denom)                         /* pointer to bcd struct */ 
{ 
  int    i; 
  int    lsd; 
 
  int    done; 
  int    step_numer = 0; 
  int    len_denom = 0; 
 
  BCDNO  _numer; 
  BCDNO  *__numer; 
  BCDNO  _remainder; 
  BCDNO  *remainder; 
   
  BCDNO  _result; 
  BCDNO  *result; 
 
  __numer = &_numer; 
  result = &_result; 
  remainder = &_remainder; 
 
  BCD_move_bcd(&_numer,BCD_abs(numer)); 
  BCD_clear(result); 
 
  /* test if numbers can be divided */ 
  if (BCD_compare(&_numer,BCD_abs(denom)) <= 0)  BCD_clear(numer); 
  else 
  { 
    /* find the number of digits in denom */ 
    for (len_denom = 0 ; len_denom < MAX_BCDNO ; len_denom++) 
    { 
      if (denom->digits[len_denom] != CNULL)  break; 
    } 
    len_denom = (MAX_BCDNO) - len_denom; 
 
    /* pull-out at-least len_denom digits from numerator */ 
    for (step_numer = 0 ; step_numer < MAX_BCDNO ; step_numer++) 
    { 
      if (numer->digits[step_numer] != CNULL)  break; 
    } 
     
    BCD_clear(&_numer); 
    for (i = 0 ; i < len_denom ; i++) 
    { 
      _BCD_mult_ten(&_numer); 
      _BCD_lsd(__numer) = numer->digits[step_numer]; 
      step_numer++; 
    } 
 
    /* subtract denom from nom, until too little remaining */ 
    done = False; 
    while (done == False) 
    { 
      _BCD_mult_ten(result); 
      while (BCD_compare(&_numer,BCD_abs(denom)) > 0) 
      { 
        BCD_incr(result); 
        BCD_subt_bcd(&_numer,BCD_abs(denom)); 
      } 
 
      if (step_numer >= MAX_BCDNO)  break; 
 
      /* save the remainder */ 
      BCD_move_bcd(remainder,&_numer); 
      BCD_clear(&_numer); 
 
      /* add remaining numer digits to end-of-result */ 
      do{ 
        _BCD_mult_ten(remainder); 
        lsd = numer->digits[step_numer]; 
        _BCD_lsd(remainder) = (signed char)lsd; 
 
        step_numer++; 
        if (step_numer >= MAX_BCDNO)  break; 
        if (BCD_compare(remainder,BCD_abs(denom)) >= 0)  break; 
        else  _BCD_mult_ten(result);  
      } while (done == False && step_numer < MAX_BCDNO); 
      BCD_move_bcd(&_numer,remainder); 
    } 
  } 
   
  BCD_move_bcd(numer,result); 
  return; 
 
}  /* BCD_div_bcd() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_div() */ 
/*******************************************************************************************************/ 
void           BCD_div(                /* function to divide a BCDNO by a binary number */ 
BCDNO          *numer,                 /* pointer to bcd struct */ 
unsigned long  denom)                  /* binary demon */ 
{ 
  BCDNO  _denom; 
 
  if (denom != 0UL) 
  { 
    /* convert denom to bcd, then divide */ 
    BCD_set(&_denom,denom); 
    BCD_div_bcd(numer,&_denom); 
    return; 
  } 
 
  /* if div by zero would have occurred, set result to 0, return */ 
  BCD_set(numer,0L); 
  return; 
 
}  /* BCD_div() */  
 
 
/*******************************************************************************************************/ 
/* BCD_out() */ 
/*******************************************************************************************************/ 
unsigned long  BCD_out(                /* function to convert bcd number in BCDNO struct to a long */ 
BCDNO          *bcd)                   /* pointer to bcd struct */ 
{ 
  int    i; 
 
  unsigned long  ulTemp = M*M*M; 
  unsigned long  rtn = 0UL; 
 
  /* unsigned long will only hold several max 4+ billion, start at billions, and roll BCD out to unsigned long */ 
  for (i = (MAX_BCDNO-1)-9 ; i < MAX_BCDNO ; i++) 
  { 
    rtn += ((unsigned long)bcd->digits[i]*ulTemp); 
    ulTemp = ulTemp/10UL; 
  } 
 
  return(rtn); 
 
}  /* BCD_out() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_move_bcd() */ 
/*******************************************************************************************************/ 
void   BCD_move_bcd(                   /* function to move BCDNO source to BCDNO dest */ 
BCDNO  *bcd,                           /* pointer to bcd struct */ 
BCDNO  *bcdsource)                     /* pointer to bcd struct */ 
{ 
  int  idx; 
 
  bcd->sign[0] = bcdsource->sign[0]; 
  for (idx = 0 ; idx < MAX_BCDNO ; idx++) 
  { 
    bcd->digits[idx] = bcdsource->digits[idx]; 
  } 
   
  return; 
 
}  /* BCD_move_bcd() */   
 
 
/*******************************************************************************************************/ 
/* BCD_zero() */ 
/*******************************************************************************************************/ 
BOOL  BCD_zero(                        /* function to set a BCDNO struct to zero */ 
BCDNO  *bcd)                           /* pointer to bcd struct */ 
{ 
  int  idx; 
 
  /* test bcd for zero */ 
  for (idx = MAX_BCDNO-1 ; idx > 0 ; idx--)
	if (bcd->digits[idx] != CNULL)
		return(False); 
 
  /* bcd is equal to zero */ 
  return(True); 
 
}  /* BCD_zero() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_compare() */ 
/*******************************************************************************************************/ 
int    BCD_compare(                    /* function to compare two bcd numbers.  returns 0 if equal */ 
BCDNO  *bcd,                           /* pointer to bcd struct */ 
BCDNO  *bcd2)                          /* pointer to bcd struct */ 
{ 
  int  idx; 
 
  /* test sign for quick result bcd sig == +, return 1 else -1 */ 
  if (BCD_sign(bcd) != BCD_sign(bcd2))
	return(BCD_sign(bcd)); 
 
  /* compare two bcd numbers, returns positive no., if bcd is higher-than bcd2 */ 
  for (idx = 0 ; idx < MAX_BCDNO ; idx++) 
  { 
    if (bcd->digits[idx] != bcd2->digits[idx] != CNULL)   
      return((int)(bcd->digits[idx]-bcd2->digits[idx])); 
  } 
 
  /* bcd is equal to bcd2 */ 
  return(0); 
 
}  /* BCD_compare() */ 
 
 
/*******************************************************************************************************/ 
/* _BCD_div_ten() */ 
/*******************************************************************************************************/ 
void   _BCD_div_ten(                   /* function to perfrom a bcd div by ten (kill lowest digit) */ 
BCDNO  *bcd)                           /* pointer to bcd struct */ 
{ 
  int  idx; 
 
  /* shift bcd rt by one BCD place */ 
  for (idx = MAX_BCDNO-1 ; idx > 1 ; idx--)
	bcd->digits[idx] = bcd->digits[idx-1];
  bcd->digits[0] = CNULL; 
  return; 
 
}  /* _BCD_div_ten() */ 
 
 
/*******************************************************************************************************/ 
/* BCD_abs() */ 
/*******************************************************************************************************/ 
BCDNO  *BCD_abs(                       /* function to perform abs() functionality on a BCDNO number */ 
BCDNO  *bcd)                           /* pointer to bcd struct */ 
{ 
  static BCDNO  bcd_abs; 
 
  BCD_move_bcd(&bcd_abs,bcd); 
  BCD_setsign((&bcd_abs),1);   
  return(&bcd_abs); 
 
}  /* BCD_abs() */ 
 
 
/*******************************************************************************************************/ 
/* _BCD_mult_ten() */ 
/*******************************************************************************************************/ 
void   _BCD_mult_ten(                  /* function to multiply a BCDNO by ten (performs a bcd <<1 op.) */ 
BCDNO  *bcd)                           /* pointer to bcd struct */ 
{ 
  int  idx; 
 
  /* shift bcd lt by one BCD place */ 
  for (idx = 0 ; idx < MAX_BCDNO-1 ; idx++)
	bcd->digits[idx] = bcd->digits[idx+1]; 
  bcd->digits[MAX_BCDNO-1] = 0; 
  return; 
 
}  /* _BCD_mult_ten() */ 
 

/*******************************************************************************************************/ 
/* DRIVER_SetPNSequence() */ 
/*******************************************************************************************************/ 
BOOL     DRIVER_SetPNSequence(         /* function to set the state of the PN machine */ 
NIM      *nim,                         /* pointer to nim */ 
BOOL     pnflag)                       /* True=on, False=off */ 
{ 
  unsigned long  ulRegVal; 
  BOOL   rtn = True; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  switch(pnflag) 
  { 
    case  True: 
    case  False: 
    { 
      ulRegVal = (pnflag == True ? 0x01UL : 0x00UL); 
      if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False); 
      break; 
    } 
    default: 
    { 
      DRIVER_SET_ERROR(nim,API_BAD_PARM); 
      rtn = False; 
      break; 
    } 
  }  /* switch(... */ 
 
  return(rtn); 
 
}  /* DRIVER_SetPNSequence() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_GetPNSequence() */ 
/*******************************************************************************************************/ 
BOOL     DRIVER_GetPNSequence(         /* function to read current state of PN */ 
NIM      *nim,                         /* pointer to nim */ 
BOOL     *pnflag)                      /* True=enabled, False=disabled */ 
{ 
  unsigned long  ulRegVal; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* enable PN bER */ 
  ulRegVal =  0x01; 
  if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
  if (pnflag == NULL) 
  { 
    DRIVER_SET_ERROR(nim,API_BAD_PARM); 
    return(False); 
  } 
 
  *pnflag = (ulRegVal == 0UL ? False : True); 
 
  return(True); 
 
}  /* DRIVER_GetPNSequence() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_BBB_errinfo() */ 
/*******************************************************************************************************/ 
void            DRIVER_BBB_errinfo(    /* function for perform common error code for BERReadyCount[21:0], BYTE,Block */ 
NIM             *nim,                  /* pointer to nim */ 
long            *last_count,           /* last error count (when changed, error count is ready) */ 
CMPLXNO         *cpx,                  /* CMPLXNO holding result */ 
unsigned long   errwindow,             /* error window size */ 
unsigned long   errmult,               /* multiplier (different for BERReadyCount[21:0], BYTE, BLOCK and DVB */ 
MSTATUS         *mstat)                /* returned measurement status: (done, saturated, not-done) */ 
{ 
  unsigned long  ulRegVal; 
  long   errcount; 
 
  /* sample is not yet ready */ 
  DRIVER_set_complex(cpx,-1L,1UL); 
  *mstat = MSTATUS_NOTDONE; 
 
  /* read the window counter (this is the initial count) */ 
  if (RegisterRead(nim,CX24130_BERSTART,&ulRegVal, DEMOD_I2C_IO) == False)
	return; 
     
  /* test if a sample is ready */ 
  if ((long)ulRegVal != *last_count) 
  { 
    /* save the current sample count */ 
    *last_count = (long)ulRegVal; 
 
    /* freeze the counter */ 
    ulRegVal = 0x00; 
    if (RegisterWrite(nim,CX24130_BERSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return; 
 
    /* test for errwindow valid range, warn user if out of valid range */ 
    if (errwindow > 0xffUL) 
    { 
      DRIVER_SET_ERROR(nim,API_PARM_RANGE); 
      errwindow = 0xffUL; 
    } 
 
    /* read the err counter */ 
    if (RegisterRead(nim,CX24130_BERCOUNT_RS,&ulRegVal, DEMOD_I2C_IO) == False)
	return; 
    errcount = (long)ulRegVal; 
    DRIVER_set_complex(cpx,errcount,(errwindow*errmult)); 
    *mstat = MSTATUS_DONE; 
 
    /* restart the counter */ 
    ulRegVal = 0x01UL; 
    if (RegisterWrite(nim,CX24130_BERSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return; 
  } 
 
  return; 
 
}  /* DRIVER_BBB_errinfo() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_error_measurements_off() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_error_measurements_off(   /* function to turn-OFF BER (i.e. BER, Byte, Block) settings */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* turn either PNBER or BER (ber, Byte, block) */ 
  if (DRIVER_BBB_error_off(nim) == False)
	return(False); 

  if (DRIVER_PNBER_error_off(nim) == False)
	return(False); 
 
  return(True); 
 
}  /* DRIVER_error_measurements_off() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_BBB_error_off() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_BBB_error_off(            /* function to turn-OFF BER (i.e. BER, Byte, Block) settings */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* if any BER measurement is on, turn it off */ 
  if (nim->berbusy != -1L)
	return(API_GetBER(nim,0UL,NULL,NULL)); 

  if (nim->bytebusy != -1L)
	return(API_GetByteErrors(nim,0UL,NULL,NULL)); 

  if (nim->blockbusy != -1L)
	return(API_GetByteErrors(nim,0UL,NULL,NULL)); 
 
  return(True); 
 
}  /* DRIVER_BBB_error_off() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_errcount_disable() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_errcount_disable(         /* function to disable error count code */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  BOOL  rtn = True; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  nim->berbusy = -1; 
  nim->bytebusy = -1; 
  nim->blockbusy = -1; 
 
  if (RegisterWrite(nim,CX24130_BERRSSELECT,RSERRCNT_NONE, DEMOD_I2C_IO) != True)
	rtn = False; 

  if (RegisterWrite(nim,CX24130_BERRSINFWINEN,0L, DEMOD_I2C_IO) != True)
	rtn = False; 
 
  return(rtn); 
 
}  /* DRIVER_errcount_disable() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_PNBER_error_off() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_PNBER_error_off(          /* function to turn-OFF PN BER settings */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* if PN BER measurement is on, turn it off */ 
  if (nim->pnberbusy != -1L)
	return(API_GetPNBER(nim,PNBER_UNDEF,(CMPLXNO*)NULL,(MSTATUS*)NULL)); 
 
  return(True); 
 
}  /* DRIVER_PNBER_error_off() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_Reset() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_Reset(                    /* function to perform a demod reset operation */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  unsigned long  ulRegVal; 
  BOOL   rtn = True; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* set demod registers to the default state */ 
  if (DRIVER_Default(nim) == False)
	rtn = False;  
   
  /* reset the demod */ 
  ulRegVal = 0x01UL; 
  if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	rtn = False; 
   
  ulRegVal = 0x00UL; 
  if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	rtn = False; 
 
  return(rtn); 
 
}  /* DRIVER_Reset() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_Preset() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_Preset(                   /* function to perform a demod preset operation */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  unsigned long  ulRegVal; 
  BOOL   rtn = True; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  switch (nim->demod_type)
  {
    case  CX24130:
    {
      /* if the count of NIMs in-use exceeds one, disallow PRESET (otherwise, allow PRESET) */
      if (nim_list.nim_cnt > 1)  break;
    }
    case  CX24123:
    case  CX24123C:
    case  CX24121:
    {
      ulRegVal = 0x02UL;
      if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	rtn = False;
  
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	rtn = False;
      break;
    }
    default:
    {
      /* demod-type was odd, report error */
      rtn = False;
    }
  }  /* switch(... */
 
  return(rtn); 
}  /* DRIVER_Preset() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_Default() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_Default(                  /* function to set demod registers into a default state */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  int    i; 
  int    j; 
 
  unsigned long  ulTemp; 
  unsigned char  addr; 
  unsigned char   reg_skip[0xff+1]; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  memset(reg_skip,CNULL,sizeof(reg_skip)); 
 
  /* find each byte-wide non-RO register that is used */ 
  for (i = 0 ; i < CX24130_REG_COUNT ; i++) 
  { 
    ulTemp = demod_register_map[i].default_value; 
    addr = demod_register_map[i].address; 
    if (demod_register_map[i].access_level != REG_RO) 
    { 
        for (j = 0 ; j < 4 ; j++) 
        { 
            if (demod_register_map[i].p_hw_mask[j] != 0) 
            { 
                /* Skip registers with values -1UL */ 
                if (ulTemp == 0xFFFFFFFFUL)  reg_skip[addr+j]++; 
            } 
        } 
    } 
  } 
 
  /* skipping the soft reset register, step through each non-test register, set its default value  */ 
  /* (only if it has a default value to set) */ 
  for (i = 1 ; i < CX24130_REG_COUNT ; i++) 
  { 
    /* only set default values for non-test registers (skip registers id'd as latching) */ 
    if ((demod_register_map[i].address < MAX_COBRA_NONTEST && demod_register_map[i].access_level != REG_RO) && 
        (reg_skip[demod_register_map[i].address] == 0) && 
        ((demod_register_map[i].regfilter & REGF_COBRA) == REGF_COBRA)) 
    { 
      /* grab the default value, if not -1, set register to its default value */ 
      ulTemp = demod_register_map[i].default_value; 
      if (ulTemp != 0xffffffffUL) 
      { 
	if (RegisterWrite(nim, (unsigned short)i, ulTemp, DEMOD_I2C_IO) == False) 
		return (False); 
      } 
    } 
  } 
 
#ifdef CAMARIC_FEATURES
  /* peform Cobra Default register set-up */ 
  if (DRIVER_Default_Camaric(nim) == False)
	return(False); 
#endif  /* #ifdef CAMARIC_FEATURES */

	return(True); 
}  /* DRIVER_Default() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_convert_twos() */ 
/*******************************************************************************************************/ 
long           DRIVER_convert_twos(    /* function to convert twos-comp no. read from demod into long */ 
unsigned long  numeric,                /* raw number read from demod */ 
int            bitslen)                /* count of lsb's to perform conversion on */ 
{ 
  long   lTemp; 
 
  /* test MSB for 1, indicating a negative number */ 
  lTemp = (0x01L<<(bitslen-1)); 
   
  if (((unsigned long)lTemp&numeric) != 0x00UL) 
  { 
    /* gen a reverse mask, or into numeric */ 
    lTemp = ~(lTemp-1L); 
    return((long)(numeric |= (unsigned long)lTemp)); 
  } 
   
  return((long)numeric); 
 
}  /* DRIVER_convert_twos() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_convert_twos_saturate() */ 
/*******************************************************************************************************/ 
unsigned long  DRIVER_convert_twos_saturate(   /* function to reduce a long integer's leading sign bits */ 
long           numeric,                        /* long integer in 2's complement notation */ 
int            bitslen)                        /* reduced bit count of the long integer */ 
{ 
  long  lTemp = 0L; 
  unsigned long ulTemp; 
 
  if (numeric >= 0) 
  { 
    /* positive number */ 
    if (numeric >= (0x01L<<(bitslen-1))-1)  lTemp = ((0x01L<<(bitslen-1L))-1L); 
    else lTemp = numeric; 
  } 
  else 
  { 
    /* negative number */ 
    if (numeric < ((0x01L<<(bitslen-1))*-1L))  lTemp = ((0x01L<<(bitslen-1))*-1L); 
    else lTemp = numeric; 
  } 
 
  /* remove leading sign bits for negative number */ 
  ulTemp = (unsigned long)(lTemp & ((0x01UL << bitslen) - 1)); 
  return (ulTemp); 
 
}  /* DRIVER_convert_twos_saturate() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_HardReset() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_HardReset(                /* function to perform a total SW reset of the demod */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  unsigned long  ulRegVal; 
 
  /* test NIM for validity */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* perform a SW hard reset */ 
  ulRegVal = 0xff; 
  if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
   
  ulRegVal = 0x0; 
  if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
  return(True); 
 
}  /* DRIVER_HardReset() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_CxType() */ 
/*******************************************************************************************************/ 
BOOL   DRIVER_CxType(                  /* function to perform a total SW reset of the demod */ 
NIM    *nim,                           /* pointer to nim */ 
DEMOD  *demod,                         /* pointer to storage holding demod id'd by sw */ 
char   **demod_str)                    /* demod string built using demod* (above) */ 
{ 
  int	i; 
  int   err = 0; 
  int	changed_count = 0;

  unsigned char  b; 
  unsigned char  accum_sel; 
  unsigned char  addr;
  unsigned char  lock_initial;
  unsigned char  lock_test;
  unsigned char  lock_read;
  unsigned char  ver; 
   
  /* Confirm that the chip-type is Cobra */ 
  if (API_ReadReg(nim,0x00,&ver) != True)
	err++; 
 
  /* determine the chip-type */ 
  switch (ver) 
  { 
    case  0x8d:
    {
      break;
    }
    case  0xe1:
    {
      /* CamarIC 80-pin  */
      *demod = CX24123C;
      if (demod_str != NULL)  *demod_str = PRODUCT_NAME_STRING_CAM80;
      return(True);
      break;
    }
    case  0xd1:
    {
      /* CamarIC single */
      *demod = CX24123;
      if (demod_str != NULL)  *demod_str = PRODUCT_NAME_STRING_CAM64;
      return(True);
      break;
    }
    case 0xd9:
    {
      *demod = CX24123;
      if (demod_str != NULL)  *demod_str = PRODUCT_NAME_STRING_CAMARO;
      return(True);
    }

    default: 
    { 
      /* unknown chip detected (unknown chip-id) */ 
      err++; 
      break; 
    } 
  }  /* switch() */ 

  /* read the demod accum sel register, nim b */
  addr = (unsigned char)(demod_register_map[CX24130_DMDACCUMSEL].address | 0x80);
  if (API_ReadReg(nim,addr,&accum_sel) != True)
	err++;

  /* write the accum sel register QEQ */
  b = 0x1c;
  if (API_WriteReg(nim,addr,&b) != True)
	err++;

  /* read the accum */
  addr = (unsigned char)(demod_register_map[CX24130_DMDACCUMVAL].address | 0x80);
  if (API_ReadReg(nim,addr,&lock_initial) != True)
	err++;

  /* write a new value to the accum */
  i = lock_initial;
  lock_test = (unsigned char)i;

  if (API_WriteReg(nim,addr,&lock_test) != True)
	err++;

  /* special op:  step-through each accum/subaccum value, looking for a change */
  for (b = 0 ; b < 32 ; b++)
  {
    /* step-through each accum-sel and sub-accum-sel */
    addr = (unsigned char)(demod_register_map[CX24130_DMDACCUMSEL].address | 0x80);
    if (API_WriteReg(nim,addr,&b) != True)
	err++;

    /* test accum value for change, if a change has occurred, then device is a cx24130 */
    addr = (unsigned char)(demod_register_map[CX24130_DMDACCUMVAL].address | 0x80);
    if (API_ReadReg(nim,addr,&lock_read) != True)
	err++;
    if (lock_read != lock_initial)
    {
      changed_count++;
      break;
    }
  }
       
  /* if the values match (i.e. written is read from LOCK accum) hardware is cx24130 */
  nim->demod_type = CX24130;
  if (changed_count > 0)
  {
    *demod = CX24130;
    if (demod_str != NULL)  *demod_str =   PRODUCT_NAME_STRING;
  }
  else
  {
    unsigned char  gpoe;
    unsigned char  gpio;

    /* set the data direction for GPIO pin 60 (aka LDB) to input */
    addr = (unsigned char)(demod_register_map[CX24130_GPIO3DIR].address | 0x80);
    if (API_ReadReg(nim,addr,&gpoe) != True)
	err++;
    b = (unsigned char)(gpoe|0x80);
    if (API_WriteReg(nim,addr,&b) != True)
	err++;

    addr = (unsigned char)(demod_register_map[CX24130_GPIO3RDVAL].address | 0x80);
    if (API_ReadReg(nim,addr,&gpio) != True)
	err++;

    /* hardware is not cx24130, so it is a cx24121 */
    if ((gpio&0x80) == 0x80)
    {
      *demod = CX24121;
      if (demod_str != NULL)  *demod_str = PRODUCT_NAME_STRING_ALT;
      nim->demod_type = CX24121;
    }
    else  
    {
      *demod = CX24121;
      if (demod_str != NULL)  *demod_str = PRODUCT_NAME_STRING_ALT;
      nim->demod_type = CX24121;
    }

    /* reset GPIO pin 60 to initial settings */
    addr = (unsigned char)(demod_register_map[CX24130_GPIO3DIR].address | 0x80);
    if (API_WriteReg(nim,addr,&gpoe) != True)
	err++;
  }


  /* reset the accum sel register:  write the initial accum sel register */
  addr = (unsigned char)(demod_register_map[CX24130_DMDACCUMSEL].address | 0x80);
  if (API_WriteReg(nim,addr,&accum_sel) != True)
	err++;
 
  if (err == 0)
	return(True); 
   
  /* unable to querry chip to determine type */ 
  DRIVER_SET_ERROR(nim,API_CXTYPE); 
  return(False); 
 
}  /* DRIVER_CxType() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_AcqSetViterbiSearchList() */ 
/*******************************************************************************************************/ 
BOOL     DRIVER_AcqSetViterbiSearchList(  /* function to set viterbi search settings */ 
NIM      *nim,                            /* pointer to nim */ 
VITLIST  *vitlist)                        /* viterbi search list settings */ 
{ 
  int   i; 
 
  unsigned long  ulRegVal; 
  unsigned char  vitsetting = 0; 
 
  TRANSPEC  transpec; 
 
  /* test NIM for validity */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  if (vitlist != NULL) 
  { 
    /* read the trans spec */ 
    if (API_GetTransportSpec(nim,&transpec) == True) 
    { 
      int  j; 
      CODERATE  coderatefromlist;
 
      switch(transpec) 
      { 
#ifdef CAMARIC_FEATURES
        case  SPEC_DVB_DSS:
#endif  /* #ifdef CAMARIC_FEATURES */
        case  SPEC_DVB: 
        { 
          /* step through each element of vitlist */ 
          for (i = 0 ; i < min(vitlist->vcnt,MAX_VLIST) ; i++) 
          { 
            /* save coderate from vitlist */ 
            coderatefromlist = vitlist->viterbi_list[i]; 
            if (coderatefromlist != CODERATE_NONE) 
            { 
              /* search possible coderates for transpec, or into vitsetting */ 
              for (j = 0 ; dvb_cr_list[j] != CODERATE_NONE ; j++) 
              { 
                if (dvb_cr_list[j] == coderatefromlist) 
                { 
                  vitsetting = (unsigned char)(vitsetting | (0x01<<(dvb_cr_equate[j]))); 
                  break; 
                } 
              } 
            } 
          } 
          break; 
        } 
        case  SPEC_DSS:
        {
          /* step through each element of vitlist */
          for (i = 0 ; i < min(vitlist->vcnt,MAX_VLIST) ; i++)
          {
            /* save coderate from vitlist */
            coderatefromlist = vitlist->viterbi_list[i];
            if (coderatefromlist != CODERATE_NONE)
            {
              /* search possible coderates for transpec, or into vitsetting */
              for (j = 0 ; dss_cr_list[j] != CODERATE_NONE ; j++)
              {
                if (dss_cr_list[j] == coderatefromlist)
                {
                  /* vitsetting = (unsigned char)(vitsetting | (0x01<<j)); */
                  vitsetting = (unsigned char)(vitsetting | (0x01<<(dss_cr_equate[j])));
                  break;
                }
              }
            }
          }
          break;
        }
        case  SPEC_DCII:
        {
          /* step through each element of vitlist */
          for (i = 0 ; i < min(vitlist->vcnt,MAX_VLIST) ; i++)
          {
            /* save coderate from vitlist */
            coderatefromlist = vitlist->viterbi_list[i];
            if (coderatefromlist != CODERATE_NONE)
            {
              /* search possible coderates for transpec, or into vitsetting */
              for (j = 0 ; dcii_cr_list[j] != CODERATE_NONE ; j++)
              {
                if (dcii_cr_list[j] == coderatefromlist)
                {
                  /* vitsetting = (unsigned char)(vitsetting | (0x01<<j)); */
                  vitsetting = (unsigned char)(vitsetting | (0x01<<(dcii_cr_equate[j])));
                  break;
                }
              }
            }
          }
          break;
        }
        default: 
        { 
          DRIVER_SET_ERROR(nim,API_BAD_PARM); 
          return(False); 
          break; 
        } 
      }  /* switch(... */ 
 
      /* if viterbi setting is to be 0, flag action to caller, but continue */ 
      if (vitsetting == 0x00)
	DRIVER_SET_ERROR(nim,API_VITSETTING); 
 
      /* write viterbi search settings to demod */ 
      ulRegVal = (unsigned long)vitsetting; 
      if (RegisterWrite(nim,CX24130_ACQCREN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
      return(True); 
    } 
     
    /* demod has reported a transpec setting that is not valid */ 
    DRIVER_SET_ERROR(nim,API_DEMOD_ERR); 
    return(False); 
  } 
 
  DRIVER_SET_ERROR(nim,API_BAD_PARM); 
  return(False); 
 
}  /* DRIVER_AcqSetViterbiSearchList() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_AcqGetViterbiSearchList() */ 
/*******************************************************************************************************/ 
BOOL     DRIVER_AcqGetViterbiSearchList(  /* function to retrieve  NIM's current viterbi search list settings */ 
NIM      *nim,                            /* pointer to nim */ 
VITLIST  *vitlist)                        /* returned current settings */ 
{ 
  int    i; 
  int    j; 
 
  unsigned long  ulRegVal; 
  unsigned char  vitsetting = 0; 
 
  TRANSPEC  transpec; 
 
  /* test NIM for validity */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  if (vitlist != NULL) 
  { 
    /* read vit search settings from demod */ 
    if (RegisterRead(nim,CX24130_ACQCREN,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
    vitsetting = (unsigned char)ulRegVal; 
 
    /* read the trans spec */ 
    if (API_GetTransportSpec(nim,&transpec) == True) 
    { 
      unsigned char  mask; 
 
      /* building new vitlist, so clear count, clear list */ 
      vitlist->vcnt = 0; 
      for (i = 0 ; i < MAX_VLIST ; i++)  vitlist->viterbi_list[i] = CODERATE_NONE; 
 
      /* step through each bit of vit searching settings from demod */ 
      for (i = 0 ; i < 8 ; i++) 
      { 
        /* gen the mask to look for */ 
        mask = (unsigned char)(0x01U<<i); 
       
        if ((vitsetting&mask) != 0) 
        { 
          switch(transpec) 
          { 
#ifdef CAMARIC_FEATURES
            case  SPEC_DVB_DSS:
#endif  /* #ifdef CAMARIC_FEATURES */
            case  SPEC_DVB: 
            { 
              /* search possible coderates for transpec, then OR into vitsetting */ 
              for (j = 0 ; dvb_cr_list[j] != CODERATE_NONE ; j++) 
              { 
                /* if coderate (cr) matches i index, then coderates match */ 
                if (dvb_cr_equate[j] == i) 
                { 
                  vitlist->viterbi_list[vitlist->vcnt] = dvb_cr_list[j]; 
                  vitlist->vcnt++; 
                  break; 
                } 
              } 
              break; 
            }
            case  SPEC_DSS:
            {
              /* search possible coderates for transpec, then OR into vitsetting */
              for (j = 0 ; dss_cr_list[j] != CODERATE_NONE ; j++)
              {
                /* if coderate (cr) matches i index, then coderates match */
                if (dss_cr_equate[j] == i)
                {
                  vitlist->viterbi_list[vitlist->vcnt] = dss_cr_list[j];
                  vitlist->vcnt++;
                  break;
                }
              }
              break;
            }
            case  SPEC_DCII:
            {
              /* search possible coderates for transpec, then OR into vitsetting */
              for (j = 0 ; dcii_cr_list[j] != CODERATE_NONE ; j++)
              {
                /* if coderate (cr) matches i index, then coderates match */
                if (dcii_cr_equate[j] == i)
                {
                  vitlist->viterbi_list[vitlist->vcnt] = (CODERATE)dcii_cr_list[j];
                  vitlist->vcnt++;
                  break;
                }
              }
              break;
            }
            default: 
            { 
              DRIVER_SET_ERROR(nim,API_BAD_PARM); 
              return(False); 
              break; 
            } 
          }  /* switch(... */ 
        } 
      }  /* for(... */ 
 
      /* vitlist has been built into struct pointer passed by caller */ 
      return(True); 
    } 
  } 
 
  DRIVER_SET_ERROR(nim,API_BAD_PARM); 
  return(False); 
 
}  /* DRIVER_AcqGetViterbiSearchList() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetViterbiRate() */ 
/*******************************************************************************************************/ 
BOOL      DRIVER_SetViterbiRate(       /* function to set the demod's primary viterbi rate  */ 
NIM       *nim,                        /* pointer to nim */ 
CODERATE  coderate)                    /* viterbi coderate */ 
{ 
  int    i; 
   
  unsigned long  ulRegVal; 
 
  TRANSPEC  transpec; 
 
  if (API_GetTransportSpec(nim,&transpec) == True) 
  { 
    switch(coderate) 
    { 
      case  CODERATE_1DIV2: 
      case  CODERATE_2DIV3: 
      case  CODERATE_3DIV4: 
      case  CODERATE_4DIV5: 
      case  CODERATE_5DIV6: 
      case  CODERATE_6DIV7: 
      case  CODERATE_7DIV8: 
      { 
        switch(transpec) 
        { 
          case  SPEC_DSS:
          {
            /* find the code rate desired, plug into demod */
            for (i = 0 ; dss_cr_list[i] != CODERATE_NONE ; i++)
            {
              if (dss_cr_list[i] == coderate)
              {
                /* set Viterbi lock threshold optimized for the coderate */
                DRIVER_SetViterbiLockThresh(nim,coderate);

                ulRegVal = (unsigned long)dss_cr_equate[i];
                return(RegisterWrite(nim,CX24130_ACQVITCRNOM,ulRegVal, DEMOD_I2C_IO));
              }
            }
            DRIVER_SET_ERROR(nim,API_BAD_PARM);
            return(False);
            break;
          }
          case  SPEC_DVB: 
#ifdef CAMARIC_FEATURES
          case  SPEC_DVB_DSS:
#endif  /* #ifdef CAMARIC_FEATURES */
          {
            /* find the code rate desired, plug into demod */ 
            for (i = 0 ; dvb_cr_list[i] != CODERATE_NONE ; i++) 
            { 
              if (dvb_cr_list[i] == coderate) 
              { 
                /* set Viterbi lock threshold optimized for the coderate */ 
                DRIVER_SetViterbiLockThresh(nim,coderate); 
 
                ulRegVal = (unsigned long)dvb_cr_equate[i]; 
                return(RegisterWrite(nim,CX24130_ACQVITCRNOM,ulRegVal, DEMOD_I2C_IO)); 
              } 
            } 
            DRIVER_SET_ERROR(nim,API_BAD_PARM); 
            return(False); 
            break; 
          } 
          case  SPEC_DCII:
          {
            /* find the code rate desired, plug into demod */
            for (i = 0 ; dcii_cr_list[i] != CODERATE_NONE ; i++)
            {
              if (dcii_cr_list[i] == coderate)
              {
#ifdef CAMARIC_FEATURES
                /* optimize Demod phase locking expiration window for the coderate */
                if (DRIVER_DCIISetACQDmdWin(nim,coderate) == False)
                {
                  return (False);
                }
#endif  /* #ifdef CAMARIC_FEATURES */

                ulRegVal = (unsigned long)dcii_cr_equate[i];
                return(RegisterWrite(nim,CX24130_ACQVITCRNOM,ulRegVal, DEMOD_I2C_IO));
              }
            }
            DRIVER_SET_ERROR(nim,API_BAD_PARM);
            return(False);
            break;
          }
          case  SPEC_UNDEF: 
          default: 
          { 
            DRIVER_SET_ERROR(nim,API_BAD_PARM); 
            return(False); 
            break; 
          } 
        }  /* switch(... */ 
        break; 
      } 
      case  CODERATE_NONE: 
      default: 
      { 
        DRIVER_SET_ERROR(nim,API_BAD_PARM); 
        return(False); 
        break; 
      } 
    }  /* switch(... */ 
  } 
   
  return(False); 
 
}  /* DRIVER_SetViterbiRate() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_GetViterbiRate() */ 
/*******************************************************************************************************/ 
BOOL      DRIVER_GetViterbiRate(       /* function to set the demod's primary viterbi rate  */ 
NIM       *nim,                        /* pointer to nim */ 
CODERATE  *coderate)                   /* viterbi coderate */ 
{ 
  int   i; 
 
  unsigned long  vitrate; 
  unsigned long  locked; 
 
  TRANSPEC  transpec; 
 
  /* test NIM for validity */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* read the trans spec */ 
  if (API_GetTransportSpec(nim,&transpec) == True) 
  { 
    /* read the current lock status. If locked, return the current code rate */ 
    if (RegisterRead(nim,CX24130_ACQFULLSYNC,&locked, DEMOD_I2C_IO) == False)
	return(False); 
    if (locked == 0x01UL) 
    { 
      /* (CR6839) read the current viterbi rate */ 
      if (RegisterRead(nim,CX24130_ACQVITCURRCR,&vitrate, DEMOD_I2C_IO) != True)
	return(False); 
    } 
    else 
    { 
      /* not locked, so read the last code rate programmed via the viterbi nom register */ 
      if (RegisterRead(nim,CX24130_ACQVITCRNOM,&vitrate, DEMOD_I2C_IO) != True)
	return(False); 
    } 
 
    /* return the viterbi rate to the caller */ 
    switch(transpec) 
    { 
      case  SPEC_DVB: 
#ifdef CAMARIC_FEATURES
      case  SPEC_DVB_DSS:
#endif  /* #ifdef CAMARIC_FEATURES */
      { 
        for (i = 0 ; dvb_cr_list[i] != CODERATE_NONE ; i++) 
        { 
          if ((int)vitrate == dvb_cr_equate[i])   
          { 
            *coderate = dvb_cr_list[i]; 
            return(True); 
          } 
        } 
        break; 
      } 
      case  SPEC_DSS:
      {
        for (i = 0 ; dss_cr_list[i] != CODERATE_NONE ; i++)
        {
          if ((int)vitrate == dss_cr_equate[i])  
          {
            *coderate = dss_cr_list[i];
            return(True);
          }
        }
        break;
      }
      case  SPEC_DCII:
      {
        for (i = 0 ; dcii_cr_list[i] != CODERATE_NONE ; i++)
        {
          if ((int)vitrate == dcii_cr_equate[i])  
          {
            *coderate = dcii_cr_list[i];
            return(True);
          }
        }
        break;
      }
      default: 
      { 
        /* demod is in an invalid mode */ 
        DRIVER_SET_ERROR(nim,API_BAD_PARM); 
        break; 
      } 
    }  /* switch(... */ 
     
  } 
   
  return(False); 
 
}  /* DRIVER_GetViterbiRate() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetSmoothClock() */ 
/*******************************************************************************************************/ 
BOOL           DRIVER_SetSmoothClock(  /* funct to set the clock smoother MPEG register */ 
NIM            *nim,                   /* pointer to nim */ 
CLOCKSMOOTHSEL cs,                     /* clk smoother setting: CLK_SMOOTHER_ON, CLK_SMOOTHER_OFF */ 
BOOL           fromCC)                 /* TRUE if this funct has been called from API_ChangeChannel() */ 
{ 
  unsigned long  locked; 
  unsigned long  smoothclkdiv; 
  unsigned long  pllmult; 
 
  unsigned long  symbolrate; 
  unsigned long  datalen = 0UL; 
  unsigned long  cwlen; 
 
  unsigned long  cr = 0UL; 
  unsigned long  crdiv = 0UL; 
  unsigned long  divno; 
   
  unsigned long  rsparitydis; 
  unsigned long  syncpunctmode; 
 
  CODERATE  coderate = CODERATE_NONE; 
  TRANSPEC  transpec; 
 
  BCDNO     bcdno; 
 
  if (API_GetTransportSpec(nim,&transpec) == False)
	return(False); 

#ifdef  DCII_DEFAULT_SMOOTHCLK
  /* (CR 8209) Added following block of code to allow default MPEG Smooth Clk Div setting */
  if (transpec == SPEC_DCII)
  {
    smoothclkdiv = DCII_HARD_SMOOTHCLK;
    if (RegisterWriteClkSmoothDiv(nim,smoothclkdiv) == False)
	return(False);
    if (DRIVER_SetSmoothClockEn(nim,cs) != True)
	return(False);
    return(True);
  }
#endif  /* #ifdef  DCII_DEFAULT_SMOOTHCLK */
 
  pllmult = (unsigned long)nim->ucPLLMult_shadow; 
  if (API_GetSymbolRate(nim,&symbolrate) == False)
	return(False); 
 
  /* (CR 7957) if locked to a signal, use that Coderate within clk.smoother.div calculation.  else, use highest Coderate */ 
  if (RegisterRead(nim,CX24130_ACQFULLSYNC,&locked, DEMOD_I2C_IO) == False)
	return(False); 
  if (locked == 0x01UL) 
  { 
    /* demod is locked, so use the locked coderate in the clkcmoother calc */ 
    if (API_GetViterbiRate(nim,&coderate) == False)
	return(False); 
    nim->CLKSMDIV_CR = coderate; 
  } 
  else 
  { 
    unsigned int  vrates; 
 
    /* nim has not locked with newest coderate yet, so flag this */ 
    if (fromCC == True || nim->CLKSMDIV_CR == CODERATE_NONE) 
    { 
      /* demod is NOT locked, so use the highest coderate within the clksmoother calc */ 
      if (API_AcqGetViterbiCodeRates(nim,&vrates) == False)
	return(False); 
      coderate = CODERATE_NONE; 
    } 
    else 
    { 
      /* cause no search rates to be found, use last-locked coderate */ 
      vrates = 0; 
      coderate = nim->CLKSMDIV_CR; 
    } 
 
    if ((vrates&CODERATE_7DIV8) !=0)  coderate = CODERATE_7DIV8; 
    else 
    { 
      if ((vrates&CODERATE_6DIV7) !=0)  coderate = CODERATE_6DIV7; 
      else 
      { 
        if ((vrates&CODERATE_5DIV6) !=0)  coderate = CODERATE_5DIV6; 
        else 
        { 
          if ((vrates&CODERATE_4DIV5) !=0)  coderate = CODERATE_4DIV5; 
          else 
          { 
            if ((vrates&CODERATE_3DIV4) !=0)  coderate = CODERATE_3DIV4; 
            else 
            { 
              if ((vrates&CODERATE_2DIV3) !=0)  coderate = CODERATE_2DIV3; 
              else 
              { 
                if ((vrates&CODERATE_1DIV2) !=0)  coderate = CODERATE_1DIV2; 
              } 
            } 
          } 
        } 
      } 
    } 
  } 
 
  switch(coderate) 
  { 
    case  CODERATE_1DIV2: 
    { 
      cr = 1; 
      crdiv = 2; 
      break; 
    } 
    case  CODERATE_2DIV3: 
    { 
      cr = 2; 
      crdiv = 3; 
      break; 
    } 
    default:  /* default coderate for sync punct */ 
    case  CODERATE_3DIV4: 
    { 
      cr = 3; 
      crdiv = 4; 
      break; 
    } 
    case  CODERATE_4DIV5: 
    { 
      cr = 4; 
      crdiv = 5; 
      break; 
    } 
    case  CODERATE_5DIV6: 
    { 
      cr = 5; 
      crdiv = 6; 
      break; 
    } 
    case  CODERATE_6DIV7: 
    { 
      cr = 6; 
      crdiv = 7; 
      break; 
    } 
    case  CODERATE_7DIV8: 
    { 
      cr = 7; 
      crdiv = 8; 
      break; 
    } 
  }  /* switch(... */ 
 
  if (RegisterRead(nim,CX24130_MPGGAPCLK,&rsparitydis, DEMOD_I2C_IO) == False)
	return(False); 
  if (RegisterRead(nim,CX24130_MPGSYNCPUNC,&syncpunctmode, DEMOD_I2C_IO) == False)
	return(False); 
 
  switch (transpec) 
  { 
    default: 
    case  SPEC_DVB: 
    { 
      cwlen = 204UL; 
      if (rsparitydis == 0UL && syncpunctmode == 0UL)  datalen = 204UL; 
      else  if (rsparitydis == 1UL && syncpunctmode == 0UL)  datalen = 188UL; 
      else  if (rsparitydis == 0UL && syncpunctmode == 1UL)  datalen = 203UL; 
      else  if (rsparitydis == 1UL && syncpunctmode == 1UL)  datalen = 187UL; 
      else  DRIVER_SET_ERROR(nim,API_BAD_PARM); 
      break; 
    } 
    case  SPEC_DSS:
    {
      cwlen = 146UL;
      if (rsparitydis == 0UL && syncpunctmode == 0UL)  datalen = 130UL;
      else  if (rsparitydis == 1UL && syncpunctmode == 0UL)  datalen = 146UL;
      else  if (rsparitydis == 0UL && syncpunctmode == 1UL)  datalen = 129UL;
      else  if (rsparitydis == 1UL && syncpunctmode == 1UL)  datalen = 145UL;
      else  DRIVER_SET_ERROR(nim,API_BAD_PARM);
      break;
    }
    case  SPEC_DCII:
    {
      cwlen = 204UL;
      if (rsparitydis == 0UL && syncpunctmode == 0UL)  datalen = 204UL;
      else  if (rsparitydis == 1UL && syncpunctmode == 0UL)  datalen = 188UL;
      else  if (rsparitydis == 0UL && syncpunctmode == 1UL)  datalen = 203UL;
      else  if (rsparitydis == 1UL && syncpunctmode == 1UL)  datalen = 187UL;
      else  DRIVER_SET_ERROR(nim,API_BAD_PARM);
      break;
    }
  }  /* switch(... */ 
 
  BCD_set(&bcdno,symbolrate); 
  BCD_mult(&bcdno,(4UL * cr * datalen)); 
  BCD_div(&bcdno,crdiv); 
  BCD_div(&bcdno,cwlen); 
  divno = BCD_out(&bcdno); 
 
  /* trap div by zero errors */ 
  if (DRIVER_div_zero(nim,divno) == False)  return(False);
  smoothclkdiv = (int)(nim->crystal_freq * pllmult) / divno; 
 
  /* write the clk smoother freq divider to hw */ 
  if (RegisterWriteClkSmoothDiv(nim, smoothclkdiv) == False) 
  { 
     return (False); 
  } 
 
  /* turn-on (i.e. Enable) the clock smoother if desired */ 
  if (DRIVER_SetSmoothClockEn(nim,cs) != True)	
	return(False); 
 
  return(True); 
 
}  /* DRIVER_SetSmoothClock() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetSmoothClockEn() */ 
/*******************************************************************************************************/ 
BOOL           DRIVER_SetSmoothClockEn(/* funct to set the clock smoother MPEG Enable register */ 
NIM            *nim,                   /* pointer to nim */ 
CLOCKSMOOTHSEL cs)                     /* clk smoother setting */ 
{ 
   unsigned long  ulRegVal; 
 
   /* turn-on/off clock smoother */ 
   switch  (cs) 
   { 
      case  CLK_SMOOTHING_OFF: 
      { 
         ulRegVal = 0UL; 
         break; 
      } 
      case  DDS_LEGACY_SMOOTHING: 
      { 
         ulRegVal = 1UL; 
         break; 
      } 
#ifdef CAMARIC_FEATURES
      case  PLL_ADVANCED_SMOOTHING: 
      { 
         if (DRIVER_Camaric(nim) != True)
         {
            /* If hardware is CX24121, set to DSS mode. */
            ulRegVal = 1UL; 
		/* Log a warning. */
            DRIVER_SET_ERROR(nim,API_SETILLEGAL);
         }
	 else
	 {
            ulRegVal = 3UL; 
	 }
         break; 
      } 
#endif  /* #ifdef CAMARIC_FEATURES */
      default: 
      { 
         /* invalid smoother selection passed to function (flow through to CLK_SMOOTHER_OFF (also Default) ) */ 
         DRIVER_SET_ERROR(nim,API_BAD_PARM); 
         return (False); 
      } 
   }  /* switch(... */ 

   if (DRIVER_Cobra(nim) == True)
   {
      if (RegisterWrite(nim,CX24130_MPGCLKSMOOTHEN,ulRegVal, DEMOD_I2C_IO) == False)
      {
         return (False);
      }
   }
#ifdef CAMARIC_FEATURES
   else if (DRIVER_Camaric(nim) == True)
   {
    if (RegisterWrite(nim,CX24123_MPGCLKSMOOTHSEL,ulRegVal, DEMOD_I2C_IO) == False) 
    { 
        return(False); 
    } 
   }
#endif  /* #ifdef CAMARIC_FEATURES */

   return (True); 
 
}  /* DRIVER_SetSmoothClockEn() */ 
 

/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/
/* Locking assistance code */
/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

/*******************************************************************************************************/
/* DRIVER_SWAssistInit() */
/*******************************************************************************************************/
BOOL   DRIVER_SWAssistInit(            /* Funct to assist demod to lock onto DCII CR == {5/11, 1/2, 3/5} */
NIM    *nim)                           /* pointer to nim */
{
  unsigned long  ulRegVal;
  unsigned char  b;

  /* read the number of bins */
  if (RegisterRead(nim,CX24130_ACQFREQRANGE,&ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
  nim->numofbins = (int)ulRegVal;

  b = (unsigned char)0x00;
  if (API_WriteReg(nim,0x16,&b) == False)
	return(False);
  b = (unsigned char)0x40;
  if (API_WriteReg(nim,0x43,&b) == False)
	return(False);

  b = (unsigned char)0x00;
  if (API_WriteReg(nim,0x0d,&b) == False)
	return(False);

  b = (unsigned char)0x39;
  if (API_WriteReg(nim,0xff,&b) == False)
	return(False);

  return(True);

}  /* DRIVER_SWAssistInit() */


/*******************************************************************************************************/
/* DRIVER_SWAssistExit() */
/*******************************************************************************************************/
BOOL   DRIVER_SWAssistExit(            /* Funct to assist demod to lock onto DCII CR == {5/11, 1/2, 3/5} */
NIM    *nim)                           /* pointer to nim */
{
  unsigned long  ulRegVal;
  unsigned char  b;

  ulRegVal = (unsigned long)nim->numofbins;
  if (RegisterWrite(nim,CX24130_ACQFREQRANGE,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);

  ulRegVal = 0x10;
  if (RegisterWrite(nim,CX24130_ACQAFCWIN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  /* modify by liuyegang to lock to 12053/v/27500 */
  ulRegVal = 0x2c;
  if (RegisterWrite(nim,CX24130_ACQACCCLREN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  b = (unsigned char)0x00;
  if (API_WriteReg(nim,0xff,&b) == False)
	return(False);

  return(True);

}  /* DRIVER_SWAssistExit() */


/*******************************************************************************************************/
/* DRIVER_SWAssistAcq() */
/*******************************************************************************************************/
BOOL   DRIVER_SWAssistAcq(             /* Funct to assist demod to lock onto DCII CR == {5/11, 1/2, 3/5} and DVB/DSS CR == 1/2 */
NIM    *nim)                           /* pointer to nim */
{
  int  i;

  long   lTemp;
  
  unsigned long  locked;
  BOOL      assist_required;
  TRANSPEC  transpec;


#ifdef CAMARIC_FEATURES
  /* (CR 8539) Remove +/-10mHz SW binning for Camaric */
  if (DRIVER_Camaric(nim) == True)
	return(True);
#endif  /* #ifdef CAMARIC_FEATURES */


  assist_required = False;

  /* only perform SW assist for DCII and cr=5/11, 1/2, 3/5, DVB/DSS 1/2 */
  if (API_GetTransportSpec(nim,&transpec) == False)
	return(False);
  switch (transpec)
  {
    case  SPEC_DCII:
    {
      if ((int)(nim->chanobj.viterbicoderates&CODERATE_5DIV11) == (int)CODERATE_5DIV11 ||
        (int)(nim->chanobj.viterbicoderates&CODERATE_1DIV2) == (int)CODERATE_1DIV2 ||
        (int)(nim->chanobj.viterbicoderates&CODERATE_3DIV5) == (int)CODERATE_3DIV5)
      {
        assist_required = True;
      }
      break;
    }
    default:
    {
      assist_required = False;
      break;
    }
  }
  
  if (assist_required == True)
  {
    if (RegisterRead(nim,CX24130_ACQFULLSYNC,&locked, DEMOD_I2C_IO) == False)
	return(False);

    if (locked == 0x01UL)
	return(True);

    if (DRIVER_SWAssistInit(nim) == False)
	return(False);

    for (i = 0 , lTemp = 0L ; i < DCII_SWACQLOOP ; i++)
    {
      locked = 0;

      if (DRIVER_SWAssistAcqBinning(nim,lTemp,&locked) == False)
	return(False);

      if (locked == 0x01UL)
      {
        if (DRIVER_SWAssistExit(nim) == False)
		return(False);
        return(True);
      }
      lTemp += 1L;
      if (lTemp > 5L)  lTemp = 0L;
    }

    if (DRIVER_SWAssistExit(nim) == False)
	return(False);
    return(True);
  }

  return(True);

}  /* DRIVER_SWAssistAcq() */


/*******************************************************************************************************/
/* DRIVER_SWAssistAcq_CR1DIV2() (CR 7286) */
/*******************************************************************************************************/
BOOL         DRIVER_SWAssistAcq_CR1DIV2(  /* Special op for Coderate 1/2 contained within Viterbi list */
NIM          *nim,                        /* pointer to nim */
unsigned int vrates)                      /* code rates OR'd together */
{
  unsigned long  ulRegVal;

#ifdef CAMARIC_FEATURES
  /* (CR 8539) Remove +/-10mHz SW binning for Camaric */
  if (DRIVER_Camaric(nim) == True)
	return(True);
#endif  /* #ifdef CAMARIC_FEATURES */

  /* (CR 7349) Cause ACQDEINTWIN value to deviate from default for CR 1/2 */
  if ((vrates&CODERATE_1DIV2) == CODERATE_1DIV2) ulRegVal = 64UL;
  else  ulRegVal = 16;

  if (RegisterWrite(nim,CX24130_ACQAFCWIN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* DRIVER_SWAssistAcq_CR1DIV2() */


/*******************************************************************************************************/
/* DRIVER_SWAssistAcqBinning */
/*******************************************************************************************************/
BOOL          DRIVER_SWAssistAcqBinning(       /* funct to attempt a form of binning for DCII CR == {5/11, 1/2, 3/5} */
NIM           *nim,                            /* pointer to nim */
long          freq,                            /* calculated bin to attempt to lock into */
unsigned long *locked)                         /* return indicates 0x01 if locked */
{
  unsigned char  b;
  unsigned char  c;

  b = (unsigned char)0x34;
  if (API_WriteReg(nim,0x5b,&b) == False)
	return(False);

  b = (unsigned char)0x08;
  if (API_WriteReg(nim,0x3c,&b) == False)
	return(False);

  switch (freq)
  {
    case  0L:
    {
      b = (unsigned char)0x00;
      c = (unsigned char)0x00;
      break;
    }
    case  1L:
    {
      /* 1/2 nom */
      b = (unsigned char)0x3f;
      c = (unsigned char)0x0f;
      break;
    }
    case  2L:
    {
      /* -1/2 nom */
      b = (unsigned char)0xc0;
      c = (unsigned char)0x01;
      break;
    }
    case  3L:
    {
      /* max nom */
      b = (unsigned char)0x7f;
      c = (unsigned char)0x0f;
      break;
    }
    case  4L:
    {
      /* -max nom */
      b = (unsigned char)0x80;
      c = (unsigned char)0x01;
      break;
    }
  }

  if (API_WriteReg(nim,0x0b,&b) == False)
	return(False);
  if (API_WriteReg(nim,0x0c,&c) == False)
	return(False);

  b = (unsigned char)0x00;
  if (API_WriteReg(nim,0x5b,&b) == False)
	return(False);
  if (API_WriteReg(nim,0x5c,&b) == False)
	return(False);

  b = (unsigned char)0x20;
  if (API_WriteReg(nim,0x72,&b) == False)
	return(False);

  b = (unsigned char)0x14;
  if (API_WriteReg(nim,0x3a,&b) == False)
	return(False);

  b = (unsigned char)127;
  if (API_WriteReg(nim,0x3b,&b) == False)
	return(False);

  if (API_ReadReg(nim,0x14,&b) == False)
	return(False);
  *locked = 0x00;
  if ((b&0x80) == 0x80)
	*locked = 0x01;

  b = (unsigned char)0x00;
  if (API_WriteReg(nim,0x72,&b) == False)
	return(False);

  b = (unsigned char)0x10;
  if (API_WriteReg(nim,0x5c,&b) == False)
	return(False);

  return(True);

}  /* DRIVER_SWAssistAcqBinning() */


/*******************************************************************************************************/
/* DRIVER_SWAssistTuner() (CR 6243) */
/*******************************************************************************************************/
BOOL   DRIVER_SWAssistTuner(           /* Funct to expand initial LNB range to 10mHz */
NIM    *nim)                           /* pointer to nim */
{
  int    bin;

  unsigned long  ulRegVal;
  unsigned char  b;
  
  unsigned long  offsetfreq;
  long   offset = 0L;
  
  BOOL  locked = False;

	
  /* test that the NIM is valid */
    DRIVER_VALIDATE_NIM(nim); 

#ifdef CAMARIC_FEATURES
  /* (CR 8538) Remove +/-10mHz SW binning for Camaric */
  if (DRIVER_Camaric(nim) == True)
	return(True);
#endif  /* #ifdef CAMARIC_FEATURES */


  /* test if assit is required */
  if (nim->tuner_offset == 0UL)
	return(True);

  /* give some time back to the caller */
  nim->swa_count++;
  if (nim->swa_count < LNB_DYN_SEARCH_ACQ_DELAY)
	return(True);
  nim->swa_count = 0;

  /* test if locked and tracking, in which case assistance is not required */
  if (RegisterRead(nim,CX24130_SYNCSTATUS,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  b = (unsigned char)ulRegVal;
  if ((b&demod_register_map[CX24130_ACQFULLSYNC].p_hw_mask[0]) != 0)
	return(True);

  /* save the desired frequency */
  if (nim->actual_tuner_offset >= 0L)  nim->saved_frequency = nim->pll_frequency - nim->actual_tuner_offset;
  else  nim->saved_frequency = nim->pll_frequency + (unsigned long)(nim->actual_tuner_offset * -1L);

  /* save the actual freq desired */
  nim->pll_frequency = nim->saved_frequency;

  /* demod did not lock initially, so look last-known, high(Fi+offset), finally low(Fi-offset),  until acquisition or timeout */
  offset = nim->actual_tuner_offset;
  nim->iostatus = offset;

  for (bin = 0 ; bin < 3 ; bin++)
  {
    locked = False;

    if (offset >= 0l)  offsetfreq = nim->pll_frequency + offset;
    else  offsetfreq = nim->pll_frequency - (offset * -1L);

    if (_DRIVER_SWAssistTunerPass(nim,offsetfreq,&locked) == False)
	return(False);
    if (locked == True)
	break;

    offset += nim->tuner_offset;
    if (offset > (long)nim->tuner_offset)  offset = ((nim->tuner_offset+LNB_TUNERLSBA) * -1L);
    nim->pll_frequency = nim->saved_frequency;
  }
  
  nim->actual_tuner_offset = 0L;
  if (locked == True)
  {
    nim->actual_tuner_offset = offset;
    return(True);
  }

  /* unable to find signal */
  nim->pll_frequency = nim->saved_frequency;
  if (DRIVER_SetTunerFrequency(nim,nim->pll_frequency) == False)
	return(False);
  return(True);

}  /* DRIVER_SWAssistTuner() (CR 6243) */


/*******************************************************************************************************/
/* _DRIVER_SWAssistTunerPass() */
/*******************************************************************************************************/
BOOL           _DRIVER_SWAssistTunerPass(      /* Funct to expand initial LNB range to 10mHz */
NIM            *nim,                           /* pointer to nim */
unsigned long  freq,                           /* freq to attempt to lock into */
BOOL           *hit)                           /* returns True if locked onto signal (False, otherwise) */
{
  
  unsigned long  ulRegVal;
  unsigned char   b = 0;

  unsigned long  __freq;
  BOOL     acqb_flag;

  *hit = False;

  /* (CR 7581) */
  /* read lock status byte, if full sync is lit, bail (all is well) */
  if (RegisterRead(nim,CX24130_SYNCSTATUS,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  b = (unsigned char)ulRegVal;
  if ((b&demod_register_map[CX24130_ACQFULLSYNC].p_hw_mask[0]) != 0)  *hit = True;
  else
  {
    /* push tuner PLL out by an additional 1/2 tuner reg-a lsb value */
    __freq = freq;
    acqb_flag = False;
    if (freq != nim->saved_frequency)
    {
      acqb_flag = True;
      if (freq > nim->saved_frequency)  __freq += LNB_TUNERLSBA;
      else  __freq -= LNB_TUNERLSBA;
    }

    /* Make sure the tuner frequency is within the LNB search limit */
    if (DRIVER_ValidTunerFreqRange(nim, __freq) == False)
    {
       return (True);
    }

    /* set the tuner freq to offset */
    if (TUNER_SetFrequency(nim,__freq) == False)
    {
      DRIVER_SET_ERROR(nim,API_BAD_SWA);
      return(False);
    }

    /* (CR 7999) added following line */
    if (acqb_flag == True)
	API_AcqBegin(nim);

    API_OS_Wait(nim,PLL_LOCK_ABORT_TIME);

    if (RegisterRead(nim,CX24130_SYNCSTATUS,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    b = (unsigned char)ulRegVal;
    if ((b&demod_register_map[CX24130_PLLLOCK].p_hw_mask[0]) != 0)
    {
      API_OS_Wait(nim,TRACKING_STATE_POLLING_TIME);

      if (RegisterRead(nim,CX24130_SYNCSTATUS,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      b = (unsigned char)ulRegVal;
      if ((b&demod_register_map[CX24130_ACQFULLSYNC].p_hw_mask[0]) != 0)
      {
        *hit = True;
        return(True);
      }
    }
  }

  return(True);

}  /* _DRIVER_SWAssistTunerPass() */

 
/*******************************************************************************************************/ 
/* DRIVER_SetLNBMode() */ 
/*******************************************************************************************************/ 
BOOL     DRIVER_SetLNBMode(            /* function to set-up LNB tone output options */ 
NIM      *nim,                         /* pointer to NIM */ 
LNBMODE  *lnbmode)                     /* pointer to struct containing LNB tone output settings */ 
{ 
  unsigned long  ulRegVal; 
 
  if (nim == NULL || lnbmode == NULL) 
  { 
    DRIVER_SET_ERROR(nim,API_BAD_PARM); 
    return(False); 
  } 
 
  /* write the LNB set-up registers */ 
  ulRegVal = lnbmode->tone_clock;
  if (RegisterWrite(nim,CX24130_LNBTONECLK,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  ulRegVal = lnbmode->cycle_count; 
  if (RegisterWrite(nim,CX24130_LNBBURSTLENGTH,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
  ulRegVal = (unsigned long)lnbmode->lnb_mode; 
  if (RegisterWrite(nim,CX24130_LNBMODE,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
  return(True); 
 
}  /* DRIVER_SetLNBMode() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SendDiseqc() */ 
/*******************************************************************************************************/ 
BOOL          DRIVER_SendDiseqc(       /* function to set-up and control the output of Diseq messages */ 
NIM           *nim,                    /* pointer to NIM */ 
unsigned char *msg,                    /* message to send */ 
unsigned char msg_len,                 /* length of message */ 
BOOL          msg_long,                /* TRUE if this message is part of a longer message */ 
BOOL          last_msg)                /* TRUE if this is the last message */ 
{ 
  int   i; 
 
  BOOL  done; 
 
  unsigned long  ulRegVal; 
  unsigned char  b; 
  unsigned long  max_loop; 
 
  /* test size of message to determine how to proceed */ 
  if (msg_len <= 6) 
  { 
    /* test for a short message */ 
    if (msg_len < 3)   
    { 
      DRIVER_SET_ERROR(nim,API_LNB_MSGLEN); 
      return(False); 
    } 
 
    /* send message to the demod */ 
    for (i = 0 ; i < msg_len ; i++) 
    { 
      /* fill the message into the demod's lnb message buffer */ 
      b = msg[i]; 
      if (API_WriteReg(nim,(unsigned char)(i + demod_register_map[CX24130_LNBMSG1].address),&b) == False)
	return(False); 
    } 
 
    /* set length of message */ 
    ulRegVal = ((unsigned long)msg_len - 3UL); 
    if (RegisterWrite(nim,CX24130_LNBMSGLENGTH,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
  } 
  else 
  { 
    /* indicate to caller that a long lnb message was attempted */ 
    DRIVER_SET_ERROR(nim,API_LNB_MSGLEN); 
    return(False); 
  } 
 
  /* (CR 7488, CR 7489) set LONGMSG flag on long message, reset MOREMSG flag */ 
  if (msg_long == True) 
  { 
    ulRegVal = 0x01UL; 
    if (RegisterWrite(nim,CX24130_LNBLONGMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
    ulRegVal = 0x00UL; 
    if (RegisterWrite(nim,CX24130_LNBMOREMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
  } 
  else if (last_msg == True) 
  { 
    ulRegVal = 0x00UL; 
    if (RegisterWrite(nim,CX24130_LNBLONGMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
    ulRegVal = 0x00UL; 
    if (RegisterWrite(nim,CX24130_LNBMOREMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
  } 
  else 
  { 
    /* message is no longer composed of multi-parts, so reset LONGMSG flag */ 
    ulRegVal = 0x00UL; 
    if (RegisterWrite(nim,CX24130_LNBLONGMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
    ulRegVal = 0x01UL; 
    if (RegisterWrite(nim,CX24130_LNBMOREMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
  } 
 
  /* send the message (or last part of the message, if a long message) */ 
  ulRegVal = 0x01; 
  if (RegisterWrite(nim,CX24130_LNBSENDMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
 
  /* loop until message has been delivered */ 
  max_loop = 0UL; 
  done = False; 
  do 
  { 
    /* test for stalled demod (inability to send diseqc message for some reason) */ 
    if (max_loop > MAX_LNBMSGWAITOSW) 
    { 
      DRIVER_SET_ERROR(nim,API_LNB_STALLED); 
      return(False); 
    } 
 
    /* test if message has been sent, if so, return */ 
    if (RegisterRead(nim,CX24130_LNBSENDMSG,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 
    if (ulRegVal == 0x01UL)
	done = True; 
 
    /* message has not been delivered, so loop until delivered, or time expires */ 
    max_loop++; 
  }  while (done == False); 
 
  return(True); 
 
}  /* DRIVER_SendDiseqc() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetTunerFrequency() */ 
/*******************************************************************************************************/ 
BOOL           DRIVER_SetTunerFrequency(         /* function to set tuner frequency */ 
NIM            *nim,                             /* pointer to nim */ 
unsigned long  freq)                             /* frequency to be set */ 
{ 
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
   unsigned long  Fa = 0UL;
   unsigned int   bestpllmult = 0U;

   nim->opt_Fs_chosen = 0UL;
   nim->opt_Fs_pllmult = 0U;
#endif  /* #ifdef OPTIMAL_FS_CODE */

   /* test for valid nim */ 
    DRIVER_VALIDATE_NIM(nim); 
 
   /* make sure the tuner frequency to be set is within the LNB search range */ 
   if (DRIVER_ValidTunerFreqRange(nim, freq) == False) 
   { 
      return (False); 
   } 
 
   if (TUNER_SetFrequency(nim,freq) == False) 
   { 
      return (False); 
   } 

#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
   if (nim->opt_fs_disable != True)
   {
      nim->optimal_Fs_set = False;
      nim->opt_Fs_chosen = 0UL;
      nim->opt_Fs_pllmult = 0U;
      if (nim->samplerate != SAMPLE_FREQ_EXT)
      {
         if (API_GetPLLFrequency(nim,&Fa) == False)
         {
            return (False);
         }
         if (DRIVER_Opt_Fs_optimizeFs(nim,Fa,&bestpllmult) == False)
         {
            return (False);
         }
         if (bestpllmult != 0U)
         {
            /* save the Fs info chosen into the nim (for use in chan.change code) */
            nim->optimal_Fs_set = True;
            nim->opt_Fs_chosen = DRIVER_compute_fs(bestpllmult,nim->crystal_freq);
            nim->opt_Fs_pllmult = bestpllmult;
         }
      }
   }
#endif  /* #ifdef OPTIMAL_FS_CODE */

   return (True); 
}  /* DRIVER_SetTunerFrequency() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_ValidTunerFreqRange() */ 
/*******************************************************************************************************/ 
BOOL           DRIVER_ValidTunerFreqRange(       /* function to test if the tuner frequency is within 
                                                    the LNB search range */ 
NIM            *nim,                             /* pointer to nim */ 
unsigned long  freq)                             /* tuner frequency  */ 
{ 
   unsigned long  ulTemp; 
 
   /* test for valid nim */ 
    DRIVER_VALIDATE_NIM(nim); 
 
   /* make sure the tuner frequency is within the LNB search range */ 
   ulTemp = (freq > nim->freq_ideal) ? (freq - nim->freq_ideal) : (nim->freq_ideal - freq); 
   if (ulTemp > nim->lnboffset) 
   { 
      DRIVER_SET_ERROR(nim,API_BAD_TUNER_FREQ); 
      return (False); 
   } 
 
   return (True); 
 
}  /* DRIVER_ValidTunerFreqRange() */ 


/*******************************************************************************************************/
/*  OPTIMAL_FS_CODE (start of functions) */
/*******************************************************************************************************/
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
/*******************************************************************************************************/
/* DRIVER_Opt_Fs_buildtable() */
/*******************************************************************************************************/
BOOL  DRIVER_Opt_Fs_buildtable(        /* function to build Fs optimation table */
NIM   *nim)                            /* pointer to nim */
{
  int  i;
  unsigned long xtal = nim->crystal_freq;

#ifdef CAMARIC_FEATURES
  /* if device is Camaric, not reason to optimize */
  if (DRIVER_Camaric(nim) == True)
	return(True);
#endif  /* #ifdef CAMARIC_FEATURES */

  if (nim == NULL || xtal == 0UL)  return(False);

  /* step-through each possible pll mult setting */
  for (i = 0 ; i <= MAX_PLL_MULT ; i++)
  {
    /* calculate the Fvco, Fs, fastclk values for each possible pll mult setting */
    nim->opt_fs.Fvco[i] = xtal * (unsigned long)i;                 /* vco = pllmult x xtal */
    nim->opt_fs.Fs[i] = nim->opt_fs.Fvco[i] / 6UL;                 /* Fs = vco / 6 */
    nim->opt_fs.fastclk[i] = (nim->opt_fs.Fs[i] * 4UL) / 3UL;      /* Fclk = Fs * 4 / 3 */
  }
  
  /* set default Fs as one chosen (this chosen Fs might also change at ChangeChannel() ) */
  /* if (API_GetAssociatedSampleFrequency(nim,SAMPLE_FREQ_NOM_VAL,&nim->opt_Fs_chosen) == False)  return(False); */

  return(True);

}  /* DRIVER_Opt_Fs_buildtable() */


/*******************************************************************************************************/
/* DRIVER_Opt_Fs_calcPLLMult() */
/*******************************************************************************************************/
BOOL           DRIVER_Opt_Fs_calcPLLMult(   /* function to build min, max pllmult range */
NIM            *nim)                        /* pointer to nim */
{
  unsigned long  xtal = nim->crystal_freq;
  unsigned long  max_sample_clock;
  unsigned long  actualsymbolrateHz;

  static BCDNO  bcd;
  static BCDNO  bcd2;

#ifdef CAMARIC_FEATURES
  /* if device is Camaric, not reason to optimize */
  if (DRIVER_Camaric(nim) == True)
	return(True);
#endif  /* #ifdef CAMARIC_FEATURES */

  if (API_GetSymbolRate(nim,&actualsymbolrateHz) == False)
	return(False);

  if (nim == NULL || actualsymbolrateHz == 0UL || xtal == 0UL)
	return(False);

  /* set-up the step-size */
  BCD_set(&bcd2,xtal);
  BCD_mult(&bcd2,1000UL);
  BCD_div(&bcd2,6UL);
  
  /* set-up the Min. oversampling ratio */
  BCD_set(&bcd,actualsymbolrateHz);
  BCD_mult(&bcd,3100UL);
  BCD_div_bcd(&bcd,&bcd2);

  /* save the min oversampling ratio */
  nim->opt_fs.min_PLL_MULT =  (unsigned int)BCD_out(&bcd);

  /* compute max pll mult */
  max_sample_clock = actualsymbolrateHz * 9UL;
  if (max_sample_clock > MAX_OPT_SAMPLECLK)  max_sample_clock = MAX_OPT_SAMPLECLK;
  BCD_set(&bcd,max_sample_clock);
  BCD_mult(&bcd,1000UL);
  BCD_div_bcd(&bcd,&bcd2);

  nim->opt_fs.max_PLL_MULT =  (unsigned int)BCD_out(&bcd);

  return(True);

}  /* DRIVER_Opt_Fs_calcPLLMult() */


/*******************************************************************************************************/
/* DRIVER_Opt_Fs_Hbelow() */
/*******************************************************************************************************/
unsigned long  DRIVER_Opt_Fs_Hbelow(   /* function to calculate harmonic below (near) channel freq */
NIM            *nim,                   /* pointer to nim */
unsigned long  Fa,                     /* Actual tuner frequency (not desired freq) */
unsigned long  clk)                    /* clock used to determine harmonics locations */
{
  unsigned int   h;
  unsigned long  ulTemp;

  if (nim == NULL)  return(False);

  h = (unsigned int)(Fa / clk);
  ulTemp = Fa - (h * clk);

  return(ulTemp);

}  /* DRIVER_Opt_Fs_Hbelow() */


/*******************************************************************************************************/
/* DRIVER_Opt_Fs_Hfastclock() */
/*******************************************************************************************************/
unsigned long  DRIVER_Opt_Fs_Hfastclock(    /* function to find fastclock harmonics */
NIM            *nim,                        /* pointer to nim */
unsigned int   harmcnt,                     /* number of harmonics in harmtable */
long  *harmtable,                  /* harmonics table */
unsigned long  fastclock)                   /* fastclock freq */
{
  unsigned int   i;

  unsigned long  clockharmplus;
  unsigned long  clockharmminus;

  unsigned long  nearharm;         /* nearest +/- clock */
  unsigned long  minharm = 0UL;    /* nearest over entire search */
  long  lTemp;

  if (nim == NULL)  return(False);

  for (i = 0 ; i < harmcnt ; i++)
  {
    lTemp = (harmtable[i] + fastclock);
    clockharmplus = lTemp >= 0 ? lTemp : (lTemp * -1L);
    lTemp = (harmtable[i] - fastclock);
    clockharmminus = lTemp >= 0 ? lTemp : (lTemp * -1L);

    nearharm = min(clockharmplus,clockharmminus);
    if (i != 0)
    {
      minharm = min(nearharm,minharm);
    }
    else
    {
      minharm = nearharm;
    }
  }

  return(minharm);

}  /* DRIVER_Opt_Fs_Hfastclock() */


/*******************************************************************************************************/
/* DRIVER_Opt_Fs_optimizeFs() */
/*******************************************************************************************************/
BOOL           DRIVER_Opt_Fs_optimizeFs(    /* function to build Fs optimation table */
NIM            *nim,                        /* pointer to nim */
unsigned long  Fa,                          /* freq, actual (freq tuner is tuner to (or desired to be tuned to) */
unsigned int   *bestpllmult)                /* best pll mult setting or zero (zero indicates default setting chosen) */
{
  int  i;
  unsigned int  pllmult;

  unsigned long  farharm[3];
  unsigned int   bestharm[3];
  unsigned long  mixharm[4];

  unsigned long  harmbelow = 0UL;
  unsigned long  harmabove = 0UL;
  unsigned long  harmnear = 0UL;
  unsigned long  samclk_harmbelow = 0UL;

  unsigned long  nearestFastClkMix = 0UL;
  unsigned long  ulTemp;
  long           lTemp;

  if (nim == NULL || Fa == 0UL)
	return(False);

#ifdef CAMARIC_FEATURES
  /* if device is Camaric, not reason to optimize */
  if (DRIVER_Camaric(nim) == True)
	return(True);
#endif  /* #ifdef CAMARIC_FEATURES */

  /* Fs optimization only viable for single-channel */
  *bestpllmult = 0U;
  if (nim->demod_type != CX24121)  return(True);

  /* initialization */
  memset(farharm,0,sizeof(farharm));
  memset(bestharm,0,sizeof(bestharm));
  memset(mixharm,0,sizeof(mixharm));

  /* search all possible PLLMult settings, and find the best sample frequency */
  for (pllmult = nim->opt_fs.min_PLL_MULT ; pllmult < nim->opt_fs.max_PLL_MULT ; pllmult++)
  {
    /* Test 0,  sample clock harmonic.  Save harmonic below chan for Test#2 use */
    harmbelow = DRIVER_Opt_Fs_Hbelow(nim,Fa,nim->opt_fs.Fs[pllmult]);
    samclk_harmbelow = harmbelow;

    /* spur above channel */
    harmabove = nim->opt_fs.Fs[pllmult] - harmbelow; 

    /* keep the location of nearer spur, because it degrades performace the most */
    harmnear = min(harmbelow,harmabove);

    /* Perform Test 0 */
    if(harmnear < (nim->antialias_bandwidthkhz * M)) 
    {
      /* did not pass the Test 0, but save H info */
      if(harmnear > farharm[0])
      {
        farharm[0] = harmnear;
        bestharm[0] = pllmult;
      }
      /* this pllmult fails Test 0, so continue with next pllmult */
      continue;
    }

    /* Test 1 set-up: VCO clock +-fast clock (start with VCO spur below channel) */
    harmbelow = DRIVER_Opt_Fs_Hbelow(nim,Fa,nim->opt_fs.Fvco[pllmult]);

    /* prepare 2 VCO harmonic locations to be mixed with fast clock */
    mixharm[0] = (harmbelow * -1L);
    mixharm[1] = nim->opt_fs.Fvco[pllmult] - harmbelow;

    /* get fastclock harmonic, and compare.  Keep only the nearest. */
    ulTemp = DRIVER_Opt_Fs_Hfastclock(nim,2,mixharm,nim->opt_fs.fastclk[pllmult]);
    harmnear = min(harmnear,ulTemp);

    /* perform Test 1 */
    if(harmnear < (nim->antialias_bandwidthkhz * 1000UL)) 
    {
      if(harmnear > farharm[1])
      {
        farharm[1] = harmnear;
        bestharm[1] = pllmult;
      }
      /* this pllmult fails Test 1, thus continue next pllmult */
      continue;
    }

    /* Test 2: Set-up: prepare four nearest sample clock neighbors that might be mixed with fast clock */
    for(i = 0 ; i < 4 ; i++) 
    { 
      lTemp = (((long)samclk_harmbelow * -1L) + ((long)(i-1) * (long)nim->opt_fs.Fs[pllmult]));
      mixharm[i] = lTemp;
    }

    nearestFastClkMix = DRIVER_Opt_Fs_Hfastclock(nim,4,mixharm,nim->opt_fs.fastclk[pllmult]);
    harmnear = min(harmnear,nearestFastClkMix);

    /* perform Test 2 */
    if(harmnear > farharm[2])
    {
      farharm[2] = harmnear;
      bestharm[2] = pllmult;
    }
  }  /* for(... */


  /* find PLLMult that passes all three tests;  else, use the result of Test 1; else use result of Test 0. */
  *bestpllmult = 0U;
  for(i = 3 ; --i >= 0 ; )
  {
    if(bestharm[i] != 0UL)
    {
      *bestpllmult = bestharm[i];
      break;
    }
  }

  return(True);

}  /* DRIVER_Opt_Fs_optimizeFs() */


#endif  /* #ifdef OPTIMAL_FS_CODE */
/*******************************************************************************************************/
/*  OPTIMAL_FS_CODE (end of functions) */
/*******************************************************************************************************/
 
/*******************************************************************************************************/ 
/* DRIVER_SetSoftDecisionThreshold() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_SetSoftDecisionThreshold(  /* funct to select and set demod soft decision threshold */ 
NIM   *nim)                             /* pointer to nim */ 
{ 
   TRANSPEC  tran; 
   CODERATE  cr; 
   long      softdec = -1L; 
 
   unsigned long  ulRegVal; 
 
   if (API_GetTransportSpec(nim,&tran) == False) 
   { 
      return (False); 
   } 
 
   if (API_GetViterbiRate(nim,&cr) == False) 
   { 
      return (False); 
   } 
 
   switch (tran) 
   { 
      case  SPEC_DSS:
      {
         if (cr == CODERATE_1DIV2 || cr == CODERATE_2DIV3)
         {
            softdec = 1L;
         }
         else
         {
            softdec = 0L;
         }
         break;
      }
      case  SPEC_DVB: 
      { 
         if (cr == CODERATE_1DIV2) 
         { 
            softdec = 1L; 
         } 
         else 
         { 
            softdec = 0L; 
         } 
         break; 
      } 
      case  SPEC_DCII:
      {
         if (cr == CODERATE_5DIV11 || cr == CODERATE_1DIV2)
         {
            softdec = 1L;
         }
         else
         {
            softdec = 0L;
         }
         break;
      }
      default: 
      { 
         break; 
      } 
   }  /* switch */ 
 
   if (softdec != -1L) 
   { 
      ulRegVal = (unsigned long)softdec; 
      if (RegisterWrite(nim,CX24130_DMDSDTHRESH,ulRegVal, DEMOD_I2C_IO) == False) 
      { 
         return (False); 
      } 
   } 
 
   return (True); 
 
}  /* DRIVER_SetSoftDecisionThreshold() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetTunerFilterBWVoltage() */ 
/*******************************************************************************************************/ 
BOOL          DRIVER_SetTunerFilterBWVoltage(  /* funct to set the voltage that controls tuner anti-alias filter BW */ 
NIM           *nim,                            /* pointer to nim */ 
unsigned long mV)                              /* the voltage in mV to set (max 3.3 volt) */ 
{ 
	unsigned long  ulRegVal; 
	long  lSigmaDelta; 
 
	if (nim->tuner_type == CX24113) 
	{ 
		/* lSigmaDelta = (mV * 0.3126) - 512 */ 
		lSigmaDelta = 0; 
		if (mV != 0UL) 
		{ 
			lSigmaDelta = (long)mV * 3126L; 
			lSigmaDelta /= 10000L; 
		} 
		lSigmaDelta -= 512L; 
 
		/* the CX24130_FILVALUE register takes 10 bit value in 2's complement format */ 
		ulRegVal = DRIVER_convert_twos_saturate(lSigmaDelta,Register_bitlength(CX24130_FILVALUE));		 
		if (RegisterWrite(nim, CX24130_FILVALUE9_2, (ulRegVal >> 2UL), DEMOD_I2C_IO) == False) 
		{ 
			return (False); 
		} 
		if (RegisterWrite(nim, CX24130_FILVALUE1_0, (ulRegVal & 0x3UL), DEMOD_I2C_IO) == False) 
		{ 
			return (False); 
		} 
	} 
	return (True); 
}  /* DRIVER_SetTunerFilterBWVoltage() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetRSCntlPin() */ 
/*******************************************************************************************************/ 
BOOL            DRIVER_SetRSCntlPin(            /* Function to set MPEG RS Cntl pin setting */ 
NIM             *nim,                           /* pointer to nim */ 
REGIDX          regIdx,                         /* index to the target register */ 
RS_CNTLPIN_SEL  RSCntlPinSel)                   /* RS pin setting */ 
{ 
   const unsigned long ulRegValueMap[] = {0x03, 0x00, 0x01, 0x02, 0x03}; 
   /* ulRegValueMap[RS_CNTLPIN_START]     = ulRegValueMap[1] = 0x00 */ 
   /* ulRegValueMap[RS_CNTLPIN_VALID]     = ulRegValueMap[2] = 0x01 */ 
   /* ulRegValueMap[RS_CNTLPIN_FAIL]      = ulRegValueMap[3] = 0x02 */ 
   /* ulRegValueMap[RS_CNTLPIN_INACTIVE]  = ulRegValueMap[4] = 0x03 */ 
 
   unsigned long  ulRegVal; 
 
   /* validate RS control pin 1 setting */ 
   if (RSCntlPinSel <= RS_CNTLPIN_UNDEF || RSCntlPinSel >= RS_CNTLPIN_ENUM_COUNT) 
   { 
      DRIVER_SET_ERROR(nim,API_BAD_PARM); 
      return (False); 
   } 
 
   /* write control pin setting to the target register */ 
   ulRegVal = ulRegValueMap[RSCntlPinSel]; 
   if (RegisterWrite(nim,(unsigned short)regIdx,ulRegVal, DEMOD_I2C_IO) == False) // RegisterWriteToHDWR ??? 
   { 
      return (False); 
   } 
 
   return (True); 
 
}  /* DRIVER_SetRSCntlPin() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetViterbiLockThresh() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_SetViterbiLockThresh(  /* set Viterbi normalization threshold optimized for specific coderates */ 
NIM       *nim,                     /* pointer to nim */ 
CODERATE  coderate)                 /* viterbi coderate */ 
{ 
   unsigned long ulRegVal; 
 
   switch (coderate) 
   { 
      case CODERATE_1DIV2: 
         ulRegVal = VITNORMTHRESH_1DIV2; 
         break; 
      case CODERATE_2DIV3: 
         ulRegVal = VITNORMTHRESH_2DIV3; 
         break; 
      default: 
         ulRegVal = VITNORMTHRESH_DEFAULT; 
         break; 
   } 
 
   return (RegisterWrite(nim,CX24130_ACQVITNORMTHRESH,ulRegVal, DEMOD_I2C_IO));  
}  /* DRIVER_SetViterbiLockThresh() */ 

/*******************************************************************************************************/
/* DRIVER_Cobra() */
/*******************************************************************************************************/
BOOL  DRIVER_Cobra(  /* funct returns True if demod attached to nim is Cobra */
NIM   *nim)          /* pointer to nim */
{
   /* return True if demod is Cobra, single or dual */
   if (nim->demod_type == CX24121 || nim->demod_type == CX24130)
   {
      return(True);
   }
   return(False);

}  /* DRIVER_Cobra() */

/*******************************************************************************************************/
/* Start of Camaric code */
/*******************************************************************************************************/
#ifdef CAMARIC_FEATURES
/*******************************************************************************************************/ 
/* DRIVER_Camaric() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_Camaric(                  /* funct returns True if demod attached to nim is Cobra */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  /* return True if demod is Cobra, including dual camaric */ 
  if (nim->demod_type == CX24123 || nim->demod_type == CX24123C)
	return(True); 
  return(False); 
 
}  /* DRIVER_Camaric() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_Default_Camaric() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_Default_Camaric(          /* funct returns True if demod attached to nim is Cobra */ 
NIM   *nim)                            /* pointer to nim */ 
{ 
  int    i; 
  unsigned long  ulTemp; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* set-up default values only for Cobra */ 
  if (DRIVER_Camaric(nim) != True)  return(True); 
 
  /* find each byte-wide non-RO register that is used */ 
  for (i = 0 ; i < CX24130_REG_COUNT ; i++) 
  { 
     /* set the default setting if the register is flagged either CAM_ONLY or CAM_DEF */ 
     if (((demod_register_map[i].regfilter & REGF_CAM_DEF)  == REGF_CAM_DEF ||  
          (demod_register_map[i].regfilter & REGF_CAM_ONLY) == REGF_CAM_ONLY) &&  
		  (demod_register_map[i].access_level != REG_RO)) 
     { 
         ulTemp = demod_register_map[i].default_value;
 
         /* watch for registers that should be skipped */ 
         if (ulTemp != 0xffffffffUL) 
         { 
		if (i == CX24123_TUNI2CRPTSTART)
		{
			continue;
		}
		if (RegisterWrite(nim, (unsigned short)i, ulTemp, DEMOD_I2C_IO) == False) 
			return (False); 
         } 
     } 
  } 
 
  /* For registers that have different default values on Cobra and Camaro, 
     set default values for Camaro */ 
  ulTemp = 0x20; 
  if (RegisterWrite(nim,CX24130_ACQDMDWINDOW,ulTemp, DEMOD_I2C_IO) == False) 
  { 
     return (False); 
  } 
 
  return(True); 
 
}  /* DRIVER_Default_Camaric() */ 

/*******************************************************************************************************/
/* DRIVER_Cobra_compat() */
/*******************************************************************************************************/
BOOL  DRIVER_Cobra_compat(             /* funct returns True if demod attached to nim is Cobra-compatible */
NIM   *nim)                            /* pointer to nim */
{
  /* test for compatiblity */
  switch(nim->demod_type)
  {
    case  CX24130:
    case  CX24121:
    case  CX24123C:
    {
      return(True);
      break;
    }
    default:
    {
      break;
    }
  }  /* switch(... */

  /* demod is not cobra-compatible */
  return(False);

}  /* DRIVER_Cobra_compat() */

/*******************************************************************************************************/
/* DRIVER_Tspec_Get_Auto() */
/*******************************************************************************************************/
TRANSPEC  DRIVER_Tspec_Get_Auto(
NIM *nim)
{
  unsigned long auto_tspec;

  /* read auto tspec -- actual transport spec locked-to, convert to useful */
  if (RegisterRead(nim,CX24123_SYSTRANAUTO,&auto_tspec, DEMOD_I2C_IO) == False)
	return(SPEC_UNDEF);

  switch (auto_tspec)
  {
    case  0x00UL:
    {
      return(SPEC_DVB);
      break;
    }
    case  0x01UL:
    {
      return(SPEC_DSS);
      break;
    }
    case  0x03UL:
    {
      return(SPEC_DCII);
      break;
    }
    default:
    {
      /* this condition may indicate a flakey lock, so next go-around should catch 'real' tspec */
      break;
    }
  }  /* switch(... */
  
  /* current spec is unknown */
  return(SPEC_DVB_DSS);

}  /* DRIVER_Tspec_Get_Auto() */
 
/*******************************************************************************************************/ 
/* DRIVER_Set_Pdmfout() */ 
/*******************************************************************************************************/ 
unsigned long DRIVER_Set_Pdmfout(      /* Function to set the pdmfout register per CPN ICD */ 
NIM           *nim,                    /* pointer to nim */ 
unsigned long symbolrate,              /* symbol rate, ideal */ 
unsigned long samplerate)              /* sample rate */ 
{ 
  unsigned long  pdmfout; 
 
  long  Fsym1; 
  long  Fsym2; 
 
  /* these are log2(K+0.5) boundaries */ 
  static unsigned long boundary[8]={141,283,566,1131,2263,4525,9051,18102}; 
 
  /* validate nim */ 
  DRIVER_VALIDATE_NIM(nim); 
 
  /* calculate a new pdmfout gain setting */ 
  pdmfout = (DRIVER_log2(samplerate/symbolrate)); 
 
  /* test if pdmfout setting rounded-up is a better choice */ 
  Fsym1 = DRIVER_calc_closest_pdmfout(symbolrate, samplerate, boundary[pdmfout]); 
  Fsym2 = DRIVER_calc_closest_pdmfout(symbolrate, samplerate, boundary[pdmfout+1]); 
 
  if (Fsym1 <= 0)  pdmfout += 1; 
  else 
  { 
    pdmfout = DRIVER_calc_pdmf_closer(Fsym1,Fsym2,samplerate,pdmfout); 
  } 
 
  /* set the pdmfout register */ 
  if (nim->pdmfout != pdmfout) 
  { 
	if (nim->symbol_rate_ideal > 4000000UL && pdmfout) 
	{ 
		pdmfout--; 
	} 
	if (RegisterWrite(nim,CX24123_DMDSAMPLEGAIN,pdmfout, DEMOD_I2C_IO) == False)
		return(False);  
	nim->pdmfout = pdmfout; 
  } 
 
  return(pdmfout); 
 
}  /* DRIVER_Set_Pdmfout() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_calc_pdmf_closer() */ 
/*******************************************************************************************************/ 
long           DRIVER_calc_pdmf_closer(     /* Assist pdmfout formula to choose best setting */ 
long           Fsym1,                       /* first setting to analyze */ 
long           Fsym2,                       /* second setting to analyze */ 
unsigned long  samplerate,                  /* sample rate */ 
long           pdin)                        /* present calculated pdmf setting */ 
{ 
  long  t1,t2; 
 
  t1 = samplerate - Fsym1; 
  t2 = samplerate - Fsym2; 
 
  if (t1 < 0 && t2 < 0)
	return((pdin-1) < 0 ? 0 : (pdin-1)); 
 
  if (t1 < t2)
	return(pdin); 
  else 
  { 
    if (t1 > (long)samplerate) return((pdin-1) < 0 ? 0 : (pdin-1)); 
    else 
    { 
      if (t2 > 0)
	return(pdin+1); 
    } 
  } 
   
  return(pdin); 
 
}  /* DRIVER_calc_pdmf_closer() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_calc_closest_pdmfout() */ 
/*******************************************************************************************************/ 
long           DRIVER_calc_closest_pdmfout( /* funct to calculate better pdmfoutgain setting */ 
unsigned long  symbolrate,                  /* symbol rate */ 
unsigned long  samplerate,                  /* sample rate */ 
unsigned long  boundary)                    /* log2(k (aka pdmf reg setting 0..7) + 0.5) */ 
{ 
  long   result; 
  static BCDNO  bcd; 
 
  /* compute symbolrate programmed to the demod: (symbolrate * 2^23) / rounded(samplerate/1000) */ 
  BCD_set(&bcd,symbolrate); 
  BCD_mult(&bcd,boundary); 
  _BCD_div_ten(&bcd); 
  _BCD_div_ten(&bcd); 
  result = BCD_out(&bcd); 
 
  result -= (long)samplerate; 
   
  return(result); 
 
}  /* DRIVER_calc_closest_pdmfout() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_log2() */ 
/*******************************************************************************************************/ 
unsigned long  DRIVER_log2(            /* Function calculates, returns a rounded Log2 integer */ 
unsigned long  x)                      /* */ 
{ 
  unsigned int   i = 0; 
 
  /* returns 0 if exception */ 
  if (x > 0UL) 
  { 
    unsigned long l = 0x01UL; 
 
    for (i = 0 ; i < 32U ; i++) 
    { 
      if ((l|(l-1UL)) > x) 
      { 
        unsigned long  diff = (l < x ? (x - l) : 0UL); 
        unsigned long  rnd = ((diff&(l>>1)) != 0 ? 1UL : 0UL); 
        return(i + rnd); 
      } 
      l <<= 1UL; 
    } 
  } 
 
  return(0UL); 
 
}  /* DRIVER_log2() */ 
 
 
/*******************************************************************************************************/ 
/* DRIVER_SetCTLTrackBW() */ 
/*******************************************************************************************************/ 
BOOL  DRIVER_SetCTLTrackBW(      /* function to optimize CTL Tracking Bandwidth based on symbol rates */ 
NIM            *nim,             /* pointer to nim */ 
unsigned long  symbolrateksps)   /* symbol rate */ 
{ 
   unsigned long  ulRegVal; 
 
   /* optimize CTL Tracking Bandwidth register value based on symbol rates */ 
   ulRegVal = (symbolrateksps <= 2000UL) ? 0UL : 1UL; 
   if (RegisterWrite(nim,CX24130_CTLTRACKBW,ulRegVal, DEMOD_I2C_IO) == False) 
   { 
      return (False); 
   } 
 
   return (True); 
 
}  /* DRIVER_SetCTLTrackBW() */ 

/*******************************************************************************************************/
/* DRIVER_DCIISetACQDmdWin() */
/*******************************************************************************************************/
BOOL  DRIVER_DCIISetACQDmdWin(  /* optimize Demod phase locking expiration window for specific Camaric DCII coderates */
NIM       *nim,                 /* pointer to nim */
CODERATE  coderate)             /* viterbi coderate */
{
   unsigned long ulRegVal;

   /* optimize for Camaric only */
   if (DRIVER_Camaric(nim) == True)
   {
      switch (coderate)
      {
         case CODERATE_1DIV2:
         case CODERATE_5DIV11:
            ulRegVal = 0xFFUL;
            break;
         default:
            ulRegVal = 0x20UL;
            break;
      }

      return (RegisterWrite(nim,CX24130_ACQDMDWINDOW,ulRegVal, DEMOD_I2C_IO));
   }

   return (True);

}  /* DRIVER_DCIISetACQDmdWin() */

#endif  /* #ifdef CAMARIC_FEATURES */
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/* CR 9509 : Add an extra newline */
