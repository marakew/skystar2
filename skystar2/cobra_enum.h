/* cobra_enum.h */ 
 
#ifndef COBRA_ENUMS_H_DEFINED 
#define COBRA_ENUMS_H_DEFINED 
 
#include "cobra_defs.h"                  /* TRUE and FALSE from cobra_defs file is needed, so added */ 
 
/*******************************************************************************************************/ 
/* error codes */ 
/*******************************************************************************************************/ 
typedef enum  ApiErrno{                /* standardized method to relate error numbers to err strings */ 
  API_NOERR = 0,                       /*   0 = no error.  (first enum err item must be 0) */ 
  API_BADPTR,                          /*   bad pointer value passed by caller */ 
  API_INVALID_NIM,                     /*   NIM passed was invalid */ 
  API_NIM_OPENED,                      /*   NIM passed was already opened */ 
  API_SBIO_NULL,                       /*   user-supplied SBRead or SBWrite funct is NULL */ 
  API_TIMER_NULL,                      /*   timer function was NULL or invalid */ 
  API_NIM_NULL,                        /*   nim passed by caller is NULL */ 
  API_IO_READERR,                      /*   error encountered at read demod */ 
  API_IO_WRITERR,                      /*   error encountered at write demod */ 
  API_INVALID_TUNER,                   /*   tuner type is not valid (or not supported by driver) */ 
  API_TUNER_PARMS,                     /*   tuner parms passed was NULL */ 
  API_CONST_IQBUF,                     /*   NULL pointer passed at ConstGetPoints() */ 
  API_CONST_IQLOW,                     /*   caller asked for more constl pts than are currently recorded */ 
  API_RANGE,                           /*   Warning: caller wrote register with value out-of-bounds */ 
  API_BAD_PARM,                        /*   bad parameter passed by caller -- see file/line to determine error */ 
  API_PARM_RANGE,                      /*   parm value passed was out of valid range */ 
  API_SETILLEGAL,                      /*   caller asked to set demod into a condition that is illegal. */ 
  API_BAD_RTNVAL,                      /*   unexpected return value received from hardware (register) */ 
  API_LOCKIND_ERR,                         /*   unable to read a lockind at GetLockIndicators() */ 
  API_REG_MATCH_IDX,                       /*   register array is corrupt */ 
  API_REG_HDWR_REWTERR,                    /*   hardware read,mask,write error during write op */ 
  API_REG_MATCH_TRX,                       /*   raw data to external data translation error */ 
  API_REG_MATCH_DTRX,                      /*   external data to raw data translation error */ 
  API_REG_VERFY_IDX,                       /*   register map index does not match linear position */ 
  API_REG_VERFY_ADDR,                      /*   reg.map addr variable is inconsistent with expected range */ 
  API_REG_VERFY_REGRW,                     /*   reg.map regrw field contains invalid data */ 
  API_REG_VERFY_REGFLT,                    /*   reg.map regfilter field contains invalid data */ 
  API_REG_VERFY_REGDTP,                    /*   reg.map regdaattype field contains invalid data */ 
  API_REG_VERFY_DFLT,                      /*   reg.map default value is out of bounds */ 
  API_REG_VERFY_BCNT,                      /*   reg.map bit count/length values are questionable */ 
  API_REG_VERFY_DTLEN,                     /*   regdatatype field/len error (i.e. field is bit, but length > 1bit) */ 
  API_REG_HDWR_REGRDO,                     /*   reg.map:  Write attempted to read-only register */ 
  API_REG_HDWR_REGWTO,                     /*   reg.map:  read attempted to write-only register */ 
  API_INIT_XTAL,                       /*   initenv...() - crystal freq is out-of-bounds */ 
  API_INIT_VCO,                        /*   initenv...() - vcoinit is neither True not False */ 
  API_INIT_MPEG,                       /*   initenv...() - mpeg (default settings) struct is NULL */ 
  API_INIT_TUNER,                      /*   initenv...() - tuner parm passed is invalid */ 
  API_DEMOD_ERR,                       /*   demod register read has invalid setting */ 
  API_VITSETTING,                      /*   demod viterbi setting is zero */ 
  API_ERRBYTE,                         /*   demod is not set to return BYTE error counts */ 
  API_NOTSUPPORT,                      /*   feature not supported by driver */ 
  API_IQ_NULL,                         /*   IQPAK pointer was NULL */ 
  API_INVALID_VCONO,                           /*   invalid vco number selected in driver */ 
  API_TUNERTYPE,                       /*   tuner type is invalid */ 
  API_BAD_BP,                          /*   attempt to set breakpoint percentage with invalid breakpoint% */ 
  API_BAD_CXCTL,                       /*   Invalid setting for tuner control bits (bit2 20, 19) */ 
  API_BAD_CXDATA,                      /*   data to be sent to tuner fails range check */ 
  API_BAD_CXMETH,                      /*   tuner i/o method is invalid see TUNER_io_method setting */ 
  API_TUNERERR,                        /*   tuner -> unable to set tuner to default i/o settings */ 
  API_TUNERIO,                         /*   tuner -> unable to perform i/o to tuner */ 
  API_TUNEREDGE,                       /*   unable to find a tuner edge */ 
  API_BAD_DIV,                         /*   API: Averted div by zero error -- results are undef */ 
  API_BAD_TUNERPARMS,                      /*   tunerparms passed by caller contains invalid settings */ 
  API_VITSET,                          /*   viterbi settings are incorrect */ 
  API_IQ_IO,                           /*   unable to read I, Q pairs from hardware (shared reg.) */ 
  API_CXTYPE,                          /*   unable to determine demod type (CxType) */ 
  API_BAD_SWA,                         /* kir */
  API_BADCXDATABND,                    /*   band data sent to tuner fails data range test */ 
  API_BADCXDATAVGA,                    /*   vga data sent to tuner fails data range test */ 
  API_BADCXDATAVCA,                    /*   vca data sent to tuner fails data range test */ 
  API_BADCXDATAPLL,                    /*   pll data sent to tuner fails data range test */ 
  API_TUNERREF,                        /*   invalid tuner reference divider setting */ 
  API_TUNERVCO,                        /*   No valid VCO exists for the selected frequency */ 
  API_LNB_MSGLEN,                      /*   LNB message less-than 3 bytes in length */ 
  API_LNB_STALLED,                     /*   LNB message could not be sent in time allocated */ 
  API_LNB_BUSY,                        /*   LNB message busy flag is not set (should be set) */ 
  API_DEMOD_REVB_SINGLE,               /*   (invalid use of Rev B SW) */ 
  API_DISEQC_RXLEN,                    /*   Rx buffer passed-into API was too short */ 
  API_DISEQC_VERSION,                  /*   Wrong version to Rx Diseqc Messages */ 
  API_DISEQC_TIMEOUT,                  /*   demod took to long to rx diseqc message */ 
  API_PDMFOUT,                         /*   error setting pdmfout register */ 
  API_BAD_TUNER_FREQ,                  /*   the tuner frequency is outside the LNB search range limit */ 
  API_SCE_DEADLOCK,                    /*   symbol clock estimator deadlocks */ 
  API_DEMOD_UNSUPPORTED,               /*   demod is not supported by the driver */ 
                                       /* --> insert error codes above this point */ 
  API_EOERR,                           /*   this item must be last "valid" error number */ 
  API_NEGONE=0xffff                    /*   (Special OP used in 2115 to get last error encountered */ 
} APIERRNO; 
 
/*******************************************************************************************************/ 
/* Register index */ 
/*******************************************************************************************************/ 
typedef enum RegIdx{                   /* register simplification ID -- this enum list MUST be in EXACT       */ 
                                       /* sequential order as entered in REGISTER struct.  When the driver   */ 
                                       /* is first started, each entry in REGISTER is tested for validity.   */ 
  CX24130_RSTCOBRA=0,                  /* first entry must be set to zero */
  CX24130_RSTCTL,                      
  CX24130_RSTBTL,
  CX24130_RSTDEMOD,
  CX24130_ACQREACQUIRE,
  CX24130_RSTVITERBI,
  CX24130_RSTRS,
  CX24130_SYSPRESET,
  CX24130_SYSRESET,
  CX24130_SYSVERSION ,
  CX24130_PLLMULT,//10
  CX24130_PLLGAIN,
  CX24130_MODULATIONREG,               /* added 9/11/01 */
  CX24130_SYSMODTYPE,
  CX24130_SYSTRANSTD,
  CX24130_DC2MODE,
  CX24130_ACQAUTOACQEN,
  CX24130_MPGDATAWIDTH,
  CX24130_MPGGAPCLK,
  CX24130_MPGCLKPOS,
  CX24130_IDLECLKDIS,//20
  CX24130_MPGSYNCPUNC,
  CX24130_MPGFAILMODE,                 /* added */
  CX24130_MPGFAILPOL,                  /* added */
  CX24130_MPGVALIDMODE,                /* added */
  CX24130_MPGVALIDPOL,                 /* added */
  CX24130_MPGSTARTMODE,
  CX24130_MPGSTARTPOL,
  CX24130_MPGTEIDIS,
  CX24130_MPGFAILNSVAL,
  CX24130_MPGCNTL1SEL,//30
  CX24130_MPGCNTL2SEL,
  CX24130_MPGCNTL3SEL,			/* added kir */
  CX24130_SYSSYMBOLRATE,
  CX24130_ACQPRFREQNOM,
  CX24130_ACQFREQRANGE,
  CX24130_ACQVITSINOM,
  CX24130_DC2MODESEL,
  CX24130_ACQVITCRNOM,
  CX24130_ACQCREN,
  CX24130_ACQSISEARCHDIS,
  CX24130_ACQREPEATCR,
  CX24130_ACQFULLSYNC,
  CX24130_ACQDEINTSYNC,
  CX24130_ACQSYNCBYTESYNC,
  CX24130_ACQVITSYNC,
  CX24130_ACQDMDSYNC,
  CX24130_SYNCSTATUS,
  CX24130_PLLLOCK,
  CX24130_ACQPRFREQCURR,
  CX24130_ACQPRFREQRDSEL,
  CX24130_ESNOSTART,
  CX24130_ESNORDY,
  CX24130_ESNOCOUNT,
  CX24130_ACQVITNORMCOUNT,
  CX24130_ACQVITCURRSI,
  CX24130_ACQVITCURRCR,
  CX24130_BERSTART,
  CX24130_STARTPNBER,
  CX24130_BERREADY,
  CX24130_BERCOUNT_RS,
  CX24130_BERCOUNT,                    /* added */
  CX24130_TUNBURSTBUSY,
  CX24130_TUNBURSTRDY,
  CX24130_TUNBURSTCLKRATE,
  CX24130_TUNDATABIT,
  CX24130_TUNCLKBIT,
  CX24130_TUNENBIT,
  CX24130_TUNPLLLOCK,
  CX24130_TUNBURSTDIS,
  CX24130_TUNBURSTCLKPOL,
  CX24130_TUNBURSTLENGTH,
  CX24130_TUNBURSTDATA,
  CX24130_TUNBTIEN,
  CX24130_TUNBTISTART,
  CX24130_TUNBTIDATA,
  CX24130_FILVALUE,
  CX24130_FILVALUE9_2,			/* added kir */
  CX24130_FILVALUE1_0,			/* added kir */
  CX24130_FILDIS,
  CX24130_FILPOL,
  CX24130_LNBDC,
  CX24130_LNBSENDMSG,
  CX24130_LNBLONGMSG,
  CX24130_LNBTONE,
  CX24130_LNBBURSTMODSEL,
  CX24130_LNBMOREMSG,
  CX24130_LNBMSGLENGTH,
  CX24130_LNBBURSTLENGTH,
  CX24130_LNBDISEQCDIS,
  CX24130_LNBMODE,
  CX24130_LNBTONECLK,
  CX24130_LNBMSG1,
  CX24130_LNBMSG2,
  CX24130_LNBMSG3,
  CX24130_LNBMSG4,
  CX24130_LNBMSG5,
  CX24130_LNBMSG6,
  CX24130_INTRENABLE,                  /* added 5/21 */
  CX24130_INTSYNCEN,
  CX24130_INTACQFAILEN,
  CX24130_INTVITUNLOCKEN,
  CX24130_INTVITLOCKEN,
  CX24130_INTDMDUNLOCKEN,
  CX24130_INTDMDLOCKEN,
  CX24130_INTRPENDING,                 /* added 5/21 */
  CX24130_INTSYNCRD,
  CX24130_INTACQFAILRD,
  CX24130_INTVITUNLOCKRD,
  CX24130_INTVITLOCKRD,
  CX24130_INTDMDUNLOCKRD,
  CX24130_INTDMDLOCKRD,
  CX24130_INTSYNC,
  CX24130_INTACQFAIL,
  CX24130_INTVITUNLOCK,
  CX24130_INTVITLOCK,
  CX24130_INTDMDUNLOCK,
  CX24130_INTDMDLOCK,
  CX24130_DMDACCUMSEL,
  CX24130_DMDSUBACMSEL,                /* added 5/10 */
  CX24130_DMDACCUMVAL,
  CX24130_DMDACCUMRST,
  CX24130_AGCTHRESH,
  CX24130_AGCPOL,
  CX24130_AGCBW,
  CX24130_CTLAFCTHRESH,
  CX24130_CTLINSEL,
  CX24130_CTLAFCGAIN,
  CX24130_CTLACQBW,
  CX24130_CTLTRACKBW,
  CX24130_DMDLDGAIN,
  CX24130_BTLBW,
  CX24130_ESNOTHRESH,
  CX24130_DMDSDTHRESH,
  CX24130_CONSTIQ,                     /* added */
  CX24130_CSTPRTAG,
  CX24130_CSTTAG,
  CX24130_CSTVAL,
  CX24130_ACQVITNORMTHRESH,
  CX24130_ACQVITNORMWIN12,
  CX24130_ACQVITNORMWIN23,
  CX24130_ACQVITNORMWIN34,
  CX24130_ACQVITNORMWIN45,
  CX24130_ACQVITNORMWIN56,
  CX24130_ACQVITNORMWIN67,
  CX24130_ACQVITNORMWIN78,
  CX24130_ACQVITNORMWIN511,
  CX24130_ACQVITNORMWIN35,
  CX24130_RSDERANDEN,
  CX24130_RSFECDIS,
  CX24130_MPGINVSYNCMODE,
  CX24130_ACQRSSYNCTHRESH,
  CX24130_BERRSSELECT,
  CX24130_BERERRORSEL,
  CX24130_BERRSINFWINEN,
  CX24130_BERPNPOL,
  CX24130_BERPNLOCK,
  CX24130_BERPNERRWIN,              /* added */
  CX24130_BERRSERRWIN,
  CX24130_MPGCLKHOLD,               /* name changed from MPGEXTHOLD 2/12/02 per TK */
  CX24130_MPGCLKSMOOTHGAP,
  CX24130_MPGCLKSMOOTHEN,
  CX24130_MPGCLKSMOOTHFREQDIV,
  CX24130_ACQLOCKTHRESH,
  CX24130_ACQUNLOCKTHRESH,
  CX24130_ACQACCCLREN,
  CX24130_ACQAFCWIN,
  CX24130_ACQDMDWINDOW,
  CX24130_ACQVITEXPWIN,
  CX24130_ACQSYNCBYTEWIN,
  CX24130_ACQFULLSYNCWIN,
  CX24130_ACQLOCKMODE,
  CX24130_GPIO3RDVAL,
  CX24130_GPIO2RDVAL,
  CX24130_GPIO1RDVAL,
  CX24130_GPIO0RDVAL,
  CX24130_GPIO3VAL,
  CX24130_GPIO2VAL,
  CX24130_GPIO1VAL,
  CX24130_GPIO0VAL,
  CX24130_GPIO3DIR,
  CX24130_GPIO2DIR,
  CX24130_GPIO1DIR,
  CX24130_GPIO0DIR,
  CX24130_GPIO3SEL,
  CX24130_GPIO2SEL,
  CX24130_GPIO1SEL,
  CX24130_GPIO0SEL,
  CX24130_GPIO4RDVAL,
  CX24130_GPIO4VAL,
  CX24130_GPIO4DIR,
  CX24130_MPGCNTL1_HIZ,
  CX24130_MPGCNTL2_HIZ,
  CX24130_MPGCLKHIZ,
  CX24130_MPGDATA_HIZ,
  CX24130_MPGDATA1_HIZ,
  CX24130_MPGDATA0_HIZ,
  CX24130_DC2CLKDIS,
  CX24130_DC2CLKDIR,
  CX24130_DC2CLKFREQ,
  CX24130_PLLEN,
  CX24130_SYSSLEEP,
  CX24130_INTRSPINSEL,
  CX24130_SYSBOARDVER,

/*  CX24130_RSPARITYDIS, */  /* TR 03/06/02 */
/*  CX24130_LOCK_ZEROEM, */  /* TR 03/06/02 */
  /*******************************************************************************************************/
  /* additional Camaric registers */
  /*******************************************************************************************************/
#ifdef CAMARIC_FEATURES
  CX24123_SYSTRANAUTO,  /* added 11/15/02 */
  CX24123_MPGNULLDATAVAL,
  CX24123_MPGFIXNULLDATAEN,
  CX24123_MPGPARSEL,
  CX24123_MPGCNTL3SEL,
  CX24123_ACQPRFREQNOMMSB,
  CX24123_ACQPRFREQNOMSIGN,
  CX24123_ACQPRFREQNOMLSB,
  CX24123_DMDSAMPLEGAIN,
  CX24123_ACQFREQRANGE,
  CX24123_ACQPRFREQCURRMSB,
  CX24123_ACQPRFREQCURRSIGN,
  CX24123_ACQPRFREQCURRLSB,
  CX24123_TUNI2CRPTEN,
  CX24123_TUNI2CRPTSTART,		/* added kir */
  CX24123_LNBDI2RXSEL,
/*  CX24123_LNBDI2RXBITEXPWIN,	*/	/* remove kir */
  CX24123_LNBSENDMSG,
  CX24123_LNBMSG1,
  CX24123_LNBSMCNTLBITS,
  CX24123_LNBSMECBITS,
  CX24123_LNBSMPOL,
  CX24123_LNBSMDELAY,
  CX24123_LNBDI2RXERRORLOC,
  CX24123_LNBDI2RXLENGTH,
  CX24123_LNBDI2RXERROR,
  CX24123_LNBDI2RXTIMEOUT,
  CX24123_LNBDI2RXAUTORDEN,
  CX24123_LNBDI2RXTAG,
  CX24123_LNBDI2RXEXPWIN,
  CX24123_LNBDCPOL,
  CX24123_LNBDI2EN,
  CX24123_INTLNBMSGRDYEN,
  CX24123_INTLNBMSGRDY,
  CX24123_LNBTONEAMP,
  CX24123_LNBDCODEN,
  CX24123_LNBSMEN,
  CX24123_LNBDI2TONEFREQMSB,
  CX24123_LNBDI2TONEFREQLSB,
/*  CX24123_LNBDI2TONEWIN,*/		/* remove kir */
/*  CX24123_LNBDI2NOTONEWIN,*/		/* remove kir */
/*  CX24123_LNBDI2TONEDETTHRESH,*/	/* remove kir */
  CX24123_DMDSYMVALUE,
  CX24123_DMDSYMUPDATE,
  CX24123_DMDSYMWIN,
  CX24123_DMDSYMREADY,
  CX24123_MPGFAILVALIDDIS,
  CX24123_MPGFAILSTARTDIS,
  CX24123_MPGCLKSMFREQDIVMSB,
  CX24123_MPGCLKSMFREQDIVMID,
  CX24123_MPGCLKSMOOTHSEL,
  CX24123_MPGCLKSMFREQDIVLSB,
  CX24123_DC2CLKSEL,
#endif  /* #ifdef CAMARIC_FEATURES */
                                       /* (blank line) */
  CX24130_REG_COUNT,                   /* count of items in RegIdx list (this item must be 2nd from last) */
  REGID_EOL=0xffff                     /* end-of-list */ 
} REGIDX; 
 
 
/******************************************************************************************* 
 * Serial bus method selection 
 *******************************************************************************************/ 
typedef enum _io_method 
{   
	DEMOD_I2C_IO = 1, 
	VIPER_I2C_IO = 2, 
	VIPER_BTI_IO = 3, 
    	IO_UNKNOWN   = 0 
} IO_METHOD; 
 
/*******************************************************************************************************/ 
/* ACQSTATE */ 
/*******************************************************************************************************/ 
typedef enum AcqState{                 /* acquisition states */ 
  ACQ_OFF=1,                           /*   demod is not trying to acquire a signal */ 
  ACQ_SEARCHING,                       /*   demod has not yet found a signal */ 
  ACQ_LOCKED_AND_TRACKING,             /*   demod is locked to a signal and tracking  */ 
  ACQ_FADE,                            /*   demod is attempting to recover from a fade  */ 
  ACQ_UNDEF=0                          /*   acq lock state is undefined  */ 
} ACQSTATE; 
 
 
/*******************************************************************************************************/ 
/* CODERATE */ 
/*******************************************************************************************************/ 
typedef enum CodeRate{                 /* viterbi code rates */ 
  CODERATE_1DIV2=0x01,                 /* code rate 1/2 */ 
  CODERATE_2DIV3=0x02,                 /*   " 2/3 */ 
  CODERATE_3DIV4=0x04,                 /*   " 3/4 */ 
  CODERATE_4DIV5=0x08,                 /*   " 4/5 */ 
  CODERATE_5DIV6=0x10,                 /*   " 5/6 */ 
  CODERATE_6DIV7=0x20,                 /*   " 6/7 */ 
  CODERATE_7DIV8=0x40,                 /*   " 7/8 */ 
  CODERATE_5DIV11=0x80,                /*   " 5/11 (DCII only) */
  CODERATE_3DIV5=0x100,                /*   " 3/5 (DCII only) */
  CODERATE_NONE=0                      /*   un-initialized code rate (or no cr specified) */ 
} CODERATE; 
 
 
/*******************************************************************************************************/ 
/* DESCRMB */ 
/*******************************************************************************************************/ 
typedef enum Descramble{               /* descramble mode */ 
  DESCRAMBLE_ON=1,                     /*   descramble is ON */ 
  DESCRAMBLE_OFF,                      /*   descramble is OFF */ 
  DESCR_UNDEF=0                        /*   descramble is undefined */ 
} DESCRMB; 
 
 
/*******************************************************************************************************/ 
/* ERRORMODE */ 
/*******************************************************************************************************/ 
typedef enum ErrorMode{                /* Demod Error Modes: */ 
  ERRMODE_VITERBI_BIT=1,               /*   Viterbi bit error rate, without PN seq. */ 
  ERRMODE_RS_BIT,                      /*   RS bit error rate without PN seq. */ 
  ERRMODE_PN_VITERBI_BIT,              /*   Viterbi bit error rate with PN enabled */ 
  ERRMODE_PN_RS_BIT,                   /*   RS bit error rate with PN seq. enabled */ 
  ERRMODE_VITERBI_BYTE,                /*   Viterbi byte error rate */ 
  ERRMODE_RS_BYTE,                     /*   RS decoder byte error rate */ 
  ERRMODE_RS_BLOCK,                    /*   RS block error rate */ 
  ERRMODE_NONE=0                       /*   (demod error mode not set or undefined) */ 
} ERRORMODE; 
 
 
/*******************************************************************************************************/ 
/* INTROPTIONS */ 
/*******************************************************************************************************/ 
typedef enum  IntrOptions{             /* interrupt options */ 
  INTR_LNB_REPLY_READY=0x40,           /*   LNB reply is ready */ 
  INTR_ACQ_SYNC=0x20,                  /*   generate intr on acq sync */ 
  INTR_ACQ_FAILURE=0x10,               /*   generate intr on acq failure */ 
  INTR_VITERBI_LOSS=0x08,              /*   gen intr when viterbi sync is lost */ 
  INTR_VITERBI_SYNC=0x04,              /*   gen intr when viterbi sync detected */ 
  INTR_DEMOD_LOSS=0x02,                /*   get intr when sync is lost */ 
  INTR_DEMOD_SYNC=0x01,                /*   gen intr when sync is achieved */ 
  INTR_CLEAR=0x00                      /*   value used to clear intr options.  INTR_CLEAR  */ 
                                       /*     can also indicate that interrupts are polled)   */ 
} INTROPTIONS;                         /*   also indicates no intr options selected */ 
 
   
/*******************************************************************************************************/ 
/* LNBPOL */ 
/*******************************************************************************************************/ 
typedef enum LnbPolarity{              /* LNB polarity */ 
  LNB_HIGH=1,                          /*   LNB polarity is high (i.e. ~17v) */ 
  LNB_LOW,                             /*   LNB polarity is low (i.e. ~13v) */ 
  LNB_UNDEF=0                          /*   LNB polarity is undefined */ 
} LNBPOL; 
 
 
/*******************************************************************************************************/ 
/* LNBTONE */ 
/*******************************************************************************************************/ 
typedef enum LnbTone{                  /* LNB tone */ 
  LNBTONE_ON=1,                        /*   Tone on */ 
  LNBTONE_OFF,                         /*   Tone off */ 
  LNBTONE_UNDEF=0                      /*   tone undefined */ 
} LNBTONE; 
 
 
/*******************************************************************************************************/ 
/* MODTYPE */ 
/*******************************************************************************************************/ 
typedef enum  ModType{                 /* modulation types: */ 
  MOD_QPSK=1,                          /*   Qpsk */ 
  MOD_BPSK,                            /*   Bpsk */ 
  MOD_QPSK_DCII_MUX,
  MOD_QPSK_DCII_SPLIT_I,
  MOD_QPSK_DCII_SPLIT_Q,
  MOD_UNDEF=0                          /*   modulation type not defined */ 
} MODTYPE; 

/*******************************************************************************************************/
/* SAMPFRQ */
/*******************************************************************************************************/
typedef enum SAMPFRQ{                  /* sample frequency */
  SAMPLE_FREQ_NOM=1,                   /*   nominal (~96mhz 3 x max sample freq of part) */
  SAMPLE_DCII_NOM=2,                   /*   (CR 8126) DCII nominal */
  SAMPLE_FREQ_EXT=999,                 /*   (use customer-specified sample freq programmed to register directly) */
  SAMPLE_FREQ_UNDEF=0                  /*   (end-of-list) */
} SAMPFRQ;

/*******************************************************************************************************/ 
/* SPECINV */ 
/*******************************************************************************************************/ 
typedef enum SpecInv{                  /* spectral inversion */ 
  SPEC_INV_OFF=1,                      /*   uninverted and check only nominal SI    */ 
  SPEC_INV_ON,                         /*   inverted and check only the nominal */ 
  SPEC_INV_ON_BOTH,                    /*   uninverted and check BOTH inversion states */ 
  SPEC_INV_OFF_BOTH,                   /*   inverted and check BOTH inversion states */ 
  SPEC_INV_UNDEF=0                     /*   undefined */ 
} SPECINV; 
 
/*******************************************************************************************************/ 
/* TRANSPEC */ 
/*******************************************************************************************************/ 
typedef enum TransSpec{                /* transport specification */ 
  SPEC_DSS=1,                          /*   Direct-satellite-service */
  SPEC_DVB,                            /*   Direct-video-broadcast */
  SPEC_DCII,                           /*   DCII (mux) (see def SPEC_DCII_MUX) */
#ifdef CAMARIC_FEATURES
  SPEC_DVB_DSS,                        /*   DVB DSS Auto Detection */
#endif  /* #ifdef CAMARIC_FEATURES */
  SPEC_UNDEF=0                         /*   trans-spec undefined */
} TRANSPEC; 
 
 
/******************************************************************************************************/ 
/* TUNER */ 
/*******************************************************************************************************/ 
typedef enum Tuner{                    /* tuner types */ 
  CX24108=1,                           /*   Rosie */ 
  CX24118,                             /*   Mongoose */ 
  CX24128,                             /*   Viper */ 
  CX24113,                             /*   Rattler */ 
  CX24118A,                            /*   Mongoose (XT RevC0) */ 
  CX24113A,                            /*   Rattler (XT RevC0) */ 
  CX24124,                             /*   Mongoose XT */ 
  Tuner_undefined=0                    /*   tuner undefined */ 
} TUNER; 
 
 
/*******************************************************************************************************/ 
/* DEMOD */ 
/*******************************************************************************************************/ 
typedef enum Demod{                    /* demod types supported by this driver */ 
  CX24130=1,                           /*   (aka Cobra) */
  CX24121,                             /*   (Single NIM Cobra) */
  CX24123,                             /*   (Single NIM Camaric. 64-pin with new features) */
  CX24123C,                            /*   (Single NIM Camaric, 80-pin, Cobra compatible) */
  CX24EOL = 0                            /*   Demod undefined */ 
} DEMOD; 
 
 
/*******************************************************************************************************/ 
/* VCODIV */ 
/*******************************************************************************************************/ 
typedef enum Vcodiv{                   /* vco divider */ 
  VCODIV2=2,                           /*   divider=2 */ 
  VCODIV4=4,                           /*   divider=4 */ 
  VCODIV_UNDEF=0                       /*   divider is undefined */ 
} VCODIV; 
 
 
/*******************************************************************************************************/ 
/* VCOSET */ 
/*******************************************************************************************************/ 
typedef enum _vcoset_{ 
  VCO1D2=0,                            /* VCO #1 */ 
  VCO2D2=1,                            /* VCO #2 */ 
  VCO3D2=2,                            /* VCO #3 */ 
  VCO4D2=3,                            /* VCO #4 */ 
  VCO5D2=4,                            /* VCO #5 */ 
  VCO6D2=5,                            /* VCO #6 */ 
  VCO7D2=6,                            /* VCO #7 */ 
  VCO8D2=7,                            /* VCO #8 */ 
  VCO6D4=8,                            /* VCO #9 */ 
  VCO7D4=9,                            /* VCO #10*/ 
  VCO8D4=10                            /* VCO #11*/ 
} VCOSET; 
 
/*******************************************************************************************************/ 
/* CPCPOL */ 
/*******************************************************************************************************/ 
typedef enum cpcpol{                   /* Tuner VCO polarity */ 
  CPC_POLARITY_POS=0,                  /* pos. polarity */ 
  CPC_POLARITY_NEG=1                   /* neg polarity */ 
} CPCPOL; 
 
 
/*******************************************************************************************************/ 
/* MPEG related */ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/* OUTPUTMODE */ 
/*******************************************************************************************************/ 
typedef enum OutputMode{               /* data output mode */ 
  PARALLEL_OUT=1,                      /*   Data is clocked out on DATA[7:0] a byte at a time */ 
  SERIAL_OUT,                          /*   Data is clocked out on DATA0 one bit at a time */ 
  OUTMODE_UNDEF=0                      /*   output mode is undefined */ 
} OUTPUTMODE; 
 
/*******************************************************************************************************/ 
/* CLKOUTEDGE */ 
/*******************************************************************************************************/ 
typedef enum ClkOutEdge{               /* clock edge -- see MPEG_OUT->ClkOutEdge (addr 0x04, bits 5,4) */ 
  CLKOUT_RISING=1,                     /*  (obsolete) data levels change on the rising edge of DATA_CLK */ 
  CLKOUT_FALLING=2,                    /*  (obsolete) data levels change on the falling edge of DATA_CLK */ 
  CLKOUT_SETUP1_HOLD7=0x10,            /*  par: 1 setup time, 7 hold time */ 
  CLKOUT_SETUP3_HOLD5=0x11,            /*  par: 3 setup time, 5 hold time */ 
  CLKOUT_SETUP5_HOLD3=0x12,            /*  par: 5 setup time, 3 hold time */ 
  CLKOUT_SETUP7_HOLD1=0x13,            /*  par: 7 setup time, 1 hold time */ 
  CLKOUT_DATALR_DATACR=0x30,           /*  ser: Zero hold time    -- DATALaunch = 0, DATACapture = 0 */ 
  CLKOUT_DATALR_DATACF=0x32,           /*  ser: Equal set-up/Hold -- DATAL = 1, DATAC = 0 */ 
  CLKOUT_DATALF_DATACR=0x31,           /*  ser: Equal Set-up/Hold -- DATAL = 0, DATAC = 1 */ 
  CLKOUT_DATALF_DATACF=0x41,           /*  ser: Zero hold         -- DATAL = 0, DATAC = 1 */ 
  CLKOUT_ENDEF=0                       /*  (undefined) */ 
} CLKOUTEDGE; 
 
/*******************************************************************************************************/ 
/* CLOCKSMOOTHSEL */ 
/*******************************************************************************************************/ 
typedef enum ClockSmoothSel            /* Clock Smoothing Select */ 
{ 
  CLK_SMOOTHING_UNDEF=0,               /* undefined */ 
  CLK_SMOOTHING_OFF,                   /* clk smoothing is Off */ 
  DDS_LEGACY_SMOOTHING,                /* DDS Legacy smoothing is ON */ 
#ifdef CAMARIC_FEATURES
  PLL_ADVANCED_SMOOTHING               /* PLL Advanced smoothing is ON */ 
#endif  /* #ifdef CAMARIC_FEATURES */
} CLOCKSMOOTHSEL; 
 
/*******************************************************************************************************/ 
/* CLOCKSMOOTHDIV */ 
/*******************************************************************************************************/ 
typedef enum ClockSmoothDiv{            /* specifies is API_Monitor() should recalc clksmooth divider */ 
  CLKSMOOTHDIV_UPDATE=1,                /*   update ClockSmoothDiv in API_Monitor() */  
  CLKSMOOTHDIV_PASS                     /*   DO NOT (further) update ClockSmoothDiv in API_Monitor() */  
}CLOCKSMOOTHDIV; 
 
#if INCLUDE_BANDWIDTHADJ 
/*******************************************************************************************************/ 
/* BANDWIDTHADJ */ 
/*******************************************************************************************************/ 
typedef enum BandwidthAdj{            /* specifies is API_Monitor() should recalc anti-alias filter bandwidth */ 
  BANDWIDTHADJ_UPDATE=1,                /*   update anti-alias filter bandwidth in API_Monitor() */  
  BANDWIDTHADJ_PASS                     /*   DO NOT (further) update anti-alias filter bandwidth in API_Monitor() */  
}BANDWIDTHADJ; 
#endif 
 
/*******************************************************************************************************/ 
/* CLKPARITYMODE */ 
/*******************************************************************************************************/ 
typedef enum ClkParityMode{            /* clock parity:  specifies continuous or punctured during */ 
                                       /* the parity portion of the MPEG payload */ 
  CLK_CONTINUOUS=1,                    /*   Clock is continuous throughout MPEG frame */ 
  CLK_GAPPED,                          /*   Clock is gapped during parity portion of MPEG frame */ 
  CLK_PARITY_UNDEF=0                   /*   clock parity is undefined */ 
} CLKPARITYMODE; 
 
/*******************************************************************************************************/ 
/* CLKHOLDTIME */ 
/*******************************************************************************************************/ 
typedef enum ClkHoldTime{              /* hold time after clocking edge:*/ 
  ZERO_HOLD_TIME=1,                    /*   zero (obsolete) */ 
  MEDIUM_HOLD_TIME,                    /*   Medium */ 
  LARGE_HOLD_TIME,                     /*   Large  */ 
  SMALL_HOLD_TIME,                     /*   Small  */ 
  CLK_HOLD_UNDEF=0                     /*   hold-time is undefined */ 
} CLKHOLDTIME; 
 
/*******************************************************************************************************/ 
/* CNTLSIGNALPOL */ 
/*******************************************************************************************************/ 
typedef enum CntlSignalPol             /* Polarity for MPEG Cntl Signal - Start, Valid, Fail */ 
{ 
   CNTL_SIGNAL_POL_UNDEF = 0,          /* undefined */ 
   ACTIVE_LOW,                         /* Active Low  */ 
   ACTIVE_HIGH                         /* Active High */ 
} CNTLSIGNALPOL; 
 
/*******************************************************************************************************/ 
/* STARTSIGNALWIDTH */ 
/*******************************************************************************************************/ 
typedef enum StartSignalWidth          /* Start Signal Width in Serial mode */ 
{                                      /* Start Signal Width is always byte wide in Parallel mode */ 
   START_SIGNAL_WIDTH_UNDEF = 0,       /* undefined  */ 
   BIT_WIDE,                           /* Bit Wide */ 
   BYTE_WIDE                           /* Byte Wide */ 
} STARTSIGNALWIDTH; 
 
/*******************************************************************************************************/ 
/* STARTSIGNALWIDTH */ 
/*******************************************************************************************************/ 
typedef enum CntlSignalMode            /* MPEG Cntl signal active mode */ 
{ 
   CNTL_SIGNAL_MODE_UNDEF = 0,         /* undefined */ 
   ENTIRE_PACKET,                      /* Cntl signal active during entire data packet */ 
   FIRST_BYTE                          /* Cntl signal active during first byte only */ 
} CNTLSIGNALMODE; 
 
/*******************************************************************************************************/ 
/* SYNCPUNCTMODE */ 
/*******************************************************************************************************/ 
typedef enum SyncPunctMode             /* MPEG Sync Word Puncture Control */ 
{ 
   SYNC_PUNCT_MODE_UNDEF = 0,          /* undefined */ 
   SYNC_WORD_PUNCTURED,                /* Sync word is punctured */ 
   SYNC_WORD_NOT_PUNCTURED             /* Sync word is not punctured */ 
} SYNCPUNCTMODE; 
 
/*******************************************************************************************************/ 
/* NOSYNCFAILVALUE */ 
/*******************************************************************************************************/ 
typedef enum NoSyncFailValue           /* Fail signal value when channel is not in sync */ 
{ 
   FAIL_VALUE_UNDEF = 0,               /* undefined */ 
   FAIL_LOW_WHEN_NO_SYNC,              /* Fail signal is low when channel is not in sync */ 
   FAIL_HIGH_WHEN_NO_SYNC              /* Fail signal is high when channel is not in sync */ 
} NOSYNCFAILVALUE; 
 
/*******************************************************************************************************/ 
/* TSTATE */ 
/*******************************************************************************************************/ 
typedef enum TState{                   /* tstate mpeg setting */ 
  TSTATE_MPEG_OFF=1,                   /*   set MPEG pins to NON-Tstate */ 
  TSTATE_MPEG_ON,                      /*   set MPEG pins to Tstate */ 
  TSTATE_UNDEF=0                       /*   tri-state is in an unknown condition */ 
} TSTATE;              
 
/*******************************************************************************************************/ 
/* RS_CNTLPIN_SEL */ 
/*******************************************************************************************************/ 
typedef enum RSCntlPinSel{               /* MPEG RS_CNTLx GPIO setting... */ 
  RS_CNTLPIN_UNDEF=0,                    /* RS_CNTLx setting is not defined */ 
  RS_CNTLPIN_START,                      /* RS_CNTLx =Start */ 
  RS_CNTLPIN_VALID,                      /* RS_CNTLx =Valid */ 
  RS_CNTLPIN_FAIL,                       /* RS_CNTLx =Fail */ 
  RS_CNTLPIN_INACTIVE,                   /* RS_CNTLx =inactive */ 
  RS_CNTLPIN_ENUM_COUNT                  /* enum item count */ 
} RS_CNTLPIN_SEL; 
 
/*******************************************************************************************************/ 
/* FREQXTALRANGE */ 
/*******************************************************************************************************/ 
typedef enum FreqXtalRange{            /* xtal frequency (in mhz), valid range */ 
  FREQ_XTAL_LOW=5,                     /*   low range 5 megahertz */ 
  FREQ_XTAL_HIGH=41                    /*   high range 20 megahertz */ 
} FREQXTALRANGE; 
 
/*******************************************************************************************************/ 
/* BINPRDRANGE */ 
/*******************************************************************************************************/ 
typedef enum BinPrdRange{              /* bin period (in millions), valid range */ 
  BIN_PRD_LOW=1,                       /*   low range 1 million */ 
  BIN_PRD_HIGH=128                     /*   high range 128 million */ 
} BINPRDRANGE; 
 
/*******************************************************************************************************/ 
/* RATESYMBOLSRANGE */ 
/*******************************************************************************************************/ 
typedef enum RateSymbolsRange{         /* symbol rates (in Ksps), valid range */ 
  SYM_RATE_LOW=10000,                  /*   10000 (x 1000) symbols-per-second */ 
  SYM_RATE_HIGH=32000,                 /*   32000 (x 1000)   " (was 30k, now 32k re:CR4330) */ 
#ifdef CAMARIC_FEATURES
  SYM_RATE_LOW_CAM=1000,               /*   1000 (x 1000) 1MM symbols-per-second */ 
  SYM_RATE_HIGH_CAM=45000,             /*   45000 (x 1000) 45MM   "  */ 
#endif  /* #ifdef CAMARIC_FEATURES */
  SYM_RATE_UNDEF=0                     /*    (not defined) */ 
} RATESYMBOLSRANGE; 
 
/*******************************************************************************************************/ 
/* FULLSYNCWAITRANGE ??? */ 
/*******************************************************************************************************/ 
typedef enum FullSyncWaitRange{        /* full sync wait period range in ms */ 
  SYNC_WAIT_LOW=0,                     /*   0ms minimum */ 
  SYNC_WAIT_HIGH=1000                  /*   1000ms maximum */ 
} FULLSYNCWAITRANGE; 
 
/*******************************************************************************************************/ 
/* BINSIZEMULTRANGE ??? */ 
/*******************************************************************************************************/ 
typedef enum BinSizeMultRange{         /* Bin size multiplier valid range */ 
  BIN_MULT_MIN=1,                      /*   divide by 1 */ 
  BIN_MULT_MAX=32                      /*   divide by 32 */ 
} BINSIZEMULTRANGE; 
 
/*******************************************************************************************************/ 
/* LNBRANGE */ 
/*******************************************************************************************************/ 
typedef enum LnbRange{                 /* LNB max range in kHz */ 
  LNB_RANGE_HIGH_STATIC=5000,          /* max tracking after initial lock */ 
  LNB_RANGE_HIGH=10000                 /* max lnb search (was 12000 pre-CR 6753) */ 
} LNBRANGE; 
 
/*******************************************************************************************************/ 
/* misc enums */ 
/*******************************************************************************************************/ 
 
/*******************************************************************************************************/ 
/* pnber */ 
/*******************************************************************************************************/ 
typedef enum Pnber{                    /* PN BERReadyCount[21:0] eunums */ 
  PNBER_2_22=1,                        /* 2^22 */ 
  PNBER_2_23=2,                        /* 2^23 (Default) */ 
  PNBER_2_25=3,                        /* 2^25 */ 
  PNBER_UNDEF=0                        /* undefined (Default will be used) */ 
} PNBER; 
 
 
/*******************************************************************************************************/ 
/* mstatus */ 
/*******************************************************************************************************/ 
typedef enum Mstatus{                  /* Measurement status (returned with CMPLXNO in PNBER, BERReadyCount[21:0], Byte, Block */ 
  MSTATUS_DONE=1,                      /* measurement is done */ 
  MSTATUS_SAT=2,                       /* measurement is saturated */ 
  MSTATUS_NOTDONE=3,                   /* measurement is not done */ 
  MSTATUS_UNDEF=0                      /* undefined (Default will be used) */ 
} MSTATUS; 
 
/*******************************************************************************************************/ 
/* ESNOMODE */ 
/*******************************************************************************************************/ 
typedef enum Esnomode{                 /* EsNo measurement required */ 
  ESNOMODE_SNAPSHOT=1,                 /* return a single reading to the caller */ 
  ESNOMODE_AVERAGE,                    /* return an average reading to the caller */ 
  ESNOMODE_UNDEF=0                     /* undefined (results in a error) */ 
} ESNOMODE; 
 
 
/*******************************************************************************************************/ 
/* LNBMODE */ 
/*******************************************************************************************************/ 
typedef enum Lnbmodeset{ 
  LNBMODE_TONE=0x00,                   /* Tone Mode (default) */ 
  LNBMODE_ENVELOPE=0x01,               /* Envelope Mode */ 
  LNBMODE_MANUAL_ZERO=0x02,            /* Static '0' (zero) */ 
  LNBMODE_MANUAL_ONE=0x03              /* Static '1' (one) */ 
} LNBMODESET; 
 
 
/*******************************************************************************************************/ 
/* LNBBURST */ 
/*******************************************************************************************************/ 
typedef enum Lnbburst{ 
  LNBBURST_MODULATED=1,                /* tone (at end-of-message) is modulated */ 
  LNBBURST_UNMODULATED,                /* tone ... is not modulated */ 
  LNBBURST_UNDEF=0                     /* undefined (results in an error) */ 
} LNBBURST; 
 
 
/*******************************************************************************************************/ 
/* Cx24123 specific Enums */
/*******************************************************************************************************/ 
#ifdef CAMARIC_FEATURES
/*******************************************************************************************************/ 
/* RXMODE */ 
/*******************************************************************************************************/ 
typedef enum RxMode{ 
  RXMODE_INTERROGATION=1,              /* Demod expects multiple devices attached */ 
  RXMODE_QUICKREPLY,                   /* demod expects 1 rx (rx is suspended after 1st rx received) */ 
  RXMODE_NOREPLY,                      /* demod expects to receive no Rx message(s) */ 
  RXMODE_DEFAULT=0                     /* use current register setting  */ 
} RXMODE; 
 
 
/*******************************************************************************************************/ 
/* DISEQC_VER */ 
/*******************************************************************************************************/ 
typedef enum DiseqcVer{   
  DISEQC_VER_1X=1,                     /* Employs DiseqC version 1.x */ 
  DISEQC_VER_2X,                       /* Employs DiseqC version 2.x */ 
  ECHOSTAR_LEGACY,                     /* Employs Echostar Legacy LNB messaging. */ 
  DISEQC_VER_UNDEF=0                   /* undefined (results in an error) */ 
} DISEQC_VER; 
 
 
/*******************************************************************************************************/ 
/* LNBDRAIN */ 
/*******************************************************************************************************/ 
typedef enum LnbDrain{ 
  LNBDRAIN_OPEN=0x01,                  /* set pin to open-drain */ 
  LNBDRAIN_DIGITAL,                    /* set pin to digital */ 
  LNBDRAIN_UNDEF=0                     /* undefined (results in an error) */ 
} LNBDRAIN; 
 
 
/*******************************************************************************************************/ 
/* ECHOPOLARITY */ 
/*******************************************************************************************************/ 
typedef enum EchoPolarity{ 
  ECHOPOL_RHCP=0x01,                   /* set HCP polarity to RHCP */ 
  ECHOPOL_LHCP,                        /* set HCP polarity to LHCP */ 
  ECHOPOL_DEFAULT,                     /* set HCP polarity to SW default setting in reg.map */ 
  ECHOPOL_UNDEF=0                      /* undefines (results in an error) */ 
} ECHOPOLARITY; 
 
 
/*******************************************************************************************************/ 
/* ECHOWIN */ 
/*******************************************************************************************************/ 
typedef enum EchoWindow{ 
  ECHOWIN_200MS=0x00,                  /* allow register to count down to zero (full 200ms window) */ 
  ECHOWIN_100MS=12                     /* allow reg to count ~1/2 way down (~100ms) */ 
}ECHOWIN; 
 
 
/*******************************************************************************************************/ 
/* NULLDATAMODE */ 
/*******************************************************************************************************/ 
typedef enum NullDataMode              /* MPEG Null Data Mode Control */ 
{ 
  NULL_DATA_MODE_UNDEF = 0,            /* undefined */ 
  FIXED_NULL_DATA_ENABLED,             /* Null data will be modified */ 
  FIXED_NULL_DATA_DISABLED             /* Null data is not modified */ 
} NULLDATAMODE; 
 
 
/*******************************************************************************************************/ 
/* NULLDATAVALUE */ 
/*******************************************************************************************************/ 
typedef enum NullDataValue             /* MPEG Null Data Value */ 
{ 
  NULL_DATA_VALUE_UNDEF = 0,           /* undefined */ 
  FIXED_NULL_DATA_HIGH,                /* Null data will be replaced with 1 */ 
  FIXED_NULL_DATA_LOW                  /* Null data will be replaced with 0 */ 
} NULLDATAVALUE; 
 
/*******************************************************************************************************/ 
/* VALIDSIGNALWHENFAIL */ 
/*******************************************************************************************************/ 
typedef enum ValidSignalWhenFail       /* Valid signal when Fail signal occurs */ 
{ 
  VALID_SIGNAL_WHEN_FAIL_UNDEF = 0,    /* undefined */ 
  VALID_SIGNAL_INACTIVE_WHEN_FAIL,     /* Valid signal becomes inactive when fail signal occurs */ 
  VALID_SIGNAL_ACTIVE_WHEN_FAIL        /* Valid signal stays active when fail signal occurs */ 
} VALIDSIGNALWHENFAIL; 
 
/*******************************************************************************************************/ 
/* STARTSIGNALWHENFAIL */ 
/*******************************************************************************************************/ 
typedef enum StartSignalWhenFail       /* Start signal when Fail signal occurs */ 
{ 
  START_SIGNAL_WHEN_FAIL_UNDEF = 0,    /* undefined */ 
  START_SIGNAL_INACTIVE_WHEN_FAIL,     /* Start signal becomes inactive when fail signal occurs */ 
  START_SIGNAL_ACTIVE_WHEN_FAIL        /* Start signal stays active when fail signal occurs */ 
} STARTSIGNALWHENFAIL; 
 
/*******************************************************************************************************/ 
/* PARITY_DATA_SEL */ 
/*******************************************************************************************************/ 
typedef enum ParityDataSel{           /* Select RS data during parity */ 
  RS_PARITY_DATA_UNDEF = 0,           /* Parity is undefined */ 
  RS_PARITY_DATA_LOW,                 /* RS_Data is 0 during parity */ 
  RS_PARITY_DATA_HIGH,                /* RS_Data is 1 during parity */ 
  RS_PARITY_DATA_UNCHANGED,           /* Parity data will not be altered */ 
  RS_PARITY_DATA_ENUM_COUNT           /* number of enum items */ 
} PARITY_DATA_SEL; 
 
#endif  /* #ifdef CAMARIC_FEATURES */
/*******************************************************************************************************/ 
/* (end-of-Cx2430X specific Enums) */ 
/*******************************************************************************************************/ 
 
/*******************************************************************************************************/ 
/* RDIVVAL */ 
/*******************************************************************************************************/ 
typedef enum RdivVal{                  /* valid reference divider values */ 
#if INCLUDE_VIPER  
  RDIV_1 = 1,                          /*   div by 1, for viper */ 
  RDIV_2 = 2,                          /*   div by 2, for viper */ 
#endif  
#if INCLUDE_ROSIE 
  RDIV_10=10,                          /*   div by 10 */ 
  RDIV_20=20,                          /*   div by 20 */ 
  RDIV_40=40,                          /*   div by 40 */ 
#endif 
  RDIV_UNDEF=0                         /*   undefined (uses default "/10" R value) */ 
} RDIVVAL; 
 
#if INCLUDE_ROSIE 
/*******************************************************************************************************/ 
/* Tuner Gain-related */ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/* VGA1VALS */ 
/*******************************************************************************************************/ 
typedef enum Vga1Vals{                 /* VGA1 Offset settings */ 
  VGA1_27_0DB=0x1fe,                   /*   -27.0dB */ 
  VGA1_28_5DB=0x1fc,                   /*   -28.5dB */ 
  VGA1_30_0DB=0x1f8,                   /*   -30.0dB */ 
  VGA1_31_5DB=0x1f0,                   /*   -31.5dB */ 
  VGA1_33_0DB=0x1e0,                   /*   -33.0dB */ 
  VGA1_34_5DB=0x1c0,                   /*   -34.5dB */ 
  VGA1_36_0DB=0x180,                   /*   -36.0dB */ 
  VGA1_37_5DB=0x100,                   /*   -37.5dB */ 
  VGA1_39_0DB=0x00                     /*   -39.0dB */ 
} VGA1VALS; 
 
/*******************************************************************************************************/ 
/* VGA2VALS */ 
/*******************************************************************************************************/ 
typedef enum Vga2Vals{                 /* VGA2 Offset settings */ 
  VGA2_35DB=0x1fe,                     /*   35dB */ 
  VGA2_32DB=0x1fc,                     /*   32dB */ 
  VGA2_29DB=0x1f8,                     /*   29dB */ 
  VGA2_26DB=0x1f0,                     /*   26dB */ 
  VGA2_23DB=0x1e0,                     /*   23dB */ 
  VGA2_20DB=0x1c0,                     /*   20dB */ 
  VGA2_17DB=0x180,                     /*   17dB */ 
  VGA2_14DB=0x100,                     /*   14dB */ 
  VGA2_11DB=0x00                       /*   11dB */ 
} VGA2VALS; 
 
/*******************************************************************************************************/ 
/* VCASLOPE */ 
/*******************************************************************************************************/ 
typedef enum VcaSlope{                 /* VCA Slope Settings */ 
  VCAS_47_0DB=0x01,                    /*   47.0dB */ 
  VCAS_49_5DB=0x03,                    /*   49.5dB */ 
  VCAS_52_0DB=0x07,                    /*   52.0dB */ 
  VCAS_54_5DB=0x0f,                    /*   54.5dB */ 
  VCAS_57_0DB=0x1f,                    /*   57.0dB */ 
  VCAS_59_5DB=0x3f,                    /*   59.5dB */ 
  VCAS_62_0DB=0x7f,                    /*   62.0dB */ 
  VCAS_64_5DB=0xff,                    /*   64.5dB */ 
  VCAS_67_0DB=0x1ff,                   /*   67.0dB */ 
  VCAS_UNDEF=0                         /*   VCA Slope undefined */ 
} VCASLOPE; 
 
/*******************************************************************************************************/ 
/* VCAOFFSET */ 
/*******************************************************************************************************/ 
typedef enum VcaOffset{                /* VCA Offset Settings */ 
  VCAO_90_0DB=0x01,                    /*   90.0dB */ 
  VCAO_94_25DB=0x03,                   /*   94.25dB */ 
  VCAO_98_5DB=0x07,                    /*   98.5dB */ 
  VCAO_102_75DB=0x0f,                  /*   102.75dB */ 
  VCAO_107_0DB=0x1f,                   /*   107dB */ 
  VCAO_111_25DB=0x3f,                  /*   111.25dB */ 
  VCAO_115_5DB=0x7f,                   /*   115.5dB */ 
  VCAO_119_75DB=0xff,                  /*   119.75dB */ 
  VCAO_124_0DB=0x1ff,                  /*   124.0dB */ 
  VCAO_UNDEF=0                         /*   VCA Offset undefined */ 
} VCAOFFSET; 
 
/*******************************************************************************************************/ 
/* CPCURRENT */ 
/*******************************************************************************************************/ 
typedef enum CpCurrent{                /* Charge Pump current */ 
  CPC_1MA=0x00,                        /*  1ma */ 
  CPC_2MA=0x01,                        /*  2ma */ 
  CPC_3MA=0x02,                        /*  3ma */ 
  CPC_4MA=0x03                         /*  4ma */ 
} CPCURRENT; 
 
/*******************************************************************************************************/ 
/* RDIVBIN ??? */ 
/*******************************************************************************************************/ 
typedef enum Rdivbin{                  /* reference divider values programmed to tuner*/ 
  RDIVBIN_10=0x03,                     /*   div by 10 */ 
  RDIVBIN_20=0x02,                     /*   div by 20 */ 
  RDIVBIN_40=0x01                      /*   div by 40 */ 
} RDIVBIN; 
 
/*******************************************************************************************************/ 
/* Ranges */ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/* FREQTUNERRANGE */ 
/*******************************************************************************************************/ 
typedef enum FreqTunerRange{           /* tuner frequency (in mhz), valid range */ 
  FREQ_TUNE_LOW=950,                   /*   950 mhz (allow searching outside of standard ranges)*/ 
  FREQ_TUNE_HIGH=2150                  /*   2150 mhz ( CR 7396) */ 
} FREQTUNERRANGE; 
 
/*******************************************************************************************************/ 
/* CPLOW_RANGE */ 
/*******************************************************************************************************/ 
typedef enum CpLow_Range{              /* Charge-pump Low Percentage */ 
  CPLOW_RL=0,                          /*   Range low-end */ 
  CPLOW_RH=50                          /*   Range high-end */ 
} CPLOW_RANGE; 
 
/*******************************************************************************************************/ 
/* CPHIGH_RANGE */ 
/*******************************************************************************************************/ 
typedef enum CpHigh_Range{             /* Charge-pump High Percentage */ 
  CPHIGH_RL=51,                        /*   Range low-end */ 
  CPHIGH_RH=100                        /*   Range high-end */ 
} CPHIGH_RANGE; 
 
/*******************************************************************************************************/ 
/* TUNERIO */ 
/*******************************************************************************************************/ 
typedef enum tunerio_meth{             /* tuner i/o communic. method */ 
  TUNER_BURST=1,                       /*   communications performed by byte */ 
  TUNER_MANUAL,                        /*   single bit at a time (sent to tuner) */ 
  TUNERIO_UNDEF=0                      /*   method is undefined (default to TUNER_BURST) */ 
} TUNERIO_METH; 
#endif 
 
#if INCLUDE_VIPER 
/*********************************************************************/ 
/* RFREQVAL */ 
/*********************************************************************/ 
typedef enum RfreqVal{   /* reference frequency selection values */ 
RFREQ_0 = 0,             /* 1 selects 40Mhz, 0 selects 20Mhz */ 
RFREQ_1 = 1, 
RFREQ_UNDEF = 0xFF 
} RFREQVAL; 
 
/*********************************************************************/ 
/* SIGNAL_LEVEL */ 
/*********************************************************************/ 
typedef enum SignalLevel           
{ 
  SIGNAL_LEVEL_LOW = 0,                
  SIGNAL_LEVEL_HIGH = 1,               
  SIGNAL_LEVEL_UNDEF               
} SIGNAL_LEVEL; 
 
 
/*********************************************************************/ 
/* VCOSTATUS */ 
/*********************************************************************/ 
typedef enum VcoStaus{   /* VCO edge detection status */ 
VCO_AUTO_FAIL = 8,            /* Auto tune fails. */ 
VCO_AUTO_DONE = 1,            /* Auto tune succeeds. */ 
VCO_UNDEF = 0 
} VCOSTATUS; 
 
/*********************************************************************/ 
/* VCOMODE */ 
/*********************************************************************/ 
typedef enum VcoMode{   /* VCO edge detection mode values */ 
VCOMODE_AUTO = 0,             /* Auto mode. */ 
VCOMODE_TEST = 1,             /* Manual mode. */ 
VCOMODE_UNDEF = 0xFF 
} VCOMODE; 
 
/*********************************************************************/ 
/* VCOBANDSHIFT */ 
/*********************************************************************/ 
typedef enum VcoBandShift{   /* Current VCO band */ 
VCOBANDSHIFT_HIGH = 0x00,     /* High band select. */ 
VCOBANDSHIFT_LOW = 0x01,      /* Low band select. */ 
VCOBANDSHIFT_UNDEF = 0xFF 
} VCOBANDSHIFT; 
 
/*********************************************************************/ 
/* VCOBANDSEL */ 
/*********************************************************************/ 
typedef enum _VcoBandSel{   /* Current VCO number */ 
  VCOBANDSEL_6 = 0x80,       /* Vco 6 select. */ /* -- Mongoose RevB -- */ 
  VCOBANDSEL_5 = 0x01,       /* Vco 5 select. */ 
  VCOBANDSEL_4 = 0x02,       /* Vco 4 select. */ 
  VCOBANDSEL_3 = 0x04,       /* Vco 3 select. */ 
  VCOBANDSEL_2 = 0x08,       /* Vco 2 select. */ 
  VCOBANDSEL_1 = 0x10,       /* Vco 1 select. */ 
  VCOBANDSEL_UNDEF = 0 
} VCOBANDSEL; 
 
/*********************************************************************/ 
/* FILTERBW */ 
/*********************************************************************/ 
typedef enum FilterBw{   /* Filter Band-width values */ 
FILTERBW_100MHZ = 0x00,        /* Filter A BW 100MHZ. */ 
FILTERBW_65MHZ = 0x01,        /* Filter A BW 65MHZ. */ 
FILTERBW_40MHZ = 0x02,        /* Filter A BW 40MHZ. */     
FILTERBW_35MHZ = 0x03,        /* Filter A BW 35MHZ. */ 
FILTERBW_UNDEF = 0xFF                      
} FILTERBW; 
 
/*********************************************************************/ 
/* ICPMODE */ 
/*********************************************************************/ 
typedef enum IcpMode{   /* ICP mode values */ 
ICPMODE_AUTO   = 0,     /* ICP auto mode. */ 
ICPMODE_MANUAL = 1,     /* ICP manual mode. */ 
ICPMODE_UNDEF  = 0xFF 
} ICPMODE; 
 
/*********************************************************************/ 
/* ICPMAN */ 
/*********************************************************************/ 
typedef enum IcpMan{   /* ICP manual analog levels */ 
ICPMAN_LEVEL1 = 0,            /* 0.5 mA. */ 
ICPMAN_LEVEL2 = 1,            /* 1.0 mA. */ 
ICPMAN_LEVEL3 = 2,            /* 1.5 mA. */ 
ICPMAN_LEVEL4 = 3,            /* 2.0 mA. */ 
ICPMAN_UNDEF = 0xFF 
} ICPMAN; 
 
/*********************************************************************/ 
/* ICPAUTO */ 
/*********************************************************************/ 
typedef enum IcpAuto{   /* ICP auto analog levels */ 
ICPAUTO_LEVEL1 = 0,           /* 0.5 mA. */ 
ICPAUTO_LEVEL2 = 1,           /* 1.0 mA. */ 
ICPAUTO_LEVEL3 = 2,           /* 1.5 mA. */ 
ICPAUTO_LEVEL4 = 3,           /* 2.0 mA. */ 
ICPAUTO_UNDEF = 0xFF 
} ICPAUTO; 
 
/*********************************************************************/ 
/* ICPDIG */ 
/*********************************************************************/ 
typedef enum IcpDig{   /* ICP digital levels */ 
ICPDIG_LEVEL1 = 0,            /* ICP times 0.5 */ 
ICPDIG_LEVEL2 = 1,            /* ICP times 1.0. */ 
ICPDIG_LEVEL3 = 2,            /* ICP times 2.0. */ 
ICPDIG_LEVEL4 = 3,            /* ICP times 3.0. */ 
ICPDIG_UNDEF = 0xFF 
} ICPDIG; 
 
/*********************************************************************/ 
/* ICPSELECT */ 
/*********************************************************************/ 
typedef enum IcpSelect{   /* ICP selected levels */ 
ICPSELECT_LEVEL1 = 8,         /* 0.5 mA. */      
ICPSELECT_LEVEL2 = 1,         /* 1.0 mA. */  
ICPSELECT_LEVEL3 = 2,         /* 1.5 mA. */   
ICPSELECT_LEVEL4 = 3,         /* 2.0 mA. */   
ICPSELECT_UNDEF = 0 
} ICPSELECT; 
 
/*********************************************************************/ 
/* AMPOUT */ 
/*********************************************************************/ 
typedef enum AmpOut{   /* Discrete gain control */ 
AMPOUT_37DB = 0x00,           /* 37DB gain. */ 
AMPOUT_34DB = 0x01,           /* 34DB gain. */ 
AMPOUT_31DB = 0x03,           /* 31DB gain. */ 
AMPOUT_28DB = 0x07,           /* 28DB gain. */  
AMPOUT_25DB = 0x0F,           /* 25DB gain. */ 
AMPOUT_UNDEF = 0xFF                                   
} AMPOUT; 
 
/*********************************************************************/ 
/* VGA1OFFSET */ 
/*********************************************************************/ 
typedef enum Vga1Offset{   /* VGA1 offset control */ 
VGA1OFFSET_0 = 0x00,          /* VGA 1 offset 0. */ 
VGA1OFFSET_1 = 0x04,          /* VGA 1 offset 1. */   
VGA1OFFSET_2 = 0x02,          /* VGA 1 offset 2. */   
VGA1OFFSET_3 = 0x06,          /* VGA 1 offset 3. */   
VGA1OFFSET_4 = 0x01,          /* VGA 1 offset 4. */   
VGA1OFFSET_5 = 0x05,          /* VGA 1 offset 5. */ 
VGA1OFFSET_6 = 0x03,          /* VGA 1 offset 6. */   
VGA1OFFSET_7 = 0x07,          /* VGA 1 offset 7. */ 
VGA1OFFSET_UNDEF = 0xFF 
} VGA1OFFSET; 
 
/*********************************************************************/ 
/* VGA2OFFSET */ 
/*********************************************************************/ 
typedef enum Vga2Offset{   /* VGA2 offset control */ 
VGA2OFFSET_0 = 0x00,          /* VGA 2 offset 0. */ 
VGA2OFFSET_1 = 0x04,          /* VGA 2 offset 1. */   
VGA2OFFSET_2 = 0x02,          /* VGA 2 offset 2. */   
VGA2OFFSET_3 = 0x06,          /* VGA 2 offset 3. */   
VGA2OFFSET_4 = 0x01,          /* VGA 2 offset 4. */   
VGA2OFFSET_5 = 0x05,          /* VGA 2 offset 5. */ 
VGA2OFFSET_6 = 0x03,          /* VGA 2 offset 6. */   
VGA2OFFSET_7 = 0x07,          /* VGA 2 offset 7. */ 
VGA2OFFSET_UNDEF = 0xFF 
} VGA2OFFSET; 
 
/*********************************************************************/ 
/* RFVGAOFFSET */ 
/*********************************************************************/ 
typedef enum RfVgaOffset{   /* RF VGA offset control */ 
RFVGAOFFSET_0 = 0x00,         /* RF VGA offset 0. */  
RFVGAOFFSET_1 = 0x01,         /* RF VGA offset 1. */    
RFVGAOFFSET_2 = 0x02,         /* RF VGA offset 2. */     
RFVGAOFFSET_3 = 0x03,         /* RF VGA offset 3. */  
RFVGAOFFSET_UNDEF = 0xFF 
} RFVGAOFFSET; 
 
/*********************************************************************/ 
/* ENABLEOPTIONS */ 
/*********************************************************************/ 
typedef enum EnableOptions{   /* Viper module enable bits. */ 
DISABLE_ALL  = 0x00,          /* Disable all modules. */ 
ENABLE_RF    = 0x01,          /* Enable RFVGA module. */ 
ENABLE_LNA   = 0x02,          /* Enable FTA LNA module. */ 
ENABLE_SERVO = 0x04,          /* Enable DC offset. */ 
ENABLE_BB    = 0x08,          /* Enable baseband. */ 
ENABLE_PS    = 0x10,          /* Enable prescaler. */ 
ENABLE_CP    = 0x20           /* Enable charge pump current. */ 
}ENABLEOPTIONS; 
 
 
/*********************************************************************/ 
/* FTA LNA GAIN SETTINGS */ 
/*********************************************************************/ 
typedef enum LnaGain{ 
    LNA_MIN_GAIN = 0, /* 5dB */ 
    LNA_MID_GAIN = 1, /* 10dB */ 
    LNA_MAX_GAIN = 2  /* 15dB */ 
}LNA_GAIN; 
 
#endif 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#endif  /* #ifndef COBRA_ENUMS_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

