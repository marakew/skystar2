/* cobra_str.h */ 
 
#ifndef COBRA_STR_H_DEFINED 
#define COBRA_STR_H_DEFINED 
 
#if INCLUDE_ROSIE 
#define CX24108_NOPROTO                /* include definitions used in the cobra_str.h file */ 
#include "cobra_cx24108.h" 
#undef CX24108_H_DEFINED 
#undef CX24108_NOPROTO 
#endif /* INCLUDE_ROSIE */ 
 
#if INCLUDE_VIPER 
#define CX24128_NOPROTO                /* include definitions used in the cobra_str.h file */ 
#include "cobra_cx24128.h" 
#undef CX24128_H_DEFINED 
#undef CX24128_NOPROTO 
#endif  /* #ifdef INCLUDE_VIPER */ 
 
#if INCLUDE_RATTLER 
#define CX24113_NOPROTO                /* include definitions used in the cobra_str.h file */ 
#include "cobra_cx24113.h" 
#undef CX24113_H_DEFINED 
#undef CX24113_NOPROTO 
#endif  /* #ifdef INCLUDE_RATTLER */ 
 
#if INCLUDE_ROSIE 
/*******************************************************************************************************/ 
/* Tuner specific */ 
/*******************************************************************************************************/ 
typedef struct VCO_EDGE 
{                                      /* vco edges: */ 
  unsigned long  lower;                /*   lower edge of VCO where lock-detect was detected */ 
  unsigned long  upperthresh;          /*   upper edge of VCO where lock-detect was detected */ 
}VCO_EDGE; 
 
/*******************************************************************************************************/ 
/* VCO_LEN */ 
/*******************************************************************************************************/ 
typedef unsigned long VCO_LEN; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
typedef struct VCO_LIST 
{                                      /* full array of VCO edges (used in LabView dll) */ 
  VCO_EDGE vco_edge[CX24108_VCOEDGES]; /* added per CR 6606 */ 
}VCO_LIST; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
typedef struct VCO_BP 
{                                      /* vco breakpoint struct */ 
  unsigned long  percentage;           /*   break-point percentage */ 
  unsigned long  breakpt;              /*   break point computed by: */ 
}VCO_BP;                               /*    (vco_edge.upper[n]+vco_edge[n+1].lower)/2 */ 

/*******************************************************************************************************/
/*******************************************************************************************************/
typedef struct SAMPFRQLST
{                                      /* sample frequency list */
signed int    cntq;                    /*   count of sample-rates gen'd for Qpsk */
  unsigned long  samplerateQ[MAX_QPLL_MULT];   /*   table of sample-rates gen'd for qpsk */
}SAMPFRQLST;

/*******************************************************************************************************/
/*******************************************************************************************************/
typedef struct BINLIST
{                                      /* Bin list */
signed int    bcnt;                    /*   count of bins contained within bins */
  unsigned long  bins[MAX_BINLIST];    /*   table of sample-rates gen'd for qpsk */
}BINLIST;

 
/*******************************************************************************************************/ 
/* Tuner Gain Structures  */ 
/*******************************************************************************************************/ 
typedef struct                         /* tuner slope struct (3 per tuner) see: TUNERPARMS */ 
{                                      /*   [0] = 1..5msps; [1] = 5..15msps; [2] = 15..45msps */ 
   VCASLOPE   VCASlope;                /*   vca slope */ 
   VCAOFFSET  VCAOffset;               /*   vca offset */ 
   VGA1VALS   VGA1Offset;              /*   vga1 offset */ 
   VGA2VALS   VGA2Offset;              /*   vga2 offset */ 
}TUNERSLOPE; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
typedef struct 
{                                      /* Charge-pump currents: */ 
  CPCURRENT  high;                     /*    " high */ 
  CPCURRENT  mid;                      /*    " mid */ 
  CPCURRENT  low;                      /*    " low */ 
 
  unsigned char LowPercentage;         /* Range: CpLow_Range */ 
  unsigned char HighPercentage;        /* Range: CpHigh_Range */ 
}TUNERCP; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
typedef struct TUNERPARMS 
{                                      /* Tuner parameters: */ 
  TUNERSLOPE SLP[CX24108_SLOPES];      /*   slope table */ 
  TUNERCP    CPC[CX24108_CPCS];        /*   charge-pump table per VCO */ 
}TUNERPARMS; 
 
/*******************************************************************************************************/ 
/* Tuner: ROSIE */ 
/*******************************************************************************************************/ 
typedef struct                         /* structure comprising multi-NIMs */ 
{                                      /* Rosie-specifc variables will be stored in this struct */ 
  int       N;                         /*   N register */ 
  int       A;                         /*   A register */ 
  RDIVVAL   R;                         /*   R reference divider */ 
  VCODIV    vcodiv;                    /*   vco divider (2 -or- 4) */ 
  VCONO     vcono;                     /*   vco number (1..8) */ 
  unsigned long     vcofreq;           /*   vco frequency (~2 -or- 4 * Fr) */ 
  VCO_BP    vco_bp[CX24108_BPCNT];     /*   structure holding VCO breakpoints for tuner */ 
  VCO_EDGE  vco_edge[CX24108_VCOEDGES];/*   structure holding VCO upper and lower threshholds */ 
  VCO_LEN   vco_len[CX24108_VCOEDGES]; /*   array holding est. length of associated VCO */ 
  struct    TUNERPARMS tunerparms;     /*   tuner gain parameters */ 
   
  int       device_temp_met;           /* device has met min. elapsed time to op. temp. */ 
 
  /* tuner current settings ****************************************************************************/ 
  int       CPCPolarity;              /*   default CPC polarity=negative */ 
  int       RefDivider;               /*   last reference divider prog'd to tuner */ 
  int       CPCCurrent;               /*   last charge-pump current setting prog'd to tuner */ 
  int       BPPercentage;             /*   last breakpoint percentage setting (often 50%) */ 
  VCASLOPE  VCASlope;                 /*   last vca slope setting */ 
  VCAOFFSET VCAOffset;                /*   last vca offset setting */ 
  VGA1VALS  VGA1Offset;               /*   last vga1 offset setting */ 
  VGA2VALS  VGA2Offset;               /*   last vga2 offset setting */ 
 
  unsigned long ulTunerData_shadow[4];/* shadow of last data sent to the tuner */ 
 
  unsigned long     lsba;             /* value of A register when incr'd in hz */ 
     
  /* tuner (RFU) ***************************************************************************************/ 
  int       testmode;                 /* tuner testmode bit (VCO bit 8) s/b 0) */ 
 
}ROSIE; 
#endif 
 
#if INCLUDE_VIPER 
/* INCLUDE_VIPER specificsection - begin */ 
/*********************************************************************/ 
/* VIPERPARMS */ 
/*********************************************************************/ 
typedef struct ViperParms{ 
    /* ICP parameters. */    
    ICPMODE        ICPmode;               /* ICP auto or manual */            
    ICPMAN         ICPman;                /* Man analog ICP level */ 
    ICPAUTO        ICPauto_Hi;            /* Auto analog ICP levels */ 
    ICPAUTO        ICPauto_MedHi; 
    ICPAUTO        ICPauto_MedLo; 
    ICPAUTO        ICPauto_Lo;       
    ICPDIG         ICPdig;                /* Digital ICP levels */ 
    ICPSELECT      ICPselect;             /* Selected ICP levels */ 
    VCOSTATUS      VCOstatus;             /* vco edge detect status. */ 
    BOOL           ACP_on;                /* Analog CP always on. */ 
    /* VCO parameters. */    
    VCOMODE        VcoMode;               /* VCO auto, manual or table mode */ 
    unsigned char  Bsdelay;               /* Band sel delay value */ 
 
    unsigned short Bsfreqcnt;             /* Band sel freq cnt */ 
    unsigned short Bsrdiv;                /* Band sel ref div value */ 
    VCOBANDSHIFT   Vcobs;                 /* Current vco band shift lo or hi */ 
    VCOBANDSEL     VcobandSel;            /* Current vco number */ 
    /* Others. */ 
    BOOL           PrescalerMode; 
    /* -- Mongoose RevB -- */ 
    BOOL           RFVGABiasControl;      /* Enable/Disable bias control circuit */  
    /* Rattler */ 
    LNA_GAIN       lna_gain;              /* LNA gain */ 
} VIPERPARMS; 
 
/*********************************************************************/ 
/* Tuner: Viper */ 
/*********************************************************************/ 
typedef struct                          
{ /* Viper-specifc variables will be stored in this struct */ 
  unsigned long   tuner_handle; 
  unsigned char   chipid; 
  unsigned char   version; 
  unsigned char   register_offset; 
  IO_METHOD	 io_method; 
  signed char     tuner_gain_thresh; /* shadowed threshold in dBm */ 
  /* SIGNAL_LEVEL_LOW = Use SET1; SIGNAL_LEVEL_HIGH = Use SET2 */ 
  SIGNAL_LEVEL    gain_setting_selection;  
 
  int             N;                  /* N register */ 
  int             F;                  /* Fractonal register */ 
  RDIVVAL   	  R;                  /* R reference divider */ 
 
  RFREQVAL  	rfreqval;           /* reference freq select. */ 
  VCODIV    	vcodiv;             /* vco divider (2 -or- 4) */ 
  FILTERBW  	filterbw;           /* A filter band width. */ 
  unsigned char   gmcbw;              /* gmc filter band width. */   
  AMPOUT    	ampout;             /* Discrete gain control*/ 
 
  VGA1OFFSET     vga1offset;         /* VGA1 offset control*/ 
  VGA2OFFSET     vga2offset;         /* VGA2 offset control*/ 
  RFVGAOFFSET    rfvgaoffset;        /* RF VGA offset control*/ 
 
  VIPERPARMS     viperparms;         /* tuner parameters */ 
 
  BOOL           clkinversion;       /* DSM clock inversion. */ 
  int            refdivider;         /* last R div prog'd */ 
}VIPER; 
/* INCLUDE_VIPER specificsection - end */ 
#endif 
 
/*******************************************************************************************************/ 
/* ACTIVE_TUNER */ 
/* Union of all tuners supported.*/ 
/* Will always hold the active tuner information.*/ 
/*******************************************************************************************************/ 
typedef union tuner_union                          /* storage holding shared memory block for various tuner chips */ 
{ 
#if INCLUDE_ROSIE 
    ROSIE        cx24108;              /* rosie tuner */ 
#endif 
#if INCLUDE_VIPER 
    VIPER        cx24128;              /* viper/mongoose tuner */ 
#endif  /* #if INCLUDE_VIPER */ 
}ACTIVE_TUNER; 

/*******************************************************************************************************/
/* OPTIMAL_FS_CODE Structures */
/*******************************************************************************************************/
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
typedef struct
{
  unsigned long  Fs[MAX_PLL_MULT+1];      /* sample freq */
  unsigned long  fastclk[MAX_PLL_MULT+1]; /* fast clock freq */
  unsigned long  Fvco[MAX_PLL_MULT+1];    /* VCO freq */
  unsigned int   min_PLL_MULT;            /* */
  unsigned int   max_PLL_MULT;            /* */
}OPTIMAL_FS;
#endif  /* #ifdef OPTIMAL_FS_CODE */

/*******************************************************************************************************/ 
/* CHANOBJ -- Channel object */ 
/*******************************************************************************************************/ 
typedef struct CHANOBJ 
{                                      /* Channel object structure passed via ChangeChannel() */ 
/* +0*/	unsigned long	frequency;		/* */
/* +4*/	MODTYPE		modtype;
/* +8*/	CODERATE	coderate;
/* +C*/	SYMBRATE	symbrate;
/*+10*/	SPECINV		specinv;
/*+14*/	SAMPFRQ		samplerate;            /* */
/*+18*/	LNBPOL		lnbpol;
/*+1C*/	LNBTONE		lnbtone;
/*+20*/	unsigned int	viterbicoderates;    /*  Additional viterbi code rates to search. */                                         
/*+24*/	TRANSPEC	transpec;
/*+28*/	unsigned long	unk28;
/*+2C*/	unsigned long	unk2C;
/*+30*/	unsigned long	unk30;
}CHANOBJ; 
 
 
/*******************************************************************************************************/ 
/* ESNO prob and estimation (obsolete) */ 
/*******************************************************************************************************/ 
typedef struct 
{ 
  unsigned long  probTable[MAX_ESNOTABLE];     /* esno estimation table TBD */ 
}ESNO_PROB; 
 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
typedef struct 
{ 
  unsigned long  prob[MAX_ESNOPROB];   /* esno estimation filtering */ 
  ESNO_PROB *ept;                      /* esno (estimation) prob table */ 
}ESNOEST; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
typedef struct ESNO 
{ 
  long       esno;                     /* current esno value */ 
  long       esno_val;                 /* temp value read from hardware */ 
  long       last_esno;                /* last esno value processed */ 
  long       esno_taps[MAX_ESNOTAPS];  /* buffer for esno calc */ 
  int        taps_idx;                 /* index into taps buffer */ 
  int        taps_cnt;                 /* count of items in taps buffer */ 
  const long *table;                   /* pointer to look-up table */ 
}ESNO; 
 
/*******************************************************************************************************/ 
/* IQPAK -- Holds I and Q data from Demod for GUI production of constellation display */ 
/*******************************************************************************************************/ 
typedef struct IQPAK                   /* Constellation data for GUI display */ 
{ 
  int    head;                         /*   head (starting point) of ring buffer (oldest data) */ 
  int    tail;                         /*   tail (ending point) of ring buffer (youngest data) */ 
  int    max;                          /*   count from GUI slider -- also  */ 
  BOOL   busy;                         /*   busy flag -> TRUE if busy (i.e. table being updated) */  
  unsigned char  I[MAX_CONSTLIQ];      /*   I samples (currently 6 bits) */ 
  unsigned char  Q[MAX_CONSTLIQ];      /*   Q samples (currently 6 bits) */ 
}IQPAK; 
 
 
/*******************************************************************************************************/ 
/* CCDATA */ 
/*******************************************************************************************************/ 
typedef struct ccdata                  /* Change-Channel saved data */ 
{ 
  long  cc_ctl;                        /*   saved CTL register */ 
  long  cc_curcentral;                 /*   saved Cur-central register */ 
  long  cc_btl;                        /*   saved BTL */ 
}CCDATA; 
 
/*******************************************************************************************************/ 
/* MPEG_OUT */ 
/*******************************************************************************************************/ 
typedef struct 
{ 
/* +0*/  OUTPUTMODE           OutputMode;            /* Setting for 0x04[7] (MPGDataWidth) */ 
/* +4*/  CLKOUTEDGE           ClkOutEdge;            /* Setting for 0x04[5:4] (MPGClkPos[1:0]) */ 
/* +8*/  CLKPARITYMODE        ClkParityMode;         /* Setting for 0x04[6] (MPGGapClk) */ 
/* +C*/  CLKHOLDTIME          HoldTime;              /* Setting for 0x58[5:4] (ClkHoldTime[1:0]) */ 
/*+10*/  CNTLSIGNALPOL        StartSignalPolarity;   /* Setting for 0x05[2] (MPGStartPol) */ 
/*+14*/  STARTSIGNALWIDTH     StartSignalWidth;      /* Setting for 0x05[3] (MPGStartMode) */ 
/*+18*/  CNTLSIGNALPOL        ValidSignalPolarity;   /* Setting for 0x05[4] (MPGValidPol) */ 
/*+1C*/  CNTLSIGNALMODE       ValidSignalActiveMode; /* Setting for 0x05[5] (MPGValidMode) */ 
/*+20*/  CNTLSIGNALPOL        FailSignalPolarity;    /* Setting for 0x05[6] (MPGFailPol) */ 
/*+24*/  CNTLSIGNALMODE       FailSignalActiveMode;  /* Setting for 0x05[7] (MPGFailMode) */ 
/*+28*/  SYNCPUNCTMODE        SyncPunctMode;         /* Setting for 0x04[0] (MPGSyncPunc) */ 
/*+2C*/  NOSYNCFAILVALUE      FailValueWhenNoSync;   /* Setting for 0x05[0] (MPGFailNSVal) */ 
/*+30*/  CLOCKSMOOTHSEL       ClkSmoothSel;          /* Setting for 0x58[0] (MPGClkSmoothEn) / 0x58[1:0] */ 
/*+34*/  RS_CNTLPIN_SEL       RSCntlPin1Sel;         /* Setting for 0x06[3:2] (MPGCntl1Sel[1:0]) */ 
/*+38*/  RS_CNTLPIN_SEL       RSCntlPin2Sel;         /* Setting for 0x06[1:0] (MPGCntl2Sel[1:0]) */ 
#ifdef CAMARIC_FEATURES
/*+3C*/  RS_CNTLPIN_SEL       RSCntlPin3Sel;         /* Setting for 0x06[5:4] (MPGCntl3Sel[1:0]) */ 
/*+40*/  NULLDATAMODE         NullDataMode;          /* Setting for 0x04[1] (MPGFixNullDataEn) */ 
/*+44*/  NULLDATAVALUE        NullDataValue;         /* Setting for 0x04[2] (MPGNullDataVal) */ 
/*+48*/  VALIDSIGNALWHENFAIL  ValidSignalWhenFail;   /* Setting for 0x55[6] (MPGFailValidDis) */ 
/*+4C*/  STARTSIGNALWHENFAIL  StartSignalWhenFail;   /* Setting for 0x55[5] (MPGFailStartDis) */ 
/*+50*/  PARITY_DATA_SEL      ParityDataSel;         /* Setting for 0x06[7:6] (MPGParSel[1:0]) */ 
#endif  /* #ifdef CAMARIC_FEATURES */
}MPEG_OUT; 
 
 
/*******************************************************************************************************/ 
/* LNBMODE */ 
/*******************************************************************************************************/ 
typedef struct 
{ 
  unsigned int  tone_clock;            /* LNBToneClk (tonefrequency = Fx / (tone_clock * 4) */ 
  unsigned int  cycle_count;           /* LNBBurstLength (Length of tone) (reg: 0x2A[7:3]) */ 
  LNBMODESET    lnb_mode;              /* LNBMode (ToneMode=0x00,EnvMode=0x01,Static0=0x02,Static1=0x03) */ 
}LNBMODE; 
 
 
/*******************************************************************************************************/ 
/* CMPLXNO (ver2.0) */ 
/*******************************************************************************************************/ 
typedef struct CMPLXNO                 /* represents a floating-type number within driver */ 
{                                      /* without using floating pt numbers. For example, */ 
  signed long   integer;               /* Example usage: */ 
  unsigned long divider;               /*  0:0 = 0;  1:0 = Div 0 Err;  1:1 = 1;  -1:1 = -1  */ 
}CMPLXNO;                              /*  double d = ((double)CMPLXNO.integer / (double)CMPLXNO.divider); */ 
 
 
/*******************************************************************************************************/ 
/* SYMBCNT */ 
/*******************************************************************************************************/ 
#ifdef CAMARIC_FEATURES
typedef struct SYMBCNT 
{ 
  unsigned char sce_data[MAX_SCE_SNAPSHOTS][MAX_SCE_RAW_BYTES];  /* raw SCE data */ 
  long          intportion;                   /* integer portion */ 
  long          fracportion;                  /* fraction portion */ 
} SYMBCNT; 
#endif  /* #ifdef CAMARIC_FEATURES */
/*******************************************************************************************************/ 
/* NIM */ 
/*******************************************************************************************************/ 
typedef struct NIM 
{ 
/* +0*/  char           nim_start; 
/* +4*/  DEMOD		 demod_type;           /* type of demod: (see enum DEMOD) */ 
/* +8*/  unsigned long  demod_handle;         /* file-handle-like number passed via InitEnv() and sent */ 
                                       /* to Read/Write demod functions */ 
/* +C*/  unsigned char  register_offset;      /* for a dual demod system, the register address offset. */ 
/*+10*/  unsigned long  version;              /* indicates that i/o can be performed, if chip ver is stored */ 
/*+14*/  int            version_minor;        /* Driver minor version number */ 
   
/*+18(+34)*/  CHANOBJ        chanobj;              /* copy of last chanobj passed through API_ChangeChannel() */ 
/*+4C*/  TRANSPEC       tspec;                /* holds copy of last transport-spec setting */ 
/*+50?*/ unsigned long	unk50;
#if INCLUDE_DEBUG 
/*+54*/  APIERRNO       __errno;              /* NIM error number last processed */ 
/*+58*/  char           *errfname;            /* file name (or reg.map var.name) where err occurred,  */ 
/*+5C*/  long           errline;              /* line number where error was encountered */ 
#endif /* INCLUDE_DEBUG */ 
 
/*+60*/  unsigned long  iostatus;             /* warning number from SBread, SBwrite user functions */ 
     
/*+64*/  IQPAK    	*iqpak;               /* pointer to I,Q constellation data */ 
 
/*+68*/  TUNER    	tuner_type;           /* tuner type: Rosie, Judy,.... */ 
 
  char           *tuner_str;           /* pointer to string containing name of tuner associated with NIM */ 
  unsigned int   vcoedgecnt;           /* counts vco edge detection attempts */ 
 
/*+74*/  unsigned long  freq_ideal;           /* Ideal frequency (Fi) (i.e. freq. entered in GUI) */ 
/*+78*/  unsigned long  pll_frequency;            /* tuned-to (attempted to) frequency */ 
 
  signed int     lsba_adj;             /* count of adjustments made to tuner A reg, to induce a 1mhx offset */ 
                                       /* required to increase performance of 2nd nim at same freq */ 
                                         
  unsigned long  tuner_nar;            /* copy of calulated nar bit-string */ 
  unsigned long  tuner_vco;            /* copy of calc'd vco bit-string */ 

/*+88*/  unsigned long  symbol_rate_ideal;    /* last symbol-rate setting read/written to demod */ 
/*+8C*/  unsigned long  symbol_rate;          /* last symbol-rate setting read/written to demod */ 
/*+90*/  unsigned long  lnboffset;            /* copy of last LNB offset */ 
 
/*+94*/  long           berbusy;              /* flag holding last ber count/status of ber collection */ 
/*+98*/  long           bytebusy;             /* flag holding last byte err count/status */ 
/*+9C*/  long           blockbusy;            /* flag holding last block err count/status */ 
   
/*+A0*/  long           pnberbusy;            /* pnber flag holding las pn ber count/status */ 
/*+A4*/  long           pnberpolarity;        /* PN polarity (current polarity being used) */ 
/*+A8*/  long           pnber_not_rdy_count;  /* PN ber not ready count.  used to flop polarity */ 
/*+AC*/  long           berwindowsize;        /* holds last window size programmed to demod for PN BERReadyCount[21:0] */ 
 
/*+B0*/  int            temp_SyncPunctMode;   /* holds last setting of MPEG->syncpunctmode before PN Ber */ 
 
/*+B4*/  unsigned long  tuner_offset;         /* tuner offset range in Hz. (CR 6243) */ 
/*+B8*/  long           actual_tuner_offset;  /* actual tuner offset in Hz. (CR 6243) */ 
  unsigned long  saved_frequency;      /* saved Fi (freq-ideal) used during SWAssistTuner() */ 
/*+C0*/  int            swa_count;            /* counter used to determine resource piggyness */ 
 
  /* esno **********************************************************************************************/ 
/*+C4(+54)*/ ESNO     	 esno;                 /* esno est samples and table pointer */ 
 
/*+11C(+8)*/  CCDATA   	 ccdata;               /* register settings saved at e.o. change-challel */ 
   
/*+128*/  ACQSTATE 	 prevstate;            /* previous acq state */ 
/*+12C*/  int            prevstatecounter;     /* counter used during API_Monitor() funct */ 
  unsigned long  binsize;              /* */     
  int            numofbins;            /* */ 
  long           centfreqloc;          /* */ 
 
#if INCLUDE_ROSIE 
  TUNERIO_METH	TUNER_io_method;      /* method at which tuner I/O is performed */ 
#endif 
 
  unsigned char  ucTunerIoDefaultSettingsFlag;  /* flag = 1 if tuner I/O default 
                                                   settings have been set once   */ 
/*?*/  ACTIVE_TUNER   tuner;                /* active tuner */ 
/*+1D0*/  unsigned long antialias_bandwidthkhz;/* (CR 7482) last anti-alais BW setting */ 
/*+1D4*/  unsigned long antialias_mV_setting;  /* (CR 7482) last anti-alias BW mV setting */ 
 
  /* pointer to user-defined wait function to be invoked when OS_Wait() is called (if NULL is passed, */ 
/*+1D8*/  BOOL (*wait)(struct NIM *nim,int mscount);/* the default DRIVER_wait() function is used) */ 
 
  /* init. values from InitEnvironment() ***************************************************************/ 
/*+1DC*/  unsigned long   xtal; 
/*+1E0*/  unsigned long   crystal_freq;        /* tuner refclockout */ 
/*+1E4*/  unsigned long   tuner_crystal_freq;  /* demod reference clock */ 
/*+1E8(+50)*/  MPEG_OUT        mpeg_out;            /* nim's internal mpeg struct, hold current settings */ 
 
  /* ser.bus Settings  *********************************************************************************/ 
/*+23C*/  WRITE_SB        SBWrite;             /* ser.bus write() function (pointer to)  */ 
/*+240*/  READ_SB         SBRead;              /* ser.bus read() function (pointer to) */ 

  /* Fs optimization  **********************************************************************************/
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
/*+244*/  unsigned long   opt_Fs_chosen;       /* Fs chosen from available list (or default) */
/*+248()*/  OPTIMAL_FS      opt_fs;              /* table of computed Fs available */
/*+520?*/  SAMPFRQ         samplerate;          /* holds temp Fs used by tuning code (to find optimal Fs) */
/*+524?*/  BOOL            optimal_Fs_set;      /* True if Fs ha been set to optimal setting */
/*+528?*/  unsigned int    opt_Fs_pllmult;      /* optimal pllmult setting chosen */
#endif  /* #ifdef OPTIMAL_FS_CODE */
/*+52C?*/  BOOL            opt_fs_disable;      /* if True, then OPTIMAL_FS_CODE is disabled until driver is re-started */

/*+530*/  CODERATE        CLKSMDIV_CR;         /* last coderate locked-to (used within clksmoothdiv set-up) */ 
/*+534*/  CLOCKSMOOTHDIV  CLKSMDIV_flag;       /* if 1, then actual locked-to coderate may be unknown (so API_Monitor() */ 
                                       /* must re-calc MPEG clkcmoother div value) */ 
#ifdef CAMARIC_FEATURES
/*+53C?*/  SYMBCNT         symbcnt;             /* storage for symbol-count data */ 
/*+54C*/  unsigned long   pdmfout;             /* pdmfout register gain setting */ 
#endif  /* #ifdef CAMARIC_FEATURES */
/*+550*/  unsigned long   sample_freq_less_than_4msps; /* for symbol rate < 4 MSps */ 
/*+554*/  unsigned long   sample_freq_nom_val; 
  /* Shadow register values */ 
/*+558*/  unsigned char   ucPLLMult_shadow;    /* shadow value of PLLMult */ 
#if INCLUDE_BANDWIDTHADJ 
/*+55C*/  unsigned long   anti_alias_bandwidth; 
/*+560*/  BOOL		unk560;
/*+564*/  BOOL            tuner_bw_adjust; 
/*+568*/  void		*ptuner;

//  unsigned long   shadowed_tuner_freq; 
//  unsigned long   shadowed_tuner_lnboffset; 
//  SYMBRATE	    shadowed_tuner_symbrate; 
#endif /* INCLUDE_BANDWIDTHADJ */ 
 
}NIM; 
 
 
/*******************************************************************************************************/ 
/* NIM_LIST */ 
/*******************************************************************************************************/ 
typedef struct 
{ 
  int  nim_cnt;                        /* count of nims in list */ 
  NIM  *nim[MAX_NIMS];                 /* list of NIMs maintained by current invocation of the driver */ 
}NIM_LIST; 
 
 
/*******************************************************************************************************/ 
/* NIM_ERROR -- struct used to store error-decoding and string info */ 
/*******************************************************************************************************/ 
typedef struct NIM_ERROR 
{ 
  APIERRNO   __errno;                  /* error number */ 
  char       *errstr;                  /* associated error string */ 
}NIM_ERROR; 
 
 
/*******************************************************************************************************/ 
/* LOCKIND */ 
/*******************************************************************************************************/ 
typedef struct LOCKIND                 
{                                      /* Lock Indicators:  used to determine LED status on GUI */ 
  BOOL   pll;                          /*   tuner LD gpio pin (indicates tuner pll lock) */ 
  BOOL   demod_pll;                    /*   PLL locked if TRUE */ 
  BOOL   demod;                        /*   DEMOD locked if TRUE */ 
  BOOL   viterbi;                      /*   Viterbi locked if TRUE */ 
  BOOL   reedsolomon;                  /*   reedsolomon (RS) locked if TRUE */ 
  BOOL   descramble;                   /*   descramble locked if TRUE */ 
  BOOL   syncbyte;                     /*   syncbyte locked */ 
}LOCKIND;                              /*   BOOL's not used in B/Q/8 psk will be set to FALSE */ 

/*******************************************************************************************************/ 
/* BCDNO */ 
/*******************************************************************************************************/ 
typedef struct BCDNO 
{                                      /* binary-coded-decimal struct */ 
  signed char  sign[1];                /* size: '-' if negative value */ 
  signed char  digits[MAX_BCDNO+1];    /* storage for bcd number */ 
}BCDNO; 
 
/*******************************************************************************************************/ 
/* VITIST (used cobra_drv.c) */ 
/*******************************************************************************************************/ 
typedef struct VITLIST 
{                                      /* viterbi search list: (allows prioritized search) */ 
  int   vcnt;                          /*   count of items in list */ 
  CODERATE  viterbi_list[MAX_VLIST];   /*   viterbi search list */   
}VITLIST; 
 
/*******************************************************************************************************/ 
/* VERDRV */ 
/*******************************************************************************************************/ 
typedef struct VERDRV 
{ 
  char     version_str[MAX_VERLEN];    /* string containing driver-specific version info */ 
}VERDRV; 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#endif  /* #ifndef COBRA_STR_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

