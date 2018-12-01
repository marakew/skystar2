/* cobra_cx24128.c */ 
 
#include "cobra.h"                     /* Cobra include files, ordered */ 
 
#if INCLUDE_VIPER 
 
extern const REGISTER_MAP viper_register_map[]; 
 
/* Variables visible to this file only */ 
BOOL CX24128_refresh_tuner_pll_lock;  /* Shadowing flag */ 
BOOL CX24128_tuner_pll_lock;          /* Tuner pll lock status (uses CX24128_refresh_tuner_pll_lock) */ 
 
BOOL CX24128_refresh_tuner_pll_freq;   /* Shadowing flag */ 
unsigned long  CX24128_tuner_pll_freq; /* Tuner PLL frequency (uses refresh_tuner_pll_freq)   */   
 
/**************************************** I2C repeater functions ***************************************/ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetDemodRepeaterMode()  
 * sets the demod to be in repeater Mode 
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_SetDemodRepeaterMode(NIM* p_nim) 
{    
   unsigned char  ucRegVal; 

   if (p_nim->demod_type == CX24123)
   { 
	if (RegisterWrite(p_nim, CX24123_TUNI2CRPTSTART, 1, DEMOD_I2C_IO) == False)
		return (False);
   } 
   return (True); 
} 
 
/******************************************************************************************************* 
 * TUNER_CX24128_RepeaterModeReadData()  
 * Sets the demod in repeater mode and reads 8-bit data. 
 *******************************************************************************************************/ 
BOOL 
TUNER_CX24128_RepeaterModeReadData(NIM* p_nim, unsigned long handle, SBaddress address, unsigned char* p_data_read) 
{ 
	if (p_data_read == 0) 
	{ 
		return (False); 
	} 

	if (TUNER_CX24128_SetDemodRepeaterMode(p_nim) == False) 
	{ 
		return(False); 
	} 
 
	*p_data_read = (*p_nim->SBRead)(p_nim->ptuner, handle, address, &p_nim->iostatus); 
 
	if (p_nim->iostatus != 0UL) 
	{ 
		/* Hardware write error */ 
		DRIVER_SET_ERROR(p_nim, API_REG_HDWR_REWTERR); 
		return (False); 
	} 
	return(True); 
} 
 
/******************************************************************************************************* 
 * TUNER_CX24128_RepeaterModeWriteData()  
 * Sets the demod in repeater mode and writes 8-bit data. 
 *******************************************************************************************************/ 
BOOL 
TUNER_CX24128_RepeaterModeWriteData(NIM* p_nim, unsigned long handle, SBaddress address, unsigned char value) 
{ 
	if (TUNER_CX24128_SetDemodRepeaterMode(p_nim) == False) 
	{ 
		return(False); 
	} 
 
	(*p_nim->SBWrite)(p_nim->ptuner, handle, address, value, &p_nim->iostatus); 
 
	if (p_nim->iostatus != 0UL)   
	{ 
		return(False); 
	} 
 
	return(True); 
} 
 
/******************************************************************************************************* 
 * TUNER_CX24128_EnableDemodRepeaterMode()  
 * Enables the demod's repeater 
 *******************************************************************************************************/ 
BOOL   
TUNER_CX24128_EnableDemodRepeaterMode(NIM *p_nim) 
{ 
       if (RegisterWrite(p_nim, CX24130_TUNBTIEN, 0, DEMOD_I2C_IO) == False)
		return (False);

       if (p_nim->demod_type == CX24123) 
       { 
	       if (RegisterWrite(p_nim, CX24123_TUNI2CRPTEN, 1, DEMOD_I2C_IO) == False)
			return (False);
       } 
       return(True); 
} 

/**************************************** Viper functions **********************************************/ 

/******************************************************************************************************* 
 * TUNER_CX24128_SetClkInversion()  
 * sets the clock inversion to both nim structure and to tuner register 
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_SetClkInversion(NIM     *p_nim,       /* pointer to nim */ 
                              BOOL    clkinversion) /* clock inversion value */ 
{ 
	unsigned long reg_value; 
 
	TUNER_CX24128_VALIDATE(p_nim); 
 
	reg_value = (unsigned long)clkinversion; 
 
	p_nim->tuner.cx24128.clkinversion = clkinversion; 

	/* set the clock inversion to the tuner */ 
	if(RegisterWrite(p_nim, CX24128_DSM_CLK, reg_value, p_nim->tuner.cx24128.io_method) != True)
		return(False); 
 
	return(True); 
}  /* TUNER_CX24128_SetClkInversion() */ 
 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetEnableRegister()  
 * sets the enable bits to tuner 
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_SetEnableRegister(NIM     *p_nim,       /* pointer to nim */ 
                                unsigned char enable) /* enable bits */ 
{ 
	unsigned long reg_value; 
 
	TUNER_CX24128_VALIDATE(p_nim); 
	reg_value = (unsigned long)enable; 
   
    /* set the enable bits to the tuner */ 
    if(RegisterWrite(p_nim, CX24128_EN, reg_value, p_nim->tuner.cx24128.io_method) != True)
		return(False); 

#if INCLUDE_RATTLER 
    /* Rattler specific */ 
    if (p_nim->tuner_type == CX24113) 
    {   /* Enable FTA LNA */ 
        if (RegisterWrite(p_nim, CX24128_LNA_EN, 1UL, p_nim->tuner.cx24128.io_method) != True)
        { 
	        return(False); 
        } 
    } 
#endif /* INCLUDE_RATTLER */ 
 
  return(True); 
}  /* TUNER_CX24128_SetEnableRegister() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GlobalSetRefSelSpi()  
 * Program the global ref_sel_spi bit 
 *******************************************************************************************************/ 
static
BOOL 
TUNER_CX24128_GlobalSetRefSelSpi(NIM *p_nim, unsigned char value)
{ 
	unsigned char reg_val = 0; 
 
	if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO) 
	{ 
		if (TUNER_CX24128_RepeaterModeReadData(p_nim, p_nim->tuner.cx24128.tuner_handle, CX24128_XTAL_02, &reg_val) == False) 
		{ 
			return (False); 
		} 
	} 
	else 
	{ 
		//BTI
		return (False); 
	} 
	 
	reg_val &= 0xFB; 
	reg_val |= ((value & 0x01) << 2); 
 
	if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO) 
	{ 
		if (TUNER_CX24128_RepeaterModeWriteData(p_nim, p_nim->tuner.cx24128.tuner_handle, CX24128_XTAL_02, reg_val) == False) 
		{ 
			return (False); 
		} 
	} 
	else 
	{ 
		//BTI
		return (False); 
	} 
	return (True); 
} 

/******************************************************************************************************* 
 * TUNER_CX24128_powerup()  
 * performs tuner power-up reset  
 *******************************************************************************************************/ 
BOOL         
TUNER_CX24128_powerup(NIM  *p_nim)  /* pointer to nim */ 
{ 
	unsigned long tuner_handle; 
	unsigned char chipid; 
	VIPERPARMS    viperparms; 
 
	CX24128_refresh_tuner_pll_lock = True;  
	 
 
	/* determine the io method */ 
        /* supports I2C repeater mode */ 
	 
	/* Initialize the register offset in case of dual tuner system */ 

	if ((p_nim->demod_type == CX24130) || (p_nim->demod_type == CX24121))
	{
		p_nim->tuner.cx24128.register_offset = GET_CX24128_BTI_REGISTER_OFFSET(p_nim->demod_handle);
		p_nim->tuner.cx24128.io_method = VIPER_BTI_IO;
	} else
	{
		p_nim->tuner.cx24128.register_offset = GET_CX24128_I2C_REGISTER_OFFSET(p_nim->demod_handle);
		p_nim->tuner.cx24128.io_method = VIPER_I2C_IO;
 	}
 
	if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO) 
	{ 
		/* First of all, detect the I2C address of Mongoose or Viper. */ 
		/* Try I2C address 1, read chip ID. */ 
		if (TUNER_CX24128_EnableDemodRepeaterMode(p_nim) == False) 
		{ 
			return (False); 
		} 
 
		printf("TUNER_CX24128_powerup: 10\n");

		tuner_handle = p_nim->demod_handle; 
		SET_DEMOD_HANDLE_I2C_ADDR(tuner_handle, CX24128_I2C_ADD1);
 
		printf("TUNER_CX24128_powerup: 20: handle: 0x%08lx\n", tuner_handle);

                  /* --- CR 21101 begin --- */ 
		if (TUNER_CX24128_SetDemodRepeaterMode(p_nim) == False) 
		{ 
			return(False); 
		} 

		printf("TUNER_CX24128_powerup: 25\n");

                 /* HW reset puts pulse out on tuner serial bus causing the very first transaction to fail.*/  
                 /* This is a dummy read to fix that.*/ 
       	        chipid = (*p_nim->SBRead)(p_nim->ptuner, tuner_handle, CX24128_CHIP_ID_00, &p_nim->iostatus); 
                 /* --- CR 21101 end --- */ 
 
		printf("TUNER_CX24128_powerup: 30: chipid: 0x%02x\n", chipid);

		if (TUNER_CX24128_SetDemodRepeaterMode(p_nim) == False) 
		{ 
			return(False); 
		} 

		printf("TUNER_CX24128_powerup: 35\n");

		chipid = (*p_nim->SBRead)(p_nim->ptuner, tuner_handle, CX24128_CHIP_ID_00, &p_nim->iostatus); 
 
		printf("TUNER_CX24128_powerup: 40: chipid: 0x%02x, status: 0x%08lx\n", chipid, p_nim->iostatus);

		if ((p_nim->iostatus != 0UL) || ((chipid != VIPER_CHIP_ID) && (chipid != 0x44) && (chipid != 0x45) && (chipid != MONGOOSE_CHIP_ID))) /* Check address 2. */ 
		{ 
			tuner_handle = p_nim->demod_handle; 
			SET_DEMOD_HANDLE_I2C_ADDR(tuner_handle, CX24128_I2C_ADD2);
 
			if (TUNER_CX24128_RepeaterModeReadData(p_nim, tuner_handle, CX24128_CHIP_ID_00, &chipid) == False) 
			{   /* Both addresses failed, return error. */ 
				return(False); 
			} 
		} 
 
		printf("TUNER_CX24128_powerup: 50: chipid: 0x%02x\n", chipid);

		/* check chip ID, make sure it is Mongoose/Viper. */ 
		if ((chipid != VIPER_CHIP_ID) && (chipid != MONGOOSE_CHIP_ID) && (chipid != 0x44) && (chipid != 0x45)) 
		{ 
			return(False); 
		} 
 
		/* Keep the chip ID value. */ 
		p_nim->tuner.cx24128.chipid = chipid; 
 
		/* Keep the tuner_handle value for later use. */ 
		p_nim->tuner.cx24128.tuner_handle = tuner_handle; 
 
		/* Get tuner version (global) */ 
		if (TUNER_CX24128_RepeaterModeReadData(p_nim, tuner_handle, CX24128_CHIP_VERSION_01, &p_nim->tuner.cx24128.version) == False) 
		{    
			return(False); 
		}

		printf("TUNER_CX24128_powerup: 60: version: 0x%02x\n", p_nim->tuner.cx24128.version);
	} else
	{
		/* BTI */
	}
	 
 
	p_nim->tuner.cx24128.tuner_gain_thresh = -50; /* -50dBm default */ 
	p_nim->tuner.cx24128.gain_setting_selection = SIGNAL_LEVEL_LOW; 
 
	/* Set reference frequency to use crystal frequency. */ 
	if (TUNER_CX24128_SetReferenceFreq(p_nim, RFREQ_0) != True)  
	{ 
		return(False); 
	} 
 
	printf("TUNER_CX24128_powerup: 70\n");

	/* Set parameters. */ 
	/* Initialize ICP parameters. */    
	viperparms.ICPmode = ICPMODE_AUTO;       /* ICP auto mode by default. */            
 
	viperparms.ICPauto_Hi    = ICPAUTO_LEVEL4;     /* Auto analog ICP levels, 2.0 mA */ 
	viperparms.ICPauto_MedHi = ICPAUTO_LEVEL4;     /* 2.0 mA */ 
	viperparms.ICPauto_MedLo = ICPAUTO_LEVEL3;     /* 1.5 mA */ 
	viperparms.ICPauto_Lo    = ICPAUTO_LEVEL1;     /* 0.5 mA */ 
 
	viperparms.ICPdig = ICPDIG_LEVEL3;      /* 2.0 times */ 
	viperparms.ICPman = ICPMAN_LEVEL1;       /* ICP manual mode default. */ 
	viperparms.ACP_on = True;             /* Analog CP off by default. */ 

	/* Initialize VCO parameters. Include set VCO mode to be AUTO. */    
	viperparms.VcoMode    = VCOMODE_AUTO; 
	viperparms.Vcobs      = VCOBANDSHIFT_HIGH; 
	viperparms.VcobandSel = VCOBANDSEL_1; 
	viperparms.Bsdelay    = 0x08;             /* Band sel delay value, default is 8 */ 
     
         /* Band sel freq cnt */ 
	viperparms.Bsfreqcnt = 0x0FFF;           
 
        /* Band sel ref div value */ 
	viperparms.Bsrdiv    = 0x0FFF;              
 
	/* Initialize other parameters. */ 
	viperparms.PrescalerMode = False; 
 
	/* Enable hardware parts. */ 
	TUNER_CX24128_SetEnableRegister(p_nim, 0x3D); /* FTA LNA is disabled */ 
 
#if INCLUDE_RATTLER 
	if (p_nim->tuner_type == CX24113) /* Rattler specific */ 
	{ 
		viperparms.lna_gain = LNA_MAX_GAIN; /* LNA gain (max) */ 
	} 
#endif /*INCLUDE_RATTLER */ 
 
	 /* -- Mongoose RevB -- */ 
         viperparms.RFVGABiasControl = True; /* Disable (=1) RFVGA bias control circuit */ 
 
	/* Set parameters to tuner and nim. */ 
	TUNER_CX24128_SetParameters(p_nim, &viperparms); 
 
	/* Set Gain default values. */ 
	TUNER_CX24128_SetGainSettings(p_nim, 0UL); 
 
	/* Set BW default settings. */ 
	TUNER_CX24128_SetFilterBandwidth(p_nim, 18025UL); 
 
	/* Set DSM clk inversion. */ 
	TUNER_CX24128_SetClkInversion(p_nim, True); 
	 
       FWindowEnable = True; 
 
    /* Set the Mongoose ref clock output (CX24128_REFSEL_SPI_DIV1 is the default setting) */ 
	if (p_nim->xtal >= CX24128_A3_XTAL_FREQ) /* div 2 for A3 */ 
	{ 
		if (TUNER_CX24128_GlobalSetRefSelSpi(p_nim, CX24128_REFSEL_SPI_DIV2) != True)
		{ 
			return(False); 
		} 
	} 
	else 
	{ 
		if (TUNER_CX24128_GlobalSetRefSelSpi(p_nim, CX24128_REFSEL_SPI_DIV1) != True)
		{ 
			return(False); 
		} 
	} 
    return(True); 
}/* TUNER_CX24128_powerup() */ 
 
#if 0
BTI
#endif

/******************************************************************************************************* 
 * TUNER_CX24128_SetReferenceFreq()  
 * sets the current reference frequency value to tuner register and nim 
 *******************************************************************************************************/ 
//static BOOL      
BOOL
TUNER_CX24128_SetReferenceFreq(NIM       *p_nim,     /* pointer to nim */ 
                               RFREQVAL  rfreqvalue) /* reference frequency value */ 
{ 
	unsigned char reg_value, read_value = 0; 
	unsigned char ref_freq_shadow; 
 
	TUNER_CX24128_VALIDATE(p_nim); 
 
	reg_value = (unsigned char)(rfreqvalue & 0x01); 
 
	if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO) 
	{ 
		if (TUNER_CX24128_RepeaterModeReadData(p_nim, p_nim->tuner.cx24128.tuner_handle, CX24128_XTAL_02, &read_value) == False) 
		{ 
			return (False); 
		} 
	} 
	else 
	{ 
		//BTI
	       return (False); 
	}  
 
          if (p_nim->tuner.cx24128.vcodiv == VCODIV4 && ((p_nim->tuner.cx24128.chipid == VIPER_CHIP_ID) || (p_nim->tuner.cx24128.chipid == 0x44) || (p_nim->tuner.cx24128.chipid == 0x45))) /* CR 21579 */ 
          { 
              reg_value = 0x01; 
          } 
       
        	if (p_nim->tuner.cx24128.register_offset == 0) /* Tuner A */ 
        	{		 
        		ref_freq_shadow = (unsigned char)((read_value >> 1) & 0x01); 
        		read_value &= 0xFD; 
        		read_value |= (reg_value << 1); 
        	}  
        	else /* Tuner B */ 
        	{		 
        		if (((p_nim->tuner.cx24128.chipid == VIPER_CHIP_ID) || (p_nim->tuner.cx24128.chipid == 0x44) || (p_nim->tuner.cx24128.chipid == 0x45)) && p_nim->xtal >= CX24128_A3_XTAL_FREQ) /* Always set reference division ratio to /2 for A3 */ 
        		{ 
        			reg_value = 0x01;	 
        		} 
        		ref_freq_shadow = (unsigned char)(read_value & 0x01); 
        		read_value &= 0xFE;		 
        		read_value |= reg_value; 
        		 
        	} 
         
        	if (ref_freq_shadow != reg_value) 
        	{ 
        		if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO) 
        		{ 
        			if (TUNER_CX24128_RepeaterModeWriteData(p_nim, p_nim->tuner.cx24128.tuner_handle, CX24128_XTAL_02, read_value) == False) 
        			{ 
        				return (False); 
        			} 
        		} 
        		else 
        		{ 
				//BTI
        		       return (False); 
        		} 
        	} 
 
	return(True); 
}  /* TUNER_CX24128_SetReferenceFreq() */ 

/******************************************************************************************************* 
 * TUNER_CX24128_SetReferenceDivider()  
 * sets the current reference divider value to nim only 
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_SetReferenceDivider(NIM     *p_nim,  /* pointer to nim */ 
                                  RDIVVAL  rvalue) /* reference divider value */ 
{ 
	TUNER_CX24128_VALIDATE(p_nim); 
 
    if (p_nim->tuner.cx24128.vcodiv == VCODIV4 && ((p_nim->tuner.cx24128.chipid == VIPER_CHIP_ID) || (p_nim->tuner.cx24128.chipid == 0x44) || (p_nim->tuner.cx24128.chipid == 0x45))) /* CR 21579 */ 
    { 
        rvalue = RDIV_2; 
    } 
 
    if (((p_nim->tuner.cx24128.chipid == VIPER_CHIP_ID) || (p_nim->tuner.cx24128.chipid == 0x44) || (p_nim->tuner.cx24128.chipid == 0x45)) && p_nim->tuner.cx24128.register_offset != 0) /* Tuner B */  
    { 
	    if (p_nim->xtal >= CX24128_A3_XTAL_FREQ) /* Always set reference division ratio to /2 for A3 */ 
	    { 
            rvalue = RDIV_2; 
	    }		 
    } 
 
  switch(rvalue) 
  { 
    case  RDIV_1: 
    case  RDIV_2: 
    { 
		p_nim->tuner.cx24128.R = rvalue; 
		break; 
    } 
    default: 
    { 
      DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
      return(False); 
      break; 
    } 
  }  /* switch(... */ 
 
  return(True); 
}  /* TUNER_CX24128_SetReferenceDivider() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetReferenceFreq()  
 * retrieves current reference frequency value from nim 
 *******************************************************************************************************/ 
BOOL       
TUNER_CX24128_GetReferenceFreq(NIM       *p_nim,    /* pointer to nim */ 
                               RFREQVAL  *p_fvalue) /* pointer to RFREQVAL where value is returned to caller */ 
{ 
	TUNER_CX24128_VALIDATE(p_nim); 
   
	*p_fvalue = p_nim->tuner.cx24128.rfreqval; 
 
	return(True); 
}  /* TUNER_CX24128_GetReferenceFreq() */ 
 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetVcoDivider()  
 * sets the vco divider to nim structure  
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_SetVcoDivider(NIM     *p_nim, /* pointer to nim */ 
                            VCODIV  vcodiv) /* vco divider value */ 
{ 
	TUNER_CX24128_VALIDATE(p_nim); 
 
	p_nim->tuner.cx24128.vcodiv = vcodiv; 
 
	return(True); 
}  /* TUNER_CX24128_SetVcoDivider() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetParameters()  
 * returns current tuner parameter settings to the caller  
 *******************************************************************************************************/ 
BOOL         
TUNER_CX24128_GetParameters(NIM         *p_nim,        /* pointer to nim */ 
                            VIPERPARMS  *p_viperparms) /* pointer to VIPERPARMS, where copy will be written */ 
{ 
  unsigned long reg_value; 
 
  TUNER_CX24128_VALIDATE(p_nim); 
 
  if (p_viperparms == 0)   
  { 
	  return(False); 
  } 
 
    /* copy tunerparms to caller storage */ 
    memcpy(p_viperparms, &p_nim->tuner.cx24128.viperparms, sizeof(VIPERPARMS)); 
 
    /* Those values may be updated by harware. */ 
    if (RegisterRead(p_nim, CX24128_VCO_BSH_SPI, &reg_value, p_nim->tuner.cx24128.io_method) != True)   
    { 
	    return(False); 
    } 
 
    p_viperparms->Vcobs = (VCOBANDSHIFT)reg_value; 
 
    /* set the vco band shift to the tuner */ 
    if (RegisterRead(p_nim, CX24128_VCO_SEL_SPI, &reg_value, p_nim->tuner.cx24128.io_method) != True)   
    { 
	    return(False); 
    } 
 
    p_viperparms->VcobandSel = (VCOBANDSEL)reg_value; 
 
    /* -- Mongoose RevB -- */ 
    if (reg_value == 0) /* could be VCO6 */ 
    { 
        if (RegisterRead(p_nim, CX24128_VCO6_SEL_SPI, &reg_value, p_nim->tuner.cx24128.io_method) != True)   
	    { 
		    return(False); 
	    } 
        if (reg_value != 0) 
        { 
            p_viperparms->VcobandSel = VCOBANDSEL_6; 
        } 
    } 
	 
	if (RegisterRead(p_nim, CX24128_BS_FREQ, &reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
	p_viperparms->Bsfreqcnt = (unsigned short)reg_value; 
	 
	if (RegisterRead(p_nim, CX24128_BS_DIV_CNT, &reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
	p_viperparms->Bsrdiv = (unsigned short)reg_value; 
 
	return(True); 
}  /* TUNER_CX24128_GetParameters() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetParameters()  
 * sets passed in parameters in nim and into tuner 
 *******************************************************************************************************/ 
BOOL         
TUNER_CX24128_SetParameters(NIM         *p_nim,        /* pointer to nim */ 
                            VIPERPARMS  *p_viperparms) /* copy of VIPERPARMS to use as default */ 
{ 
	unsigned long    reg_value; 
 
	TUNER_CX24128_VALIDATE(p_nim);
 
	if (p_viperparms == 0)   
	{ 
		return(False); 
	} 
 
	/* copy from user-buffer to tuner buffer */ 
	memcpy(&p_nim->tuner.cx24128.viperparms, p_viperparms, sizeof(VIPERPARMS)); 
   
	/* Set ICP settings to tuner. */ 
	/* ICP mode */ 
	reg_value = (unsigned long)p_viperparms->ICPmode; 
 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_MOR), reg_value, p_nim->tuner.cx24128.io_method) != True)
	{ 
		return(False); 
	} 
   
	/* ICP manual level. It's O.K if manual level is undefined.*/ 
	if (p_viperparms->ICPman != ICPMAN_UNDEF) 
	{ 
		switch(p_viperparms->ICPman) 
		{ 
			case ICPMAN_LEVEL1: 
			case ICPMAN_LEVEL2: 
			case ICPMAN_LEVEL3: 
			case ICPMAN_LEVEL4: 
				reg_value = (unsigned long)p_viperparms->ICPman; 
				break; 
			default: 
			{ 
				/* Need define a new ERROR number. */ 
				//DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
				return(False); 
			} 
		} 
		if (RegisterWrite(p_nim, (unsigned short)(CX24128_ICP_MAN), reg_value, p_nim->tuner.cx24128.io_method) != True)   
		{ 
			return(False); 
		} 
	} 
 
	/* ICP auto level. */ 
	switch(p_viperparms->ICPauto_Lo) 
	{ 
		case ICPAUTO_LEVEL1: 
		case ICPAUTO_LEVEL2: 
		case ICPAUTO_LEVEL3: 
		case ICPAUTO_LEVEL4: 
			reg_value = (unsigned long)p_viperparms->ICPauto_Lo; 
			break; 
		default: 
		{ 
			/* Need define a new ERROR number. */ 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
			return(False); 
		} 
	} 
 
	switch(p_viperparms->ICPauto_MedLo) 
	{ 
		case ICPAUTO_LEVEL1: 
		case ICPAUTO_LEVEL2: 
		case ICPAUTO_LEVEL3: 
		case ICPAUTO_LEVEL4: 
			reg_value |= (unsigned long)p_viperparms->ICPauto_MedLo << 2UL; 
			break; 
		default: 
		{ 
			/* Need define a new ERROR number. */ 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
			return(False); 
		} 
	} 
 
	switch(p_viperparms->ICPauto_MedHi) 
	{ 
		case ICPAUTO_LEVEL1: 
		case ICPAUTO_LEVEL2: 
		case ICPAUTO_LEVEL3: 
		case ICPAUTO_LEVEL4: 
			reg_value |= (unsigned long)p_viperparms->ICPauto_MedHi << 4UL; 
			break; 
		default: 
		{ 
			/* Need define a new ERROR number. */ 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
			return(False); 
		} 
	} 
 
	switch(p_viperparms->ICPauto_Hi) 
	{ 
		case ICPAUTO_LEVEL1: 
		case ICPAUTO_LEVEL2: 
		case ICPAUTO_LEVEL3: 
		case ICPAUTO_LEVEL4: 
			reg_value |= (unsigned long)p_viperparms->ICPauto_Hi << 6UL; 
			break; 
		default: 
		{ 
			/* Need define a new ERROR number. */ 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
			return(False); 
		} 
	} 
 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_ICP_LEVEL), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
   
	/* ICP digital. */ 
	switch(p_viperparms->ICPdig) 
	{ 
		case ICPDIG_LEVEL1: 
		case ICPDIG_LEVEL2: 
		case ICPDIG_LEVEL3: 
		case ICPDIG_LEVEL4: 
			reg_value = (unsigned long)p_viperparms->ICPdig; 
			break; 
		default: 
		{ 
			/* Need define a new ERROR number. */ 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); 
			return(False); 
		} 
	} 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_IDIG_SEL), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
 
#if INCLUDE_RATTLER 
         /* Rattler specific */ 
         if (p_nim->tuner_type == CX24113) 
         { 
             switch(p_viperparms->lna_gain) 
             { 
     	        case LNA_MIN_GAIN: 
     	        case LNA_MID_GAIN: 
     	        case LNA_MAX_GAIN: 
     		        reg_value = (unsigned long)p_viperparms->lna_gain; 
     		        break; 
     	        default: 
     	        { 
     		        return(False); 
     	        } 
             } 
             if (RegisterWrite(p_nim, (unsigned short)(CX24128_LNA_GC), reg_value, p_nim->tuner.cx24128.io_method) != True)   
             { 
     	        return(False); 
             } 
         } 
#endif /* INCLUDE_RATTLER */ 
 
	/* Analog CP ON. */ 
	reg_value = (unsigned long)p_viperparms->ACP_on; 
 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_ACP_ON_ALW), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
 
	/* Set VCO settings to tuner. */ 
	switch(p_viperparms->VcoMode) 
	{ 
		case VCOMODE_AUTO: 
		case VCOMODE_TEST: 
			reg_value = (unsigned long)p_viperparms->VcoMode; 
			break; 
		default: 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); /* need define a new err number. */ 
			return(False); 
	} 
 
	/* set the vco mode to the tuner */ 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_BS_VCOMT), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
 
	switch(p_viperparms->Vcobs) 
	{ 
	case VCOBANDSHIFT_HIGH: 
	case VCOBANDSHIFT_LOW: 
		reg_value = (unsigned long)p_viperparms->Vcobs; 
		break; 
	default: 
		//DRIVER_SET_ERROR(p_nim,API_TUNERREF); /* need define a new err number. */ 
		return(False); 
	} 
 
	/* set the vco band shift to the tuner */ 
	if(RegisterWrite(p_nim, (unsigned short)(CX24128_VCO_BSH_SPI), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
   
	switch(p_viperparms->VcobandSel) 
	{ 
		case VCOBANDSEL_1: 
		case VCOBANDSEL_2: 
		case VCOBANDSEL_3: 
		case VCOBANDSEL_4: 
		case VCOBANDSEL_5: 
        case VCOBANDSEL_6: /* -- Mongoose RevB -- */ 
			reg_value = (unsigned long)p_viperparms->VcobandSel; 
			break; 
		default: 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); /* need define a new err number. */ 
			return(False); 
	} 
 
    /* set the vco band shift to the tuner */ 
    if (p_viperparms->VcobandSel == VCOBANDSEL_6) 
    { 
	    if (RegisterWrite(p_nim, (unsigned short)(CX24128_VCO6_SEL_SPI), reg_value, p_nim->tuner.cx24128.io_method) != True)
	    { 
		    return(False); 
	    } 
    } 
    else 
    { 
	    if (RegisterWrite(p_nim, (unsigned short)(CX24128_VCO_SEL_SPI), reg_value, p_nim->tuner.cx24128.io_method) != True)
	    { 
		    return(False); 
	    } 
    } 
 
	/* Set other VCO parameters. */ 
	reg_value = (unsigned long)p_viperparms->Bsdelay; 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_BS_DELAY), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
 
	reg_value = (unsigned long)p_viperparms->Bsfreqcnt; 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_BS_FREQ), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
 
	reg_value = (unsigned long)p_viperparms->Bsrdiv; 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_BS_DIV_CNT), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
      
	/* Set pre-scaler mode. */ 
	switch(p_viperparms->PrescalerMode) 
	{ 
		case True: 
			reg_value = 0x01UL; 
			break; 
		case False: 
			reg_value = 0x00UL; 
			break; 
		default: 
			//DRIVER_SET_ERROR(p_nim,API_TUNERREF); /* need define a new err number. */ 
			return(False); 
			break; 
	} 
 
	if (RegisterWrite(p_nim, (unsigned short)(CX24128_PS_TEST), reg_value, p_nim->tuner.cx24128.io_method) != True)   
	{ 
		return(False); 
	} 
 
         /* -- Mongoose RevB -- */ 
         reg_value = (p_viperparms->RFVGABiasControl == True) ? 0x01 : 0x00; 
         if (RegisterWrite(p_nim, (unsigned short)(CX24128_RFBC_DISABLE), reg_value, p_nim->tuner.cx24128.io_method) != True) 
     	{ 
     		return(False); 
     	} 
 
	return(True); 
}  /* TUNER_CX24128_SetParameters() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_set_NFRregisters()  
 * function to set N, F, R registers in tuner  
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_SetNFRRegisters(NIM               *p_nim, /* pointer to nim */ 
                              unsigned int      nvalue, /* N register */ 
                              int               fvalue, /* F register */ 
                              RDIVVAL           rvalue) /* R register */ 
{ 
	unsigned long reg_value,reg_vlue_temp; 
	unsigned long pllfreq; 
	unsigned long Nfrac; 
	BCDNO         bcd;
 
	/* save last n,a,r settings */ 
	p_nim->tuner.cx24128.N = nvalue; 
	p_nim->tuner.cx24128.F = fvalue; 
	p_nim->tuner.cx24128.R = rvalue; 
 
	reg_value = nvalue;
          
         if(RegisterWrite(p_nim, CX24128_INT/*0x13*/, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)
         { 
            return(False); 
         } 
          

	reg_vlue_temp = fvalue;

         if(RegisterWrite(p_nim, CX24128_FRACTN/*0x14*/, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)   
         { 
            return(False); 
         }	 
          
         
         reg_value = (unsigned long)(rvalue - 1); 
          
         if (TUNER_CX24128_SetReferenceFreq(p_nim, (RFREQVAL)reg_value) == False) 
         { 
            return (False); 
         } 
 
 
	/* multiplication factor of 10000 is used to improve precision */ 
	Nfrac = ( (fvalue * 10000L) / CX24128_DIVIDER) + (nvalue * 10000UL) + (32UL * 10000UL); 
 
	BCD_set(&bcd, (p_nim->tuner_crystal_freq / p_nim->tuner.cx24128.R)); 
 
	BCD_mult(&bcd, Nfrac); 
	BCD_mult(&bcd, 2); 
	BCD_div(&bcd, 10000); 
 
	BCD_div(&bcd, p_nim->tuner.cx24128.vcodiv); 
    	if ((p_nim->tuner_crystal_freq) / MM >= 20) 
	{ 
		BCD_mult(&bcd, 2); 
	} 
 
	pllfreq = BCD_out(&bcd);  
 
	/* Now tuner is set to a new frequency. */ 
	p_nim->pll_frequency = pllfreq; 
 
	return(True); 
}  /* TUNER_CX24128_set_NFRregisters() */ 
 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetFrequency()  
 * Set specified tuner pll frequency  
 ******************************************************************************************************/ 
BOOL            
TUNER_CX24128_SetFrequency(NIM      *p_nim, /* pointer to nim */ 
                  unsigned long  frequency) /* pointer to unsigned long  */ 
{ 
    unsigned short nvalue; 
    int   fvalue; 
    unsigned long  reg_value; 
 
    TUNER_CX24128_VALIDATE(p_nim); 
 
    CX24128_refresh_tuner_pll_lock = True; 
 
    p_nim->pll_frequency = frequency; 
 
    if (RegisterWrite(p_nim, (unsigned short)(CX24128_BS_VCOMT/*0xA*/), 0, p_nim->tuner.cx24128.io_method) != True)
         return(False); 

    if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO)
    {
	if (TUNER_CX24128_RepeaterModeReadData(p_nim, p_nim->tuner.cx24128.tuner_handle, 2, &reg_value) != True)
		return (False);

	if (p_nim->tuner.cx24128.register_offset)
		reg_value |= 4;
	else	reg_value |= 8;

	if (TUNER_CX24128_RepeaterModeWriteData(p_nim, p_nim->tuner.cx24128.tuner_handle, 2, reg_value) != True)
		return (False);
    }

    if (RegisterWrite(p_nim, (unsigned short)(CX24128_PS_TEST/*0x1*/), 0, p_nim->tuner.cx24128.io_method) != True)
         return(False); 
 
    /* Calculate N,F,R by specified frequency value. */ 
    if (TUNER_CX24128_calc_pllNF(p_nim,&nvalue,&fvalue) != True)
		return(False);

    /* Set N,F,R to tuner registers. */ 
    if (TUNER_CX24128_SetNFRRegisters(p_nim,nvalue,fvalue,p_nim->tuner.cx24128.R) != True)
		return(False);

    reg_value = (p_nim->tuner.cx24128.vcodiv != VCODIV2); 

    if (RegisterWrite(p_nim, (unsigned short)(CX24128_PS_TEST/*0x1*/), reg_value, p_nim->tuner.cx24128.io_method) != True)
         return(False); 

    //DELAY(5000);	//kir++

    if (RegisterWrite(p_nim, (unsigned short)(CX24128_DIV24_SEL/*0x15*/), 1, p_nim->tuner.cx24128.io_method) != True)
         return(False); 

    if (p_nim->tuner.cx24128.io_method == VIPER_I2C_IO)
    {
	if (TUNER_CX24128_RepeaterModeReadData(p_nim, p_nim->tuner.cx24128.tuner_handle, 2, &reg_value) != True)
		return (False);

	if (p_nim->tuner.cx24128.register_offset)
		reg_value &= 0xFB;
	else	reg_value &= 0xF7;

	if (TUNER_CX24128_RepeaterModeWriteData(p_nim, p_nim->tuner.cx24128.tuner_handle, 2, reg_value) != True)
		return (False);
    }
 
   return(True); 
}  /* TUNER_CX24128_SetFrequency() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetFilterBandwidth()  
 * function to get the filter bandwidth for tuner's anti-alias filter  
 *******************************************************************************************************/ 
BOOL            
TUNER_CX24128_GetFilterBandwidth(NIM            *p_nim,             /* pointer to nim */ 
                                 FILTERBW       *p_filterbandwidth, /* filter bandwidth */ 
                                 unsigned short *p_gmcbandwidth)    /* gmc bandwidth */ 
{  
  unsigned short gmcbw; 
 
  TUNER_CX24128_VALIDATE(p_nim); 

  if (p_gmcbandwidth == 0 || p_filterbandwidth == 0)
	return(False); 
 
  *p_filterbandwidth = (FILTERBW)((p_nim->antialias_bandwidthkhz & 0xFC0) >> 6); 
 
  gmcbw = (unsigned short)(p_nim->antialias_bandwidthkhz & 0x3F); 
  /* convert gmc bandwidth to be in KHz. */ 
  gmcbw = (unsigned short)(gmcbw * CX24128_GMCBW_STEP_SIZE + CX24128_MIN_GMCBW); 
  *p_gmcbandwidth = gmcbw; 
 
  return(True); 
} /* TUNER_CX24128_GetFilterBandwidth */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetFilterBandwidth()  
 * function to set the filter bandwidth for tuner's anti-alias filter  
 *******************************************************************************************************/ 
BOOL            
TUNER_CX24128_SetFilterBandwidth(NIM            *p_nim,          /* pointer to nim */ 
                                 unsigned long  bandwidthkhz)    /* bandwidth in khz */ 
{  
	unsigned long  reg_value = 0; 
	FILTERBW       filterbandwidth = FILTERBW_35MHZ; 
 
	TUNER_CX24128_VALIDATE(p_nim); 
 
	/* gmcbandwidth is in KHz. */ 
	if ((bandwidthkhz > CX24128_MAX_GMCBW) || (bandwidthkhz < CX24128_MIN_GMCBW)) 
	{ 
		return(False); 
	} 
 
	if (bandwidthkhz <= 19000UL) /* 19 MHz */ 
	{ 
		filterbandwidth = FILTERBW_35MHZ; 
	} 
	else if (bandwidthkhz > 19000UL && bandwidthkhz <= 25000UL) /* 19 MHz to 25 MHz */ 
	{ 
		filterbandwidth = FILTERBW_40MHZ; 
	} 
	else /* > 25 MHz */ 
	{ 
		filterbandwidth = FILTERBW_65MHZ; 
	} 
 
	reg_value = filterbandwidth;

         if (RegisterWrite(p_nim, CX24128_FILTER_BW, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)
         { 
              return(False); 
         } 

        reg_value = (unsigned long)((bandwidthkhz * 10UL - CX24128_MIN_GMCBW * 10UL) / CX24128_GMCBW_STEP_SIZE); 
	reg_value += 5UL; 
	reg_value /= 10UL; 
 
         if (RegisterWrite(p_nim, CX24128_GMC_BW, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)
         { 
              return(False); 
         } 
 
	p_nim->antialias_bandwidthkhz = ((unsigned long)filterbandwidth << 6UL) | reg_value; 
 
	return(True); 
} /* TUNER_CX24128_SetFilterBandwidth */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetGainSettings()  
 * function to get the tuner gain settings  
 *******************************************************************************************************/ 
BOOL            
TUNER_CX24128_GetGainSettings(NIM           *p_nim,             /* pointer to nim */ 
                              AMPOUT        *p_ampout,          /* Discrete gain control*/ 
                              VGA1OFFSET    *p_vga1offset,      /* VGA1 offset control*/ 
                              VGA2OFFSET    *p_vga2offset,      /* VGA2 offset control*/ 
                              RFVGAOFFSET   *p_rfvgaoffset)     /* RF VGA offset control*/ 
                                  
{  
	TUNER_CX24128_VALIDATE(p_nim); 

	if (p_ampout == 0 || p_vga1offset == 0 || p_vga2offset == 0 || p_rfvgaoffset == 0)
	{ 
		return(False); 
	} 
 
	*p_ampout      = p_nim->tuner.cx24128.ampout; 
	*p_vga1offset  = p_nim->tuner.cx24128.vga1offset; 
	*p_vga2offset  = p_nim->tuner.cx24128.vga2offset; 
	*p_rfvgaoffset = p_nim->tuner.cx24128.rfvgaoffset; 
 
  return(True); 
} /* TUNER_CX24128_GetGainSettings */ 
 
 
/******************************************************************************************************* 
 * TUNER_CX24128_SetGainSettigns()  
 * function to set the gain settings to tuner and nim   
 *******************************************************************************************************/ 
BOOL            
TUNER_CX24128_SetGainSettings(NIM           *p_nim,          /* pointer to nim */ 
                              unsigned long  symbolrateksps)/* symbol rate determines the VCA, VGA settings */ 
{    
	unsigned long reg_value; 
	signed char  power_estimated = 0; 
	AMPOUT        ampout; 
	VGA1OFFSET    vga1offset; 
	VGA2OFFSET    vga2offset; 
	RFVGAOFFSET   rfvgaoffset; 
 
	TUNER_CX24128_VALIDATE(p_nim); 
 
         symbolrateksps = 0; /* Unused. However, the prototype should match TUNER_SetGainSettings function pointer */ 
 
	if (API_GetPowerEstimation(p_nim, &power_estimated) == False)
	{ 
		return (False); 
	} 
 
	if (power_estimated >= p_nim->tuner.cx24128.tuner_gain_thresh) /* in dBm */ 
	{ /* SET2 (Optimized for IIP3) - High seignal level */ 
		p_nim->tuner.cx24128.gain_setting_selection = SIGNAL_LEVEL_HIGH; 
		rfvgaoffset = RFVGAOFFSET_0;  
		vga1offset  = VGA1OFFSET_7;    
		vga2offset  = VGA2OFFSET_7;   
		ampout      = AMPOUT_25DB; 
	} 
	else /* SET1 (Optimized for NF) - low signal level */ 
	{ 
		p_nim->tuner.cx24128.gain_setting_selection = SIGNAL_LEVEL_LOW;  
		rfvgaoffset = RFVGAOFFSET_2;  
		vga1offset  = VGA1OFFSET_2;    
		vga2offset  = VGA2OFFSET_6;   
		ampout      = AMPOUT_25DB;  
	} 
 
	reg_value = ((unsigned char)ampout); 
          
         if(RegisterWrite(p_nim, CX24128_AMP_OUT/*0x16*/, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)
         { 
            return(False); 
         } 
          
         p_nim->tuner.cx24128.ampout = ampout; 
       
         reg_value = ((unsigned char)vga1offset);

         if (RegisterWrite(p_nim, CX24128_VGA1_OFFSET/*0x1A*/, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)
         { 
            return(False); 
         } 
          
         p_nim->tuner.cx24128.vga1offset = vga1offset; 
          
         reg_value = ((unsigned char)vga2offset);
       
         if(RegisterWrite(p_nim, CX24128_VGA2_OFFSET/*0x19*/, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)   
         { 
            return(False); 
         } 

         p_nim->tuner.cx24128.vga2offset = vga2offset; 
       
         reg_value = ((unsigned char)rfvgaoffset); 
          
         if(RegisterWrite(p_nim, CX24128_RF_OFFSET/*0x1C*/, (unsigned long)reg_value, p_nim->tuner.cx24128.io_method) != True)
         { 
            return(False); 
         } 
          
         p_nim->tuner.cx24128.rfvgaoffset = rfvgaoffset; 

	return(True); 
} /* TUNER_CX24128_SetGainSettings */ 
 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetClkInversion()  
 * gets the current clock inversion status, returns value to caller  
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_GetClkInversion(NIM      *p_nim,          /* pointer to nim */ 
                              BOOL     *p_clkinversion) /* clock inversion value */ 
{ 
  TUNER_CX24128_VALIDATE(p_nim); 
 
  *p_clkinversion = p_nim->tuner.cx24128.clkinversion; 
 
  return(True); 
 
}  /* TUNER_CX24128_GetClkInversion() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetEnableRegister()  
 * read the enable bits from tuner  
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_GetEnableRegister(NIM     *p_nim,       /* pointer to nim */ 
                             unsigned char *p_enable) /* enable bits */ 
{ 
  unsigned long reg_value; 
 
  TUNER_CX24128_VALIDATE(p_nim); 
 
  /* read the enable bits from the tuner */ 
  if(RegisterRead(p_nim, CX24128_EN, &reg_value, p_nim->tuner.cx24128.io_method) != True)
	return(False); 
 
  *p_enable = (unsigned char)reg_value; 
 
  return(True); 
}  /* TUNER_CX24128_GetEnableRegister() */ 
 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetVcoStatus()  
 * reads tuner register to get the Vco edge detection status 
 *******************************************************************************************************/ 
BOOL      
TUNER_CX24128_GetVcoStatus(NIM     *p_nim,         /* pointer to nim */ 
                           VCOSTATUS *p_vcostatus) /* pointer to the returned vco status. */ 
{ 
  unsigned long reg_value; 
  VCOMODE       VcoMode; 
 
  TUNER_CX24128_VALIDATE(p_nim); 
 
  VcoMode = p_nim->tuner.cx24128.viperparms.VcoMode; 
 
  switch(VcoMode) 
  { 
    case VCOMODE_AUTO: 
    case VCOMODE_TEST: 
    { 
      if(RegisterRead(p_nim, CX24128_BS_ERR, &reg_value, p_nim->tuner.cx24128.io_method) != True)  return(False); 
 
      if (reg_value == 0x01UL) 
      { 
        *p_vcostatus = VCO_AUTO_FAIL; 
      } 
      else 
      { 
        *p_vcostatus = VCO_AUTO_DONE; 
      } 
      break; 
    } 
    default: 
    { 
      //DRIVER_SET_ERROR(p_nim,API_TUNERREF); /* need define a new err number. */ 
      return(False); 
      break; 
    } 
  } 
 
  p_nim->tuner.cx24128.viperparms.VCOstatus = *p_vcostatus; 
 
  return(True); 
}/* TUNER_CX24128_GetVcoStatus() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetAnalogICPLevel()  
 * reads tuner register to get the selected ICP level 
 *******************************************************************************************************/ 
BOOL   
TUNER_CX24128_GetAnalogICPLevel(NIM *p_nim, ICPSELECT *p_icplevel) 
{ 
  unsigned long reg_value; 
 
  TUNER_CX24128_VALIDATE(p_nim);

  /* get the selected level from the tuner */ 
  if(RegisterRead(p_nim, CX24128_ICP_SEL, &reg_value, p_nim->tuner.cx24128.io_method) != True)
		return(False); 
 
  if (reg_value == 0) 
  { 
    *p_icplevel = ICPSELECT_LEVEL1; 
  } 
  else 
  { 
    *p_icplevel = (ICPSELECT)reg_value; 
  } 
 
  p_nim->tuner.cx24128.viperparms.ICPselect = *p_icplevel; 
 
  return(True); 
} 
 
/******************************************************************************************************* 
 * TUNER_CX24128_GetPLLFrequency()  
 * returns current frequency. programmed into the tuner pll register  
 *******************************************************************************************************/ 
BOOL            
TUNER_CX24128_GetPLLFrequency(NIM            *p_nim,     /* pointer to nim */ 
                              unsigned long  *p_pllfreq) /* pointer to unsigned long where pll frequency.*/  
                                                         /* in Hz. will be returned */ 
{ 
	unsigned short nvalue; /* Returned N value */ 
	/*unsigned long*/int  fvalue;  
	signed long    fvalue_signed; 
	unsigned long  Nfrac; 
	RDIVVAL        R; 
	VCODIV         vcodiv; 
 
	BCDNO bcd; 
 
	/* test for valid nim */ 
	TUNER_CX24128_VALIDATE(p_nim); 
 
	if (p_pllfreq == 0) 
	{ 
		DRIVER_SET_ERROR(p_nim, API_BAD_PARM); 
		return(False); 
	} 
 
	/* save last n,a,r settings */ 
	nvalue = (unsigned short)p_nim->tuner.cx24128.N;  
	fvalue = p_nim->tuner.cx24128.F; 
 
	fvalue_signed = (signed long)(fvalue); 
 
    R = p_nim->tuner.cx24128.R; 
    vcodiv = p_nim->tuner.cx24128.vcodiv; 
 
	/* catch any div by zero errors */ 
	if ((R != 0UL) && (vcodiv != 0UL)) 
	{ 
		/* 
			The frequency can be calculated from the crystal frequency and divider settings as follows: 
 
			FLO = [2 * Nfrac * (Fxtal/R)]/D 
 
			,where R = reference divider setting (either 1 or 2) 
			D = LO divider setting (either 2 or 4) 
			Fxtal = actual crystal frequency 
			Nfrac = total fractional divide ratio and can be calculated from 
			Nfrac = (Dividend/262144) + Nreg + 32 
 
			where, Dividend = DSM setting 18-bit signed value (between +/-131072) 
			Nreg = 9-bit divider setting 
		*/ 
		/* multiplication factor of 10000 is used to improve precision */ 
		Nfrac = ( (fvalue_signed * 10000L) / CX24128_DIVIDER) + (nvalue * 10000UL) + (32UL * 10000UL); 
		BCD_set (&bcd, (p_nim->tuner_crystal_freq / R)); 
 
		BCD_mult(&bcd, Nfrac); 
		BCD_mult(&bcd, 2); 
		BCD_div (&bcd, 10000); 
		BCD_div (&bcd, vcodiv); 
    		if ((p_nim->tuner_crystal_freq) / MM >= 20) 
		{ 
			BCD_mult(&bcd, 2); 
		} 
 
		*p_pllfreq = BCD_out(&bcd);  
	} 
	else 
	{ 
		DRIVER_SET_ERROR(p_nim,API_BAD_DIV); 
		return(False); 
	} 
    return(True); 
}/* TUNER_CX24128_GetPLLFrequency() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_pll_status 
 * reads current tuner pll lock status  
 *******************************************************************************************************/ 
BOOL   
TUNER_CX24128_pll_status(NIM  *p_nim,    /* nim pointer */ 
                   BOOL       *p_locked) /* BOOL pointer, where tuner pll lock status is returned */ 
{ 
  unsigned long  reg_value = 0; 
 
    if(CX24128_refresh_tuner_pll_lock == True) 
	{ 
		/* read the tuner register to get tuner pll lock status */ 
		if(RegisterRead(p_nim, CX24128_LOCK_DET, &reg_value, p_nim->tuner.cx24128.io_method) != True)
		{ 
			return(False); 
		} 
	              
		if (reg_value == 0UL)   
		{ 
			*p_locked = False; 
			CX24128_tuner_pll_lock = False; 
		} 
		else   
		{ 
			*p_locked = True; 
			CX24128_tuner_pll_lock = True; 
		} 
		CX24128_refresh_tuner_pll_lock = False; 
		 
	} 
	else 
	{		 
		*p_locked = CX24128_tuner_pll_lock; 
	} 
 
	return(True); 
}  /* TUNER_CX24128_pll_status() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_calc_pllNF()   
 * function to calc pll settings (n,f) using bcd functions. 
 * input is the specified pll frequency, output is N, F values to be set 
 * to tuner and nim. 
 *******************************************************************************************************/ 
BOOL  
TUNER_CX24128_calc_pllNF(NIM *p_nim,        /* nim pointer */ 
                         unsigned short *nvalue, /* Returned N value. */ 
                         int  *fvalue) /* Returned F value. */ 
{ 
  long           N; 
  long           F; 
  RDIVVAL        R; 
  VCODIV         vcodiv; 
  BCDNO          bcd; 
  unsigned char  factor; 
  unsigned char reg_value; 
 
  /* set frequency to a default setting, if not presently set, test xtal for zero before divide. */ 

  if (p_nim->pll_frequency == 0UL)
	p_nim->pll_frequency = NIM_DEFAULT_FREQ;

  if (p_nim->tuner_crystal_freq == 0UL)
	p_nim->tuner_crystal_freq = NIM_DEFAULT_XTAL; 
 
  if ((p_nim->tuner_crystal_freq) /MM < 20) 
  { 
    factor = 1; 
  } 
  else 
  { 
    factor = 2; 
  } 
 
	/* Set lo divider to tuner and nim. */ 
	if (p_nim->tuner_type == CX24113) 
	{ 
		if (((p_nim->pll_frequency) / MM) >= CX24113_LO_DIV_BREAKPOINT) 
		{ 
			vcodiv = VCODIV2; 
			if (TUNER_CX24128_SetVcoDivider(p_nim,vcodiv) != True) return(False); 
		} 
		else 
		{ 
			vcodiv = VCODIV4; 
			if (TUNER_CX24128_SetVcoDivider(p_nim,vcodiv) != True) return(False); 
		} 
	} 
	else 
	{ 
		if (((p_nim->pll_frequency) / MM) >= CX24128_LO_DIV_BREAKPOINT) 
		{ 
			vcodiv = VCODIV2; 
			if (TUNER_CX24128_SetVcoDivider(p_nim,vcodiv) != True) return(False); 
		} 
		else 
		{ 
			vcodiv = VCODIV4; 
			if (TUNER_CX24128_SetVcoDivider(p_nim,vcodiv) != True) return(False); 
		} 
	}   
 
  /* Set reference divider to nim. */ 
  TUNER_CX24128_SetReferenceDivider(p_nim,RDIV_1); 
 
  R = p_nim->tuner.cx24128.R; 
 
  /* calculate tuner PLL settings: */ 
  /* Calculate N first. No need to use BCD functions. */   
  N = (p_nim->pll_frequency / 100UL * vcodiv) * R; 
  N /= ((p_nim->tuner_crystal_freq /M * factor) * 2UL); 
  N += 5UL;     /* For round up. */ 
  N /= 10UL; 
  N -= 32UL; 
 
  /* N has to be >= than 6. */ 
  if (N < 6) 
  { 
    /* Set reference divider to nim. */ 
    TUNER_CX24128_SetReferenceDivider(p_nim,RDIV_2); 
 
    R = p_nim->tuner.cx24128.R; 
 
    /* Calculate N again. No need to use BCD functions. */   
    N = (p_nim->pll_frequency * vcodiv / 100UL) * R; 
    N /= ((p_nim->tuner_crystal_freq * factor / M) * 2UL); 
    N += 5UL;     /* For round up. */ 
    N /= 10UL; 
    N -= 32UL; 
 
    if (N < 6) 
    { 
      return(False); 
    } 
  } 
 
  /* Now calculate F. */ 
  BCD_set(&bcd,p_nim->pll_frequency); 
  BCD_mult(&bcd,((unsigned long)R * (unsigned long)vcodiv * CX24128_DIVIDER)); 
  BCD_div(&bcd,(p_nim->tuner_crystal_freq * factor * 2UL)); 
  F = BCD_out(&bcd); 
  F -= (N + 32) * CX24128_DIVIDER; 
 
  if (FWindowEnable == True) 
  { 
    if (F > (CX24128_DIVIDER / 2 - CX24128_F_SIDE_WINDOW)) 
    { 
      F = CX24128_DIVIDER / 2 - CX24128_F_SIDE_WINDOW; 
    } 
    if (F < (-CX24128_DIVIDER / 2 + CX24128_F_SIDE_WINDOW)) 
    { 
      F = -CX24128_DIVIDER / 2 + CX24128_F_SIDE_WINDOW; 
    } 
    if ((F < CX24128_F_0_WINDOW && F > 0)|| (F > -CX24128_F_0_WINDOW && F < 0)) 
    { 
      F = 0; 
         /* Enable ps_test mode. */ 

	  if (RegisterWrite(p_nim, CX24128_PS_TEST, 1, p_nim->tuner.cx24128.io_method) != True)
	   return (False);
    } 
  } 
 
  *nvalue = (unsigned short)N; 
  *fvalue = (int)F; 
 
  p_nim->tuner.cx24128.N = *nvalue;  
  p_nim->tuner.cx24128.F = *fvalue;  
 
  return(True); 
}  /* TUNER_CX24128_calc_pllNF() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_calculateNFR() - GUI only 
 * function to calc pll settings (n,f) using bcd functions  
 * input is the specified frequency and specified R value, the output is 
 * N, F values to be displayed in GUI. 
 *******************************************************************************************************/ 
BOOL            
TUNER_CX24128_CalculateNFR(NIM            *p_nim,   /* nim pointer */ 
                           unsigned long  Fdesired, /* desired frequency */ 
                           RDIVVAL        R,        /* input R value */ 
                           unsigned int   *nvalue,  /* returned N value */ 
                           long           *fvalue)  /* returned F value */ 
{ 
  unsigned long  N; 
  long           F; 
  unsigned long  vcodiv; 
  unsigned char  factor; 
  BCDNO          bcd; 
 
  if ((p_nim->tuner_crystal_freq / MM) < 20) 
  { 
    factor = 1; 
  } 
  else 
  { 
    factor =2; 
  } 
 
	if (p_nim->tuner_type == CX24113) 
	{ 
		if ((Fdesired / MM) >= CX24113_LO_DIV_BREAKPOINT) 
		{ 
			vcodiv = 2; 
		} 
		else 
		{ 
			vcodiv = 4; 
		} 
	} 
	else 
	{ 
		if ((Fdesired / MM) >= CX24128_LO_DIV_BREAKPOINT) 
		{ 
			vcodiv = 2; 
		} 
		else 
		{ 
			vcodiv = 4; 
		} 
	} 
 
  /* Calculate N first. No need to use BCD functions. */   
  N = (Fdesired * R / 100UL) * vcodiv; 
  N /= ((p_nim->tuner_crystal_freq * factor / M) * 2UL); 
  N += 5UL;     /* For round up. */ 
  N /= 10UL; 
  N -= 32UL; 
 
  /* Now calculate F. */ 
  BCD_set(&bcd,Fdesired); 
 
  BCD_mult(&bcd,(vcodiv * R * CX24128_DIVIDER)); 
  BCD_div(&bcd,(p_nim->tuner_crystal_freq * factor * 2UL)); 
 
  F = BCD_out(&bcd); 
  F -= (N + 32) * CX24128_DIVIDER; 
 
  // if (FWindowEnable == True) 
  { 
    if (F > (CX24128_DIVIDER / 2 - CX24128_F_SIDE_WINDOW)) 
    { 
      F = CX24128_DIVIDER / 2 - 1; 
    } 
    if (F < (-CX24128_DIVIDER / 2 + CX24128_F_SIDE_WINDOW)) 
    { 
      F = -CX24128_DIVIDER / 2; 
    } 
    if ((F < CX24128_F_0_WINDOW && F > 0)|| (F > -CX24128_F_0_WINDOW && F < 0)) 
    { 
      F = 0; 
    } 
  } 
 
  *nvalue = (unsigned short)N; 
  *fvalue = F; 
 
  return(True); 
}  /* TUNER_CX24128_CalculateNFR() */ 
 
/******************************************************************************************************* 
 * TUNER_CX24128_VALIDATE()  
 * performs nim and other validation prior to tuner operations  
 *******************************************************************************************************/ 
#if INCLUDE_DEBUG
BOOL
TUNER_CX24128_validate(NIM   *p_nim)  /* pointer to nim */  
{ 
  if (p_nim == 0) 
  { 
    DRIVER_SET_ERROR(p_nim,API_NIM_NULL); 
    return(False); 
  } 
 
  /* validate that the tuner-type is valid */ 
  if (p_nim->tuner_type == CX24128 || p_nim->tuner_type == CX24113)
  { 
	  return(True); 
  } 
 
  /* tuner setting not supported */ 
  DRIVER_SET_ERROR(p_nim,API_NOTSUPPORT); 
 
  return(False); 
}  /* TUNER_CX24128_validate() */ 
#endif /* INCLUDE_DEBUG */ 
 
#endif 
 
/* CR 9509 : Add an extra newline */
