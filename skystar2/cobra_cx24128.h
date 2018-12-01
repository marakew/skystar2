/* cobra_cx24128.h */ 
 
#ifndef CX24128_H_DEFINED
#define CX24128_H_DEFINED
 
#define CX24128_DEMOD_HANDLE_REPEATER_SHIFT		16
#define CX24128_COMMON_REG_MAX				0x03
#define VIPER_CHIP_ID					0x43
#define MONGOOSE_CHIP_ID				0x23
#define MONGOOSE_XT_CHIP_ID				0x24
 
#define MONGOOSE_REVB_CHIP_VERSION			0x02 /* -- Mongoose RevB -- */ 
#define MONGOOSEXT_REVC_CHIP_VERSION			0x03 /* XT chip */ 
 
#define CX24128_I2C_ADD1				0x14	/*0xA8 */
#define CX24128_I2C_ADD2				0x54	/*0x28 */
#define CX24128_ICPS					4 
#define CX24128_MAX_GMCBW				44350 
#define CX24128_MIN_GMCBW				2000    /* in kHz */ 
#define CX24128_GMCBW_STEP_SIZE		        	1000    /* in kHz */ 
#define CX24128_DIVIDER				        262144 
#define CX24128_F_0_WINDOW			        3277    /* +/- 0.025 * CX24128_DIVIDER/2 */ 
#define CX24128_F_SIDE_WINDOW				1638    /* +/- 0.0125 * CX24128_DIVIDER/2 */ 
#define CX24113_LO_DIV_BREAKPOINT			1100    /* MHz */  
#define CX24128_LO_DIV_BREAKPOINT			1165    /* MHz */  
#define CX24128_REFSEL_SPI_DIV2				0x01 
#define CX24128_REFSEL_SPI_DIV1				0x00 
#define T1_DEMOD_HIGHEST_BIT				0x100 
#define CX24128_ICP_XTAL_THRESH				(11UL*MM) 
#define CX24128_BS_FREQ_XTAL_THRESH			(40UL*MM) 
#define CX24128_A3_XTAL_FREQ				(40UL*MM) 
 
#define CX24128_TUNER_A_I2C_OFFSET			(0x00) 
#define CX24128_TUNER_B_I2C_OFFSET			(0x20) 
#define CX24128_TUNER_A_BTI_OFFSET			(0x00) 
#define CX24128_TUNER_B_BTI_OFFSET			(0x09) 
 
#define BTI_OPER_MODE_WR				(0x00000000) 
#define BTI_OPER_MODE_RD_ADDR				(0x00800000) 
#define BTI_OPER_MODE_RD_DATA				(0x00A00000) 
 
#define MAX_VCO_SELECTIONS				12
 
/* I2C offset = either 0 or CX24128_TUNER_B_I2C_OFFSET depending on GET_DEMOD_HANDLE_UNIT() */ 
#define GET_CX24128_I2C_REGISTER_OFFSET(handle)		(GET_TUNER_HANDLE_UNIT(handle) * CX24128_TUNER_B_I2C_OFFSET) 
#define GET_CX24128_BTI_REGISTER_OFFSET(handle)		(GET_TUNER_HANDLE_UNIT(handle) * CX24128_TUNER_B_BTI_OFFSET) 
 
#ifndef CX24128_NOPROTO 
/*******************************************************************************************************/ 
/* prototypes */ 
/*******************************************************************************************************/ 
BOOL  TUNER_CX24128_powerup(NIM *p_nim);
void  TUNER_CX24128_initialize(NIM *p_nim);
BOOL  TUNER_CX24128_GetReferenceFreq(NIM *p_nim,RFREQVAL *rfreqvalue);
BOOL  TUNER_CX24128_SetVcoDivider(NIM *p_nim,VCODIV vcodiv);
BOOL  TUNER_CX24128_SetParameters(NIM *p_nim,VIPERPARMS *viperparms);
BOOL  TUNER_CX24128_GetParameters(NIM *p_nim,VIPERPARMS *p_viperparms);
BOOL  TUNER_CX24128_SetFrequency(NIM *p_nim,unsigned long freq);
BOOL  TUNER_CX24128_SetFilterBandwidth(NIM *p_nim, unsigned long  bandwidthkhz);
BOOL  TUNER_CX24128_GetFilterBandwidth(NIM *p_nim,FILTERBW *p_filterbandwidth,unsigned short *p_gmcbandwidth);
BOOL  TUNER_CX24128_GetGainSettings(NIM *p_nim,AMPOUT *p_ampout,VGA1OFFSET *p_vga1offset,VGA2OFFSET *p_vga2offset,RFVGAOFFSET *p_rfvgaoffset);
BOOL  TUNER_CX24128_SetGainSettings(NIM *p_nim, unsigned long  symbolrateksps);
BOOL  TUNER_CX24128_GetClkInversion(NIM *p_nim,BOOL *p_clkinversion);
BOOL  TUNER_CX24128_GetEnableRegister(NIM *p_nim, unsigned char *p_enable);
BOOL  TUNER_CX24128_GetVcoStatus(NIM *p_nim, VCOSTATUS *p_vcostatus);
BOOL  TUNER_CX24128_GetAnalogICPLevel(NIM *p_nim, ICPSELECT *p_icplevel);
BOOL  TUNER_CX24128_GetPLLFrequency(NIM *p_nim,unsigned long *pllfreq);
BOOL  TUNER_CX24128_pll_status(NIM *p_nim,BOOL *locked);
BOOL  TUNER_CX24128_calc_pllNF(NIM *p_nim,unsigned short *nvalue,int *fvalue);
BOOL  TUNER_CX24128_CalculateNFR(NIM *p_nim,unsigned long Fdesired,RDIVVAL R,unsigned int *nvalue,long *fvalue);
BOOL  TUNER_CX24128_SetDemodRepeaterMode(NIM* p_nim);
BOOL  TUNER_CX24128_EnableDemodRepeaterMode(NIM *p_nim);
 
/* low-level I2C operations */ 
BOOL  TUNER_CX24128_RepeaterModeReadData(NIM* p_nim, unsigned long handle, SBaddress address, unsigned char* p_data_read);
BOOL  TUNER_CX24128_RepeaterModeWriteData(NIM* p_nim, unsigned long handle, SBaddress address, unsigned char value);
 
#if INCLUDE_DEBUG 
BOOL  TUNER_CX24128_validate(NIM *p_nim); 
#endif /* INCLUDE_DEBUG */ 
 
#endif  /* #ifndef CX24128_NOPROTO */ 
 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
#endif  /* #ifndef CX24128_H_DEFINED */ 
/* CR 9509 : Add an extra newline */

