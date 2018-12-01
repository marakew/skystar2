/* cobra_regs.h */ 
 
#ifndef COBRA_REGS_H_DEFINED 
#define COBRA_REGS_H_DEFINED 
 
/*******************************************************************************************************/ 
/* Include Cobra internal */ 
/*******************************************************************************************************/ 
#include "cobra_gbls.h"                /* Cobra Globals */ 
 
 
#ifdef __cplusplus 
extern "C" { 
#else 
#endif  /* __cplusplus */ 
 
/*******************************************************************************************************/ 
/* Register-specific Enums */ 
/*******************************************************************************************************/ 
enum RegDataType{                      /* all possible types of register data must be defined */ 
  REGT_BIT=1,                          /* bit (most common type) */ 
  REGT_BYTE,                           /* byte-wide register */ 
  REGT_MINT,                           /* int with n-bit msb in first register, remaining bits in next reg(s) */ 
  REGT_INT,                            /* int type */ 
  REGT_UINT,                           /* unsigned int register type */ 
  REGT_LONG,                           /* standard long */ 
  REGT_ULONG,                          /* unsigned long */ 

  REGT_BITHL,                          /* special-purpose: bit is set high, then immediatly set low */ 
  REGT_NULL,                           /* byte-wide register */ 
  REGT_EOLIST=0                        /* end-of-list indicator */ 
}; 
 
/*******************************************************************************************************/ 
/* REGRW */ 
/*******************************************************************************************************/ 
enum RegRW{                            /* register access controls */ 
  REG_RW=1,                            /* register has full read/write capabilities */ 
  REG_RO=2,                            /* register is read-only */ 
  REG_WO=3,                            /* register is write-only */ 
  REG_UNUSED=0                         /* register is unused (also EOL indicator) */ 
}; 
 
 
/*******************************************************************************************************/ 
/* REGFILTER (upgraded to Cobra) */ 
/*******************************************************************************************************/ 
#define REGF  unsigned int 
typedef enum RegFilter{
  REGF_COBRA=0x01,                     /* register item is part of Cobra-class */ 
  REGF_CAM_DEF=0x02,                   /* register is used within camaric-only default settings */ 
  REGF_CAM_EXT=0x04,                   /* Register is an extended register (to acomodate Cobra) */ 
  REGF_CAM_ONLY=0x08,                  /* Register is a Cobra-only register */ 
  REGF_CAM_RED=0x10,                   /* Register is reduced to accomodate Cobra (none id'd yet) */ 
  REGF_VIPER=0x20,                     /* Viper/Mongoose register */ 
  REGF_ZEROB6=0x80,                    /* zero bit 6 before write (accomodate register 0x29) */ 
  REGF_NONE=0                          /* no particular filter attached to a register item */ 
}_REGF; 
 
/*******************************************************************************************************/ 
/* REGISTER Struct */ 
/*******************************************************************************************************/ 
typedef struct                  /* REGISTER.  This struct will be used to control access to */ 
{                               /* register mapped items in the various demods */ 
  char    *regname;             /*   asciz name of register */ 
  unsigned short bit_field;     /*   register simplification ID */ 
  unsigned char address;        /*   address from register map (hex) */ 
  unsigned char start_bit;      /*   starting pt. of bits in addr */ 
  unsigned char bit_count;      /*   length of reg item in bits */ 
  enum   RegRW  access_level;   /*   Indicates if register HW can be read/written */ 
  REGF   regfilter;             /*   Additional layer of filterization */ 
  enum   RegDataType reg_type;  /*   type of data held in register(s): */ 
                                /*     ...  BIT, COMP2, COMP1, FLOAT, UINT, INT ,....*/ 
  unsigned long  default_value; /*   holds default value (for reset) to write to the register */ 
  char   *p_hw_mask;            /*   holds hardware mask string -- 1st byte of mask is first reg addr */ 
}REGISTER_MAP; 
 
 
/*******************************************************************************************************/ 
/* external reference to Register array in cobra_reg.c */ 
/*******************************************************************************************************/ 
extern const REGISTER_MAP demod_register_map[]; 
 
#if INCLUDE_VIPER 
/********************************************************************************** 
 * Viper tuner register address enumerations  
 **********************************************************************************/ 
typedef enum viper_register_address { 
    CX24128_CHIP_ID_00 = 0x00, 
    CX24128_CHIP_VERSION_01 = 0x01, 
    CX24128_XTAL_02 = 0x02, 
    CX24128_TEST_MUX_SEL_03 = 0x03, 
    CX24128_LO_TEST = 0x10, 
    CX24128_LO_CTL1 = 0x11, 
    CX24128_LO_CTL2 = 0x12, 
    CX24128_BAND_SEL2_14 = 0x14, 
    CX24128_BAND_SEL3_15 = 0x15, 
    CX24128_BAND_SEL4_16 = 0x16, 
    CX24128_BAND_SEL5_17 = 0x17, 
    CX24128_VCO_18 = 0x18, 
    CX24128_LO1_19 = 0x19, 
    CX24128_LO2_1A = 0x1A, 
    CX24128_LO3_1B = 0x1B, 
    CX24128_LO4_1C = 0x1C, 
    CX24128_BB1_1D = 0x1D, 
    CX24128_BB2_1E = 0x1E, 
    CX24128_BB3_1F = 0x1F, 
    CX24128_RF_20 = 0x20, 
    CX24128_ENABLE_21 = 0x21 
}VIPER_REGISTER_ADDRESS; 
 
/********************************************************************************** 
 * Viper tuner register bit-field enumerations 
 **********************************************************************************/ 
typedef enum viper_register_bit_field {   
    /* T1 */ 
/*0 */   CX24128_DSM_CLK,                       /* Address 0x10[7], RW */ 
/*1 */   CX24128_PS_TEST,                       /* Address 0x10[6], RW */     
/*2 */   CX24128_ICP_MAN,                       /* Address 0x10[5:4], RW */  
/*3 */   CX24128_IDIG_SEL,                      /* Address 0x10[3:2], RW */ 
/*4 */   CX24128_LOCK_DET,                      /* Address 0x10[1], RO */ 
/*5 */   CX24128_MOR,                           /* Address 0x10[0], RW */ 
/*6 */   CX24128_ICP_LEVEL,                     /* Address 0x11[7:0], RW */ 
/*7 */   CX24128_BS_DELAY,                      /* Address 0x12[7:4], RW */ 
/*8 */   CX24128_ACP_ON_ALW,                    /* Address 0x12[2], RW */ 
/*9 */   CX24128_ICP_SEL,                       /* Address 0x12[1:0], RO */ 
 
/*A */   CX24128_BS_VCOMT,                      /* Address 0x14[7:6], ISEL */ 
/*B */   CX24128_BS_FREQ,                       /* Address 0x14[4:0] and 0x15[7:0], RW */ 
/*C */   CX24128_BS_DIV_CNT,                    /* Address 0x16[7:0] and 0x17[7:4], RW */ 
#if 0	/*kir*/
/*D */   CX24128_BS_???,			   /* Address 0x17[1], RO */
#endif
/*E */   CX24128_BS_ERR,                        /* Address 0x17[0], RO */ 
/*F */   CX24128_DIV24_SEL,                     /* Address 0x18[6], RW */ 
/*10*/    CX24128_VCO_SEL_SPI,                   /* Address 0x18[5:1], RW */ 
 
    /* -- Mongoose RevB -- */ 
/*11*/    CX24128_VCO6_SEL_SPI,                  /* Address 0x18[7], RW */ 
/*12*/    CX24128_VCO_BSH_SPI,                   /* Address 0x18[0], RW */ 
/*13*/    CX24128_INT,                           /* Address 0x19[7:0] and 0x1A[7], RW */ 
/*14*/    CX24128_FRACTN,                        /* Address 0x1A[6:0] and 0x1B[7:0] and 0x1C[7:5], RW */ 
/*15*/    CX24128_SYS_RESET,                     /* Address 0x1C[4], WO */ 
/*16*/    CX24128_AMP_OUT,                       /* Address 0x1D[3:0], RW */ 
/*17*/    CX24128_FILTER_BW,                     /* Address 0x1E[7:6], RW */ 
/*18*/    CX24128_GMC_BW,                        /* Address 0x1E[5:0], RW */ 
/*19*/    CX24128_VGA2_OFFSET,                   /* Address 0x1F[5:3], RW */ 
/*1A*/    CX24128_VGA1_OFFSET,                   /* Address 0x1F[2:0], RW */ 
 
    /* -- Mongoose RevB -- */ 
/*1B*/    CX24128_RFBC_DISABLE,                  /* Address 0x20[4],   RW */ 
/*1C*/    CX24128_RF_OFFSET,                     /* Address 0x20[3:2], RW */ 
/*1D*/    CX24128_LNA_GC,                        /* Address 0x20[1:0], RW */ 
/*1E*/    CX24128_EN,                            /* Address 0x21[5:0], RW */ 
/*1F*/    CX24128_LNA_EN,                        /* Address 0x21[1], RW */ 
 
    VIPER_REGISTER_BITFIELD_INVALID=0xFFFF 
}VIPER_REGISTER_BIT_FIELD; 
 
 
/********************************************************************************** 
 * Viper tuner BTI register address enumerations  
 **********************************************************************************/ 
typedef enum viper_bti_register_address { 
	/* global */ 
	BTI_CX24128_CHIP_ID_AND_VERSION = 0x00, 
	BTI_CX24128_XTAL = 0x01, 
	BTI_CX24128_TEST_MUX_SEL = 0x01, 

	/* tuner */ 
	BTI_CX24128_LO_TEST = 0x02, 
	BTI_CX24128_LO_CTL1 = 0x02, 
 
	BTI_CX24128_LO_CTL2 = 0x03, 
 
	BTI_CX24128_BAND_SEL2 = 0x04, 
	BTI_CX24128_BAND_SEL3 = 0x04, 
 
	BTI_CX24128_BAND_SEL4 = 0x05, 
	BTI_CX24128_BAND_SEL5 = 0x05, 
 
	BTI_CX24128_VCO = 0x06, 
	BTI_CX24128_LO1 = 0x06, 
 
	BTI_CX24128_LO2 = 0x07, 
	BTI_CX24128_LO3 = 0x07, 
 
	BTI_CX24128_LO4 = 0x08, 
	BTI_CX24128_BB1 = 0x08, 
 
	BTI_CX24128_BB2 = 0x09, 
	BTI_CX24128_BB3 = 0x09, 
 
	BTI_CX24128_RF = 0x0A, 
	BTI_CX24128_ENABLE = 0x0A 
}VIPER_BTI_REGISTER_ADDRESS; 
 
/********************************************************************************** 
 * Viper tuner BTI register bit-field enumerations 
 **********************************************************************************/ 
typedef enum bti_register_bit_field {   
    /* T1 */ 
    BTI_CX24128_DSM_CLK,                       /* Address 0x02[15], RW */ 
    BTI_CX24128_PS_TEST,                       /* Address 0x02[14], RW */     
    BTI_CX24128_ICP_MAN,                       /* Address 0x02[13:12], RW */  
    BTI_CX24128_IDIG_SEL,                      /* Address 0x02[11:10], RW */ 
    BTI_CX24128_LOCK_DET,                      /* Address 0x02[9], RO */ 
    BTI_CX24128_MOR,                           /* Address 0x02[8], RW */ 
    BTI_CX24128_ICP_LEVEL,                     /* Address 0x02[7:0], RW */ 
 
    BTI_CX24128_BS_DELAY,                      /* Address 0x03[15:12], RW */ 
    BTI_CX24128_ACP_ON_ALW,                    /* Address 0x03[10], RW */ 
    BTI_CX24128_ICP_SEL,                       /* Address 0x03[9:8], RO */ 
 
    BTI_CX24128_BS_VCOMT,                      /* Address 0x04[15:14], ISEL */ 
    BTI_CX24128_BS_FREQ,                       /* Address 0x04[12:8] and 0x04[7:0], RW */ 
 
    BTI_CX24128_BS_DIV_CNT,                    /* Address 0x05[15:8] and 0x05[7:4], RW */ 
#if 0	/*kir*/
    CX24128_BS_???,			   /* Address 0x17[1], RO ???????*/
#endif
    BTI_CX24128_BS_ERR,                        /* Address 0x05[0], RO */ 
 
    BTI_CX24128_DIV24_SEL,                     /* Address 0x06[14], RW */ 
    BTI_CX24128_VCO_SEL_SPI,                   /* Address 0x06[13:9], RW */ 
    BTI_CX24128_VCO6_SEL_SPI,                  /* Address 0x06[15], RW */ 
    BTI_CX24128_VCO_BSH_SPI,                   /* Address 0x06[8], RW */ 
    BTI_CX24128_INT,                           /* Address 0x06[7:0] and 0x07[15], RW */ 
 
    BTI_CX24128_FRACTN,                        /* Address 0x07[14:8] and 0x07[7:0] and 0x08[15:13], RW */ 
 
    BTI_CX24128_SYS_RESET,                     /* Address 0x08[12], WO */ 
    BTI_CX24128_AMP_OUT,                       /* Address 0x08[11:8], RW */ 
 
    BTI_CX24128_FILTER_BW,                     /* Address 0x09[15:14], RW */ 
    BTI_CX24128_GMC_BW,                        /* Address 0x09[13:8], RW */ 
    BTI_CX24128_VGA2_OFFSET,                   /* Address 0x09[5:3], RW */ 
    BTI_CX24128_VGA1_OFFSET,                   /* Address 0x09[2:0], RW */ 
 
    BTI_CX24128_RFBC_DISABLE,                  /* Address 0x0A[12],   RW */ 
    BTI_CX24128_RF_OFFSET,                     /* Address 0x0A[11:10], RW */ 
    BTI_CX24128_LNA_GC,                        /* Address 0x0A[9:8], RW */ 
    BTI_CX24128_EN,                            /* Address 0x0A[5:0], RW */ 
    BTI_CX24128_LNA_EN                         /* Address 0x0A[1], RW */ 
}VIPER_BTI_REGISTER_BIT_FIELD; 
 
extern const REGISTER_MAP viper_register_map[]; 
 
#endif 
 
#if INCLUDE_RATTLER 
BOOL  
TunerRegisterWrite(NIM*	p_nim, 
			unsigned short	reg_field_index,  
			unsigned long	value, 
			IO_METHOD	io_method); 
BOOL  
TunerRegisterRead(NIM*	p_nim, 
			unsigned short	reg_field_index,  
			unsigned long*	p_value, 
			IO_METHOD	io_method); 
#endif 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#ifdef __cplusplus 
} 
#endif 
 
#endif  /* #ifndef COBRA_REGS_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

