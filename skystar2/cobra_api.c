/* cobra_api.c */

#include "cobra.h"                     /* Cobra include files: ordered */
#include "cobra_regs.h"                /* Cobra Internal */

#if INCLUDE_ROSIE
extern BOOL TUNER_CX24108_SetVCOEdges(NIM *nim,VCO_EDGE *vcoedge);
#endif /* INCLUDE_ROSIE */

/*******************************************************************************************************/
/* static arrays visible to this file only */
/*******************************************************************************************************/
//static
BOOL API_AddNim(NIM* p_nim);

static  SAMPFRQ  sampfrq_list[] =   {SAMPLE_FREQ_NOM,SAMPLE_DCII_NOM,SAMPLE_FREQ_UNDEF};
static  unsigned long sampfrq_eqlist[] = {SAMPLE_FREQ_NOM_VAL,SAMPLE_DCII_NOM_VAL,SAMPLE_FREQ_ENDLIST};

/*******************************************************************************************************/
/* API_InitEnvironment() */
/* function to initialize a NIM* passed by application */
/*******************************************************************************************************/
BOOL           
API_InitEnvironment(NIM	*nim,                              /* nim pointer used by all API functions */
	unsigned long	demodhandle,                       /* demod handle to be associated with this NIM (user-specific) */
		WRITE_SB  SBwrite,                           /* pointer to user-supplied low-level read I/O function */
		READ_SB   SBread,                            /* pointer to user-supplied low-level read I/O function */	
		BOOL (*TUNER_install)(NIM *nim),      /* function that will install a specific tuner  */
		unsigned long    crystalfreq,                       /* crystal (,freq of) attached to NIM card */
		VCOINIT   vcoinit,                           /* TRUE if VCO edges are to be detected for tuner attached to present NIM */
		MPEG_OUT  *mpeg,                       /* default MPEG settings */
		LNBMODE   *lnbmode,                    /* pointer to struct that will set initial settings for LNB (if NULL, defaults are used) */
		BOOL (*waitfunct)(NIM *nim,int mscount),
		void *ptuner)					/* pointer to user-defined wait function (if null, default function is used) */
{
	unsigned long  ulRegVal;
	unsigned char bytData;

	/* perform a couple of stupidity tests... */
	if (nim == (NIM*)NULL)
	{
		DRIVER_SET_ERROR(nim,API_INVALID_NIM);
		return(False);
	}

	if (DRIVER_ValidNim(nim) == True)
	{
		/* invalid nim or already allocated */
		DRIVER_SET_ERROR(nim,API_NIM_OPENED);
		return(False);
	}

	/* clear the entire nim struct */
	memset(nim,CNULL,sizeof(NIM));

	/* un-install any previously installed wait function (The correct calling pattern */
	/* --> for two-nim sys:  Init(1),SetWait(1); Init(2), SetWait(2) ) */
	if (API_SetDriverWait(nim,waitfunct) == False)  
	{
		return(False);
	}

	/* reset various nim fields */
	nim->berbusy = -1;
	nim->bytebusy = -1;
	nim->blockbusy = -1;
	nim->pnberbusy = -1;
	nim->pnberpolarity = 0x01L;
	nim->CLKSMDIV_flag = CLKSMOOTHDIV_UPDATE;
	nim->CLKSMDIV_CR = CODERATE_NONE;

	/* clear the esno fields */
	memset(nim->esno.esno_taps,CNULL,sizeof(nim->esno.esno_taps));
	nim->esno.esno = 0;
	nim->esno.last_esno = -1L;
	nim->esno.taps_idx = 0;
	nim->esno.table = NULL;

#if INCLUDE_DEBUG
	/* indicate no present error */
	nim->__errno = API_NOERR;
#endif /* INCLUDE_DEBUG */

	/* No tuner selection */
	if (TUNER_install == 0)
	{
		DRIVER_SET_ERROR(nim, API_INIT_TUNER);
		return(False);
	}

	nim->opt_fs_disable = False;
#ifdef CAMARIC_FEATURES
	if (DRIVER_Camaric(nim) == True)
	{
		/* for Camaric chip, disable fs optimization for Cobra by default */
		nim->opt_fs_disable = True;
	}
#endif /* #ifdef CAMARIC_FEATURES */

	nim->pll_frequency = NIM_DEFAULT_FREQ;
	nim->lnboffset = NIM_DEFAULT_LNB;
	nim->symbol_rate_ideal = NIM_DEFAULT_SYMB;
	nim->symbol_rate = NIM_DEFAULT_SYMB;
	nim->prevstate = ACQ_OFF;

	nim->tuner_offset = 0UL;             /* (CR 6243) */
	nim->actual_tuner_offset = 0L;       /* (CR 6243) */
	nim->swa_count = 0;                  /* (CR 6243) */

	/* place the new nim into the NIM_list */
	API_AddNim(nim);

	/* Verify register map */
	REGMAP_TEST(nim);

	/* save the demod handle, i/o function pointers */
	nim->demod_handle = demodhandle;
	nim->SBWrite = SBwrite;
	nim->SBRead = SBread;
	nim->ptuner = ptuner;

	/* flag bad sbread/sbwrite functions as errors */
	if (nim->SBWrite == NULL || nim->SBRead == NULL)
	{
		DRIVER_SET_ERROR(nim,API_SBIO_NULL);
		return(False);
	}

	/* grab chip version */
	RegisterRead(nim,CX24130_SYSVERSION,&nim->version, DEMOD_I2C_IO);

	/* test for valid crystal */
	if (crystalfreq < (unsigned long)(FREQ_XTAL_LOW*MM) || crystalfreq > (unsigned long)(FREQ_XTAL_HIGH*MM))
	{
		/* xtal presented at init...() is out-of-bounds */
		DRIVER_SET_ERROR(nim,API_INIT_XTAL);
		return(False);
	}

	nim->xtal = crystalfreq;
	
        if (crystalfreq <= 20000000)
        {
            nim->crystal_freq = crystalfreq;
            nim->tuner_crystal_freq = nim->crystal_freq;
            nim->sample_freq_less_than_4msps = 16 * (nim->tuner_crystal_freq * nim->crystal_freq) / 6;
            nim->sample_freq_nom_val = 59 * (nim->tuner_crystal_freq * nim->crystal_freq) / 6;

        } else
        {
           nim->crystal_freq = crystalfreq/2;
           nim->tuner_crystal_freq = nim->crystal_freq;
           nim->sample_freq_less_than_4msps = 16 * (nim->tuner_crystal_freq * nim->crystal_freq) / 12;
           nim->sample_freq_nom_val = 59 * (nim->tuner_crystal_freq * nim->crystal_freq) / 12;
        }

	/* Viper uses 40Mhz crystal and 10Mhz, in the case of 40Mhz, demod
	 * uses half the frequency. Since Mustang/Rosie always uses <= 20Mhz,
	 * the below calculations should be O.K. with Rosie. */
	// Overide - Using dual xtals for Virgo board.
	//nim->xtal		= 10000000;
	//nim->crystal_freq	= 10000000;
	//nim->tuner_crystal_freq = 10111000;

	/* When XTAL freq < 20MHz */
	//nim->sample_freq_less_than_4msps = (SAMPLE_FREQ_LT_4MSPS_PLL_MULT * nim->crystal_freq) / 6;
	//nim->sample_freq_nom_val         = (SAMPLE_FREQ_NOM_VAL_PLL_MULT  * nim->crystal_freq) / 6;

	sampfrq_eqlist[0] = nim->sample_freq_nom_val;
	sampfrq_eqlist[1] = SAMPLE_DCII_NOM_VAL;
	sampfrq_eqlist[2] = SAMPLE_FREQ_ENDLIST;

#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
	if (DRIVER_Opt_Fs_buildtable(nim) == False)
		return(False);
#endif  /* #ifdef OPTIMAL_FS_CODE */
  
	/* determine the type of demod */
	if (DRIVER_CxType(nim,&nim->demod_type,NULL) != True)  
	{
		return(False);
	}

	nim->register_offset = 0;

        if (nim->demod_type == CX24130)
        {
		nim->register_offset = GET_CX24128_BTI_REGISTER_OFFSET(nim->demod_handle);
        }

	/* place the demod into a known state (if demod is unknon, record a warning) */
	if (DRIVER_Preset(nim) != True)  
	{
		DRIVER_SET_ERROR(nim,API_CXTYPE);
	}

	if (DRIVER_Reset(nim) != True)  
	{
		return(False);
	}

	/* initialize PLLMult shadow with default value */
	nim->ucPLLMult_shadow = (unsigned char)demod_register_map[CX24130_PLLMULT].default_value;

	/* initialize the pdmfout register gain setting with the default power on value */	
	ulRegVal = 0UL;
	if(RegisterRead(nim, CX24123_DMDSAMPLEGAIN, &ulRegVal, DEMOD_I2C_IO) == FALSE) return(FALSE);
	nim->pdmfout = ulRegVal;

	/* Install the tuner specified by the user */
	if ((*TUNER_install)(nim) == False)
	{
		DRIVER_SET_ERROR(nim,API_INIT_TUNER);
		return(False);
	}

#if INCLUDE_ROSIE
	nim->TUNER_io_method = TUNERIO_UNDEF;

	if (nim->tuner_type == CX24108)
	{
		nim->TUNER_io_method = TUNER_BURST; /* default tuner i/o method */
	}
#endif /* INCLUDE_ROSIE */

	/* test that a valid tuner (i.e. function pointer!=NULL) was installed */
	if (TUNER_Initialize == NULL)
	{
		DRIVER_SET_ERROR(nim,API_INIT_TUNER);
		return(False);
	}

	/* initialize the tuner specified by the caller in TUNER_install passed into this function */
	if ((*TUNER_Initialize)(nim) == False)
	{
		DRIVER_SET_ERROR(nim,API_INIT_TUNER);
		return(False);
	}
	
#if INCLUDE_ROSIE
	if (vcoinit == True && nim->tuner_type == CX24108)
	{
		/* find tuner VCO edges */
		if (API_FindVCOEdges(nim, DEFAULT_RDIVVAL) == False)  
		{
			return(False);
		}
	}
#else
	vcoinit = False;
#endif

	/* initiate MPEG system */
	nim->temp_SyncPunctMode = -1;
	if (mpeg == NULL)
	{
		DRIVER_SET_ERROR(nim,API_INIT_MPEG);
		return(False);
	}

	if (API_SetOutputOptions(nim,mpeg) == False)  
	{
		return(False);
	}

	/* set the LNB mode parameters */
	if (API_SetLNBMode(nim,lnbmode) == False)  
	{
		return(False);
	}

#if INCLUDE_BANDWIDTHADJ
        nim->unk560 = False;
	nim->tuner_bw_adjust = False;
#endif

	return(True);
}  /* API_InitEnvironment() */


/*******************************************************************************************************/
/* API_ChangeChannel() */
/* High-level function to perform channel-change operation */
/*******************************************************************************************************/
BOOL     
API_ChangeChannel(NIM      *nim,                      /* nim pointer */
                 CHANOBJ  *chanobj)
				  
{
  long      acqoffset;
  long      tuneroffset;
  long      freqoffset;
#if INCLUDE_RATTLER
  long      div_cnt_val = 0;
  long      freq_val = 0;
#endif  
  unsigned char byte_data;
  unsigned int  vrates;        /* temp viterbi search rates */

	DRIVER_VALIDATE_NIM(nim);

	/* if chanobj is bad, bail immediately */
	if (chanobj == NULL)
	{
		DRIVER_SET_ERROR(nim,API_BAD_PARM);
		printf("API_ChangeChannel: 10\n");
		return(False);
	} 
	
	/* set the transport spec */
	if (nim->tspec != chanobj->transpec)
	{
		if (API_SetTransportSpec(nim,chanobj->transpec) == False)  
		{
			printf("API_ChangeChannel: 20\n");
			return(False);
		}
	}

	if (RegisterWrite(nim, CX24130_AGCTHRESH, demod_register_map[CX24130_AGCTHRESH].default_value, DEMOD_I2C_IO) == False)
	{
		printf("API_ChangeChannel: 30\n");
		return(False);
	}

  /* (CR 7957) */
  nim->CLKSMDIV_flag = CLKSMOOTHDIV_UPDATE;
  nim->CLKSMDIV_CR = CODERATE_NONE;
  /* if locked to a signal, the acq-offset will be valid, otherwise use 0 */
  nim->actual_tuner_offset = 0L;  /* (CR 6243) */
  acqoffset = 0L;

//kir
  switch (chanobj->unk28)
  {
  case 0:
        nim->unk50 = 600;
        break;
  case 1:
        nim->unk50 = 625;
        break;
  case 2:
        nim->unk50 = 675;
        break;
  default:
	printf("API_ChangeChannel: 40\n");
        return(False);

  }

  /* (CR 7672) added line below */
  if (API_SetSymbolRate(nim,chanobj->symbrate) == False)
  {
	printf("API_ChangeChannel: 50\n");
	return(False);
  }
  /* Set optimal preload values for the auto-tuning system.*/
  /*CR 29373: Change LO Breakpoint from 1165MHz to 1100MHz. Change the frequency where the preload value changes from 1165MHz to 1100MHz.*/
  /*    We just change the following constant from 1165 to 1100*/
#if INCLUDE_RATTLER  
  if (nim->tuner_type == CX24113)  
  {   
       div_cnt_val = 1024;       
       /* >= 950 && <= 1100 || >= 1800 && <= 2150 */       
       if ( (chanobj->frequency >= CX24113_PRELOAD_VALUE_FREQ_THRESH1_KHZ && chanobj->frequency <= CX24113_PRELOAD_VALUE_FREQ_THRESH2_KHZ) ||
       (chanobj->frequency >= CX24113_PRELOAD_VALUE_FREQ_THRESH3_KHZ && chanobj->frequency <= CX24113_PRELOAD_VALUE_FREQ_THRESH4_KHZ) )
       {       
           freq_val = 1000;                         
       } /* > 1100 && < 1800 */  
       else   
       {  
           freq_val = 1024;  
       }
       
       if (RegisterWrite(nim, (unsigned short)(CX24128_BS_FREQ), freq_val, nim->tuner.cx24128.io_method) != True)  
       {  
   	     printf("API_ChangeChannel: 60\n");
            return(False);  
       }
       
       if (RegisterWrite(nim, (unsigned short)(CX24128_BS_DIV_CNT), div_cnt_val, nim->tuner.cx24128.io_method) != True)  
       {
   	     printf("API_ChangeChannel: 70\n");
            return(False);
       }  
  }
#endif

	if (chanobj->unk2C == 0)
	{
		tuneroffset = 0;
		if (API_SetTunerFrequency(nim,chanobj->frequency) == False)
		{
			printf("API_ChangeChannel: 80\n");
			return(False);
		}
	} else
	{
		if (API_SetTunerFrequency(nim,chanobj->frequency+chanobj->unk30) == False)
		{
			printf("API_ChangeChannel: 90\n");
			return(False);
		}
		if (API_GetFrequencyOffset(nim,&tuneroffset) == False)
		{
			printf("API_ChangeChannel: 100\n");
			return(False);
		}
		freqoffset = acqoffset + tuneroffset;
	}             

	if (API_AcqBegin(nim) == False)
	{
		printf("API_ChangeChannel: 110\n");
		return(False);
	}
	if (API_SetSpectralInversion(nim,chanobj->specinv) == False)
	{
		printf("API_ChangeChannel: 120\n");
		return(False);
	}
	if (API_SetLNBDC(nim,chanobj->lnbpol) == False)
	{
		printf("API_ChangeChannel: 130\n");
		return(False);
	}
	if (API_SetLNBTone(nim,chanobj->lnbtone) == False)
	{
		printf("API_ChangeChannel: 140\n");
		return(False);
  	}
	/* set the Viterbi code rate settings  */
	vrates = (chanobj->viterbicoderates | chanobj->coderate);
	if (API_AcqSetViterbiCodeRates(nim,vrates) == False)
	{
		printf("API_ChangeChannel: 150\n");
		return(False);  /* new style */
	}

	/* (CR7349) 4/23/02 cw */
	if (DRIVER_SWAssistAcq_CR1DIV2(nim,vrates) == False)
	{
		printf("API_ChangeChannel: 160\n");
		return(False);
	}
	if (API_SetModulation(nim,chanobj->modtype) == False)
	{
		printf("API_ChangeChannel: 170\n");
		return(False);
	}
	if (API_SetViterbiRate(nim,chanobj->coderate) == False)
	{
		printf("API_ChangeChannel: 180\n");
		return(False);
	}
	/* if external sample freq is selected, do not touch the sample freq register */
	if (chanobj->samplerate != SAMPLE_FREQ_EXT)
	{
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
		if (nim->opt_Fs_pllmult != 0 && nim->opt_fs_disable == False)
		{
			if (__API_SetSampleFrequency(nim,nim->opt_Fs_chosen) == False)
			{
				printf("API_ChangeChannel: 190\n");
				return(False);
			}
		}
		else
#endif  /* #ifdef OPTIMAL_FS_CODE */
		if (API_SetSampleFrequency(nim,chanobj->samplerate) == False)
		{
			printf("API_ChangeChannel: 200\n");
			return(False);
		}
#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
		/* set default Fs as one chosen */
		/* (CR 7672) line below commented */
		/* --> if (API_GetAssociatedSampleFrequency(nim,chanobj->samplerate,&nim->opt_Fs_chosen) == False)
			return(False); */
	}
	else
	{
		/* Fs is externally set, so get current default Fs from by reading the demod */
		if (API_GetSampleFrequency(nim,&nim->opt_Fs_chosen) == False)
		{
			printf("API_ChangeChannel: 210\n");
			return(False);
		}
#endif  /* #ifdef OPTIMAL_FS_CODE */
	}

	/* (CR 7672) removed line below */
	if (API_SetSymbolRate(nim,chanobj->symbrate) == False)
	{
		printf("API_ChangeChannel: 220\n");
		return(False);
	}
	/* successful channel change operation, save copy in the nim */
	memcpy(&nim->chanobj,chanobj,sizeof(CHANOBJ));

	/* perform SW assist as required */
	if (DRIVER_SWAssistAcq(nim) == False)
	{
		DRIVER_SET_ERROR(nim,API_BAD_SWA);
		printf("API_ChangeChannel: 230\n");
		return(False);
	}

	/* (CR 6243) */
	if(DRIVER_SWAssistTuner(nim) == False)
	{
		DRIVER_SET_ERROR(nim,API_BAD_SWA);
		printf("API_ChangeChannel: 240\n");
		return(False);
	}

	/* (CR6838) clear the esno average buffer */
	if (API_GetChannelEsNo(nim,ESNOMODE_UNDEF,NULL,NULL) == False)
	{
		printf("API_ChangeChannel: 250\n");
		return(False); 
	}
	/* set the clock smoother freq if required (otherwise, clksmooth freq will be set at API_SetOutputOptions() ) */
	if (nim->mpeg_out.ClkSmoothSel == DDS_LEGACY_SMOOTHING)
        {
		if (DRIVER_SetSmoothClock(nim,nim->mpeg_out.ClkSmoothSel,True) == False)
		{
			printf("API_ChangeChannel: 260\n");
			return(False);
		}
	}
#ifdef CAMARIC_FEATURES
        else if (nim->mpeg_out.ClkSmoothSel == PLL_ADVANCED_SMOOTHING)
	{
		if (DRIVER_SetSmoothClock(nim,nim->mpeg_out.ClkSmoothSel,True) == False)
		{
			printf("API_ChangeChannel: 270\n");
			return(False);
		}
	}
#endif /* #ifdef CAMARIC_FEATURES */

#if INCLUDE_BANDWIDTHADJ
        nim->unk560 = True;
	nim->tuner_bw_adjust = False;//True
#endif

	return(True);

}  /* API_ChangeChannel() */


/*******************************************************************************************************/
/* API_Monitor() */
/*******************************************************************************************************/
BOOL      API_Monitor(                 /* Monitors the demod after a successful channel change operation */
NIM       *nim,                        /* pointer to NIM */
ACQSTATE  *state,                      /* storage for returned acqusition state of demod */
LOCKIND   *lockinds)                   /* storage for returned demod lock indicators */
{
  unsigned long  locked;

#if INCLUDE_BANDWIDTHADJ  
  long			 lnboffset;
  unsigned long  bandwidth;
  long		     sigmadelta;

  SYMBRATE symbol_rate;
  TRANSPEC transpec;
#endif
   
	/* test for valid nim */
	DRIVER_VALIDATE_NIM(nim);//1

	if (state == NULL || lockinds == NULL)  
	{
		DRIVER_SET_ERROR(nim,API_BAD_PARM);
		return(False);//0xA
	}

	/* if an i/o error occurred during last i/o attempt, do not allow a Monitor deadlock condition to occur */
	if (nim->iostatus != 0UL)
		return(False);//0x14

	/* retrieve  lock indicators */
	if (API_GetLockIndicators(nim,lockinds) == False)
		return(False);//0x1E

	/* give some time to the acq engine */
	if (API_AcqContinue(nim,state) == False)
		return(False);//0x28

	/* perform SW assist acq when NOT locked*/
	if (*state == ACQ_SEARCHING || *state == ACQ_FADE)
	{
          /* perform SW assist as required */
          if (DRIVER_SWAssistAcq(nim) == False)
          {
             DRIVER_SET_ERROR(nim,API_BAD_SWA);
             return(False);//0x32
          }

          /* re-read the acq state after we re-gained lock */
          if (API_AcqContinue(nim,state) == False)
		return(False);//0x3C
	}

	/* retrieve  lock indicators */
	if (API_GetLockIndicators(nim,lockinds) == False)
		return(False);//0x46

	/* perform performance changes once */
	if (lockinds->reedsolomon == TRUE)
	{
		/* We are locked. */
		if (nim->prevstatecounter == 0)
		{
			if (DRIVER_SetSoftDecisionThreshold(nim) == False)
			{
				return (False);//0x50
			}

			nim->prevstatecounter = 1;
		}
	}
	else
	{
		/* we are NOT locked, set the state as such. */
		nim->prevstatecounter = 0;
	}
  
  /* (CR 6243) */
  if(DRIVER_SWAssistTuner(nim) == False)
  {
    DRIVER_SET_ERROR(nim,API_BAD_SWA);
    return(False);//0x5A
  }

#if INCLUDE_BANDWIDTHADJ
	if (*state == ACQ_LOCKED_AND_TRACKING) /* narrow */
	{
          if (nim->unk560 == True)
          {
		if (API_GetAcquisitionOffset(nim,&lnboffset) != True)
			return(False);//0x64
		if (API_GetSymbolRate(nim, &symbol_rate) != True)
			return(FALSE);//0x6E

		API_GetTransportSpec(nim, &transpec);
		switch(transpec)
		{
		case  SPEC_DVB:
			//kir??
    			bandwidth = (((((symbol_rate/100UL) * nim->unk50)/10UL)/100UL) + (unsigned long)labs(lnboffset/100UL) + 5)/10UL;
			API_SetTunerBW(nim, (bandwidth+1000UL), &sigmadelta); /* Add 500kHz for rounding */
			break;
		default:
			break;
		}      

                nim->unk560 = False;
		nim->tuner_bw_adjust = True;
		//nim->shadowed_tuner_lnboffset = 0; /* next acquisition should re-program */ 
		//nim->shadowed_tuner_symbrate  = 0; /* next acquisition should re-program */
          }
	}
	else if (*state == ACQ_FADE) /* open */
	{
		if (nim->tuner_bw_adjust == True) // just do it the moment fade is detected
		{            
			API_SetTunerBW(nim, nim->anti_alias_bandwidth, &sigmadelta); /* use shadowed value */
			nim->unk560 = True;
			nim->tuner_bw_adjust = False; 
		}
	}

#endif /* COBRA_INCLUDE_BANDWIDTHADJ */

	/* (CR 9436) set the clksmoothdiv only if required.  (after Chan.Change or after lock/unlock transition) */
	if (nim->CLKSMDIV_flag == CLKSMOOTHDIV_UPDATE)
	{
		/* test for first API_Monitor after change-channel.  If so, test lock */
		if (RegisterRead(nim,CX24130_ACQFULLSYNC,&locked, DEMOD_I2C_IO) == False)
			return(False);//0x78

		if (locked == 0x01UL)
		{
			/* Demod is locked.  This is first API_Monitor after Chan.Change, so set ClkSmootherDiv rate */
			/* when the clock smoother is turned on */
			if (nim->mpeg_out.ClkSmoothSel == DDS_LEGACY_SMOOTHING   /* CR9436 */
#ifdef CAMARIC_FEATURES
			|| nim->mpeg_out.ClkSmoothSel == PLL_ADVANCED_SMOOTHING)
#else
			)
#endif
			{
				unsigned long  nominal;
				unsigned long  curr;
						 
				/* Read the Current Viterbi code rate */
				if (RegisterRead(nim,CX24130_ACQVITCURRCR,&curr, DEMOD_I2C_IO) != True)
					return(False);//0x82

				/* Read the nominal Viterbi code rate */
				if (RegisterRead(nim,CX24130_ACQVITCRNOM,&nominal, DEMOD_I2C_IO) != True)
					return(False);//0x8C

				/* CR9436: - No need to set ClkSmoothFreqDiv if code rates are equal */
				if (curr != nominal)
				{
					if (DRIVER_SetSmoothClock(nim,nim->mpeg_out.ClkSmoothSel,False) == False)
						return(False);//0x96
				}  
				/* cause API_Monitor() to not adjust clk smooth div until next chan.change */
				nim->CLKSMDIV_flag = CLKSMOOTHDIV_PASS;
			}
		}
	}

	return(True);

}  /* API_Monitor() */


/*******************************************************************************************************/
/* NIMGetChipInfo() */
/*******************************************************************************************************/
BOOL      NIMGetChipInfo(          /* function to return NIM (demod+tuner) info to caller */
NIM       *nim,                        /* pointer to NIM */
char      **demod_string,              /* returns name of demod to caller */
char      **tuner_string,              /* returns name of tuner associated to demod */
int       *demod_type,                 /* returns demod type (aka chip version) to caller */
int       *tuner_type,                 /* returns tuner type to caller */
int       *board_type)                 /* returns user board-type info (aka board version) to caller */
{
  unsigned long  ulRegVal = 0;

  //static int   board_ver = -1;
  static char  *demods[3] = {NULL,NULL,NULL};
  static char  *tuners[5] = {NULL,"CX24108","CX24128","CX24113",NULL};

  BOOL   rtn = 0;
  DEMOD  demod;
  
  /* set the default driver product id */
  demods[1] = PRODUCT_NAME_STRING;

  DRIVER_VALIDATE_NIM(nim);

  /* determine the type of demod, if possible */
  if (DRIVER_CxType(nim,&demod,&demods[1]) == False)  return(False);

  *board_type = 0x00;

    /* read demod type from chip, zero, then return if error */
    rtn = RegisterRead(nim,CX24130_SYSVERSION,&ulRegVal, DEMOD_I2C_IO);
    *demod_type = (int)ulRegVal;
    if (rtn == False)  return(False);

    /* grab the tuner type from the nim */
    *tuner_type = (int)nim->tuner_type;

    /* save the strings: tuner-type and demod string */
    *demod_string = demods[1];  
    *tuner_string = tuners[nim->tuner_type];
    nim->tuner_str = tuners[nim->tuner_type];              /* save the tuner string */

  return(True);

}  /* NIMGetChipInfo() */


/*******************************************************************************************************/
/* BOOL  API_GetDriverVersion() */
/*******************************************************************************************************/
BOOL   API_GetDriverVersion(           /* function to return driver version data to caller */
NIM    *nim,                           /* pointer to nim */
VERDRV *verdrv)                        /* pointer to address where version string struct will be stored */
{
  int   i;

  if (nim == NULL || verdrv == NULL)  return(False);

  /* place a copy of the driver version into user-storage */
  memset(verdrv,CNULL,sizeof(VERDRV));
  strncpy(verdrv->version_str,PRODUCT_VERSION_STRING,(sizeof(VERDRV)-1));

  /* extract the minor-version from the version string, save to nim */
  nim->version_minor = 0;
  for (i = MAX_VERLEN -1 ; i > 0 ; i--)
  {
    if (verdrv->version_str[i] != CNULL)
    {
      for ( ; i > 0 ; i--)
      {
        if (isdigit((int)(verdrv->version_str[i])) == 0)
        {
          //nim->version_minor = atoi(&verdrv->version_str[i+1]);
          nim->version_minor = strtol(&verdrv->version_str[i+1], NULL, 10);
          break;
        }
      }
      break;
    }
  }

  return(True);

}  /* BOOL  API_GetDriverVersion() */

/*******************************************************************************************************/
/* API_AddNim() */
/*******************************************************************************************************/
BOOL  API_AddNim(NIM* p_nim)	 /* Function to add a nim pointer to the nim list */
{
    int i;

    if (p_nim == 0) 
    {
	return False;
    }

    /* place the new nim into the NIM_list */
    for (i = 0 ; i < MAX_NIMS ; i++)
    {
        if (nim_list.nim[i] == NULL)
        {
            nim_list.nim[i] = p_nim;
            nim_list.nim_cnt += 1;
            break;
        }
    }
   return(True);
}

/*******************************************************************************************************/
/* API_ReleaseEnvironment() */
/*******************************************************************************************************/
BOOL  API_ReleaseEnvironment(          /* Function to "close" an opened NIM */
NIM   *nim)                            /* pointer to opened NIM */
{
  int  i;

  /* release a previously-saved nim (gives example of how to pre-test nims) */
  if (DRIVER_ValidNim(nim) == True)
  {
    for (i = 0 ; i < MAX_NIMS ; i++)
    {
      if (nim_list.nim[i] == nim)
      {
        /* release the nim from the stored list */
        nim_list.nim[i] = NULL;
        nim_list.nim_cnt -= 1;

        /* clear the entire nim struct */
        memset(nim,CNULL,sizeof(NIM));
        
        return(True);
      }
    }
  }

  return(False);

}  /* API_ReleaseEnvironment() */



/*******************************************************************************************************/
/* API_SetOutputOptions() */
/*******************************************************************************************************/
BOOL      API_SetOutputOptions(        /* Function to set the demod MPEG output pins */
NIM       *nim,                        /* pointer to nim */
MPEG_OUT  *mpeg_out)                   /* mpeg settings struct */
{
  unsigned long  ulRegVal;

  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  if (mpeg_out == NULL)
  {
	DRIVER_SET_ERROR(nim,API_BADPTR);
  } else
  {
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    /* verify/set the output mode */
    switch(mpeg_out->OutputMode)
    {
      case  PARALLEL_OUT:
      case  SERIAL_OUT:
      {
        /* set l to 0 if par, 1 if serial, bail on error */
        ulRegVal = (unsigned long)((mpeg_out->OutputMode) == PARALLEL_OUT ? BIT_ZERO : BIT_ONE);
        if (RegisterWrite(nim,CX24130_MPGDATAWIDTH,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      case  OUTMODE_UNDEF:
      default:
      {
        /* passed parameter was invalid, flag and bail */
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->ClkOutEdge)
    {
      case  CLKOUT_SETUP1_HOLD7:
      case  CLKOUT_SETUP3_HOLD5:
      case  CLKOUT_SETUP5_HOLD3:
      case  CLKOUT_SETUP7_HOLD1:
      {
        if (mpeg_out->OutputMode == PARALLEL_OUT)  
        {
          ulRegVal = (unsigned long)(mpeg_out->ClkOutEdge&0x0fUL);
          if (RegisterWrite(nim,CX24130_MPGCLKPOS,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
          break;
        }
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
      case  CLKOUT_DATALR_DATACR:
      case  CLKOUT_DATALR_DATACF:
      case  CLKOUT_DATALF_DATACR:
      case  CLKOUT_DATALF_DATACF:
      {
        /* serial out */
        if (mpeg_out->OutputMode == SERIAL_OUT)  
        {
          ulRegVal = (unsigned long)(mpeg_out->ClkOutEdge&0x0fUL);
          if (RegisterWrite(nim,CX24130_MPGCLKPOS,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
          break;
        }
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
      case  CLKOUT_RISING:
      case  CLKOUT_FALLING:
      case  CLKOUT_ENDEF:
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->ClkParityMode)
    {
      case  CLK_CONTINUOUS:
      case  CLK_GAPPED:
      {
        ulRegVal = (unsigned long)(mpeg_out->ClkParityMode == CLK_GAPPED ? 1 : 0);
        if (RegisterWrite(nim,CX24130_MPGGAPCLK,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      case  CLK_PARITY_UNDEF:
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->HoldTime)
    {
      case  ZERO_HOLD_TIME:
      case  SMALL_HOLD_TIME:
      case  MEDIUM_HOLD_TIME:
      case  LARGE_HOLD_TIME:
      {
        switch(mpeg_out->HoldTime)
        {
          case  ZERO_HOLD_TIME:
          case  SMALL_HOLD_TIME:
          {
            ulRegVal = 0x02UL;
            break;
          }
          case  MEDIUM_HOLD_TIME:
          {
            ulRegVal = 0x00UL;
            break;
          }
          case  LARGE_HOLD_TIME:
          {
            ulRegVal = 0x01UL;
            break;
          }
          default:
          {
            break;
          }
        }  /* switch(... */

        /* write the hold-time registers */
        if (RegisterWrite(nim,CX24130_MPGCLKHOLD,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      case  CLK_HOLD_UNDEF:
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->StartSignalPolarity)
    {
      case  ACTIVE_LOW:
      case  ACTIVE_HIGH:
      {
        ulRegVal = (mpeg_out->StartSignalPolarity == ACTIVE_HIGH) ? 0x01UL : 0x00UL;
        if (RegisterWrite(nim,CX24130_MPGSTARTPOL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->StartSignalWidth)
    {
      case  BIT_WIDE:
      case  BYTE_WIDE:
      {
        ulRegVal = (mpeg_out->StartSignalWidth == BIT_WIDE) ? 0x01UL : 0x00UL;
        if (RegisterWrite(nim,CX24130_MPGSTARTMODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->ValidSignalPolarity)
    {
      case  ACTIVE_LOW:
      case  ACTIVE_HIGH:
      {
        ulRegVal = (mpeg_out->ValidSignalPolarity == ACTIVE_HIGH) ? 0x00UL : 0x01UL;
        if (RegisterWrite(nim,CX24130_MPGVALIDPOL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->ValidSignalActiveMode)
    {
      case  ENTIRE_PACKET:
      case  FIRST_BYTE:
      {
        ulRegVal = (mpeg_out->ValidSignalActiveMode == FIRST_BYTE) ? 0x01UL : 0x00UL;
        if (RegisterWrite(nim,CX24130_MPGVALIDMODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->FailSignalPolarity)
    {
      case  ACTIVE_LOW:
      case  ACTIVE_HIGH:
      {
        ulRegVal = (mpeg_out->FailSignalPolarity == ACTIVE_HIGH) ? 0x00UL : 0x01UL;
        if (RegisterWrite(nim,CX24130_MPGFAILPOL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->FailSignalActiveMode)
    {
      case  ENTIRE_PACKET:
      case  FIRST_BYTE:
      {
        ulRegVal = (mpeg_out->FailSignalActiveMode == FIRST_BYTE) ? 0x01UL : 0x00UL;
        if (RegisterWrite(nim,CX24130_MPGFAILMODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->SyncPunctMode)
    {
      case  SYNC_WORD_PUNCTURED:
      case  SYNC_WORD_NOT_PUNCTURED:
      {
        ulRegVal = (mpeg_out->SyncPunctMode == SYNC_WORD_PUNCTURED) ? 0x01UL : 0x00UL;
        if (RegisterWrite(nim,CX24130_MPGSYNCPUNC,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->FailValueWhenNoSync)
    {
      case  FAIL_LOW_WHEN_NO_SYNC:
      case  FAIL_HIGH_WHEN_NO_SYNC:
      {
        ulRegVal = (mpeg_out->FailValueWhenNoSync == FAIL_HIGH_WHEN_NO_SYNC) ? 0x01UL : 0x00UL;
        if (RegisterWrite(nim,CX24130_MPGFAILNSVAL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    switch(mpeg_out->ClkSmoothSel)
    {
#ifdef CAMARIC_FEATURES
      case  PLL_ADVANCED_SMOOTHING:
      {
        if (DRIVER_Camaric(nim) != True)
        {
          /* CR9364, if board is CX24121, switch to DSS mode. */
		  mpeg_out->ClkSmoothSel = DDS_LEGACY_SMOOTHING;
		  /* Set error but continue from here. No break, it will fall through. */
          DRIVER_SET_ERROR(nim,API_SETILLEGAL);
        }
      }
#endif   /* #ifdef CAMARIC_FEATURES */
      case  DDS_LEGACY_SMOOTHING:
      case  CLK_SMOOTHING_OFF:
      {
        if (DRIVER_SetSmoothClock(nim,mpeg_out->ClkSmoothSel,False) == False)  return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }  /* switch(... */
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    if (DRIVER_SetRSCntlPin(nim,CX24130_MPGCNTL1SEL,mpeg_out->RSCntlPin1Sel) == False)
    {
      return (False);
    }
    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    if (DRIVER_SetRSCntlPin(nim,CX24130_MPGCNTL2SEL,mpeg_out->RSCntlPin2Sel) == False)
    {
      return (False);
    }

#ifdef CAMARIC_FEATURES
    /* Camaric specific settings */
    if (DRIVER_Camaric(nim) == True)
    {
      /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
      if (DRIVER_SetRSCntlPin(nim,CX24130_MPGCNTL3SEL,mpeg_out->RSCntlPin3Sel) == False)
      {
        return (False);
      }
      /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     switch (mpeg_out->NullDataMode)
     {
       case  FIXED_NULL_DATA_ENABLED:
       case  FIXED_NULL_DATA_DISABLED:
       {
         ulRegVal = (mpeg_out->NullDataMode == FIXED_NULL_DATA_ENABLED) ? 0x01UL : 0x00UL;
         if (RegisterWrite(nim,CX24123_MPGFIXNULLDATAEN,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
         break;
       }
       default:
       {
         DRIVER_SET_ERROR(nim,API_BAD_PARM);
         return(False);
         break;
       }
     }  /* switch(... */
     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     switch (mpeg_out->NullDataValue)
     {
       case  FIXED_NULL_DATA_HIGH:
       case  FIXED_NULL_DATA_LOW:
       {
         ulRegVal = (mpeg_out->NullDataValue == FIXED_NULL_DATA_HIGH) ? 0x01UL : 0x00UL;
         if (RegisterWrite(nim,CX24123_MPGNULLDATAVAL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
         break;
       }
       default:
       {
         DRIVER_SET_ERROR(nim,API_BAD_PARM);
         return(False);
         break;
       }
     }  /* switch(... */
     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     switch (mpeg_out->ParityDataSel)
     {
       case RS_PARITY_DATA_LOW:
          ulRegVal = 0x01UL;
         break;
       case RS_PARITY_DATA_HIGH:
          ulRegVal = 0x02UL;
          break;
       case RS_PARITY_DATA_UNCHANGED:
          ulRegVal = 0x00UL;
          break;
       default:
          DRIVER_SET_ERROR(nim,API_BAD_PARM);
          return(False);
     } /* switch */

     if (RegisterWrite(nim,CX24123_MPGPARSEL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     switch (mpeg_out->ValidSignalWhenFail)
     {
       case  VALID_SIGNAL_INACTIVE_WHEN_FAIL:
       case  VALID_SIGNAL_ACTIVE_WHEN_FAIL:
       {
         ulRegVal = (mpeg_out->ValidSignalWhenFail == VALID_SIGNAL_INACTIVE_WHEN_FAIL) ? 0x01UL : 0x00UL;
         if (RegisterWrite(nim,CX24123_MPGFAILVALIDDIS,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
         break;
       }
       default:
       {
         DRIVER_SET_ERROR(nim,API_BAD_PARM);
         return(False);
        break;
       }
     }  /* switch(... */
     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     switch (mpeg_out->StartSignalWhenFail)
     {
       case  START_SIGNAL_INACTIVE_WHEN_FAIL:
       case  START_SIGNAL_ACTIVE_WHEN_FAIL:
       {
         ulRegVal = (mpeg_out->StartSignalWhenFail == START_SIGNAL_INACTIVE_WHEN_FAIL) ? 0x01UL : 0x00UL;
         if (RegisterWrite(nim,CX24123_MPGFAILSTARTDIS,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
       }
       default:
       {
         DRIVER_SET_ERROR(nim,API_BAD_PARM);
         return(False);
         break;
       }
      }  /* switch(... */
    }  /* if (DRIVER_Camaric(nim) == True) */
#endif  /* #ifdef CAMARIC_FEATURES */

    /* no errors encountered, indicate such to caller */
    memcpy(&nim->mpeg_out,mpeg_out,sizeof(MPEG_OUT));

    return(True);
  }


  return(False);

}  /* API_SetOutputOptions() */


/*******************************************************************************************************/
/* API_GetOutputOptions() */
/*******************************************************************************************************/
BOOL      API_GetOutputOptions(        /* function to return current mpeg settings to the caller */
NIM       *nim,                        /* pointer to nim */
MPEG_OUT  *mpeg_out)                   /* pointer to where copy of mpeg settings will be written */
{
  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  if (mpeg_out == NULL)
  {
	DRIVER_SET_ERROR(nim,API_BADPTR);
  } else
  {
    /* copy mpeg setting to caller buffer */
    memcpy(mpeg_out,&nim->mpeg_out,sizeof(MPEG_OUT));
    return(True);
  }

  return(False);

}  /* API_GetOutputOptions() */


/*******************************************************************************************************/
/* API_SetInterruptOptions() */
/*******************************************************************************************************/
BOOL       API_SetInterruptOptions(    /* function to set interrupt options */
NIM        *nim,                       /* pointer to nim */
INTEROPTS  interopts)                  /* interrupt settings */
{
  unsigned long  ulRegVal = (unsigned long)interopts;
  unsigned long  valid_ints;

  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  /* test for valid interrupt settings */
  valid_ints = ((unsigned long)INTR_ACQ_SYNC|(unsigned long)INTR_ACQ_FAILURE|(unsigned long)INTR_VITERBI_LOSS|(unsigned long)INTR_VITERBI_SYNC|
    (unsigned long)INTR_DEMOD_LOSS|(unsigned long)INTR_DEMOD_SYNC);

  /* or-in new interrupts */
  valid_ints |= (unsigned long)INTR_LNB_REPLY_READY;

  /* test value to be written to demod for range error (bad range indicates invalid parameter passed) */
  if (ulRegVal > valid_ints)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  /* write new interrupt options to the demod */
  return(RegisterWrite(nim,CX24130_INTRENABLE,ulRegVal, DEMOD_I2C_IO));
  
}  /* API_SetInterruptOptions() */


/*******************************************************************************************************/
/* API_SetSearchRangeLimit() */
/*******************************************************************************************************/
BOOL           API_SetSearchRangeLimit(     /* function to set LNB search range limit */
NIM            *nim,                        /* pointer to nim */
unsigned long  lnboffset,                   /* desired LNB offset limit in hz (if in KHz, converted to Hz) */
unsigned long  *actual)                     /* returned actual LNB offset limit */
{
  unsigned long  bins;
  unsigned long  symbolrate;
  unsigned long  sr;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  /* validate input parameters */
  if (lnboffset == 0UL || actual == NULL)
  {
     DRIVER_SET_ERROR(nim,API_BAD_PARM);
     return(False);
  }

  /* use the requested symbol rate if it is available, otherwise use the real symbol rate */
  if ((symbolrate = (nim->symbol_rate_ideal * M)) == 0UL)
  {
     if (API_GetSymbolRate(nim,&symbolrate) == False)
     {
        return (False);
     }
  }

  /* save the requested LNB offset */
  nim->lnboffset = lnboffset;

  /* remove excess LNB offset that the driver/hardware doesn't support */
  if (lnboffset > (LNB_RANGE_HIGH * M))  lnboffset = (LNB_RANGE_HIGH * M);

  /* Cobra hardware can handle up to 5MHz through binning, the LNB offset
     beyond 5MHz is handled by moving the tuner PLL */
  if (DRIVER_Cobra(nim) == True)
  {
        nim->tuner_offset = 0UL;
	if (lnboffset > LNB_OFFSET_LIMIT)
        {
		nim->tuner_offset = (lnboffset - LNB_OFFSET_LIMIT);
                lnboffset -= nim->tuner_offset;
        }
  }

  *actual = 0UL;
  sr = (symbolrate/8UL);

   if (DRIVER_div_zero(nim, sr) == False)
   {
      return (False);
   }

  /* compute the number of bins needed */
  bins = (((unsigned long)labs((long)lnboffset)) / (sr));
  if ((bins * sr) < lnboffset)  bins += 1UL;

  if (bins < MIN_NO_BINS)  bins = MIN_NO_BINS;
 
  if (DRIVER_Cobra(nim) == True)
  {
    if (bins > MAX_NO_BINS)  bins = MAX_NO_BINS;

    if (RegisterWrite(nim, CX24130_ACQFREQRANGE,bins, DEMOD_I2C_IO) == False)
	return(False);
  }
#ifdef CAMARIC_FEATURES
  else if (DRIVER_Camaric(nim) == True)
  {
    if (bins > MAX_NO_BINS_CAM)  bins = MAX_NO_BINS_CAM;

    if (RegisterWrite(nim, CX24123_ACQFREQRANGE,bins, DEMOD_I2C_IO) == False)
	return(False);
  }
#endif /* #ifdef CAMARIC_FEATURES */

  /* compute actual search rage limit, return to caller */
  *actual = bins * (sr);

  /* replace the previously removed excess lnboffset for Cobra */
  if (DRIVER_Cobra(nim) == True)
  {
     *actual += nim->tuner_offset;
  }

  return(True);

}  /* API_SetSearchRangeLimit() */


/*******************************************************************************************************/
/* API_GetSearchRangeLimit() */
/*******************************************************************************************************/
BOOL           API_GetSearchRangeLimit(     /* function to return current search range limit to caller */
NIM            *nim,                        /* pointer to nim */
unsigned long  *lnboffset)                  /* returned lnb offset */
{
  unsigned long  bins;
  unsigned long  symbolrate;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  /* validate input parameters */
  if (lnboffset == NULL)
  {
     DRIVER_SET_ERROR(nim,API_BAD_PARM);
     return(False);
  }

  if (API_GetSymbolRate(nim,&symbolrate) == True)
  {

      if (DRIVER_Cobra(nim) == True)
      {
       if (RegisterRead(nim,CX24130_ACQFREQRANGE,&bins, DEMOD_I2C_IO) == False)
		return(False);
      }
#ifdef CAMARIC_FEATURES
      else if (DRIVER_Camaric(nim) == True)
      {
       if (RegisterRead(nim,CX24123_ACQFREQRANGE,&bins, DEMOD_I2C_IO) == False)
		return(False);
      }
#endif /* #ifdef CAMARIC_FEATURES */
    else
    {
       DRIVER_SET_ERROR(nim,API_DEMOD_UNSUPPORTED);
       return (False);
    }

    /* compute actual search range limit, return to caller */
    /* *lnboffset = bins * (symbolrate/8UL) * M; <-- CR6870 */
    *lnboffset = bins * (symbolrate/8UL);

    /* (CR 6243) */
    /* (CR 6323) removed because else would never be executed */
    /* if (nim->tuner_offset >= 0)  *lnboffset = ((*lnboffset) + (unsigned long)nim->tuner_offset); */
    /* else  *lnboffset = ((*lnboffset) - (unsigned long)(nim->tuner_offset * -1L)); */

    *lnboffset = ((*lnboffset) + (unsigned long)nim->tuner_offset);

    return(True);
  }

  return(False);

}  /* API_GetSearchRangeLimit() */


/*******************************************************************************************************/
/* API_SetModulation() */
/*******************************************************************************************************/
BOOL     API_SetModulation(            /* function to set the modulation type */
NIM      *nim,                         /* NIM pointer */
MODTYPE  modtype)                      /* modulation type: QPSK, BPSK */
{
  unsigned long  ulRegVal;

  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  switch(modtype)
  {
    case  MOD_QPSK:
    case  MOD_BPSK:
    {
      ulRegVal = (modtype == MOD_QPSK ? BIT_ZERO : BIT_ONE);
      if (RegisterWrite(nim,CX24130_SYSMODTYPE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      break;
    }
    case  MOD_QPSK_DCII_MUX:
    {
          /* set DCII mode to "combined" */
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      break;
    }
    case  MOD_QPSK_DCII_SPLIT_I:
    {
      /* set DCII mode to Split I */
      ulRegVal = 0x03UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      break;
    }
    case  MOD_QPSK_DCII_SPLIT_Q:
    {
          /* set DCII mode to Split Q */
      ulRegVal = 0x02UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      break;
    }
    case  MOD_UNDEF:
    default:
    {
      /* bad parameter passed */
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }
  }  /* switch(... */

  return(True);

}  /* API_SetModulation() */

/*******************************************************************************************************/
/* API_GetModulation() */
/*******************************************************************************************************/
BOOL     API_GetModulation(            /* function to retrieve  current modulation setting from the nim */
NIM      *nim,                         /* pointer to nim */
MODTYPE  *modtype)                     /* returned modulation type */
{
  unsigned long  ulRegVal;

  /* test for valid NIM and parm not NULL */
  DRIVER_VALIDATE_NIM(nim);
  if (modtype == NULL)  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  else
  {
    /* read modulation type from cobra */
    if (RegisterRead(nim,CX24130_SYSMODTYPE,&ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
  
    /* convert returned raw value to enum */
    /* (removed per CR 6323) modtype = (ulRegVal == BIT_ZERO ? MOD_QPSK : MOD_BPSK); */
    if (ulRegVal == BIT_ZERO)
	  *modtype = MOD_QPSK;
    else  *modtype = MOD_BPSK;

    return(True);
  }

  return(False);

}  /* API_GetModulation() */

/*******************************************************************************************************/
/*  API_GetAssociatedSampleFrequency() */
/*******************************************************************************************************/
BOOL           API_GetAssociatedSampleFrequency( /* function to return sample freq to the caller */
NIM            *nim,                             /* pointer to nim */
SAMPFRQ        sampfrq,                          /* sample frequency */
unsigned long  *AssdFs)                          /* returned Fs to caller */
{
  int    i;

  unsigned long  pll_mult;
  unsigned long  samplerate;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (AssdFs != NULL)
  {
    for (i = 0 ; sampfrq_list[i] != SAMPLE_FREQ_UNDEF ; i++)
    {
      if (sampfrq_list[i] == sampfrq)
      {
        /* get sample frequency from caller's selection from list */
        samplerate = sampfrq_eqlist[i];

        /* compute the demod pll multiplier */
        pll_mult = DRIVER_compute_demod_pll_mult(nim,samplerate,nim->crystal_freq);
        *AssdFs = DRIVER_compute_fs(pll_mult,nim->crystal_freq);
        
        /* return Ass'd Fs to caller -> use progFs (Fs-value programmed to the demod) * Fc / 6 */
        return(True);
      }
    }
  }

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_GetAssociatedSampleFrequency() */


/*******************************************************************************************************/
/* API_SetSampleFrequency() */
/*******************************************************************************************************/
BOOL    API_SetSampleFrequency(        /* function to set the sample frequency */
NIM     *nim,                          /* pointer to nim */
SAMPFRQ sampfrq)                       /* sample freq number (from allowable sample freq. list) */
{
  int    i;

  unsigned long  pllmult;
  unsigned long  samplerate = 0UL;
  
  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  /* match sampfrq enum (1..n) passed in by caller to an associated Fs in a list */
  for (i = 0 ; sampfrq_list[i] != SAMPLE_FREQ_UNDEF ; i++)
  {
    /* test each item in list for a match */
    if (sampfrq_list[i] == sampfrq)
    {
      /* extract the associated sample freq from the list */
      samplerate = sampfrq_eqlist[i];
      break;
    }
  }
  if (sampfrq_list[i] == SAMPLE_FREQ_UNDEF)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }
  
#ifdef CAMARIC_FEATURES
  if (DRIVER_Camaric(nim) == True)
  {
     if (nim->symbol_rate_ideal <= 4000UL)
     {
        /* set sampling freq to 26.963MHz if symbol rate is less than 4Msps */
        samplerate = nim->sample_freq_less_than_4msps; //SAMPLE_FREQ_LT_4MSPS;
     }
     else
     {
        /* set sampling freq to 99.425MHz if it is less than 2 times of symbol rate */
        if (samplerate < (nim->symbol_rate_ideal * M * 2UL))
        {
           samplerate = nim->sample_freq_nom_val; //SAMPLE_FREQ_NOM_VAL;
        }
     }
  }
#endif /* #ifdef CAMARIC_FEATURES */

  /* compute the PLL value for the sample rate */
  pllmult = DRIVER_compute_demod_pll_mult(nim,samplerate,nim->crystal_freq);

  /* set the value to write to the demod */
  if (RegisterWritePLLMult(nim,pllmult) == False)
     return(False);

  return (True);

}  /* API_SetSampleFrequency() */


/*******************************************************************************************************/
/* __API_SetSampleFrequency() */
/*******************************************************************************************************/
BOOL           __API_SetSampleFrequency(    /* function to set the sample frequency without using ENUM SAMPFRQ */
NIM            *nim,                        /* pointer to nim */
unsigned long  sampleratehz)                /* sample freq number (not ENUM) */
{
  unsigned long  pllmult;

  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);
      
  /* compute the PLL value for the sample rate */
  pllmult = DRIVER_compute_demod_pll_mult(nim,sampleratehz,nim->crystal_freq);

  /* set the value to write to the demod */
  if (RegisterWritePLLMult(nim,pllmult) == False)
     return(False);

  return(True);

}  /* __API_SetSampleFrequency() */


/*******************************************************************************************************/
/* API_GetSampleFrequency() */
/*******************************************************************************************************/
BOOL           API_GetSampleFrequency( /* function to read demod's current sample rate */
NIM            *nim,                   /* pointer to nim */
unsigned long  *samplerate)            /* returned sample rate */
{
  unsigned long  pllmult;
  unsigned long  rate;

  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  if (samplerate != NULL)
  {
    /* read the pll mult from the demod, find the associated SAMPFRQ */
    pllmult = (unsigned long)nim->ucPLLMult_shadow;
    rate = DRIVER_compute_fs(pllmult,nim->crystal_freq);
    *samplerate = rate;
    return(True);
  }

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_GetSampleFrequency() */


/*******************************************************************************************************/
/* API_SetTransportSpec() */
/*******************************************************************************************************/
BOOL      API_SetTransportSpec(        /* function to set the NIM's transport spec */
NIM       *nim,                        /* pointer to nim */
TRANSPEC  transpec)                    /* transport specification */
{
  unsigned long  ulRegVal;
  unsigned char b;

  /* test for valid NIM and parm not NULL */
  DRIVER_VALIDATE_NIM(nim);

  if (transpec == SPEC_DCII)
  {
    ulRegVal = 0x00;
    if (RegisterWrite(nim,CX24130_DC2CLKDIS,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    ulRegVal = 0x00;
    if (RegisterWrite(nim,CX24130_DC2CLKDIR,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    ulRegVal = 0x0A;
    if (RegisterWrite(nim,CX24130_DC2CLKFREQ,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  } else
  {
     /* CR13148, set register 0x66 again after reset */
      b = 0xff;
      if (API_WriteReg(nim,0x66,&b) == False)  
          return(False);
  }

  /* 9/11/01 code added to test if DCII mode register must be reset for NIM B (s/b reset via DRIVER_default() */
  if (RegisterRead(nim,CX24130_DC2MODE,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  if (ulRegVal != 0x00UL)
  {
    ulRegVal = 0x00;
    if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  }

  /* set the transport spec */
  switch (transpec)
  {
    case  SPEC_DSS:
    {
      /* reset the sub-dcii mode */
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* set modulation to DSS */
      ulRegVal = 0x01UL;
      if (RegisterWrite(nim,CX24130_SYSTRANSTD,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* (CR 8114) This code executed only if Rev C or single demod */
      ulRegVal = 0x08UL;
      if (RegisterWrite(nim,CX24130_ACQSYNCBYTEWIN,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x05UL;
      if (RegisterWrite(nim,CX24130_ACQRSSYNCTHRESH,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      ulRegVal = 0xff;
      if (RegisterWrite(nim,CX24130_ACQVITNORMTHRESH,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x09;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN12,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x15;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN23,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x56;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN67,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x10;
      if (RegisterWrite(nim,CX24130_ACQFULLSYNCWIN,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      break;
    }

    case  SPEC_DVB:
    {
      /* reset the sub-dcii mode */
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* modulation in BVDs */
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_SYSTRANSTD,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* new transpec mods 8/29/01 */
      ulRegVal = 0x08UL;
      if (RegisterWrite(nim,CX24130_ACQSYNCBYTEWIN,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      ulRegVal = 0x05UL;
      if (RegisterWrite(nim,CX24130_ACQRSSYNCTHRESH,ulRegVal, DEMOD_I2C_IO) == False)  return(False);

      /* (CR 7930) */
      ulRegVal = 0xfe;
      if (RegisterWrite(nim,CX24130_ACQVITNORMTHRESH,ulRegVal, DEMOD_I2C_IO) == False)
	return(False); 

      ulRegVal = 0x09;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN12,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 0x15;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN23,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 0x56;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN67,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 0x10;
      if (RegisterWrite(nim,CX24130_ACQFULLSYNCWIN,ulRegVal, DEMOD_I2C_IO) == False) 
	return(False);
      break;
    }
    case  SPEC_DCII:
    {
      /* set modulation to DCII */
      ulRegVal = 0x03UL;
      if (RegisterWrite(nim,CX24130_SYSTRANSTD,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* set DCII mode to "combined" */
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* new transpec mods per cp 8/29/01 */
      ulRegVal = 0x20UL;
      if (RegisterWrite(nim,CX24130_ACQSYNCBYTEWIN,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x02UL;
      if (RegisterWrite(nim,CX24130_ACQRSSYNCTHRESH,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      ulRegVal = 0x09;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN12,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x15;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN23,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x56;
      if (RegisterWrite(nim,CX24130_ACQVITNORMWIN67,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      ulRegVal = 0x10;
      if (RegisterWrite(nim,CX24130_ACQFULLSYNCWIN,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* (CR 8218) Set DCII ClkDis */
      ulRegVal = 0x00;
      if (RegisterWrite(nim,CX24130_DC2CLKDIS,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

#ifdef  DCII_DEFAULT_SMOOTHCLK
      /* (CR 8209) Added following block of code to allow default MPEG Smooth Clk Div setting */
      if (transpec == SPEC_DCII)
      {
        ulRegVal = DCII_HARD_SMOOTHCLK;
        if (RegisterWriteClkSmoothDiv(nim,ulRegVal) == False)
		return(False);
      }
#endif  /* #ifdef  DCII_DEFAULT_SMOOTHCLK */
      break;
    }
#ifdef CAMARIC_FEATURES
    case  SPEC_DVB_DSS:
    {
      /* reset the sub-dcii mode */
      ulRegVal = 0x00UL;
      if (RegisterWrite(nim,CX24130_DC2MODE,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* modulation is either DVB or DSS */
      ulRegVal = 0x02UL;
      if (RegisterWrite(nim,CX24130_SYSTRANSTD,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);

      /* additional set-up may be required */

      break;
    }
#endif  /* #ifdef CAMARIC_FEATURES */
    case  SPEC_UNDEF:
    default:
    {
      nim->tspec = SPEC_UNDEF;

      /* bad parameter passed */
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }
  }  /* switch(... */

  /* (CR 7951) save a copy of the current transport spec to the nim */
  nim->tspec = transpec;

  return(True);

}  /* API_SetTransportSpec() */



/*******************************************************************************************************/
/* API_GetTransportSpec() */
/*******************************************************************************************************/
BOOL      API_GetTransportSpec(        /* function to return NIM's transport spec */
NIM       *nim,                        /* pointer to nim */
TRANSPEC  *transpec)                   /* returned transport spec setting */
{
   int  temp_modtype;

   unsigned long  ulRegVal;

   /* test for valid NIM and parm not NULL */
   DRIVER_VALIDATE_NIM(nim);

   if (transpec == NULL)
   {
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return (False);
   }

   *transpec = SPEC_UNDEF;
   if (nim->tspec == SPEC_UNDEF)
   {
      /* read the modulation type */
      if (RegisterRead(nim,CX24130_SYSTRANSTD,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      temp_modtype = (int)ulRegVal;

      /* translate Cobra raw values to application-usable */
      switch (temp_modtype)
      {
         /* DVB */
         case  0x00:
         {
            nim->tspec = SPEC_DVB;
            break;
         }
         /* DSS */  
         case  0x01:
         {
            nim->tspec = SPEC_DSS;
            break;
         }
         /* DCII */
         case  0x03:
         {
            /* DCII has several sub-modes */
            nim->tspec = SPEC_DCII;
            break;
         }
#ifdef CAMARIC_FEATURES
         case 0x02:
         {
            nim->tspec = SPEC_DVB_DSS;
            break;
         }
#endif  /* #ifdef CAMARIC_FEATURES */
         default:
         {
            /* hardware returned an invalid transspec value */
            DRIVER_SET_ERROR(nim,API_BAD_RTNVAL);
            return(False);
         }
      }  /* switch(... */
   }  /* if (nim->tspec == SPEC_UNDEF) */
   
   *transpec = nim->tspec;

#ifdef CAMARIC_FEATURES
   if (nim->tspec == SPEC_DVB_DSS)
   {
      /* read full sync flag */
      if (RegisterRead(nim,CX24130_ACQFULLSYNC,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

      /* if locked, read actual locked-to transpec value */
      if (ulRegVal == 1UL)
      {
         *transpec = DRIVER_Tspec_Get_Auto(nim);
      }
   }
#endif  /* #ifdef CAMARIC_FEATURES */

   return(True);

}  /* API_GetTransportSpec() */

//API_SetDescramble
//API_GetDescramble

/*******************************************************************************************************/
/* API_SetSymbolRate() */
/*******************************************************************************************************/
BOOL      API_SetSymbolRate(           /* function to set the demod's symbol rate */
NIM       *nim,                        /* pointer to nim */
SYMBRATE  symbolrate)                  /* symbol rate (in Ksps) */
{
  long   temp_symbolrate;

  unsigned long  ulRegVal;
  unsigned long  samplerate;

  unsigned long  lnboffset;
  unsigned long  bandwidth;
  unsigned long  mV;


  SYMBRATE  low_test;
  SYMBRATE  high_test;

  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  low_test = SYM_RATE_LOW;
  high_test = SYM_RATE_HIGH;

#ifdef CAMARIC_FEATURES
  /* set-up default values only for Cobra */
  if (DRIVER_Camaric(nim) == True)
  {
     low_test = SYM_RATE_LOW_CAM;
     high_test = SYM_RATE_HIGH_CAM;
  }
#endif  /* #ifdef CAMARIC_FEATURES */

  /* range test symbolrate */
  if (symbolrate >= (unsigned long)low_test && symbolrate <= (unsigned long)high_test)
  {
	/* CR9879, always reset LNB search range to take cases where
	 * NIM_DEFAULT_LNB is greater than the front end LNB offset limit. */
    nim->symbol_rate_ideal = symbolrate;

    /* read the sample rate */
    if (API_GetSampleFrequency(nim,&samplerate) == False)
	return(False);

#ifdef CAMARIC_FEATURES
    /* (CR 8797 set the pdmfout and sample rate registers) */
    if (DRIVER_Camaric(nim) == True)
    {
     nim->pdmfout = DRIVER_Set_Pdmfout(nim,(nim->symbol_rate_ideal*(1UL*M)),samplerate);
    }
#endif  /* #ifdef CAMARIC_FEATURES */

    /* compute the symbol rate */
    temp_symbolrate = DRIVER_symbolrate_in(symbolrate,samplerate);
    nim->symbol_rate = (unsigned long)symbolrate;

    /* write computed value to demod */
    ulRegVal = (unsigned long)temp_symbolrate;
    if (RegisterWrite(nim,CX24130_SYSSYMBOLRATE,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* when the symbol rate changes, the number of bins searched will be modified to reflect */
    /* the default min lnb search range required per CR #5703 */
    /* nim->lnboffset = NIM_DEFAULT_LNB; */
    
    /* (REMOVED per CR 6748/6747) ... retrieve  the search-range-limit, convert actual to khz */
    /* if (API_GetSearchRangeLimit(nim,&actual) == False)  return(False); */
    /* actual /= 1000UL; */

    lnboffset = nim->lnboffset;
    symbolrate *= 1000;

//kir??
    bandwidth = (((((symbolrate/100UL) * nim->unk50)/10UL)/100UL) + (unsigned long)labs(lnboffset/100UL) + 5)/10UL;

    bandwidth += 1000;
    nim->anti_alias_bandwidth = bandwidth;

    if (API_SetTunerBW(nim, bandwidth, &mV) == False)
	return(False);

    if(TUNER_SetGainSettings(nim,symbolrate) == False)
	return(False);

#ifdef CAMARIC_FEATURES
    /* optimize CTL Tracking Bandwidth based on symbol rate */
    if(DRIVER_SetCTLTrackBW(nim,symbolrate) == False)
	return(False);
#endif  /* #ifdef CAMARIC_FEATURES */

    /* CR9879, always reset LNB search range to take cases where
     * NIM_DEFAULT_LNB is greater than the front end LNB offset limit. */

    /* (CR 8514) Reset the LNB Search Range (bins) if required. */
   /*
    if(nim->symbol_rate_ideal>10000UL)
    {
        nim->lnboffset =  5*MM;
    }
    else
    {
        nim->lnboffset =  3*MM;
    }
    */

#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
    if (nim->opt_fs_disable != True)
    {
	 if (DRIVER_Opt_Fs_calcPLLMult(nim) == False)
		return(False);
    }
#endif  /* #ifdef OPTIMAL_FS_CODE */

    if (API_SetSearchRangeLimit(nim,nim->lnboffset,&ulRegVal) == False)
	return(False);

    return(True);
  }

  DRIVER_SET_ERROR(nim,API_PARM_RANGE);
  return(False);

}  /* API_SetSymbolRate() */


/*******************************************************************************************************/
/* API_GetSymbolRate() */
/*******************************************************************************************************/
BOOL      API_GetSymbolRate(           /* function to return demod's current symbol rate setting */
NIM       *nim,                        /* pointer to nim */
SYMBRATE  *symbolrate)                 /* returned current symbol rate setting */
{
  unsigned long  temp_symbolrate;
  unsigned long  samplerate;

  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  /* read the sample rate */
  if (API_GetSampleFrequency(nim,&samplerate) == False)
	return(False);

  /* read the symbolrate teh demod is set to */
  if (RegisterRead(nim,CX24130_SYSSYMBOLRATE,symbolrate, DEMOD_I2C_IO) == False)
	return(False);

   /* compute the symbolrate to be returned to the caller */
  temp_symbolrate = (unsigned long)DRIVER_symbolrate_out(*symbolrate,samplerate);
  nim->symbol_rate = temp_symbolrate;
  *symbolrate = temp_symbolrate;

  return(True);

}  /* API_GetSymbolRate() */


/*******************************************************************************************************/
/* API_GetMinSymbolRate() */
/*******************************************************************************************************/
BOOL           API_GetMinSymbolRate(   /* function to return demod's minimum symbol rate (in Khz)*/
NIM            *nim,                   /* pointer to nim */
unsigned long  *minsymbolrate)         /* returned min. symbol rate (in Khz) */
{
  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  if (DRIVER_Camaric(nim) != True || nim->demod_type == CX24123C)
    *minsymbolrate = SYM_RATE_LOW_CAM*1UL;
  else
    *minsymbolrate = SYM_RATE_LOW*1UL;

  return(True);
}  /* API_GetMinSymbolRate() */


/*******************************************************************************************************/
/* API_GetMaxSymbolRate() */
/*******************************************************************************************************/
BOOL           API_GetMaxSymbolRate(   /* function to return the demod's max symbol rate (in Khz) */
NIM            *nim,                   /* pointer to nim */
unsigned long  *maxsymbolrate)         /* returned max. symbol rate (in Khz) */
{
  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  /* validate arguments */
  if (maxsymbolrate == NULL)
  {
     DRIVER_SET_ERROR(nim,API_BAD_PARM);
     return(False);
  }

  if (DRIVER_Camaric(nim) != True || DRIVER_Cobra_compat(nim) == True)
     *maxsymbolrate = SYM_RATE_HIGH*1UL;
  else
     *maxsymbolrate = SYM_RATE_HIGH_CAM*1UL;

  return(True);
}  /* API_GetMaxSymbolRate() */


/*******************************************************************************************************/
/* API_SetViterbiRate() */
/*******************************************************************************************************/
BOOL      API_SetViterbiRate(          /* function to set the demod viterbi rate */
NIM       *nim,                        /* pointer to nim */
CODERATE  coderate)                    /* viterbi rate setting */
{
  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  return(DRIVER_SetViterbiRate(nim,coderate));

}  /* API_SetViterbiRate() */


/*******************************************************************************************************/
/* API_GetViterbiRate() */
/*******************************************************************************************************/
BOOL      API_GetViterbiRate(          /* function to read the demod's current viterbi rate setting */
NIM       *nim,                        /* pointer to nim */
CODERATE  *coderate)                   /* returned current viterbi code rate setting */
{
  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  return(DRIVER_GetViterbiRate(nim,coderate));

}  /* API_GetViterbiRate() */


/*******************************************************************************************************/
/* API_SetSpectralInversion() */
/*******************************************************************************************************/
BOOL     API_SetSpectralInversion(     /* function to set the demod's spectral inversion setting */
NIM      *nim,                         /* pointer to nim */
SPECINV  specinv)                      /* spectral inv. setting */
{
  unsigned long  ulRegVal;

  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

#if INCLUDE_ROSIE
  if(nim->tuner_type == CX24108)
  {
	  switch(specinv)
	  {
		case SPEC_INV_OFF:
			specinv = SPEC_INV_ON;
			break;
		case SPEC_INV_ON:
			specinv = SPEC_INV_OFF;
			break;
		case SPEC_INV_ON_BOTH:
			specinv = SPEC_INV_OFF_BOTH;
			break;
		case SPEC_INV_OFF_BOTH:
			specinv = SPEC_INV_ON_BOTH;
			break;
		case SPEC_INV_UNDEF:
			break;
	  }
  }
#endif		   
  switch(specinv)
  {
    case  SPEC_INV_OFF:
    {
      /* write zero to ACQVitSINom, one to ACQSISearchDis */
      ulRegVal = 0UL;
      if (RegisterWrite(nim,CX24130_ACQVITSINOM,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 1UL;
      if (RegisterWrite(nim,CX24130_ACQSISEARCHDIS,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      break;
    }
    case  SPEC_INV_ON:
    {
      /* write one to ACQVitSINom, one to ACQSISearchDis */
      ulRegVal = 1UL;
      if (RegisterWrite(nim,CX24130_ACQVITSINOM,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 1UL;
      if (RegisterWrite(nim,CX24130_ACQSISEARCHDIS,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      break;
    }
    case  SPEC_INV_ON_BOTH:
    {
      /* write one to AcqVitSINom, zero to ACQSISearchDis */
      ulRegVal = 1UL;
      if (RegisterWrite(nim,CX24130_ACQVITSINOM,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 0UL;
      if (RegisterWrite(nim,CX24130_ACQSISEARCHDIS,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      break;
    }
    case  SPEC_INV_OFF_BOTH:
    {
      /* write one to AcqVitSINom, zero to ACQSISearchDis */
      ulRegVal = 0UL;
      if (RegisterWrite(nim,CX24130_ACQVITSINOM,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      ulRegVal = 0UL;
      if (RegisterWrite(nim,CX24130_ACQSISEARCHDIS,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      break;
    }
    default:
    {
      /* inform caller of error */
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
    }
  }  /* switch(... */
  
  return(True);

}  /* API_SetSpectralInversion() */


/*******************************************************************************************************/
/* API_GetSpectralInversion() */
/*******************************************************************************************************/
BOOL      API_GetSpectralInversion(    /* function to read the demod's spectral inversion setting */
NIM       *nim,                        /* pointer to nim */
SPECINV   *specinv)                    /* returned spectral inv. settings */
{
  unsigned long   spec_curr;

  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  /* read current viterbi spec inv */
  if (RegisterRead(nim,CX24130_ACQVITCURRSI,&spec_curr, DEMOD_I2C_IO) == False)
	return(False);

  /* report spec inv setting */  
  if (spec_curr == 0x01UL)
	*specinv = SPEC_INV_ON;
  else  *specinv = SPEC_INV_OFF;  

#if INCLUDE_ROSIE
  if(nim->tuner_type == CX24108)
  {
	  switch(*specinv)
	  {
		case SPEC_INV_OFF:
			*specinv = SPEC_INV_ON;
			break;
		case SPEC_INV_ON:
			*specinv = SPEC_INV_OFF;
			break;
		default:
			break;
	  }
  }
#endif

  return(True);

}  /* API_GetSpectralInversion() */


/*******************************************************************************************************/
/* API_AcqBegin() */
/*******************************************************************************************************/
BOOL  API_AcqBegin(                    /* function to start/restart demod's auto. acq. engine */
NIM   *nim)                            /* pointer to nim */
{
  unsigned long  ulRegVal;

  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);
  nim->prevstate = ACQ_OFF;

  /* read viterbi spec inv disabled flag */
  ulRegVal = 0x10UL;
  if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  ulRegVal = 0x00UL;
  if (RegisterWrite(nim,CX24130_RSTCOBRA,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* API_AcqBegin() */


/*******************************************************************************************************/
/* API_AcqContinue() */
/*******************************************************************************************************/
BOOL      API_AcqContinue(             /* function to test and report state of demod's acq. engine */
NIM       *nim,                        /* pointer to nim */
ACQSTATE  *acqstate)                   /* returned state */
{
  unsigned long  trackingstate;

  /* test NIM for validity */
  DRIVER_VALIDATE_NIM(nim);

  /* allow a reset of the last-known prev-state */
  if (acqstate == NULL)
  {
    nim->prevstate = ACQ_OFF;
    return(False);
  }

  /* read viterbi spec inv disabled flag */
  if (RegisterRead(nim,CX24130_ACQFULLSYNC,&trackingstate, DEMOD_I2C_IO) == False)
	return(False);

  /* force acq state from undefined to known state */
  if (nim->prevstate == ACQ_UNDEF)  nim->prevstate = ACQ_OFF;

  /* if locked, save good news, else perform some more */
  if (trackingstate == 1UL)
  {
    *acqstate = ACQ_LOCKED_AND_TRACKING;
  }
  else
  {
    /* state is not locked, see what the demod WAS doing */
    if (nim->prevstate == ACQ_OFF || nim->prevstate == ACQ_SEARCHING)  *acqstate = ACQ_SEARCHING;
    else
    {
      if (nim->prevstate == ACQ_LOCKED_AND_TRACKING)  *acqstate = ACQ_FADE;
      else
      {
        if (nim->prevstate == ACQ_FADE)  *acqstate = ACQ_FADE;
      }
    }
  }
  
  /* save the last-known state */
  nim->prevstate = *acqstate;

  return(True);

}  /* API_AcqContinue() */


/*******************************************************************************************************/
/* API_AcqSetViterbiCodeRates() */
/*******************************************************************************************************/
BOOL          API_AcqSetViterbiCodeRates(   /* function to set viterbi search code rates using or'd bit flags */
NIM           *nim,                         /* pointer to NIM */
unsigned int  vcr)                          /* valid Viterbi coderates OR'd together */
{
  unsigned int  i;
  
  static  VITLIST  v;

  memset(&v,CNULL,sizeof(v));
  v.vcnt = 0;

  /* step-through each possible bit setting for the viterbi code rates */
  for (i = 0x8000 ; i != 0x00U ; )
  {
    if ((vcr&i) != 0U)  
    {
      /* when a code-rate bit mask is found, set it into the viterbi list, and incr the count */
      v.viterbi_list[v.vcnt] = (CODERATE)i;
      v.vcnt++;
    }
    i = (i>>1);
  }

  /* use the built viterbi list to set the code rates */
  if (DRIVER_AcqSetViterbiSearchList(nim,&v) != True)
  {
    DRIVER_SET_ERROR(nim,API_VITSET);
    return(False);
  }

  return(True);

}  /* API_AcqSetViterbiCodeRates() */


/*******************************************************************************************************/
/* API_AcqGetViterbiCodeRates() */
/*******************************************************************************************************/
BOOL          API_AcqGetViterbiCodeRates(   /* function to read current viterbi code rate settings from demod */
NIM           *nim,                         /* pointer to NIM */
unsigned int  *vcr)                         /* pointer to where current demod viterbi search settings will be returned */
{
  int  i;
  static  VITLIST  v;

  memset(&v,CNULL,sizeof(v));
  v.vcnt = 0;

  *vcr = 0;

  if (DRIVER_AcqGetViterbiSearchList(nim,&v) == False)
	return(False);

  /* step-through each item, or it to the return mask */
  for (i = 0 ; i < v.vcnt ; i++)
  {
    *vcr |= (unsigned int)v.viterbi_list[i];
  }

  return(True);

}  /* API_AcqGetViterbiCodeRates() */


/*******************************************************************************************************/
/* API_GetPendingInterrupts()  (see also API_SetInterruptOptions() ) */
/*******************************************************************************************************/
BOOL       API_GetPendingInterrupts(   /* function to read demod's pending interrupts */
NIM        *nim,                       /* pointer to nim */
INTEROPTS  *interopts)                 /* pending interrupts (bit mask of) */
{
  unsigned long  ulRegVal;
  
  BOOL   rtn = False;

  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  if (interopts == NULL)
  {
	DRIVER_SET_ERROR(nim,API_BADPTR);
  } else
  {
    /* read interrupt options from demod */
    rtn = RegisterRead(nim,CX24130_INTRPENDING,&ulRegVal, DEMOD_I2C_IO);
    *interopts = (INTEROPTS)ulRegVal;
  }
  
  return(rtn);

}  /* API_GetPendingInterrupts() */


/*******************************************************************************************************/
/* API_ClearPendingInterrupts() */
/*******************************************************************************************************/
BOOL      _API_ClearPendingInterrupts( /* function to clear (write zero to) pending intr register */
NIM       *nim,                        /* pointer to nim */
INTEROPTS interopts)
{
  unsigned long  ulRegVal = (unsigned long)interopts;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  ulRegVal &= ~INTR_LNB_REPLY_READY;

  /* clear the pending demod interrupt */
  return(RegisterWrite(nim,CX24130_INTRPENDING,ulRegVal, DEMOD_I2C_IO));

}  /* API_ClearPendingInterrupts() */


/*******************************************************************************************************/
/* API_GetLockIndicators() */
/*******************************************************************************************************/
BOOL      API_GetLockIndicators(       /* function to read all demod lock indicators */
NIM       *nim,                        /* pointer to nim */
LOCKIND   *lockinds)                   /* returned lock indicator struct */
{
  unsigned char  b;
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (lockinds == NULL)
  {
	DRIVER_SET_ERROR(nim,API_BADPTR);
  } else
  {
    /* pre-set lock indicators to False */
    lockinds->pll = False;
    lockinds->demod_pll = False;
    lockinds->demod = False;
    lockinds->viterbi = False;
    lockinds->reedsolomon = False;
    lockinds->descramble = False;
    lockinds->syncbyte = False;

    /* read the entire register, pull-out pertinent data */
    if (RegisterRead(nim,CX24130_SYNCSTATUS,&ulRegVal, DEMOD_I2C_IO) == True)  
    {
      b = (unsigned char)ulRegVal;

      /* mask each lock ind. from the register */
      if ((b & demod_register_map[CX24130_PLLLOCK].p_hw_mask[0]) != 0)
	lockinds->demod_pll = True;
      if ((b & demod_register_map[CX24130_ACQDMDSYNC].p_hw_mask[0]) != 0)
	lockinds->demod = True;
      if ((b & demod_register_map[CX24130_ACQVITSYNC].p_hw_mask[0]) != 0)
	lockinds->viterbi = True;
      if ((b & demod_register_map[CX24130_ACQFULLSYNC].p_hw_mask[0]) != 0)
	lockinds->reedsolomon = True;
      if ((b & demod_register_map[CX24130_ACQSYNCBYTESYNC].p_hw_mask[0]) != 0)
	lockinds->descramble = True;
      if ((b & demod_register_map[CX24130_ACQFULLSYNC].p_hw_mask[0]) != 0)
	lockinds->syncbyte = True;

      /* line added per CR 5720.b */
      if (lockinds->syncbyte == True)
	lockinds->demod = True;

      /* retrieve tuner pll lock status from tuner LD pin */
      TUNER_GetPLLLockStatus(nim,&lockinds->pll);

      return(True);
    }

    /* report error encountered */
    DRIVER_SET_ERROR(nim,API_LOCKIND_ERR);
  }

  return(False);

}  /* API_GetLockIndicators() */

/*******************************************************************************************************/
/* API_SetTunerGainThreshold() */
/*******************************************************************************************************/
BOOL 
API_SetTunerGainThreshold(NIM* p_nim, signed char threshold_dBm)
{
    /* test for valid nim */
    DRIVER_VALIDATE_NIM(p_nim);
#if INCLUDE_ROSIE
	if (p_nim->tuner_type == CX24108)
	{
		return(False);
	}
#endif
#if INCLUDE_VIPER
	/* Shadow the threshold_dBm parameter here */
	p_nim->tuner.cx24128.tuner_gain_thresh = threshold_dBm;
#endif /* INCLUDE_VIPER */

	return (True);
}

/*******************************************************************************************************/
/* API_GetTunerGainThreshold() */
/*******************************************************************************************************/
BOOL 
API_GetTunerGainThreshold(NIM* p_nim, signed char* p_threshold_dBm)
{
    *p_threshold_dBm = -50;

#if INCLUDE_ROSIE
	if (p_nim->tuner_type == CX24108)
	{
		return(False); 
	}
#endif
#if INCLUDE_VIPER
	/* Return the shadowed threshold_dBm parameter here */
	*p_threshold_dBm = p_nim->tuner.cx24128.tuner_gain_thresh;
#endif /* #if INCLUDE_VIPER */
	return (True);
}


/*******************************************************************************************************/
/* ERROR MESSAGE PROCESSING FUNCTIONS */
/*******************************************************************************************************/
#if INCLUDE_DEBUG
/*******************************************************************************************************/
/* API_GetErrorMessage() */
/*******************************************************************************************************/
char      *API_GetErrorMessage(        /* function to return error info string to caller */
NIM       *nim,                        /* pointer to nim */
APIERRNO  __errno)                     /* error number associated with an internal error string (-1 gets last err) */
{
  /* test for valid NIM, then if asked-for errno is -1, load with errno from nim */
  if (nim != NULL)
  {
    if (__errno == API_NEGONE)  __errno = nim->__errno;
  }

  return(DRIVER_GetError(__errno));

}  /* API_GetErrorMessage() */

/*******************************************************************************************************/
/* API_GetLastError() */
/*******************************************************************************************************/
int  API_GetLastError(                 /* function to retrieve  APIERRNO of last encountered error */
NIM  *nim)                             /* pointer to nim */
{
  APIERRNO  __errno = nim->__errno;

  /* save last errno, reset error, report last error */
  nim->__errno = API_NOERR;
  return((int)__errno);

}  /* API_GetLastError() */

/*******************************************************************************************************/
/* API_GetErrorFilename() (NOTE:  non-standard return value) */
/*******************************************************************************************************/
char *API_GetErrorFilename(            /* function to return file name associated with last recorded error */
NIM  *nim)                             /* pointer to nim */
{
  return(nim->errfname);
}  /* API_GetErrorFilename() */


/*******************************************************************************************************/
/* API_Error_errline() (NOTE:  non-standard return value) */
/*******************************************************************************************************/
unsigned long API_GetErrorLineNumber(  /* Function to return line-number associated with last recorded error */
NIM *nim)                              /* pointer to nim */
{
  return(nim->errline);
}  /* API_GetErrorLineNumber() */

#endif  /* INCLUDE_DEBUG */

/*******************************************************************************************************/
/* ERROR MODE FUNCTIONS */
/*******************************************************************************************************/

/*******************************************************************************************************/
/* API_SetDemodErrorMode() */
/*******************************************************************************************************/
BOOL       API_SetDemodErrorMode(      /* function to set demod into a specific error measurement mode */
NIM        *nim,                       /* pointer to nim */
ERRORMODE  errmode)                    /* error mode to set demod to */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  /* enable/disable PN */
  switch(errmode)
  {
    case  ERRMODE_PN_VITERBI_BIT:
    case  ERRMODE_PN_RS_BIT:
    {
      if (DRIVER_SetPNSequence(nim,True) == False)
		return(False);
      break;
    }
    default:
    {
      if (DRIVER_SetPNSequence(nim,False) == False)
		return(False);
      break;
    }
  }  /* switch(... */


  /* enable/disable RS correction */
  switch(errmode)
  {
    case  ERRMODE_VITERBI_BIT:
    case  ERRMODE_PN_VITERBI_BIT:
    case  ERRMODE_VITERBI_BYTE:
    {
      if (API_EnableRSCorrection(nim,True) == False)
		return(False);
      break;
    }
    default:
    {
      if (API_EnableRSCorrection(nim,False) == False)
		return(False);
      break;
    }
  }  /* switch(... */

  /* set the RsCountMode */
  switch(errmode)
  {
    case  ERRMODE_RS_BIT:
    case  ERRMODE_PN_VITERBI_BIT:
    {
      ulRegVal = 0x03;
      break;
    }  
    case  ERRMODE_VITERBI_BYTE:
    case  ERRMODE_RS_BYTE:
    {
      ulRegVal = 0x02;
      break;
    }
    case  ERRMODE_RS_BLOCK:
    {
      ulRegVal = 0x01;
      break;
    }
    default:
    {
      /* no need to set the rs error count, so bail */
      return(True);
    }
  }  /* switch(... */

  return(RegisterWrite(nim,CX24130_BERRSSELECT,ulRegVal, DEMOD_I2C_IO));

}  /* API_SetDemodErrorMode() */


/*******************************************************************************************************/
/* API_GetDemodErrorMode() */
/*******************************************************************************************************/
BOOL       API_GetDemodErrorMode(      /* function to read current demod error measurement mode */
NIM        *nim,                       /* pointer to nim */
ERRORMODE  *errmode)                   /* returned current error measurement mode */
{
  unsigned long  rserr;
  unsigned long  rserrcountmode;

  BOOL  pnopt = 0;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  /* read PN enabled/disabled; rs corr.; */
  if (DRIVER_GetPNSequence(nim,&pnopt) == False)
	return(False);

  if (RegisterRead(nim,CX24130_RSFECDIS,&rserr, DEMOD_I2C_IO) == False)
	return(False);

  if (RegisterRead(nim,CX24130_BERRSSELECT,&rserrcountmode, DEMOD_I2C_IO) == False)
	return(False);

  *errmode = ERRMODE_NONE;

  if (pnopt == False)
  {
    if (rserr == 1UL && rserrcountmode == 3UL)  *errmode = ERRMODE_VITERBI_BIT;
    else  if (rserr == 0UL && rserrcountmode == 3UL)  *errmode = ERRMODE_RS_BIT;
    else  if (rserr == 1UL && rserrcountmode == 2UL)  *errmode = ERRMODE_VITERBI_BYTE;
    else  if (rserr == 0UL && rserrcountmode == 2UL)  *errmode = ERRMODE_RS_BYTE;
    else  if (rserr == 0UL && rserrcountmode == 1UL)  *errmode = ERRMODE_RS_BLOCK;
  }
  else
  {
    if (rserr == 0UL)  *errmode = ERRMODE_PN_RS_BIT;
    else  if(rserr == 1UL)  *errmode = ERRMODE_VITERBI_BIT;
  }

  if (*errmode == ERRMODE_NONE)
  {
    /* bad parameter passed */
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  return(True);

}  /* API_GetDemodErrorMode() */


/*******************************************************************************************************/
/* Static EsNo Table data */
/*   Esno = The ratio of Energy per Symbol to noise density. Where Es is energy per symbol.  */
/*******************************************************************************************************/
static const long ESNO_lowSNR[] = {
/*           .0     .1     .2     .3     .4     .5     .6     .7     .8     .9 */      
/*   2.X */17008, 16859, 16693, 16574, 16428, 16251, 16120, 15951, 15787, 15631, 
/*   3.X */15459, 15293, 15136, 14979, 14804, 14639, 14464, 14268, 14096, 13941, 
/*   4.X */13763, 13535, 13336, 13178, 12997, 12812, 12626, 12413, 12228, 12044, 
/*   5.X */11816, 11588, 11372, 11180, 10988, 10832, 10660, 10444, 10242, 10021, 
/*   6.X */ 9819,  9599,  9384,  9178,  8968,  8790,  8542,  8316,  8104,  7890, 
/*   7.X */ 7703,  7495,  7295,  7116,  6901,  6650,  6436,  6237,  6051,  5852, 
/*   8.X */ 5670,  5502,  5294,  5099,  4915,  4737,  4564,  4395,  4229,  4048, 
/*   9.X */ 3886,  3756,  3608,  3460,  3299,  3121,  2990,  2889,  2766,  2621, 
/*  10.X */ 2487,  2361,  2239,  2120,  2027,  1911,  1800,  1725,  1632,  1526, 
/*  11.X */ 1432,  1348,  1271,  1207,  1156,  1097,  1033,   976,   906,   850, 
/*  12.X */  788,   729,   692,   648,   601,   558,   515,   472,   436,   408, 
/*  13.X */  377,   344,   316,   291,   268,   245,   222,   205,   188,   170, 
/*  14.X */  156,   142,   127,   116,   104,    95,    85,    73,    67,    62, 
/*  15.X */   55,    50,    46,    41,    36,    31,    27,    23,    20,    19, 
/*  16.X */   17,    15,    13,    11,    10,     9,     8,     7,     6,     5, 
/*  17.X */    5,     4,     3,     3,     2,     2,     2,     2,     2,     1, 
/*  18.X */    1,     0,     0,     0,     0,     0,     0,     0,     0,     0, 
/*  19.X */    0,     0,     0,     0,     0,     0,     0,     0,     0,     0, 
/*  20.X */    0 
}; 

static const long ESNO_highSNR[] = {
/*           .0     .1     .2     .3     .4     .5     .6     .7     .8     .9 */      
/*   2.X */35246, 35134, 35037, 34921, 34790, 34692, 34597, 34466, 34318, 34175, 
/*   3.X */34076, 33942, 33808, 33703, 33578, 33457, 33343, 33206, 33062, 32956, 
/*   4.X */32837, 32699, 32552, 32402, 32259, 32121, 31963, 31788, 31660, 31535, 
/*   5.X */31372, 31189, 31007, 30845, 30707, 30555, 30326, 30121, 29962, 29823, 
/*   6.X */29653, 29469, 29310, 29112, 28921, 28690, 28489, 28326, 28120, 27910, 
/*   7.X */27723, 27550, 27341, 27122, 26895, 26691, 26522, 26324, 26090, 25875, 
/*   8.X */25608, 25390, 25264, 24938, 24717, 24661, 24469, 24223, 24009, 23796, 
/*   9.X */23549, 23325, 23116, 22854, 22652, 22458, 22190, 21985, 21779, 21521, 
/*  10.X */21277, 21032, 20807, 20591, 20356, 20101, 19838, 19595, 19347, 19122, 
/*  11.X */18919, 18653, 18387, 18193, 17953, 17686, 17471, 17220, 16929, 16691, 
/*  12.X */16443, 16158, 15899, 15674, 15459, 15234, 15012, 14762, 14502, 14284, 
/*  13.X */14069, 13809, 13532, 13302, 13073, 12848, 12618, 12396, 12171, 11908, 
/*  14.X */11668, 11457, 11259, 11037, 10821, 10586, 10343, 10148,  9923,  9696, 
/*  15.X */ 9465,  9270,  9090,  8845,  8640,  8448,  8219,  8016,  7833,  7630, 
/*  16.X */ 7450,  7276,  7096,  6897,  6698,  6551,  6400,  6225,  6045,  5874, 
/*  17.X */ 5721,  5553,  5392,  5242,  5099,  4973,  4836,  4679,  4537,  4407, 
/*  18.X */ 4278,  4128,  3992,  3883,  3759,  3643,  3524,  3397,  3265,  3171, 
/*  19.X */ 3062,  2930,  2853,  2770,  2680,  2582,  2481,  2376,  2265,  2190, 
/*  20.X */ 2133 
};

/*******************************************************************************************************/
/* API_GetChannelEsNo() */
/*******************************************************************************************************/
BOOL     API_GetChannelEsNo(           /* function to measure channel esno (ver 1.0) */
NIM      *nim,                         /* pointer to nim */
ESNOMODE emode,                        /* type of results to return:  Snapshot or Average  (0 resets avg.buffer) */
CMPLXNO  *esno,                        /* returned esno value */
MSTATUS  *mstat)                       /* returned measurement status: (done, saturated, not-done) */
{
  int    softdec;
  long   esno_avg;
  long   esnoest;
  
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  /* test for allocated storage to hold results.  If none, reset esno to one-shot, clear avg. buffer */
  if (esno == NULL || mstat == NULL || emode == ESNOMODE_UNDEF)
  {
    nim->esno.last_esno = -1L;
    nim->esno.taps_idx = 0;
    nim->esno.taps_cnt = 0;

    /* clear the esno average buffer */
    memset(nim->esno.esno_taps,CNULL,sizeof(nim->esno.esno_taps));

    /* stop (possible) continious esno measurement */
    ulRegVal = 0x00UL;
    if (RegisterWrite(nim,CX24130_ESNOSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    return(True);
  }

  *mstat = MSTATUS_NOTDONE;

  /* read the esno value */
  if (RegisterRead(nim,CX24130_ESNORDY,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  nim->esno.esno_val = (long)ulRegVal;

  if (nim->esno.last_esno != -1L)
  {
    int   i;

    if (nim->esno.esno_val == nim->esno.last_esno)
    {
      /* esno not ready */
      DRIVER_set_complex(esno,-1L,1UL);
      *mstat = MSTATUS_NOTDONE;
      return(True);
    }

    /* read the esno estimate value */
    if (RegisterRead(nim,CX24130_ESNOCOUNT,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    esnoest = (long)ulRegVal;

    /* save the last esno counter, to look for change */
    nim->esno.last_esno = nim->esno.esno_val;

    /* (CR6780) calc esno_avg -> either actual average, or last value used as average */
    if (emode == ESNOMODE_SNAPSHOT)
    {
      nim->esno.taps_idx = 0;
      nim->esno.taps_cnt = 0;
    }

    /* place read esno into buffer, incr buffer count, roll when needed */
    nim->esno.esno_taps[nim->esno.taps_idx] = esnoest;
    nim->esno.taps_idx++;
    if (nim->esno.taps_cnt < MAX_ESNOTAPS)  nim->esno.taps_cnt++;
    if (nim->esno.taps_idx >= MAX_ESNOTAPS)  nim->esno.taps_idx = 0;
 
    /* measure esno average */
    for (i = 0 , esno_avg = 0 ; i < min(MAX_ESNOTAPS,nim->esno.taps_cnt) ; i++)
    {
      esno_avg += nim->esno.esno_taps[i];
    }
    esno_avg /= min(MAX_ESNOTAPS,nim->esno.taps_cnt);

    /* determine the table to read */
    if (RegisterRead(nim,CX24130_DMDSDTHRESH,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    softdec = (int)ulRegVal;

    /* set the table to look-up value into nim */
    if (softdec == 0)
	  nim->esno.table = ESNO_lowSNR;
    else  nim->esno.table = ESNO_highSNR;

    /* look-up esno in table */
    if (esno_avg > nim->esno.table[0])
    {
      nim->esno.esno = 20L;
      DRIVER_set_complex(esno,nim->esno.esno,10UL);
      *mstat = MSTATUS_DONE;
    }
    else
    {
      if (esno_avg < nim->esno.table[180])
      {
        nim->esno.esno = 200;
        DRIVER_set_complex(esno,nim->esno.esno,10UL);
        *mstat = MSTATUS_DONE;
      }
      else
      {
        for ( i = 0 ; i < 181 ; i++)
        {
          if ((esno_avg <= nim->esno.table[i]) && (esno_avg >= nim->esno.table[i+1]))
          {
            nim->esno.esno = (long)(i + 20L);
            DRIVER_set_complex(esno,nim->esno.esno,10UL);
            *mstat = MSTATUS_DONE;
            break;
          }
        }
      }
    }

    return(True);
  }
  else
  {
    /* cause one esno measurement */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_ESNOSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* save the last esno counter, to look for change */
    nim->esno.last_esno = nim->esno.esno_val;

    memset(nim->esno.esno_taps,CNULL,sizeof(nim->esno.esno_taps));
    nim->esno.taps_idx = 0;
  }
  
  return(True);

}  /* API_GetChannelEsNo() */


/*******************************************************************************************************/
/* API_GetPNBER() */
/*******************************************************************************************************/
BOOL     API_GetPNBER(                 /* function to turn-on system PN engine (only one NIM at a time) */
NIM      *nim,                         /* pointer to nim */
PNBER    errwindow,                    /* window size (one of a list of possible PN BERReadyCount[21:0] window sizes) */
CMPLXNO  *pnber,                       /* PN ber error rate */
MSTATUS  *mstat)                       /* returned measurement status: (done, saturated, not-done) */
{
  int    iberwinsz;          /* int ber window size */
  long   ber;

  unsigned long  ulRegVal;

  /* validate nim */
  if (DRIVER_ValidateNim(nim) == False)
  {
    nim->pnberbusy = -1;
    return(False);
  }

  if (pnber == NULL)
  {
    /* user passed NULL, so turn-off error collection, turn-off cont. measuremt. */
    nim->pnberbusy = -1;
    nim->pnber_not_rdy_count = 0;

    /* reset NIM->MPEG sync punct struct to initial state */
    if (nim->temp_SyncPunctMode != -1)
	nim->mpeg_out.SyncPunctMode = (SYNCPUNCTMODE)nim->temp_SyncPunctMode;

    /* write the current syncpunct setting to the hardware */
    ulRegVal = (nim->mpeg_out.SyncPunctMode == SYNC_WORD_PUNCTURED) ? 0x01UL : 0x00UL;
    if (RegisterWrite(nim,CX24130_MPGSYNCPUNC,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* disable PN BERReadyCount[21:0] */
    ulRegVal = 0x00;
    if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    return(True);
  }

  if (nim->pnberbusy == -1)
  {
    /* make sure that any other type of error measurement is turned-off before turning another one on */
    if (DRIVER_error_measurements_off(nim) == False)  return(False);

    nim->pnberbusy = 0;
    nim->pnber_not_rdy_count = 0;

    /* save current NIM->MPEG sync punct struct */
    nim->temp_SyncPunctMode = (int)nim->mpeg_out.SyncPunctMode;

    /* write the new syncpunct setting to the hardware */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_MPGSYNCPUNC,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* enable PN BERReadyCount[21:0] */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* set the initial PN polarity */
    nim->pnberpolarity = 0x01L;
    if (RegisterWrite(nim,CX24130_BERPNPOL,(unsigned long)nim->pnberpolarity, DEMOD_I2C_IO) == False)
	return(False);

    /* set the error window size, or use a default size */
    nim->berwindowsize = 0L;
    switch(errwindow)
    {
      case  PNBER_2_22:
      {
        nim->berwindowsize = 0x00L;
        break;
      }
      default:
      case  PNBER_2_23:
      {
        nim->berwindowsize = 0x01L;
        break;
      }
      case  PNBER_2_25:
      {
        nim->berwindowsize = 0x02L;
        break;
      }
    }
    if (RegisterWrite(nim,CX24130_BERPNERRWIN,(unsigned long)nim->berwindowsize, DEMOD_I2C_IO) == False)
	return(False);

    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_BERPNLOCK,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* start pn ber */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_STARTPNBER,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* set PN BERReadyCount[21:0] to ready */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_BERREADY,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* set the ber to 'not done' */
    CMPLX_set(pnber,-1L,1UL);
    *mstat = MSTATUS_NOTDONE;
        
  }
  else
  {
    unsigned long  pnberready;
    unsigned long  win = 0UL;

    /* set the ber to 'not done' */
    CMPLX_set(pnber,-1L,1UL);
    *mstat = MSTATUS_NOTDONE;

    /* Test if PNBER is ready */
    if (RegisterRead(nim,CX24130_BERREADY,&pnberready, DEMOD_I2C_IO) == False)
	return(False);
    if (pnberready != 0x01UL)
    {
      /* if PNBER is not ready for an extended amount of time, then flop the polarity, try again */
      nim->pnber_not_rdy_count++;
      if (nim->pnber_not_rdy_count > MAX_PNBER_NOTRDY)
      {
        /* try a new PN polarity */
        nim->pnberpolarity += 1L;
        nim->pnberpolarity &= 0x01L;
        if (RegisterWrite(nim,CX24130_BERPNPOL,(unsigned long)nim->pnberpolarity, DEMOD_I2C_IO) == False)
		return(False);
        nim->pnber_not_rdy_count = 0L;
      }
      return(True);
    }

    /* pnber is ready */
    nim->pnber_not_rdy_count = 0L;

    /* read the BERReadyCount[21:0] value from the demod */
    if (RegisterRead(nim,CX24130_BERCOUNT,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    ber = (long)ulRegVal;

    iberwinsz = (int)nim->berwindowsize;
    switch(iberwinsz)
    {
      case  0x00:  /* PNBER_2_22: */
      {
        win = (0x01L<<22L);
        break;
      }
      case  0x01:  /* PNBER_2_23: */
      {
        win = (0x01L<<23L);
        break;
      }
      case  0x02:  /* PNBER_2_25: */
      {
        win = (0x01L<<25L);
        break;
      }
      default:
      {
        /* driver should never hold this value in the window size */
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
        break;
      }
    }

    /* compute the PN BERReadyCount[21:0] rate, return to caller */
    CMPLX_set(pnber,ber,win);
    *mstat = MSTATUS_DONE;

    /* set PN BERReadyCount[21:0] to ready, continue (wait for next reading) */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_BERREADY,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  }
     
  return(True);

}  /* API_GetPNBER() */


/*******************************************************************************************************/
/* API_GetBER() */
/*******************************************************************************************************/
BOOL           API_GetBER(             /* function to retrieve  bit error rate value */
NIM            *nim,                   /* pointer to nim */
unsigned long  errwindow,              /* window size */
CMPLXNO        *berest,                /* returned BERReadyCount[21:0] estimate */
MSTATUS        *mstat)                 /* returned measurement status: (done, saturated, not-done) */
{
  unsigned long  ulRegVal;
    
  /* validate nim */
  if (DRIVER_ValidateNim(nim) == False)
  {
    nim->berbusy = -1;
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }
  else  if (berest == NULL)
  {
    /* user passed NULL, so turn-off error collection, turn-off cont. measuremt. */
    nim->berbusy = -1;
    DRIVER_errcount_disable(nim);
    if (RegisterWrite(nim,CX24130_BERSTART,0x00L, DEMOD_I2C_IO) == False)
	return(False);
    return(True);
  }

  if (nim->berbusy == -1)
  {
    /* make sure that any other type of error measurement is turned-off before turning another one on */
    if (DRIVER_error_measurements_off(nim) == False)
	return(False);

    /* enable BERReadyCount[21:0] collection */
    DRIVER_errcount_disable(nim);
    DRIVER_errcount_enable(nim,RSERRCNT_BIT);

    /* set the  BERReadyCount[21:0] window size */
    ulRegVal = errwindow;
    if (RegisterWrite(nim,CX24130_BERRSERRWIN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* read the window counter (this is the initial count) */
    if (RegisterRead(nim,CX24130_BERSTART,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    nim->berbusy = (long)ulRegVal;

    /* set continuous measurement on */
    ulRegVal = 0x01;
    if (RegisterWrite(nim,CX24130_BERSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* disable PN ber */
    ulRegVal = 0x00;
    if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* sample is not yet ready */
    DRIVER_set_complex(berest,-1L,1UL);
    *mstat = MSTATUS_NOTDONE;

  } 
  else
  {
    unsigned long  errmult;
    TRANSPEC  transpec;

    /* compute error mult, then get error count if ready */
    if (API_GetTransportSpec(nim,&transpec) == False)
	return(False);
    errmult =  (256UL * 204UL * 8UL);
    DRIVER_BBB_errinfo(nim,&nim->berbusy,berest,errwindow,errmult,mstat);
  }

  return(True);

}  /* API_GetBER() */


/*******************************************************************************************************/
/* API_GetByteErrors() */
/*******************************************************************************************************/
BOOL           API_GetByteErrors(      /* function to retrieve  byte error rate */
NIM            *nim,                   /* pointer to nim */
unsigned long  errwindow,              /* window size */
CMPLXNO        *byteerr,               /* byte error rate */
MSTATUS        *mstat)                 /* returned measurement status: (done, saturated, not-done) */
{
  unsigned long  ulRegVal;

  /* validate nim */
  if (DRIVER_ValidateNim(nim) == False)
  {
    nim->bytebusy = -1;
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }
  else  if (byteerr == NULL)
  {
    /* user passed NULL, so turn-off error collection, turn-off cont. measuremt. */
    nim->bytebusy = -1;
    DRIVER_errcount_disable(nim);
    if (RegisterWrite(nim,CX24130_BERSTART,0x00L, DEMOD_I2C_IO) == False)
	return(False);
    return(True);
  }

  if (nim->bytebusy == -1)
  {
    /* make sure that any other type of error measurement is turned-off before turning another one on */
    if (DRIVER_error_measurements_off(nim) == False)  return(False);

    /* enable BERReadyCount[21:0] collection */
    DRIVER_errcount_disable(nim);
    DRIVER_errcount_enable(nim,RSERRCNT_BYTE);

    /* set the default BERReadyCount[21:0] window size */
    ulRegVal = errwindow;
    if (RegisterWrite(nim,CX24130_BERRSERRWIN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* read the window counter (this is the initial count) */
    if (RegisterRead(nim,CX24130_BERSTART,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    nim->bytebusy = (long)ulRegVal;

    /* set continuous measurement on */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_BERSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* disable PN ber */
    ulRegVal = 0x00UL;
    if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* sample is not yet ready */
    DRIVER_set_complex(byteerr,-1L,1UL);
    *mstat = MSTATUS_NOTDONE;

  } 
  else
  {
    unsigned long  errmult;
    TRANSPEC  transpec;

    /* compute error mult, then get error count if ready */
    if (API_GetTransportSpec(nim,&transpec) == False)
	return(False);

    /* get error count if ready */
    errmult = (256UL * 204UL);
    DRIVER_BBB_errinfo(nim,&nim->bytebusy,byteerr,errwindow,errmult,mstat);
  }

  return(True);

}  /* API_GetByteErrors() */


/*******************************************************************************************************/
/* API_GetBlockErrors() */
/*******************************************************************************************************/
BOOL           API_GetBlockErrors(     /* function to return error count */
NIM            *nim,                   /* pointer to nim */
unsigned long  errwindow,              /* error window length */
CMPLXNO        *blkerrcnt,             /* block error count */
MSTATUS        *mstat)                 /* returned measurement status: (done, saturated, not-done) */
{
  unsigned long  ulRegVal;

  /* validate nim */
  if (DRIVER_ValidateNim(nim) == False)
  {
    nim->blockbusy = -1;
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }
  else  if (blkerrcnt == NULL)
  {
    /* user passed NULL, so turn-off error collection, turn-off cont. measuremt. */
    nim->blockbusy = -1L;
    DRIVER_errcount_disable(nim);
    if (RegisterWrite(nim,CX24130_BERSTART,0x00L, DEMOD_I2C_IO) == False)
	return(False);
    return(True);
  }

  if (nim->blockbusy == -1L)
  {
    /* make sure that any other type of error measurement is turned-off before turning another one on */
    if (DRIVER_error_measurements_off(nim) == False)
	return(False);

    /* enable BERReadyCount[21:0] collection */
    DRIVER_errcount_disable(nim);
    DRIVER_errcount_enable(nim,RSERRCNT_BLOCK);

    /* if the passed-in errorwindow size is zero, set infinite window size */    
    if (errwindow == 0UL)
    {
      ulRegVal = 0x01UL;
      RegisterWrite(nim,CX24130_BERRSINFWINEN,ulRegVal, DEMOD_I2C_IO);
    }

    /* set the window size */
    ulRegVal = errwindow;
    if (RegisterWrite(nim,CX24130_BERRSERRWIN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* read the window counter (this is the initial count) */
    if (RegisterRead(nim,CX24130_BERSTART,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
    nim->blockbusy = (long)ulRegVal;

    /* set continuous measurement on */
    ulRegVal = 0x01UL;
    if (RegisterWrite(nim,CX24130_BERSTART,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* disable PN ber */
    ulRegVal = 0x00UL;
    if (RegisterWrite(nim,CX24130_BERERRORSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

    /* sample is not yet ready */
    DRIVER_set_complex(blkerrcnt,-1L,1UL);
    *mstat = MSTATUS_NOTDONE;

  } 
  else
  {
    /* get error count if ready */
    unsigned long  errmult = 256UL;
    DRIVER_BBB_errinfo(nim,&nim->blockbusy,blkerrcnt,errwindow,errmult,mstat);
  }

  return(True);

}  /* API_GetBlockErrors() */


/*******************************************************************************************************/
/* API_GetNormCount() */
/*******************************************************************************************************/
BOOL           API_GetNormCount(       /* function to return normalization counter */
NIM            *nim,                   /* pointer to nim */
unsigned char  *normcounter)           /* normalization counter */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  /* read the entire register, pull-out pertinent data */
  if (RegisterRead(nim,CX24130_ACQVITNORMCOUNT,&ulRegVal, DEMOD_I2C_IO) != True)
	return(False);

  if (normcounter == NULL)
  {
	DRIVER_SET_ERROR(nim,API_BADPTR);
  } else
  {
    *normcounter = (unsigned char)ulRegVal;
    return(True);
  }

  return(False);

}  /* API_GetNormCount() */


/*******************************************************************************************************/
/* API_GetFrequencyOffset() */
/*******************************************************************************************************/
BOOL   API_GetFrequencyOffset(         /* function to calculate frequency offset value */
NIM    *nim,                           /* pointer to nim */
long   *freqoffset)                    /* returned calculated freq. offset value */
{
  long   diff;                   /* difference from actual freq. (pllfreq) and requested (nim->freq.) */

  unsigned long  pllfreq;        /* pll freq tuner is tuned to */

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (freqoffset != NULL)
  {
    /* get the tuner pll settings, calculate the deviation */
    if (TUNER_GetPLLFrequency(nim,&pllfreq) == False)
	return(False);

    diff = (long)(pllfreq - nim->pll_frequency) * -1L;

    /* if (pllfreq > nim->frequency)
	diff = (long)(pllfreq-nim->frequency)*-1L; */
    /* else  diff = (long)(nim->frequency - pllfreq); */
  
    *freqoffset = diff;
    return(True);
  }

  DRIVER_SET_ERROR(nim,API_BADPTR);
  return(False);

}  /* API_GetFrequencyOffset() */


/*******************************************************************************************************/
/* API_GetAcquisitionOffset() */
/*******************************************************************************************************/
BOOL   API_GetAcquisitionOffset(       /* function to return acq. offset */
NIM    *nim,                           /* pointer to nim */
long   *lnboffset)                     /* returned acq. offset value */
{
  int    neg = 1;

  long   freqoffset;

  unsigned long  ulRegVal;
  unsigned long  samplerate = 0UL;
  unsigned long  curcent = 0UL;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (lnboffset != NULL)
  {
    if (API_GetSampleFrequency(nim,&samplerate) == True)
    {
      BCDNO  bcd;
      long   freq;
      long   fi_diff;

      ulRegVal = 0UL;
      if (RegisterWrite(nim,CX24130_ACQPRFREQRDSEL,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);

      if (DRIVER_Cobra(nim) == True)
      {
        if (RegisterRead(nim,CX24130_ACQPRFREQCURR,&ulRegVal, DEMOD_I2C_IO) != True)
             return(False);

         *lnboffset = DRIVER_convert_twos(ulRegVal,12);
      } else
      if (DRIVER_Camaric(nim) == True)
      {
        if (RegisterReadCentralFreq(nim,&curcent) != True)
	    return(False);

          /* sign-extend the 13-bit two's comp value to a 32 bit long */
         *lnboffset = DRIVER_convert_twos(curcent,13);
      }

      if (*lnboffset < 0L)
      {
		neg = -1;
		*lnboffset = ((*lnboffset)*-1L);
      }

      /* calculate the acq offset */
      BCD_set(&bcd,(unsigned long)*lnboffset);
      BCD_mult(&bcd,samplerate);
      BCD_mult(&bcd,100000L);
      BCD_div(&bcd,(0x01L<<15));
      BCD_div(&bcd,100000L);
      
      /* return computed lnb offset to the caller */
      *lnboffset = (long)BCD_out(&bcd);
      *lnboffset *= neg;

      if (API_GetCTL(nim,&freqoffset) == False)
	return(False);

      *lnboffset = (*lnboffset + freqoffset);
      *lnboffset *= -1L;
      if (API_GetFrequencyOffset(nim,&freq) == False)
	return(False);
      *lnboffset -= freq;
 
      fi_diff = 0L;
      if (nim->freq_ideal >= nim->pll_frequency)
	fi_diff = ((long)(nim->freq_ideal - nim->pll_frequency) * -1L);
      fi_diff = (long)(nim->pll_frequency - nim->freq_ideal);

      *lnboffset += fi_diff;

#if INCLUDE_ROSIE
	  if(nim->tuner_type != CX24108)
	  {
		  *lnboffset *= -1L;
	  }
#endif

      return(True);
    }
  }

  return(False);

}  /* API_GetAcquisitionOffset() */


/*******************************************************************************************************/
/* API_SetCentralFreq() */
/*******************************************************************************************************/
BOOL   API_SetCentralFreq(             /* function to set demod's central frequency register */
NIM    *nim,                           /* pointer to nim */
long   centralfreq)                    /* central freq. value */
{
	int    neg = 1;
	long   freq;

	unsigned long  ulRegVal;
	unsigned long  samplerate;

	BCDNO  bcd;

	/* validate nim */
	DRIVER_VALIDATE_NIM(nim);

	if (API_GetSampleFrequency(nim, &samplerate) == False)  return(False);

	if (centralfreq < 0L)
	{
		neg = -1;
		centralfreq = (centralfreq * -1L);
	}

	BCD_set(&bcd,(unsigned long)centralfreq);
	BCD_mult(&bcd,(0x01<<15));
	BCD_div(&bcd,samplerate);

	freq = (long)BCD_out(&bcd);
	freq = (freq * neg);

	if (DRIVER_Cobra(nim) == True)
	{
		ulRegVal = DRIVER_convert_twos_saturate(freq, Register_bitlength(CX24130_ACQPRFREQNOM));

		if (RegisterWrite(nim,CX24130_ACQPRFREQNOM,ulRegVal, DEMOD_I2C_IO) != True)
		return(False);
	} else
	if (DRIVER_Camaric(nim) == True)
	{
		/* convert the 32 bit long to 13-bit two's comp value */
		ulRegVal = DRIVER_convert_twos_saturate(freq,13);
		if (RegisterWriteCentralFreq(nim,ulRegVal) != True)
		{
			return(False);
		}
	}

	return(True);
}  /* API_SetCentralFreq() */


/*******************************************************************************************************/
/* API_GetCentralFreq() */
/*******************************************************************************************************/
BOOL   API_GetCentralFreq(             /* function to read demod's central freq. register */
NIM    *nim,                           /* pointer to nim */
long   *centralfreq)                   /* returned central freq. value */
{
  int    neg = 1;

  long   freq;

  unsigned long  ulRegVal;
  unsigned long  samplerate;

  BCDNO  bcd;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (API_GetSampleFrequency(nim,&samplerate) == False)
	return(False);

  if (centralfreq != NULL)
  {
    /* read the current register setting, reset it as required */
    ulRegVal = 0UL;  /* was 0x01 (FEDR) prior to CR 5710 */
    if (RegisterWrite(nim,CX24130_ACQPRFREQRDSEL,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);

	if (DRIVER_Cobra(nim) == True)
	{
	    if (RegisterRead(nim,CX24130_ACQPRFREQCURR, &ulRegVal, DEMOD_I2C_IO) != True)
		return(False);

	    /* sign-extend the 13-bit two's comp value to a 32 bit long */
            freq = DRIVER_convert_twos(ulRegVal, Register_bitlength(CX24130_ACQPRFREQCURR));
	} else
	if (DRIVER_Camaric(nim) == True)
	{
            /* read the current central freq register */
            if (RegisterReadCentralFreq(nim,&ulRegVal) != True)
              return(False);

            /* sign-extend the 13-bit two's comp value to a 32 bit long */
            freq = DRIVER_convert_twos(ulRegVal,13);
	}

    if (freq < 0L)
    {
      neg = -1;
      freq = (freq * -1L);
    }

    BCD_set(&bcd,(unsigned long)freq);
    BCD_mult(&bcd,samplerate);
    BCD_div(&bcd,(0x01UL<<15));

    *centralfreq = (long)BCD_out(&bcd);
    *centralfreq *= neg;

    return(True);
  }
  
  DRIVER_SET_ERROR(nim,API_BADPTR);
  return(False);

}  /* API_GetCentralFreq() */


/*******************************************************************************************************/
/* API_GetCTL() */
/*******************************************************************************************************/
BOOL  API_GetCTL(                      /* function to read demod's CTL register */
NIM   *nim,                            /* pointer to nim */
long  *ctl)                            /* returned ctl value to caller */
{
  int    neg = 1;

  long   laccum;

  unsigned long  ulRegVal;
  unsigned long  samplerate;
  unsigned long  accum;

  BCDNO  bcd;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (ctl != NULL)
  {
    /* set accum, sub-accum (to allow read desired count) */
	ulRegVal = 3UL;
    if (RegisterWrite(nim,CX24130_DMDACCUMSEL,ulRegVal,DEMOD_I2C_IO) != True)
	return(False);
    ulRegVal = 0UL;
    if (RegisterWrite(nim,CX24130_DMDSUBACMSEL,ulRegVal,DEMOD_I2C_IO) != True)
	return(False);

    
    /* read the shared accum register */
    if (RegisterRead(nim,CX24130_DMDACCUMVAL,&accum, DEMOD_I2C_IO) != True)
	return(False);

    laccum = DRIVER_convert_twos(accum,Register_bitlength(CX24130_DMDACCUMVAL));

    if (laccum < 0L)
    {
      neg = -1;
      laccum = (laccum * -1L);
    }

    /* read the sample rate */
    if (API_GetSampleFrequency(nim,&samplerate) == True)
    {
      /* compute the CTL value */
      BCD_set(&bcd,(unsigned long)laccum);
      BCD_mult(&bcd,samplerate);
      BCD_div(&bcd,(0x01UL<<11L));

      *ctl = (long)BCD_out(&bcd);
      *ctl *= neg;

      return(True);
    }
  }

  DRIVER_SET_ERROR(nim,API_BADPTR);
  return(False);

}  /* API_GetCTL() */


/*******************************************************************************************************/
/* API_EnableRSCorrection() */
/*******************************************************************************************************/
BOOL  API_EnableRSCorrection(          /* function to enable demod's RS correction */
NIM   *nim,                            /* pointer to nim */
BOOL  opt)                             /* True=enables, False=disables */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  switch(opt)
  {
    case  True:
    case  False:
    {
      /* write the RS correction flag to the demod */
      ulRegVal = (opt == True ? 0x00UL : 0x01UL);
      if (RegisterWrite(nim,CX24130_RSFECDIS,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      return(True);
      break;
    }
    default:
    {
      break;
    }
  }

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_EnableRSCorrection() */


/*******************************************************************************************************/
/* API_GetAGCAcc() */
/*******************************************************************************************************/
BOOL    API_GetAGCAcc(                 /* function to read the demod AGC accumulator value */
NIM     *nim,                          /* pointer to nim */
AGCACC  *agcacc)                       /* AGC accumulator value returned to caller */
{
  unsigned long  ulTemp;
  unsigned long  accum;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (agcacc != NULL)
  {
    /* set accum, sub-accum (to allow read desired count) */
    ulTemp = 0UL;
    if (RegisterWrite(nim,CX24130_DMDACCUMSEL,ulTemp,DEMOD_I2C_IO) != True)
	return(False);
    ulTemp = 0UL;
    if (RegisterWrite(nim,CX24130_DMDSUBACMSEL,ulTemp,DEMOD_I2C_IO) != True)
	return(False);
	

    /* read the shared accum register */
    if (RegisterRead(nim,CX24130_DMDACCUMVAL,&accum, DEMOD_I2C_IO) != True)
	return(False);

    ulTemp = DRIVER_convert_twos(accum,Register_bitlength(CX24130_DMDACCUMVAL));
    *agcacc = (AGCACC)ulTemp;

    return(True);
  }

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_GetAGCAcc() */


/*******************************************************************************************************/
/* API_GetBTL() */
/*******************************************************************************************************/
BOOL   API_GetBTL(                     /* function to read the BTL register from the demod */
NIM    *nim,                           /* pointer to nim */
long   *btl)                           /* BLT register value returned to caller */
{
  unsigned long  ulRegVal;
  unsigned long  accum;

  BCDNO  bcd;
  unsigned long  samplerate;
  long   laccum;
  int    neg = 1;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (btl != NULL)
  {
    /* set accum, sub-accum (to allow read desired count) */
    ulRegVal = 2UL;
    if (RegisterWrite(nim,CX24130_DMDACCUMSEL,ulRegVal,DEMOD_I2C_IO) != True)
	return(False);
    ulRegVal = 0UL;
    if (RegisterWrite(nim,CX24130_DMDSUBACMSEL,ulRegVal,DEMOD_I2C_IO) != True)
	return(False);
	

    /* read the shared accum register */
    if (RegisterRead(nim,CX24130_DMDACCUMVAL,&accum, DEMOD_I2C_IO) != True)
	return(False);

    /* convert to two's comp number */
    laccum = DRIVER_convert_twos(accum,Register_bitlength(CX24130_DMDACCUMVAL));

    if (laccum < 0L)  
    {
      neg = -1;
      laccum = (laccum * -1);
    }

    /* read the sample rate */
    if (API_GetSampleFrequency(nim,&samplerate) == False)
	return(False);

    /* compute BTL -- (accum * samplerate) / 2^12 */
    BCD_set(&bcd,(unsigned long)laccum);
    BCD_mult(&bcd,samplerate);
    BCD_mult(&bcd,100000L);
    BCD_div(&bcd,(0x01L<<12));
    BCD_div(&bcd,100000L);
    laccum = (long)BCD_out(&bcd);
    
    laccum *= neg;
    
    *btl = laccum;

    return(True);
  }

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_GetBTL () */


/*******************************************************************************************************/
/* API_SetLNBDC() */
/*******************************************************************************************************/
BOOL    API_SetLNBDC(                  /* sets voltage to LNb to either high (~18v) or low (~13v) */
NIM     *nim,                          /* pointer to nim */
LNBPOL  lnbpol)                        /* LNB_HIGH or LNB_LOW */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  switch(lnbpol)
  {
    case  LNB_HIGH:
    case  LNB_LOW:
    {
      ulRegVal = (lnbpol == LNB_HIGH ? 0x01UL : 0x00UL);
      if (RegisterWrite(nim,CX24130_LNBDC,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      return(True);
      break;
    }
    case  LNB_UNDEF:
    default:
    {
      break;
    }
  }  /* switch(... */
  
  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_SetLNBDC() */



/*******************************************************************************************************/
/* API_SetLNBMode() */
/*******************************************************************************************************/
BOOL      API_SetLNBMode(              /* Function to set LNB mode register settings */
NIM       *nim,                        /* pointer to nim */
LNBMODE   *lnbmode)                    /* pointer to LNB mode struct (or NULL to use default settings) */
{
  /* validate nim and mpeg storage */
  DRIVER_VALIDATE_NIM(nim);

  /* if user has passed-in a NULL, use default settings */
  if (lnbmode == NULL)
  {
    LNBMODE  lmode;

    /* extract default value from the register settings */
    lmode.cycle_count = (unsigned int)demod_register_map[CX24130_LNBBURSTLENGTH].default_value;

    /* set the default value from Datasheet */
    lmode.lnb_mode = LNBMODE_TONE;

    return(DRIVER_SetLNBMode(nim,&lmode));

  }

  /* send the user-defined settings to the driver */
  return(DRIVER_SetLNBMode(nim,lnbmode));

}  /* API_SetLNBMode() */


/*******************************************************************************************************/
/* API_SetLNBTone() */
/*******************************************************************************************************/
BOOL     API_SetLNBTone(               /* function to turn-on or off LNB tone */
NIM      *nim,                         /* pointer to nim */
LNBTONE  lnbtone)                      /* LNBTONE_ON or LNBTONE_OFF */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  switch(lnbtone)
  {
    case  LNBTONE_ON:
    case  LNBTONE_OFF:
    {
      ulRegVal = (lnbtone == LNBTONE_ON ? 0x01UL : 0x00UL);
      if (RegisterWrite(nim,CX24130_LNBTONE,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      return(True);
      break;
    }
    case  LNBTONE_UNDEF:
    default:
    {
      break;
    }
  }

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_SetLNBTone() */



/*******************************************************************************************************/
/* API_SendDiseqcMessage() */
/*******************************************************************************************************/
BOOL          API_SendDiseqcMessage(   /* function to send diseqc message */
NIM           *nim,                    /* pointer to nim */ 
unsigned char *message,                /* message to send */
unsigned char message_length,          /* length in BYTEs of message to send */
BOOL          last_message,            /* indicates if current message is the last message */
LNBBURST      bursttype)               /* burst-type at last-message: (modulated/un-mod'd) */
{
  unsigned char  outlen;
  unsigned char  last_len;
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);
  
  /* test for bad message pointer */
  if (message == NULL || message_length == 0)
  {
     DRIVER_SET_ERROR(nim,API_BAD_PARM);
     return(False);
  }

  /* (CR 7489) */
  if (last_message == True)
  {
    switch(bursttype)
    {
      case  LNBBURST_MODULATED:
      case  LNBBURST_UNMODULATED:
      {
        /* write tone-burst-type to reg.  0x01 = modulated; 0x00 = un-modulated */
        ulRegVal = (bursttype == LNBBURST_MODULATED  ? 0x01 : 0x00 );
        if (RegisterWrite(nim,CX24130_LNBBURSTMODSEL,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        break;
      }
      default:
      {
        DRIVER_SET_ERROR(nim,API_BAD_PARM);
        return(False);
      }
    }  /* switch(... */
  }

  /* demod should be ready to receive new message (if not, record warning, continue) */
  if (RegisterRead(nim,CX24130_LNBSENDMSG,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  if (ulRegVal != 0x01UL)
  {
	DRIVER_SET_ERROR(nim,API_LNB_BUSY);
  }

  /* load the nim with the message to be sent (API_Monitor() will process long messages) */
  if (message_length <= 6)
  {
    if (DRIVER_SendDiseqc(nim,message,message_length,False,last_message) == False)
	return(False);
  }
  else
  {
    BOOL  longmessage;

    /* break long message into a number of smaller parts and pieces */
    outlen = (unsigned char)(3+(message_length%3));
    if (DRIVER_SendDiseqc(nim,message,outlen,True,last_message) == False)
	return(False);

    while (outlen < message_length)
    {
      last_len = (unsigned char)((message_length - outlen) > 6 ? 6 : (message_length - outlen));
      
      /* determine if this is the last message (lastmessage == False if this is the last message) */
      longmessage = True;
      if ((last_len+outlen) >= message_length)  longmessage = False;

      /* (CR 7488) added two lines: */
      ulRegVal = (longmessage == True ? 0x01UL : 0x00UL);
      if (RegisterWrite(nim,CX24130_LNBLONGMSG,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

      /* special processing for last message */
      if (last_message == True)
      {
        /* last message... if  current message is last portion, then set the more-msg flag to zero */
        if ((outlen + last_len) >= message_length)  
        {
          /* current msg is last, so set tone-burst to play after end-of-current message */
          longmessage = False;
          ulRegVal = 0x00UL;
          if (RegisterWrite(nim,CX24130_LNBMOREMSG,ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        }
      }

      /* send portion of long message to demod (via driver) */
      if (DRIVER_SendDiseqc(nim,&message[outlen],last_len,longmessage,last_message) == False)
	return(False);
      outlen = (unsigned char)(outlen + last_len);
    }
  }

  return(True);

}  /* API_SendDiseqcMessage() */


/*******************************************************************************************************/
/* API_ReadReg() */
/*******************************************************************************************************/
BOOL          API_ReadReg(             /* function to read a byte-wide demod register */
NIM           *nim,                    /* pointer to nim */
int           reg,                     /* register to read */
unsigned char *data)                   /* pointer to data where read BYTE will be placed */
{
   /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (data == NULL)  return(False);

  /* read byte from the hardware */
  data[0] = (unsigned char)((*nim->SBRead)(nim->ptuner, nim->demod_handle,(unsigned char)reg,&nim->iostatus)); 
  if (nim->iostatus != 0UL)
	return(False);
  
  return(True);

}  /* API_ReadReg() */


/*******************************************************************************************************/
/* API_WriteReg() */
/*******************************************************************************************************/
BOOL           API_WriteReg(           /* function to write to a byte-wide demod register */
NIM            *nim,                   /* pointer to nim */
int            reg,                    /* register to write to */
unsigned char  *data)                  /* pointer to data (unsigned char) to write */
{
  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (data == NULL)  return(False);

  /* write byte to the hardware */
  (*nim->SBWrite)(nim->ptuner, nim->demod_handle,(unsigned char)(reg),data[0],&nim->iostatus);
  if (nim->iostatus != 0UL)
	return(False);
  
  return(True);

}  /* API_WriteReg() */



/*******************************************************************************************************/
/* API_GetPLLFrequency() */
/*******************************************************************************************************/
BOOL           API_GetPLLFrequency(    /* function to retrieve  the tuner PLL freq. during lock/acq */
NIM            *nim,                   /* pointer to nim */
unsigned long  *pllfreq)               /* pointer to storage for return value */
{
  unsigned long  freq = 0UL;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  /* watch for div by zero, too */
  if (pllfreq == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  if (TUNER_GetPLLFrequency(nim,&freq) == False)
	return(False);
  *pllfreq = freq;

  return(True);

}  /* API_GetPLLFrequency() */


/*******************************************************************************************************/
/* API_SetSleepMode() */
/*******************************************************************************************************/
BOOL  API_SetSleepMode(                /* function to set demod sleep mode */
NIM   *nim,                            /* pointer to nim */
BOOL  sleep)                           /* sleep option: True = set demod to sleep, False = wake demod */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  switch (sleep)
  {
    case  True:
    case  False: 
    {
      ulRegVal = (sleep == True ? 0x01UL : 0x00UL);
      if (RegisterWrite(nim,CX24130_SYSSLEEP,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      return(True);
      break;
    }
    default:
    {
      break;
    }
  }  /* switch(... */

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_SetSleepMode() */


/*******************************************************************************************************/
/* API_OS_Time() --> API_Time()  */
/*******************************************************************************************************/
unsigned long  API_OS_Time()               /* function to get system time or clock count */
{
  /* return ANSI standard 'C' clock() system timer */
//  return((unsigned long)clock());
   return 0;
  
}  /* API_Time() */


/*******************************************************************************************************/
/* API_OS_Wait() --> API_Wait() */
/*******************************************************************************************************/
BOOL     API_OS_Wait(                      /* function to wait a number of ms */
NIM      *nim,                         /* pointer to nim */
int      waitms)                       /* no. of ms to wait */
{
  /* allow wait function to call used-defined wait, unless none is specified */
  if (nim->wait == NULL)
	nim->wait = &_DRIVER_wait;

  /* execute driver wait (use os_timer() supplied by app */
  return(nim->wait(nim,waitms));

}  /* API_OS_Wait() */


/*******************************************************************************************************/
/* API_SetDriverWait() */
/*******************************************************************************************************/
BOOL  API_SetDriverWait(                    /* Function to set used-defined wait function */
NIM   *nim,                                 /* pointer to nim */
BOOL  (*waitfunct)(NIM *nim,int mscount))   /* pointer to used-defined wait function */
{
  if (waitfunct != NULL)
	nim->wait = waitfunct;
  else  nim->wait = &_DRIVER_wait;

  return(True);

}  /* API_SetDriverWait() */


/*******************************************************************************************************/
/* API Tuner Functions *********************************************************************************/
/*******************************************************************************************************/

/*******************************************************************************************************/
/* API_SetTunerFrequency() - function to set the tuner frequency */
/*******************************************************************************************************/
BOOL  API_SetTunerFrequency(NIM *nim, unsigned long freq)
{	/* test for valid nim */
	DRIVER_VALIDATE_NIM(nim);

#if INCLUDE_ROSIE
	if(nim->tuner_type == CX24108)
	{
        /* test that desired freq is within valid range */
        if (freq < (((unsigned long)FREQ_TUNE_LOW)*MM) || freq > (((unsigned long)FREQ_TUNE_HIGH)*MM))
        {
            DRIVER_SET_ERROR(nim,API_PARM_RANGE);
            return(False);
        }
	}
#endif /* INCLUDE_ROSIE */

	/* preserve the desired frequency */
	nim->freq_ideal = freq;

	return (DRIVER_SetTunerFrequency(nim,freq));
}  /* API_SetTunerFrequency() */


/*******************************************************************************************************/
/* API_GetTunerFrequency() */
/*******************************************************************************************************/
BOOL           API_GetTunerFrequency(  /* function to retrieve  last tuner freq. setting from the nim */
NIM            *nim,                   /* pointer to nim */
unsigned long  *freq)                  /* pointer to storage for return value */
{
	/* test for valid nim */
	DRIVER_VALIDATE_NIM(nim);

	if (freq == NULL)
	{
		DRIVER_SET_ERROR(nim,API_BAD_PARM);
		return(False);
	}

	*freq = nim->pll_frequency;

	return(True);
}  /* API_GetTunerFrequency() */


/*******************************************************************************************************/
/* API_SetTunerBW() */
/*    inputs:  nim, tunerBW in kHz */
/*******************************************************************************************************/
BOOL    API_SetTunerBW(       /* function to set the tunerBW in kHz */
NIM     *nim,                 /* pointer to nim */
unsigned long tunerBW,        /* tunerBW in kHz */
long    *sigmaDelta)          /* returned sigmaDelta. */
{
   unsigned long  ulRegValue;

   *sigmaDelta = 0;
   /* calculate the voltage setting */
   if (TUNER_SetFilterBandwidth(nim,tunerBW) == False)
	return(False);

   /* write voltage setting to the demod */
   if (DRIVER_SetTunerFilterBWVoltage(nim, nim->antialias_mV_setting) == False)
	return(False);

   *sigmaDelta = 0;
#if INCLUDE_ROSIE
   if (nim->tuner_type == CX24108)
   {
        if (RegisterRead(nim,CX24130_FILVALUE,&ulRegValue, DEMOD_I2C_IO) == False)
		return (False);
        *sigmaDelta = DRIVER_convert_twos(ulRegValue, 10);
   }
#endif

   return True;
}  /* API_SetTunerBW() */

/*******************************************************************************************************/
/* API_TstateMpegOutputs() */
/*******************************************************************************************************/
BOOL     API_TstateMpegOutputs(        /* function to set the tstate mpeg output pins on the demod */
NIM      *nim,                         /* nim pointer */
TSTATE   tvalue)                       /* TSTATE value to program to demod */
{
  unsigned long  ulRegVal;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  switch(tvalue)
  {
    case  TSTATE_MPEG_OFF:
    case  TSTATE_MPEG_ON:
    {
      /* set the tstate mpeg output pins as desired */
      ulRegVal = (tvalue == TSTATE_MPEG_ON ? 0x01UL : 0x00UL);

      /* write setting to the mpeg hiz registers */
      if (RegisterWrite(nim,CX24130_MPGCNTL1_HIZ,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      if (RegisterWrite(nim,CX24130_MPGCNTL2_HIZ,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      if (RegisterWrite(nim,CX24130_MPGCLKHIZ,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      if (RegisterWrite(nim,CX24130_MPGDATA_HIZ,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      if (RegisterWrite(nim,CX24130_MPGDATA1_HIZ,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      if (RegisterWrite(nim,CX24130_MPGDATA0_HIZ,ulRegVal, DEMOD_I2C_IO) != True)
	return(False);
      return(True);
      break;
    }
    case  TSTATE_UNDEF:
    default:
    {
      break;
    }

  }  /* switch(... */

  DRIVER_SET_ERROR(nim,API_BAD_PARM);
  return(False);

}  /* API_TstateMpegOutputs() */


/*******************************************************************************************************/
/* API_GetEffectiveFrequency() */
/*******************************************************************************************************/
BOOL           API_GetEffectiveFrequency(   /* function to return effective (actual locked) frequency */
NIM            *nim,                        /* nim pointer */
unsigned long  *effectfrq)                  /* effective (actual) frequency */
{
  unsigned long  effect;
  unsigned long  Ft;
  long   lnboffset;

  /* validate nim */
  DRIVER_VALIDATE_NIM(nim);

  if (effectfrq == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  /* retrieve  PLL and lnb freqs */
  /* (CR 8136) if (API_GetPLLFrequency(nim,&pll) != True)  return(False);*/

  if (API_GetTunerFrequency(nim,&Ft) != True)
	return(False);

  if (API_GetAcquisitionOffset(nim,&lnboffset) != True)
	return(False);

  if (nim->actual_tuner_offset <= 0)
	Ft += (labs(nim->actual_tuner_offset));
  else  Ft -= labs(nim->actual_tuner_offset);

  /* compute effective freq */
  if (lnboffset < 0L)
	effect = Ft-(unsigned long)labs(lnboffset);
  else  effect = Ft+(unsigned long)labs(lnboffset);


  *effectfrq = effect;

  return(True);

}  /* API_GetEffectiveFrequency() */


/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

#ifdef OPTIMAL_FS_CODE                 /* Fs optimization */
/*******************************************************************************************************/
/* API_Opt_Fs_Enable() */
/*******************************************************************************************************/
BOOL  API_Opt_Fs_Enable(               /* TEMP function to disable (CR 7707)  */
NIM   *nim,                            /* pointer to nim */
BOOL  flag)                            /* If True, Fs code is enabled;  If False, Fs code is disabled */
{
  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  nim->opt_fs_disable = (flag == False ? True : False);

  return(True);

}  /* API_Opt_Fs_Enable() */


/*******************************************************************************************************/
/* API_Opt_Fs_Disable() */
/*******************************************************************************************************/
BOOL  API_Opt_Fs_Disable(              /* TEMP function to disable (CR 7482)  */
NIM   *nim)                            /* pointer to nim */
{
  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  nim->opt_fs_disable = True;

  return(True);

}  /* API_Opt_Fs_Disable() */
#endif  /* #ifdef OPTIMAL_FS_CODE */


/*******************************************************************************************************/
/* Cobra specific APIs */
/*******************************************************************************************************/
/*******************************************************************************************************/
/* API_DiseqcGetVersion() - common for both diseqc and echostar legacy */
/*******************************************************************************************************/
BOOL        API_DiseqcGetVersion(      /* funct to get current demod Diseqc setting */
NIM         *nim,                      /* pointer to nim */
DISEQC_VER  *dv)                       /* storage where value will be returned */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  if (dv == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  /* set the Diseqc mode */
  if (RegisterRead(nim,CX24123_LNBSMEN,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  *dv = DISEQC_VER_UNDEF;

  switch ((int)ulRegVal)
  {
    case  0x00:
    {
      /* if demod is cobra, then diseqc type is 1.x;  if Cobra, then 2.x */
      *dv = DISEQC_VER_1X;
      if (DRIVER_Camaric(nim) == True && nim->demod_type != CX24123C)
	*dv = DISEQC_VER_2X;
      break;
    }
    case  0x01:
    {
      *dv = ECHOSTAR_LEGACY;
      break;
    }
    default:
    {
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }
  }  /* switch(... */

  return(True);

}  /* API_DiseqcGetVersion() */

/*******************************************************************************************************/
/* Cx24123 specific APIs */
/*******************************************************************************************************/
#ifdef CAMARIC_FEATURES
#ifdef INCLUDE_DISEQC2
/*******************************************************************************************************/
/* API_DiseqcReceiveMessage() */
/*******************************************************************************************************/
BOOL    API_DiseqcReceiveMessage(      /* funct to extract Rx'd Diseqc message from demod */
NIM     *nim,                          /* pointer to nim */
unsigned char  *buffer,                /* pointer to allocated storage where Rx'd Diseqc message will be stored */
int     buffer_len,                    /* length in bytes of buffer */
int     *received_len,                 /* returned number of bytes read from demod */
RXMODE  rxmode,                        /* Rx mode setting */
int     *parityerrors)                 /* Parity errors:  0 indicates no errors; binary 1 indicates an error: */
{                                      /* in associated byte.  (i.e. 0x01=Byte1 contains error; 0x08=Byte4) */
  int  i;

  unsigned long  ulRegVal;
  unsigned int   counter;

  RXMODE      rxm;
  DISEQC_VER  dv;
  INTEROPTS   interopts;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  if (buffer == NULL || received_len == NULL || parityerrors == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  if (buffer_len < 1)
  {
    DRIVER_SET_ERROR(nim,API_DISEQC_RXLEN);
    return(True);
  }

  /* test that demod-type is valid for rx diseqc messages */
  if (API_DiseqcGetVersion(nim,&dv) != True)
	return(False);

  /* get present rx mode setting, set new setting */
  if (API_DiseqcGetRxMode(nim,&rxm) != True)
	return(False);
  if (API_DiseqcSetRxMode(nim,rxmode) != True)
  {
    /* reset the demod to previous rxmode setting */
    if (API_DiseqcSetRxMode(nim,rxm) != True)
	return(False);
    return(False);
  }
  
  /* test for valid diseqc version.  if not generate warning */
  if (dv != DISEQC_VER_2X)
  {
    /* set warning (not error) */
    DRIVER_SET_ERROR(nim,API_DISEQC_VERSION);
    return(True);
  }

  /* start pulling bytes from the demod */
  counter = 0;
  *received_len = 0;
  do
  {
    /* check for an existing rx message */
    if (API_GetPendingInterrupts(nim,&interopts)== False)
	return(False);
    if ((interopts&INTR_LNB_REPLY_READY) == INTR_LNB_REPLY_READY)
    {
      /* clear the INTR_LNB_REPLY_READY interrupt */
      API_ClearDiseqcInterrupt(nim);
      
      /* data should be available to read, get length */
      if (RegisterRead(nim,CX24123_LNBDI2RXLENGTH,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      *received_len = (int)ulRegVal;

      /* grab parity error info, save it */
      if (RegisterRead(nim,CX24123_LNBDI2RXERRORLOC,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
      *parityerrors = (int)ulRegVal;

      /* Read 7 bytes from LNBMSG1 buffer - This is to compensate for a Cobra hardware error */
	  /* The circular buffer start on the 7th read. */
	  for (i = 0 ; i < MAX_RXBYTES - 1 ; i++)
      {      
        /* read the byte */
        if (RegisterRead(nim,CX24123_LNBMSG1,&ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
      }

      for (i = 0 ; i < MAX_RXBYTES ; i++)
      {
        /* make sure that buffer is not exceeded */
        if (i+1 > buffer_len)  break;

        /* read the byte */
        if (RegisterRead(nim,CX24123_LNBMSG1,&ulRegVal, DEMOD_I2C_IO) == False)
		return(False);
        buffer[i] = (unsigned char)ulRegVal;
      }
      break;
    }
    counter++;  
  }  while(counter <= MAX_LNBREAD_LOOP);

  /* test for wait exceeded */
  if (counter > MAX_LNBREAD_LOOP)
  {
    /* set warning (not error) */
    DRIVER_SET_ERROR(nim,API_DISEQC_TIMEOUT);
    return(True);
  }

  return(True);

}  /* API_DiseqcReceiveMessage() */


/*******************************************************************************************************/
/* API_DiseqcSetRxMode() */
/*******************************************************************************************************/
BOOL    API_DiseqcSetRxMode(           /* funct to set Diseqc Rx mode */
NIM     *nim,                          /* pointer to nim */
RXMODE  rxmode)                        /* rx mode to demod to */
{
  unsigned long ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  switch (rxmode)
  {
    case  RXMODE_INTERROGATION:
    {
      ulRegVal = 0x00UL;
      break;
    }
    case  RXMODE_QUICKREPLY:
    {
      ulRegVal = 0x01UL;
      break;
    }
    case  RXMODE_NOREPLY:
    case  RXMODE_DEFAULT:
    {
      ulRegVal = 0x02UL;
      break;
    }
    default:
    {
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }
  }  /* switch(... */

  /* write the rx mode setting to the demod */
  if (RegisterWrite(nim,CX24123_LNBDI2RXSEL,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* API_DiseqcSetRxMode() */


/*******************************************************************************************************/
/* API_DiseqcGetRxMode() */
/*******************************************************************************************************/
BOOL    API_DiseqcGetRxMode(           /* funct to retreive current rx mode setting */
NIM     *nim,                          /* pointer to nim */
RXMODE  *rxmode)                       /* storage where retreived rx mode will be stored */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  if (rxmode == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  /* write the rx mode setting to the demod */
  if (RegisterRead(nim,CX24123_LNBDI2RXSEL,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  switch ((int)ulRegVal)
  {
    case  0x00:
    {
      *rxmode = RXMODE_INTERROGATION;
      break;
    }
    case  0x01:
    {
      *rxmode = RXMODE_QUICKREPLY;
      break;
    }
    case  0x02:
    case  0x03:
    {
      *rxmode = RXMODE_NOREPLY;
      break;
    }
    default:
    {
      DRIVER_SET_ERROR(nim,API_BAD_RTNVAL);
      return (False);
    }
  }  /* switch(... */

  return(True);

}  /* API_DiseqcGetRxMode() */


/*******************************************************************************************************/
/* API_Diseqc22KHzSetAmplitude() */
/*******************************************************************************************************/
BOOL  API_Diseqc22KHzSetAmplitude(     /* funct to set Diseqc Tx message amplitude */
NIM   *nim,                            /* pointer to nim */
int   amplitude)                       /* amplitude value to set */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  /* test that amplitude is within valid range */
  if (amplitude < 0 || amplitude > ((0x01<<Register_bitlength(CX24123_LNBTONEAMP))-1))  
  {
    DRIVER_SET_ERROR(nim,API_PARM_RANGE);
    return(False);
  }

  /* write the amplitude setting */
  ulRegVal = (unsigned long)amplitude;
  if (RegisterWrite(nim,CX24123_LNBTONEAMP,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* API_Diseqc22KHzSetAmplitude() */


/*******************************************************************************************************/
/* API_Diseqc22KHzGetAmplitude() */
/*******************************************************************************************************/
BOOL  API_Diseqc22KHzGetAmplitude(     /* funct to retreive current Diseqc Amplitude seting */
NIM   *nim,                            /* pointer to nim */
int   *amplitude)                      /* storage where amplitude will be stored */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  if (amplitude == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  /* read the amplitude value */
  if (RegisterRead(nim,CX24123_LNBTONEAMP,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);
  *amplitude = (int)ulRegVal;

  return(True);

}  /* API_Diseqc22KHzGetAmplitude() */


/*******************************************************************************************************/
/* API_DiseqcSetVersion() */
/*******************************************************************************************************/
BOOL        API_DiseqcSetVersion(      /* funct to set demod Diseqc/EchoLegacy mode */
NIM         *nim,                      /* pointer to nim */
DISEQC_VER  dv)                        /* Diseqc version */
{
  unsigned long  SwitchMode = 0x00;
  unsigned long  EnableD21 = 0x00;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  switch (dv)
  {
    case  DISEQC_VER_2X:
    {
      /* Diseqc 2.1 will cause Diseqc2.1 register to be set to 0x01 */
      EnableD21 = 0x01;
    }
    case  DISEQC_VER_1X:
    {
      SwitchMode = 0x00;
      break;
    }
    case  ECHOSTAR_LEGACY:
    {
      SwitchMode = 0x01;
      EnableD21 = 0x00;
      break;
    }
    default:
    case  DISEQC_VER_UNDEF:
    {
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }
  }  /* switch(... */

  /* set the Diseqc mode */
  if (RegisterWrite(nim,CX24123_LNBSMEN,SwitchMode, DEMOD_I2C_IO) == False)
	return(False);
  if (RegisterWrite(nim,CX24123_LNBDI2EN,EnableD21, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* API_DiseqcSetVersion() */


/*******************************************************************************************************/
/* API_SetDiseqcInterrupt() */
/*******************************************************************************************************/
BOOL  API_SetDiseqcInterrupt(          /* funct to enable diseqc interrupt */
NIM   *nim,                            /* pointer to nim */ 
BOOL  on)                              /* True to turn-on the interrupt, False to turn it off */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  /* read new interrupt options to the demod */
  if (RegisterRead(nim,CX24130_INTRENABLE,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  /* turn-off the interrupt bit in question */
  ulRegVal &= ~(INTR_LNB_REPLY_READY);

  /* turn it on, if desired */
  ulRegVal |= (on == True ? INTR_LNB_REPLY_READY : 0x00);

  /* write new interrupt options to the demod */
  if (RegisterWrite(nim,CX24130_INTRENABLE,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* API_SetDiseqcInterrupt() */


/*******************************************************************************************************/
/* API_GetDiseqcInterrupt() */
/*******************************************************************************************************/
BOOL       API_GetDiseqcInterrupt(     /* funct to enable diseqc interrupt */
NIM        *nim,                       /* pointer to nim */ 
INTEROPTS  *interopts)                 /* storage where interrupt source will be stored */
{

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  /* test for valid parameters */
  if (interopts == NULL)
  {
     DRIVER_SET_ERROR(nim,API_BAD_PARM);
     return(False);
  }

  /* turn-on the specified interrupt */
  if (API_GetPendingInterrupts(nim,interopts) != True)
	return(False);
  
  /* return only the pertinent bit */
  *interopts = (unsigned char)((*interopts & INTR_LNB_REPLY_READY) == 0 ? INTR_CLEAR : INTR_LNB_REPLY_READY);

  return(True);

}  /* API_GetDiseqcInterrupt() */


/*******************************************************************************************************/
/* API_ClearDiseqcInterrupt() */
/*******************************************************************************************************/
BOOL  API_ClearDiseqcInterrupt(          /* funct to enable diseqc interrupt */
NIM   *nim)                            /* pointer to nim */ 
{
  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  /* clear the specified interrupt */
  if (_API_ClearPendingInterrupts(nim,INTR_LNB_REPLY_READY) != True)
	return(False);

  return(True);

}  /* API_ClearDiseqcInterrupt() */
#endif /* #ifdef API_INCLUDE_DISEQC2 */

/*******************************************************************************************************/
/* (end-of-Cx24123 specific APIs) */
/*******************************************************************************************************/
#endif  /* #ifdef CAMARIC_FEATURES */

/*******************************************************************************************************/
/* API_LNBSetDrain() */
/*******************************************************************************************************/
BOOL      API_LNBSetDrain(             /* funct to set demod LNB Drain setting */
NIM       *nim,                        /* pointer to nim */
LNBDRAIN  lnbdrain)                    /* LNB Drain setting */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  switch (lnbdrain)
  {
    case  LNBDRAIN_OPEN:
    {
      ulRegVal = 0x01UL;
      break;
    }
    case  LNBDRAIN_DIGITAL:
    {
      ulRegVal = 0x00UL;
      break;
    }
    default:
    case  LNBDRAIN_UNDEF:
    {
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }

  }  /* switch(... */

  /* write the drain setting */
  if (RegisterWrite(nim,CX24123_LNBDCODEN,ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  return(True);

}  /* API_LNBSetDrain() */


/*******************************************************************************************************/
/* API_LNBGetDrain() */
/*******************************************************************************************************/
BOOL      API_LNBGetDrain(             /* funct to retreive LNB Drain setting */
NIM       *nim,                        /* pointer to nim */
LNBDRAIN  *lnbdrain)                   /* storage where setting will be stored */
{
  unsigned long  ulRegVal;

  /* test for valid nim */
  DRIVER_VALIDATE_NIM(nim);

  if (lnbdrain == NULL)
  {
    DRIVER_SET_ERROR(nim,API_BAD_PARM);
    return(False);
  }

  /* read the drain setting */
  if (RegisterRead(nim,CX24123_LNBDCODEN,&ulRegVal, DEMOD_I2C_IO) == False)
	return(False);

  switch ((int)ulRegVal)
  {
    case  0x01:
    {
      *lnbdrain = LNBDRAIN_OPEN;
      break;
    }
    case  0x00:
    {
      *lnbdrain = LNBDRAIN_DIGITAL;
      break;
    }
    default:
    {
      *lnbdrain = LNBDRAIN_UNDEF;
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return(False);
      break;
    }
  }  /* switch(... */

  return(True);

}  /* API_LNBGetDrain() */

//API_SCESetWindowSize
//API_SCEGetWindowSize
//API_SCEGetSymbolCount??
//API_SetDemodClockOut
//API_GetDemodClockOut
//API_EchostarLegacyPolarity
//API_EchostarLegacySendMessage

/*******************************************************************************************************
 * API_GetPowerEstimation() 
 *
 * Function to get the input power estimation in dBm units.
 *
 * Parameters:
 * I/O	Parameters	                 Descriptions
 * --------------------------------------------------------------------------------
 * IN	p_nim	                     Pointer to NIM structure allocated by application.
 * IN	char power                   Range -25 dBm to -70 dBm. Estimated input power.
 *
 *******************************************************************************************************/
BOOL  
API_GetPowerEstimation(NIM* p_nim, signed char* p_power)
{	
	const signed char power_table[MAX_AGC_TABLE_LENGTH]  = {-25, -30, -35, -40, -45, -50, -55, -60, -65, -70};
#if INCLUDE_VIPER
    /* Mongoose/Rattler */
	//AGCACC agc_table_high0[MAX_AGC_TABLE_LENGTH]  = {-28, -24, -20, -16, -11, -5, 0, 5,  9,  13}; /* Additional points: 18, 24, 32, 42 */
	AGCACC agc_table_high0[MAX_AGC_TABLE_LENGTH]  = {-54, -41, -35, -30, -25, -21, -16, -10,  -6,  -2};
	//AGCACC agc_table_low0[MAX_AGC_TABLE_LENGTH]   = {-21, -17, -13, -9,  -3,   1, 6, 12, 19, 25}; /* Additional points: 33, 42, 54, 74 */
	AGCACC agc_table_low0[MAX_AGC_TABLE_LENGTH]   = {-39, -35, -30, -25, -19, -15, -11, -5, 1, 9};
	/* Viper */
	AGCACC agc_table_high1[MAX_AGC_TABLE_LENGTH]  = {-52, -42, -37, -32, -28, -23, -19, -14, -10, -6};
	AGCACC agc_table_low1[MAX_AGC_TABLE_LENGTH]   = {-40, -36, -32, -27, -22, -18, -14,  -9,  -4,  2}; 
#endif /* INCLUDE_VIPER */

#if INCLUDE_ROSIE
 /* Rosie */
	const signed char rosie_power_table[MAX_AGC_TABLE_LENGTH] = {-32, -35, -40, -45, -50, -55, -60, -65, -68, -70};
	AGCACC rosie_agc_table[MAX_AGC_TABLE_LENGTH]       = { -8,  -6,  -2,   3,  11,  30,  51,  68,  80, 104};
#endif

	AGCACC   agc_accum;
	AGCACC*  p_agc_table; 
	const signed char* p_power_table = &power_table[0];
	int            index;

	/* test for valid nim */
	DRIVER_VALIDATE_NIM(p_nim);

	if (API_GetAGCAcc(p_nim, &agc_accum) == False)
	{
		return(False);
	}

/* INCLUDE_VIPER specificsection - begin */
#if INCLUDE_VIPER
	if (p_nim->tuner.cx24128.chipid == MONGOOSE_CHIP_ID)   /* Mongoose/Rattler */
    {
        p_agc_table = &agc_table_high0[0];
    }
    else if ((p_nim->tuner.cx24128.chipid == VIPER_CHIP_ID) || (p_nim->tuner.cx24128.chipid == 0x44) || (p_nim->tuner.cx24128.chipid == 0x45)) /* Viper */
    {
        p_agc_table = &agc_table_high1[0];
    }
    else 
#endif /* INCLUDE_VIPER */
#if INCLUDE_ROSIE
	if (p_nim->tuner_type == CX24108)            /* Rosie */
    {
        p_power_table = &rosie_power_table[0];
        p_agc_table = &rosie_agc_table[0];
    }
    else
#endif
    {
        *p_power = 0;
        return (True);
    }

#if INCLUDE_VIPER	
	if (p_nim->tuner.cx24128.gain_setting_selection == SIGNAL_LEVEL_LOW) /* low signal level */
	{
        if (p_nim->tuner.cx24128.chipid == MONGOOSE_CHIP_ID)   /* Mongoose/Rattler */
        {
            p_agc_table = &agc_table_low0[0];
        }
        else if ((p_nim->tuner.cx24128.chipid == VIPER_CHIP_ID) || (p_nim->tuner.cx24128.chipid == 0x44) || (p_nim->tuner.cx24128.chipid == 0x45)) /* Viper */
        {
            p_agc_table = &agc_table_low1[0];
        }
	}

#endif /* INCLUDE_VIPER */

	for (index = 0; index < MAX_AGC_TABLE_LENGTH; index++)
	{
		if (agc_accum < p_agc_table[index])
		{
			break;
		}
	}

	if (index == 0)
	{
		*p_power = -25;
	}
	else if (index == 10)
	{
		*p_power = -70;
	}
	else
	{       
		/* estimated input power = [input_power(A) x (B - x) + input_power(B) x (x - A)] / (B - A) */
		*p_power = (signed char)(((p_power_table[index-1] * (p_agc_table[index] - agc_accum)) + 
				   (p_power_table[index] * (agc_accum - p_agc_table[index-1])))/(p_agc_table[index] - p_agc_table[index-1]));
	}
	return (True);
}

//???

/*******************************************************************************************************/
/* API_GetLNBDC() */
/*******************************************************************************************************/
BOOL   API_GetLNBDC(     /* get LNB voltage level - either high (~18v) or low (~13v) */
NIM    *nim,             /* pointer to nim */
LNBPOL *lnbpol)          /* pointer to returned LNB_HIGH or LNB_LOW value */
{
   unsigned long  ulRegVal;

   /* validate nim */
   DRIVER_VALIDATE_NIM(nim);

   /* validate input parameters */
   if (lnbpol == NULL)
   {
      DRIVER_SET_ERROR(nim,API_BAD_PARM);
      return (False);
   }
   *lnbpol = LNB_UNDEF;

   /* read the current LNB DC voltage setting */
   if (RegisterRead(nim,CX24130_LNBDC,&ulRegVal, DEMOD_I2C_IO) != True)
   {
      return(False);
   }

   if (ulRegVal == 0UL)
   {
      *lnbpol = LNB_LOW;
   }
   else if (ulRegVal == 1UL)
   {               
      *lnbpol = LNB_HIGH;
   }
   else
   {
      return (False);
   }
     
   return (True);

}  /* API_GetLNBDC() */
/*******************************************************************************************************/
/* (end-of-Cx24130 specific APIs) */
/*******************************************************************************************************/

/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/
/* CR 9509 : Add an extra newline */
