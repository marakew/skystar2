/* cobra_defs.h */ 
 
#ifndef COBRA_DEFS_H_DEFINED 
#define COBRA_DEFS_H_DEFINED 
 
/*******************************************************************************************************/ 
 
#ifndef NULL 
#define NULL  0 
#endif 
#ifndef CNULL 
#define CNULL  '\0' 
#endif 
 
#ifndef TRUE 
#define TRUE    1                    /* from msdn: The bool type participates in integral promotions.*/ 
#define FALSE   0                    /* ... An r-value of type bool can be converted to an r-value */ 
#endif                               /* ... of type int, with false becoming zero and true becoming one.*/ 
 
#ifndef True 
#define True    TRUE  
#define False   FALSE 
#endif                
 
/*******************************************************************************************************/ 
/* common-use types */ 
/*******************************************************************************************************/ 
#ifndef BOOL 
typedef int     BOOL; 
#endif  /* BOOL */ 
 
/*******************************************************************************************************/ 
/* common-use equivs */ 
/*******************************************************************************************************/ 
typedef int            AGCACC; 
typedef unsigned char  INTEROPTS; 
typedef unsigned long  SYMBRATE; 
typedef unsigned char  VCONO; 
typedef BOOL           VCOINIT; 
 
/*******************************************************************************************************/ 
/* function prototypes passed by application (aka user) */ 
/*******************************************************************************************************/ 
typedef unsigned char   SBaddress; 
typedef unsigned long*  SBstatus; 
typedef unsigned long (*READ_SB)(void *, unsigned long, SBaddress, SBstatus);         /* type pointer to function to read byte to ser.bus */ 
typedef void (*WRITE_SB)(void *, unsigned long, SBaddress, unsigned long, SBstatus);  /* type pointer to function to write byte to ser.bus */ 
 
/*******************************************************************************************************/ 
/* common equates */ 
/*******************************************************************************************************/ 
#define SPEC_DCII_MUX		SPEC_DCII		/* SPEC_DCII and SPEC_DCII_MUX are the same */
#define INTR_POLLING		INTR_CLEAR		/* Interrupt processing will be polled */ 
#define SPEC_INV_BOTH		SPEC_INV_ON_BOTH	/*   equiv to search -inv, failing that, search inv */ 
#define ROSIE_TYPE_STRING	"CX24108/109" 
#define VIPER_TYPE_STRING	"CX24118/128" 
#define RATTLER_TYPE_STRING	"CX24113" 
 
/*******************************************************************************************************/ 
/*                               --- Demod Handle Management ---                                       */ 
/*******************************************************************************************************/ 
#define DEMOD_HANDLE_I2C_ADDR_MASK      0x000000FF  
#define DEMOD_HANDLE_I2C_ADDR_SHIFT     0UL  
#define DEMOD_HANDLE_UNIT_MASK          0x00000100  
#define TUNER_HANDLE_UNIT_MASK          0x00000200  
#define DEMOD_HANDLE_UNIT_SHIFT         8UL  
#define TUNER_HANDLE_UNIT_SHIFT         9UL  
#define DEMOD_HANDLE_CLIENT_MASK        0xFFFF0000  
#define DEMOD_HANDLE_CLIENT_SHIFT       16UL  
 
#define EMOD_HANDLE_RESERVED_MASK      ~(DEMOD_HANDLE_I2C_ADDR_MASK | \
                                                DEMOD_HANDLE_UNIT_MASK     | \
                                                DEMOD_HANDLE_CLIENT_MASK)  
 
/* Access macros allowing position independent extraction and insertion of handle fields */ 
#define GET_DEMOD_HANDLE_I2C_ADDR(handle) \
   (((handle) & DEMOD_HANDLE_I2C_ADDR_MASK) >> DEMOD_HANDLE_I2C_ADDR_SHIFT) 
 
#define GET_DEMOD_HANDLE_CLIENT_INFO(handle) \
   (((handle) & DEMOD_HANDLE_CLIENT_MASK) >> DEMOD_HANDLE_CLIENT_SHIFT) 
        
#define GET_DEMOD_HANDLE_UNIT(handle) \
   (((handle) & DEMOD_HANDLE_UNIT_MASK) >> DEMOD_HANDLE_UNIT_SHIFT) 
 
#define GET_TUNER_HANDLE_UNIT(handle) \
   (((handle) & TUNER_HANDLE_UNIT_MASK) >> TUNER_HANDLE_UNIT_SHIFT) 
           
#define SET_DEMOD_HANDLE_I2C_ADDR(handle, addr)           \
   (handle) = ((((handle) & ~DEMOD_HANDLE_I2C_ADDR_MASK) | \
                (((addr) << DEMOD_HANDLE_I2C_ADDR_SHIFT) & DEMOD_HANDLE_I2C_ADDR_MASK)) & 0xFF)
 
#define SET_DEMOD_HANDLE_CLIENT_INFO(handle, value)       \
   (handle) = (((handle) & ~DEMOD_HANDLE_CLIENT_MASK) |   \
               (((value) << DEMOD_HANDLE_CLIENT_SHIFT) & DEMOD_HANDLE_CLIENT_MASK))
        
#define SET_DEMOD_HANDLE_UNIT(handle, unit)               \
   (handle) = (((handle) & ~DEMOD_HANDLE_UNIT_MASK) |     \
               (((unit) << DEMOD_HANDLE_UNIT_SHIFT) & DEMOD_HANDLE_UNIT_MASK))
 
 
#define CX2430X_DEMOD_A_I2C_OFFSET			(0x00) 
#define CX2430X_DEMOD_B_I2C_OFFSET			(0x80) 
 
/* Obtain the demod's register address offset using the demod handle */ 
#define GET_CX2430X_I2C_REGISTER_OFFSET(handle)		\
				(GET_DEMOD_HANDLE_UNIT(handle) * CX2430X_DEMOD_B_I2C_OFFSET) 

/*******************************************************************************************************/ 
/* common definitions */ 
/*******************************************************************************************************/ 
#define MAX_REGISTER_LEN  4            /* Max length of a multi-byte register field is 4 bytes */ 
#define MAX_VLIST         20           /* max number of Viterbi code rates in list */ 
#define MAX_NIMS          5            /* max number of NIMs driver will handle (default is 2) */ 
#define MAX_ESNOPROB      16           /* max ulongs used to estimate esno */ 
#define MAX_ESNOTABLE     4096         /* max size of esno est./prob table */ 
#define MAX_CONSTLIQ      10000        /* max number of I and Q samples to take for GUI display */ 
#define MAX_COBRA_ADDR    0xff         /* max allowable cobra address for reg.map test */ 
#define MAX_COBRA_NONTEST 0x68         /* max register number (byte-address, NOT regidx) of non-test registers */ 
#define MAX_COBRA_REGLEN  4            /* max length of cobra register is 4 bytes */ 
#define MAX_COBRA_MPEGREG 12           /* number of MPEG-related reg.map registers */ 
#define MAX_BCDNO         18           /* max number of bcd digits in bcdno structure */ 
#define MAX_TUNERSUPPORT  (3+1)        /* max number of tuners supported */ 
#define MAX_TUNER_RETRY   100          /* max attempts to retry tx to demod-attached tuner */ 

#define MAX_NO_BINS       7UL          /* max number of bins */ 
#define MIN_NO_BINS       1UL          /* min number of bins (default setting) */ 
#ifdef CAMARIC_FEATURES
#define MAX_NO_BINS_CAM   127UL        /* max number of bins for Cobra */ 
#endif  /* #ifdef CAMARIC_FEATURES */
 
//#define SYMBOL_RATE_BREAKPOINT		 4000UL 
 
#define MAX_PNBER_NOTRDY  10L          /* max number of PN BERReadyCount[21:0] not-ready cycles until polarity is swapped */ 
#define MAX_VERLEN        25           /* max length of driver-version string */ 
 
#define NIM_DEFAULT_MONRATE    5       /* default NIM monitor rate (in ms) */ 
#define NIM_DEFAULT_XTAL  (10111UL * M)/* default xtal rate */ 
#define NIM_DEFAULT_FREQ  (1200UL*MM)  /* default tuner freq */ 
#define NIM_DEFAULT_SYMB  20000UL      /* default symbol rate */ 
#define NIM_DEFAULT_LNB   10000000UL   /* default search range, user can re-define it.*/ 
#define LNB_OFFSET_LIMIT  5000000UL    /* min. search range limit, it's one of the chip attributes.*/ 
#define MAX_LNBMSG        255          /* max length of LNB message buffer (contained within NIM) */ 
#define MAX_RXBYTES	  8	       /* max number of bytes in receive buffer*/ 
#define MAX_LNBSTAT       10           /* max number of messages able to be held within LNBMSG struct */ 
#define MIN_LNBMSGINTERDELAY  25UL     /* required min. delay between messages */ 
#define MAX_LNBMSGLEN     6            /* max no. of lnb registers contained within the demod */ 
#define MAX_LNBMSGWAIT    5000UL       /* number of loops allowed to send Diseqc message */ 
#define MAX_LNBMSGWAITOSW 50000UL      /* number of loops allowed to send Diseqc message when OS_Wait() is employed*/ 
 
#define MAX_SCE_RAW_BYTES  5           /* number of bytes used to hold SCE data */ 
#define MAX_SCE_SNAPSHOTS  2           /* number of SCE snapshots to calculate SCE */ 
#define MAX_SCE_DEADLOCK   5           /* max number of loops to perform while attemptig to retreive SCE bucket */ 
#define MAX_LNBREAD_LOOP   25          /* max number of loops to perform while awaiting a diseqc rx message */ 
#define MAX_ECHOREAD_LOOP  25          /* max number of loops to perform while attempting to send echo legacy message */ 
 
#define MAX_SWASSIST_HWTIME    20      /* est number of register reads req'd to equal about 50ms */ 
#define MAX_SWASSIST_RDSTROBE  0       /* div rate at which CX2430X_ACQFULLSYNC will be read */ 
 
#define MAX_TUNER_VCOERROR    10UL     /* (VCO edge values below this number, indicate a serious VCO edge-detection error) */ 
#define TUNER_VCOERROR_ZERO   0UL      /* binary-search to find limits of VCO failed */ 
#define TUNER_VCOERROR_ONE    1UL      /* initial detection of VCO mid-pt. failed when detecting low-edge */ 
#define TUNER_VCOERROR_TWO    2UL      /* unable to detect VCO high-edge (starting mid-pt.) because of bad low-edge */ 
#define TUNER_LDWAIT          1        /* number of ms to wait in order to get correct lock detector status */ 

#define DCII_SWACQLOOP        (100)    /* max loops performed when attemting SW lock to DC2 */

#ifdef  DCII_DEFAULT_SMOOTHCLK
#define DCII_HARD_SMOOTHCLK   7        /* DCII MPGCLKSMOOTHEN divider setting */
#endif

/*******************************************************************************************************/
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
#define MAX_PLL_MULT       59          /* max PLLMult settings */
#define MAX_OPT_SAMPLECLK  (100UL*MM)  /* max sample clock rate */
#endif  /* #ifdef OPTIMAL_FS_CODE */
 
/*******************************************************************************************************/ 
#define ESNO_ADJUSTMENT			0            /* standard esno adjustment */ 
#define MAX_ESNOTAPS			16           /* taps for FIR filter */ 
#define MAX_AGC_TABLE_LENGTH		10 
/*******************************************************************************************************/ 
/* default definitions */ 
/*******************************************************************************************************/ 
#define DEFAULT_RDIVVAL   RDIV_10      /* used at API_InitEnvironment() if vcoinit == TRUE */ 
#define MM                1000000UL    /* (one million) easier to read MM */ 
#define M                 1000UL       /* (one thousand) easier to read  */ 
#define BIT_ZERO          0x0UL        /* easier to read that 0 */ 
#define BIT_ONE           0x1UL        /* (ditto) */ 
#define SAMPLE_FREQ_NOM_VAL  99425000UL/* nom value */
#define SAMPLE_DCII_NOM_VAL  96026000UL/* DCII nom value */
 
#define MAX_SAMPFRQ_EQLIST                3 
#define SAMPLE_FREQ_LT_4MSPS_PLL_MULT     16  
#define SAMPLE_FREQ_NOM_VAL_PLL_MULT      59  
 
/* #define SAMPLE_FREQ_LT_2MSPS  13481334UL */ /* for symbol rate < 2 MSps */ 
//#define SAMPLE_FREQ_LT_4MSPS  26666666UL /* for symbol rate < 4 MSps */ 
#define SAMPLE_FREQ_ENDLIST  0UL       /* end-of-list indicator */ 
#define LNB_DYN_SEARCH_RANGE 2500000UL /* LNB dynamic search range */ 
#define LNB_DYN_SEARCH_ACQ_TIME  10    /* LNB dynamic search max acq time (delay based upon i2c tx time) */ 
#define LNB_DYN_SEARCH_ACQ_DELAY 2     /* LNB dynamic search min delay before retry (share resources!) */ 
#define LNB_DYN_TRACKING_POLLCNT 5     /* LNB dynamic tracking loop counter (CR 7508) */ 
 
#define PLL_LOCK_ABORT_TIME          10  /* (CR 7581) */ 
#define VIPER_LOCK_TEST_WAIT_TIME    1   
#define TRACKING_STATE_POLLING_TIME  80  /* (CR 7581) */ 
 
#define VITNORMTHRESH_1DIV2    (164UL)   /* Viterbi Normalization Threshold optimized for coderate 1/2 */ 
#define VITNORMTHRESH_2DIV3    (128UL)   /* Viterbi Normalization Threshold optimized for coderate 2/3 */ 
#define VITNORMTHRESH_DEFAULT  (126UL)   /* Viterbi Normalization Threshold optimized for other coderates */ 
 
/*******************************************************************************************************/ 
/* Macros (to keep backwards compat.) */ 
/*******************************************************************************************************/ 
#define API_Error_filename(n) (API_GetErrorFilename(n)) 
#define API_Error_errline(n)  (API_GetErrorLineNumber(n)) 
 
#if INCLUDE_RATTLER 
/*******************************************************************************************************/ 
/* Marcos for CX24113 (CR 29373)   */ 
/*******************************************************************************************************/ 
#define CX24113_PRELOAD_VALUE_FREQ_THRESH1_KHZ      950000000UL     
#define CX24113_PRELOAD_VALUE_FREQ_THRESH2_KHZ      1100000000UL 
#define CX24113_PRELOAD_VALUE_FREQ_THRESH3_KHZ      1800000000UL 
#define CX24113_PRELOAD_VALUE_FREQ_THRESH4_KHZ      2150000000UL 
#endif 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#endif  /* #ifndef COBRA_DEFS_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

