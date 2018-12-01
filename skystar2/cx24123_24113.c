/*
 *	DVB-S CX24123 (CX24113)
 */

#include <sys/types.h>

//cobra
#include "cobra.h"

//
#include "../include/frontend.h"
#include "skystar2.h"
#include "i2c.h"
#include "frontend.h"
#include "cx24123_24113.h"
#include "debug.h"

//static int debug = 0;

static struct dvb_frontend_info cx24123_24113_info = {
	name: "CX24123_24113",
        type: FE_QPSK,
};


int
DemodRead(struct dvbtuner *tun, u_int8_t reg, u_int8_t *data, u_int16_t len)
{
	return FLEXI2C_busRead(tun->sc, &tun->demod, reg, data, len) == len;
}

int
DemodWrite(struct dvbtuner *tun, u_int8_t reg, u_int8_t *data, u_int16_t len)
{
	return FLEXI2C_busWrite(tun->sc, &tun->demod, reg, data, len) == len;
}

int
LnbWrite(struct dvbtuner *tun, u_int8_t reg, u_int8_t *data, u_int16_t len)
{
	return FLEXI2C_busWrite(tun->sc, &tun->lnb, reg, data, len) == len;
}

void
SBWrite(void *ptuner, unsigned int handle, unsigned char reg, unsigned char data, unsigned int *status)
{
	struct dvbtuner *tun = (struct dvbtuner *)ptuner;
	unsigned char ucValue;

	tun->demod.unk4 = handle;
	ucValue = data;
	*status = DemodWrite(tun, reg, &ucValue, 1) == 0;
}

unsigned char
SBRead(void *ptuner, unsigned int handle, unsigned char reg, unsigned int *status)
{
	struct dvbtuner *tun = (struct dvbtuner *)ptuner;
	unsigned char ucValue;

	tun->demod.unk4 = handle;

	*status = DemodRead(tun, reg, &ucValue, 1) == 0;
	return ucValue;
}

int
cx24123_send_diseqc_msg(struct adapter *sc, struct dvb_diseqc_master_cmd *cmd)
{
	struct dvbtuner *tun = (struct dvbtuner *)(sc->fe->frontend.data);

	API_SendDiseqcMessage(&tun->nim, cmd->msg, cmd->msg_len, 1, 1);

	return 0;
}

int
cx24123_send_diseqc_burst(struct adapter *sc, fe_sec_mini_cmd_t burst)
{
	struct dvbtuner *tun = (struct dvbtuner *)(sc->fe->frontend.data);

	if (RegisterWrite(&tun->nim, CX24130_LNBDISEQCDIS, 1, DEMOD_I2C_IO))
	{
		if (RegisterWrite(&tun->nim, CX24130_LNBBURSTMODSEL, (burst == SEC_MINI_A)?0:1, DEMOD_I2C_IO))
		{
			if (RegisterWrite(&tun->nim, CX24130_LNBSENDMSG, 1, DEMOD_I2C_IO))
			{
				int i = 0;
				do {
					DELAY(100);
				} while(RegisterRead(&tun->nim, CX24130_LNBSENDMSG, &i, DEMOD_I2C_IO) && i == 0);					

			}


		}

	}
	RegisterWrite(&tun->nim, CX24130_LNBDISEQCDIS, 0, DEMOD_I2C_IO);

        return 0;
}

int
cx24123_set_tone(struct adapter *sc, fe_sec_tone_mode_t tone)
{
	struct dvbtuner *tun = (struct dvbtuner *)(sc->fe->frontend.data);

        if (tone == SEC_TONE_ON)
	{
		API_SetLNBTone(&tun->nim, LNBTONE_ON);
		tun->lnbtone = tone;
	} else
	if (tone == SEC_TONE_OFF)
	{
		API_SetLNBTone(&tun->nim, LNBTONE_OFF);
		tun->lnbtone = tone;
        }
	return 0;
}

int
cx24123_set_voltage(struct adapter *sc, fe_sec_voltage_t voltage)
{
	struct dvbtuner *tun = (struct dvbtuner *)(sc->fe->frontend.data);

	switch (voltage)
	{
	case SEC_VOLTAGE_13:
		tun->lnbval &= 0xFB;
		tun->lnbval |= 0xA;
		LnbWrite(tun, 1, &tun->lnbval, 1);
		break;

	case SEC_VOLTAGE_18:
		tun->lnbval |= 0xE;
		LnbWrite(tun, 1, &tun->lnbval, 1);
		break;

	case SEC_VOLTAGE_OFF:
	default:;
		tun->lnbval &= 0xF9;
		LnbWrite(tun, 1, &tun->lnbval, 1);
	}
	return 0;
}

void
GetStatus(struct dvbtuner *tun)
{
	memset(&tun->lockind, 0, sizeof(LOCKIND));

	if (!API_Monitor(&tun->nim, &tun->acqstate, &tun->lockind))
	{
		memset(&tun->lockind, 0, sizeof(LOCKIND));

		printf("API_Monitor: error\n");
	} else
	{
#if 0
		printf("pll(%x), +demod_pll(%x), demod(%x), +viterbi(%x), +reedsolomon(%x), descramble(%x), +syncbytes(%x)\n",
			tun->lockind.pll,
			tun->lockind.demod_pll,
			tun->lockind.demod,
			tun->lockind.viterbi,
			tun->lockind.reedsolomon,
			tun->lockind.descramble,
			tun->lockind.syncbyte);
#endif
	}
}

int
UpdateStatus(struct dvbtuner *tun)
{
	CMPLXNO		cmplxno;
	MSTATUS		mstatus;

	GetStatus(tun);


	if (API_GetChannelEsNo(&tun->nim, ESNOMODE_SNAPSHOT, &cmplxno, &mstatus))
	{
		if (mstatus == MSTATUS_DONE || mstatus == MSTATUS_SAT)
		{
			tun->esno = (cmplxno.integer * 1000)/cmplxno.divider;
			printf("EsNo %d\n", tun->esno);
		}
	}

	if (API_GetBER(&tun->nim, 255, &cmplxno, &mstatus))
	{
		if (mstatus == MSTATUS_DONE || mstatus == MSTATUS_SAT)
		{
			printf("ber %d/%d\n", cmplxno.integer, cmplxno.divider);
		}
	}

	return 1;
}

int
Reset(struct dvbtuner *tun)
{
	NIM *nim = &tun->nim;

	//TUNER_CX24128_SetClkInversion
	RegisterWrite(nim, CX24128_DSM_CLK, True, nim->tuner.cx24128.io_method);

	//TUNER_CX24128_SetParameters
	RegisterWrite(nim, CX24128_IDIG_SEL, 2, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_ICP_LEVEL, 0xfa, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_BS_DELAY, 8, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_ACP_ON_ALW, 1, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_BS_VCOMT, 0, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_BS_FREQ, 0xfff, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_BS_DIV_CNT, 0xfff, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_VCO6_SEL_SPI, 0x10, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_VCO_SEL_SPI, 0, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_VCO_BSH_SPI, 0, nim->tuner.cx24128.io_method);

	RegisterWrite(nim, CX24128_DIV24_SEL, 0, nim->tuner.cx24128.io_method);

	//TUNER_CX24128_SetFilterBandwidth
	RegisterWrite(nim, CX24128_FILTER_BW, 2, nim->tuner.cx24128.io_method);
	RegisterWrite(nim, CX24128_GMC_BW, 0x11, nim->tuner.cx24128.io_method);

	//TUNER_CX24128_SetEnableRegister
	if (RegisterWrite(nim, CX24128_EN, 0x3f, nim->tuner.cx24128.io_method)
         && RegisterWrite(nim, CX24128_LNA_EN, 1, nim->tuner.cx24128.io_method))
		return 1;
	return 0;
}

int
SetChannel(struct dvbtuner *tun)
{
	CHANOBJ		chanobj;
	LNBTONE		lnbtone;
	CODERATE	coderate;
	unsigned int	viterbicoderates;
	int		res;

	Reset(tun);

	viterbicoderates = 0;

	switch (tun->coderate)
	{
	case FEC_1_2:
		coderate = CODERATE_1DIV2;
		break;
	case FEC_2_3:
		coderate = CODERATE_2DIV3;
		break;
	case FEC_3_4:
		coderate = CODERATE_3DIV4;
		break;
	case FEC_5_6:
		coderate = CODERATE_5DIV6;
		break;
	case FEC_6_7:
		coderate = CODERATE_6DIV7;
		break;
	case FEC_7_8:
		coderate = CODERATE_7DIV8;
		break;
	default:
		coderate = CODERATE_1DIV2;
		viterbicoderates = CODERATE_1DIV2|CODERATE_2DIV3|CODERATE_3DIV4|CODERATE_5DIV6|CODERATE_7DIV8;
		break;
		;
	}

	lnbtone = LNBTONE_OFF;
	if (tun->lnbtone == SEC_TONE_ON)
		lnbtone = LNBTONE_ON;

	printf("freq %u\n", tun->freq);
	printf("sr %u\n", tun->symbrate);

	chanobj.frequency = tun->freq;
	chanobj.modtype = MOD_QPSK;
	chanobj.coderate = coderate;
	chanobj.symbrate = tun->symbrate;	/*	*/
	chanobj.specinv = SPEC_INV_OFF_BOTH;
	chanobj.samplerate = SAMPLE_FREQ_NOM;
	chanobj.lnbpol = LNB_LOW;
	chanobj.lnbtone = lnbtone;
	chanobj.viterbicoderates = viterbicoderates;
	chanobj.transpec = SPEC_DVB;
	chanobj.unk28 = 2;
	chanobj.unk2C = 0;
	chanobj.unk30 = 0;


	res = API_ChangeChannel(&tun->nim, &chanobj);
	if (res != 0)
	{
		printf("API_ChangeChannel error: %s %s %d\n",
				API_GetErrorFilename(&tun->nim),
				API_GetErrorMessage(&tun->nim,
					API_GetLastError(&tun->nim)),
					API_GetErrorLineNumber(&tun->nim) );

		return 0;
	}
	return 1;
}

int
IsThisTunerInstalled(struct dvbtuner *tun)
{
	unsigned char id;
	int res;

	res = DemodRead(tun, 0, &id, 1);

	printf("demod ID=%x\n", id);

	if (res)
		res = (id == 0xD1);
	return res;
}

/*----------------------------------------------------------------*/
int
cx24123_ioctl(struct dvb_frontend *fe, unsigned int cmd, void *arg)
{
	struct adapter *sc = (struct adapter *)fe->sc;
	struct dvbtuner *tun = (struct dvbtuner *)fe->data;
	int ret, i;

	switch(cmd){

	case FE_GET_INFO:
		memcpy(arg, &cx24123_24113_info, sizeof(struct dvb_frontend_info));
		break;

	case FE_READ_STATUS:
		{
		fe_status_t *status = (fe_status_t *)arg;

		UpdateStatus(tun);

                *status = 0;
		//if (tun->lockind.pll)		*status |= FE_HAS_SIGNAL;
		if (tun->lockind.demod_pll)	*status |= FE_HAS_SIGNAL;

                if (tun->lockind.demod)		*status |= FE_HAS_CARRIER;
                if (tun->lockind.viterbi)	*status |= FE_HAS_VITERBI;
                if (tun->lockind.descramble)	*status |= FE_HAS_SYNC;
                if (tun->lockind.syncbyte)	*status |= FE_HAS_LOCK;
		} break;

	case FE_READ_SIGNAL_STRENGTH:
		{
			*((u_int32_t *)arg) = 0;
			//DBG1("QUALITY=%d\n", snr);

		} break;

	case FE_READ_SNR:
		{
			*((u_int32_t *)arg) = 0;
			//DBG1("SNR=%d\n", snr);
		} break;

	case FE_SET_FRONTEND:
			//DBG1("FE_SET_FRONTEND\n");
		{
			struct dvb_frontend_parameters *p = (struct dvb_frontend_parameters *)arg;

			printf("SET_FRONTEND\n");

			tun->freq = p->frequency;
			tun->coderate = p->u.qpsk.fec_inner;
			tun->symbrate = p->u.qpsk.symbol_rate/1000;

			SetChannel(tun);
			//DBG1("FE_SET_FRONEND -end\n");
		} break;

	case FE_GET_FRONTEND:
			//DBG1("FE_GET_FRONTEND\n");
		{
			struct dvb_frontend_parameters *p = (struct dvb_frontend_parameters *)arg;

		} break;

	case FE_SLEEP:
			break;

	case FE_INIT:
			break;

	case FE_RESET:
			break;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}


/*----------------------------------------------------------------*/
int
cx24123_detach(struct adapter *sc)
{
	struct fe_data *fe = sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;

	sc->set_voltage(sc, SEC_VOLTAGE_OFF);

	if (frontend->data)
		free(frontend->data, M_DEVBUF);

	return 0;
}

/*----------------------------------------------------------------*/
int
cx24123_attach(struct adapter *sc)
{
	struct fe_data *fe = sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;
	struct dvbtuner *tun;

	MPEG_OUT	mpeg;

	//DBG("\n");

	tun = (struct dvbtuner *)malloc(sizeof(struct dvbtuner), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (tun == NULL)
	{
		//DBG("can't alloc mem for state\n");
		return -ENOMEM;
	}

	tun->sc = sc;

	//DEMOD
	tun->demod.device = 0x10000000;
	tun->demod.unk4 = 0x55;
	tun->demod.retr = 3;
	tun->demod.unkC = 0;
	tun->demod.unkD = 0;
	tun->demod.unkE = 0;
	tun->demod.unkF = 0;
	tun->demod.eeprom = 0;
	tun->demod.unk14 = 0;
	tun->demod.unk18 = 0;
	tun->demod.wait = 0;

	//LNB
	tun->lnb.device = 0x30000000;
	tun->lnb.unk4 = 8;
	tun->lnb.retr = 1;
	tun->lnb.unkC = 1;//0
	tun->lnb.unkD = 0;
	tun->lnb.unkE = 0;
	tun->lnb.unkF = 0;
	tun->lnb.eeprom = 0;
	tun->lnb.unk14 = 0;
	tun->lnb.unk18 = 0;
	tun->lnb.wait = 0;

	tun->lnbval = 2;	//0x20 ??

	if (!IsThisTunerInstalled(tun))
	{
		free(tun, M_DEVBUF);
		return -ENODEV;
	}

	//
	mpeg.OutputMode =               PARALLEL_OUT;
	mpeg.ClkOutEdge =               CLKOUT_SETUP7_HOLD1;
	mpeg.ClkParityMode =            CLK_CONTINUOUS;
	mpeg.HoldTime =                 SMALL_HOLD_TIME;
	mpeg.StartSignalPolarity =      ACTIVE_HIGH;
	mpeg.StartSignalWidth =         BYTE_WIDE;
	mpeg.ValidSignalPolarity =      ACTIVE_HIGH;
	mpeg.ValidSignalActiveMode =    ENTIRE_PACKET;
	mpeg.FailSignalPolarity =       ACTIVE_HIGH;
	mpeg.FailSignalActiveMode =     ENTIRE_PACKET;
	mpeg.SyncPunctMode =            SYNC_WORD_NOT_PUNCTURED;
	mpeg.FailValueWhenNoSync =      FAIL_LOW_WHEN_NO_SYNC;
	mpeg.ClkSmoothSel =             CLK_SMOOTHING_OFF;
	mpeg.RSCntlPin1Sel =            RS_CNTLPIN_START;
	mpeg.RSCntlPin2Sel =            RS_CNTLPIN_VALID;

	mpeg.RSCntlPin3Sel =            RS_CNTLPIN_FAIL;
	mpeg.NullDataMode =             FIXED_NULL_DATA_DISABLED;
	mpeg.NullDataValue =            FIXED_NULL_DATA_LOW;
	mpeg.ValidSignalWhenFail =      VALID_SIGNAL_ACTIVE_WHEN_FAIL;
	mpeg.StartSignalWhenFail =      START_SIGNAL_ACTIVE_WHEN_FAIL;
	mpeg.ParityDataSel =            RS_PARITY_DATA_UNCHANGED;

	if (!API_InitEnvironment(&tun->nim, 0x55, SBWrite, SBRead,
			TUNER_install_CX24113, 10111000, 1, &mpeg, 0, 0, tun))
	{
			printf("API_InitEnvironment error: %s %s %d\n",
				API_GetErrorFilename(&tun->nim),
				API_GetErrorMessage(&tun->nim,
					API_GetLastError(&tun->nim)),
					API_GetErrorLineNumber(&tun->nim) );
			return -ENODEV;
	}

	fe->info = &cx24123_24113_info;

	frontend->data = (struct dvbtuner *)tun;

        frontend->ioctl = cx24123_ioctl;
        frontend->detach = cx24123_detach;

	sc->diseqc_send_master_cmd = cx24123_send_diseqc_msg;
	sc->diseqc_send_burst = cx24123_send_diseqc_burst;
	sc->set_tone = cx24123_set_tone;
	sc->set_voltage = cx24123_set_voltage;
	return 0;
}

