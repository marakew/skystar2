/*
 *	DVB-S SAMSUNG TBMU24112 Frontend STV0299 (PLL synth SL1935)
 */

#include <sys/types.h>

#include "../include/frontend.h"
#include "skystar2.h"
#include "i2c.h"
#include "frontend.h"
#include "samsung_tbmu24112.h"
#include "diseqc.h"
#include "debug.h"

static int debug = 0;
static int stv0299_status = 0;

static struct dvb_frontend_info samsung_tbmu24112_info = {
	name: "Samsung TBMU24112(PLL SL1935)",
        type: FE_QPSK,
        frequency_min: 950000,
        frequency_max: 2150000,
        frequency_stepsize: 125,   /* kHz for QPSK frontends */
        frequency_tolerance: M_CLK/2000,
        symbol_rate_min: 1000000,
        symbol_rate_max: 45000000,
        symbol_rate_tolerance: 500,  /* ppm */
        notifier_delay: 0,
        caps: FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
              FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
              FE_CAN_QPSK
#if 0
              | FE_CAN_INVERSION_AUTO |
              FE_CAN_CLEAN_SETUP
#endif
};

/*----------------------------------------------------------------*/
static int
stv0299_writereg(struct adapter *sc, u_int8_t reg, u_int8_t data)
{
	int ret;
	u_int8_t buf[] = { reg, data };
	struct i2c_msg msg = {addr: 0x68, flags: 0, buf: buf, len: 2};
#if 0
	DBG(">> reg[0x%02x] = 0x%02x\n", reg, data);
#endif
	ret = master_xfer(sc, &msg, 1);
	if (ret != 1)
	{
		DBG("error ret = %d\n", ret);
	}
	return (ret != 1) ? -1: 0;
}

/*----------------------------------------------------------------*/
static u_int8_t
stv0299_readreg(struct adapter *sc, u_int8_t reg)
{
	int ret;
	u_int8_t b0[] = { reg };
	u_int8_t b1[] = { 0 };
	struct i2c_msg msg[] = {{addr: 0x68, flags: 0       , buf: b0, len: 1},
				{addr: 0x68, flags: I2C_M_RD, buf: b1, len: 1}};

	ret = master_xfer(sc, msg, 2);

	if (ret != 2)
	{
		DBG("error ret = %d\n", ret);
	}
#if 0
	DBG("<< reg[0x%02x] = 0x%02x\n", reg, b1[0]);
#endif
	return b1[0];
}

/*----------------------------------------------------------------*/
static int
stv0299_readregs(struct adapter *sc, u_int8_t reg1, u_int8_t *b, u_int8_t len)
{
	int ret, i;
	struct i2c_msg msg[] ={{addr: 0x68, flags: 0,    buf: &reg1, len: 1},
			       {addr: 0x68, flags: I2C_M_RD, buf: b, len: len}};
	ret = master_xfer(sc, msg, 2);

	if (ret != 2)
	{
		DBG("error ret = %d\n", ret);
	}
#if 0
	for (i = 0; i < len; i++){
		DBG("reg[0x%02x] = 0x%02x\n", reg1, b[i]);
	}
#endif
	return ret == 2 ? 0 : ret;
}

/*----------------------------------------------------------------*/
static int
stv0299_set_bits(struct adapter *sc, u_int8_t op, u_int8_t addr, u_int8_t mask, u_int8_t bits)
{
	u_int8_t val = stv0299_readreg(sc, addr);

	if (op)
		val = (val & ~mask) | bits;
	else
		val = (val & ~mask);

	return stv0299_writereg(sc, addr, val);
}

/*----------------------------------------------------------------*/
static int
pll_write(struct adapter *sc, u_int8_t *data, int len)
{
	int ret, i;
	struct i2c_msg msg = {addr: 0x61, flags: 0, buf: data, len: len};

	stv0299_set_bits(sc, 1, 0x05, 0x80, 0x80);

	ret = master_xfer(sc, &msg, 1);

	stv0299_set_bits(sc, 0, 0x05, 0x80, 0x80);

	if (ret != 1)
	{
		DBG("i/o error ret = %d\n", ret);
	}
#if 0
	for (i = 0; i < len; i++){
		DBG("reg[0x%02x] = 0x%02x\n", addr, data[i]);
	}
#endif
	return (ret != 1) ? -1 : 0;
}

/*----------------------------------------------------------------*/
static int
stv0299_set_timing(struct adapter *sc, int h, int l)	// 4, 7
{
	return stv0299_writereg(sc, 0x0e, (h & 7) << 4 | (l & 7) );
}

/*----------------------------------------------------------------*/
static int
pll_set_freq(struct adapter *sc, u_int32_t freq)
{
	u_int8_t buf[4];
        u_int32_t div;

	div = (freq / 125);

	DBG1("freq = %d, div = %d\n", freq, div);

        buf[0] = (div >> 8) & 0x7f;		//0x1D
        buf[1] = div & 0xff;			//0xB0
        buf[2] = 0x84; /* 0x80   | ratio; */	//0x84
	buf[3] = 0x08;				//0x08

	if (freq < 1500000)
		buf[3] |= 0x10;
	else
		buf[3] &= 0xEF;
 
	return pll_write(sc, buf, sizeof(buf));
}

/*----------------------------------------------------------------*/
static void
stv0299_init(struct adapter *sc)
{
	int i;
	u_int8_t reg, val;

	DBG1("\n");

	for (i = 0; i < sizeof(init_tab_samsung); i+=2)
	{
		reg = (u_int8_t)init_tab_samsung[i];
		val = (u_int8_t)init_tab_samsung[i+1];
		stv0299_writereg(sc, reg, val);
	}
}

/*----------------------------------------------------------------*/
static int
stv0299_check_inversion(struct adapter *sc)
{
	DBG1("\n");
	if ((stv0299_readreg(sc, 0x1b) & 0x98) != 0x98)
	{
		DELAY(30000);
                if ((stv0299_readreg(sc, 0x1b) & 0x98) != 0x98)
		{
                        u_int8_t val = stv0299_readreg(sc, 0x0c);
                        DBG("toggle inversion\n");
                        return stv0299_writereg(sc, 0x0c, val ^ 0x01);
                }
        }
        return 0;
}

/*----------------------------------------------------------------*/
static int
stv0299_set_FEC(struct adapter *sc, fe_code_rate_t fec)
{

	DBG("SET_FEC\n");
	switch(fec){
        case FEC_1_2:
		DBG("_1_2\n");
                return stv0299_writereg(sc, 0x31, 0x01);
        case FEC_2_3:
		DBG("_2_3\n");
                return stv0299_writereg(sc, 0x31, 0x02);
        case FEC_3_4:
		DBG("_3_4\n");
                return stv0299_writereg(sc, 0x31, 0x04);
        case FEC_5_6:
		DBG("_5_6\n");
                return stv0299_writereg(sc, 0x31, 0x08);
        case FEC_7_8:
		DBG("_7_8\n");
                return stv0299_writereg(sc, 0x31, 0x10);
        case FEC_AUTO:
		DBG("_AUTO\n");
                return stv0299_writereg(sc, 0x31, 0x1f);
        default:
		DBG(" %d -EINVAL\n", fec);
                return -EINVAL;
        }
}

/*----------------------------------------------------------------*/
static fe_code_rate_t
stv0299_get_FEC(struct adapter *sc)
{
	static fe_code_rate_t reg2fec[5] = { FEC_2_3, FEC_3_4, FEC_5_6,
                                             FEC_7_8, FEC_1_2 };
        u_int8_t index;
        DBG1("\n");
        index = stv0299_readreg(sc, 0x1b);
	index &= 0x7;
        if (index > 4)
                return FEC_AUTO;
        return reg2fec[index];
}

/*----------------------------------------------------------------*/
static int
stv0299_wait_diseqc_fifo(struct adapter *sc, int timeout)
{
	unsigned long start = (time_t)time_second;
	DBG1("\n");
	while (stv0299_readreg(sc, 0x0a) & 1)
	{
                if (((time_t)time_second - start) > timeout)
		{
                        DBG("timeout!!\n");
                        return -ETIMEDOUT;
                }
		DELAY(10000);
        };
        return 0;
}

/*----------------------------------------------------------------*/
static int
stv0299_wait_diseqc_idle(struct adapter *sc, int timeout)
{
	unsigned long start = (time_t)time_second;
	DBG1("\n");
	while ((stv0299_readreg(sc, 0x0a) & 3) != 2 )
	{
                if (((time_t)time_second - start) > timeout)
		{
                        DBG("timeout!!\n");
                        return -ETIMEDOUT;
                }
		DELAY(10000);
        };
        return 0;
}

/*----------------------------------------------------------------*/
static int
stv0299_send_diseqc_msg(struct adapter *sc, struct dvb_diseqc_master_cmd *cmd)
{
	u_int8_t val;
	int i;
	DBG1("\n");
	if (stv0299_wait_diseqc_idle(sc, 100) < 0)
                return -ETIMEDOUT;

        val = stv0299_readreg(sc, 0x08);

        if (stv0299_writereg(sc, 0x08, (val & ~0x7) | 0x6))  /* DiSEqC mode */
                return -EIO;

        for (i=0; i < cmd->msg_len; i++)
	{
                if (stv0299_wait_diseqc_fifo(sc, 100) < 0)
                        return -ETIMEDOUT;

                if (stv0299_writereg(sc, 0x09, cmd->msg[i]))
                        return -EIO;
	}
	if (stv0299_wait_diseqc_idle(sc, 100) < 0)
                return -ETIMEDOUT;

        return 0;		
}

/*----------------------------------------------------------------*/
static int
stv0299_send_diseqc_burst(struct adapter *sc, fe_sec_mini_cmd_t burst)
{
	u_int8_t val;
	DBG1("\n");
        if (stv0299_wait_diseqc_idle(sc, 100) < 0)
                return -ETIMEDOUT;

        val = stv0299_readreg(sc, 0x08);

        if (stv0299_writereg(sc, 0x08, (val & ~0x7) | 0x2))   /* burst mode */
                return -EIO;

        if (stv0299_writereg(sc, 0x09, burst == SEC_MINI_A ? 0x00 : 0xff))
                return -EIO;

        if (stv0299_wait_diseqc_idle(sc, 100) < 0)
                return -ETIMEDOUT;

        if (stv0299_writereg(sc, 0x08, val))
                return -EIO;

        return 0;
}

/*----------------------------------------------------------------*/
static int
stv0299_set_tone(struct adapter *sc, fe_sec_tone_mode_t tone)
{
	u_int8_t val;
	DBG1("\n");

        if (stv0299_wait_diseqc_idle(sc, 100) < 0)
                return -ETIMEDOUT;

        val = stv0299_readreg(sc, 0x08);

        switch (tone){
        case SEC_TONE_ON:
		DBG1("SEC_TONE_ON\n");
                return stv0299_writereg(sc, 0x08, val | 0x3);
        case SEC_TONE_OFF:
		DBG1("SEC_TONE_OFF\n");
                return stv0299_writereg(sc, 0x08, (val & ~0x3) | 0x02);
        default:
		DBG1("SEC_TONE_ %d -EINVAL\n", tone);
                return -EINVAL;
        };
}

/*----------------------------------------------------------------*/
static int
stv0299_set_symbolrate(struct adapter *sc, u_int32_t srate)
{
	u_int64_t big = srate;
	u_int32_t ratio;
        u_int32_t tmp;
        u_int8_t aclk = 0, bclk = 0;
	u_int8_t m1;
	int Mclk = M_CLK, Fin;

	DBG1("\n");

	if ((srate < 1000000) || (srate > 45000000))
		 return -EINVAL;

	DBG1("srate %d\n", srate);

#define FIN (M_CLK >> 4)
	Fin = Mclk >> 4;

        tmp = srate << 4;
        ratio = tmp / Fin;

        tmp = (tmp % Fin) << 8;
        ratio = (ratio << 8) + tmp / Fin;

        tmp = (tmp % Fin) << 8;
        ratio = (ratio << 8) + tmp / Fin;
#if 0
	big = big << 20;
	big += (Mclk-1);

/*	do_div(big, Mclk);*/
	DBG(" big = %u Mclk = %u\n", big, Mclk);
	big = __udivdi3(big, Mclk);

	ratio = big << 4;
#endif
	ratio &= 0xfffff0; /* be sure for not overflow */

	DBG1("ratio == %u (%x)\n", ratio, ratio);

	     if (srate <=  1500000) { aclk = 0xb7; bclk = 0x47; }
	else if (srate <=  3000000) { aclk = 0xb7; bclk = 0x4b; }
	else if (srate <=  7000000) { aclk = 0xb7; bclk = 0x4f; }
	else if (srate <= 14000000) { aclk = 0xb7; bclk = 0x53; }
	else if (srate <= 30000000) { aclk = 0xb6; bclk = 0x53; }
	else if (srate <= 45000000) { aclk = 0xb4; bclk = 0x51; }

	stv0299_writereg(sc, 0x13, aclk);
	stv0299_writereg(sc, 0x14, bclk);

	stv0299_writereg(sc, 0x1f, (ratio >> 16) & 0xff);
	stv0299_writereg(sc, 0x20, (ratio >>  8) & 0xff);
	stv0299_writereg(sc, 0x21, (ratio      ) & 0xf0);

//	m1 = 0x12;
//	stv0299_writereg(sc, 0x0f, (stv0299_readreg(sc, 0x0f) & 0xc0) | m1);

	return 0;
}

/*----------------------------------------------------------------*/
static int
stv0299_get_symbolrate(struct adapter *sc)
{
	u_int32_t Mclk = M_CLK/4096L;
        u_int32_t srate;
        int32_t offset;
        u_int8_t sfr[3];
        int8_t rtf;

	DBG1("\n");

        stv0299_readregs(sc, 0x1f, sfr, 3);
        stv0299_readregs(sc, 0x1a, &rtf, 1);

	DBG1("sfr[0] = %d sfr[1] = %d sfr[2] = %d rtf = %d\n", sfr[0],sfr[1],sfr[2],rtf);

        srate = (sfr[0] << 8) | sfr[1];
        srate *= Mclk;
        srate /= 16;
        srate += (sfr[2] >> 4) * Mclk / 256;

        offset = (int32_t)rtf * (srate / 4096L);
        offset /= 128;

	DBG1("srate = %d\n", srate);
        DBG1("offset = %d\n", offset);

        srate += offset;

        srate += 1000;
        srate /= 2000;
        srate *= 2000;

	DBG1("ret srate = %u\n", srate);
        return srate;
}

/*----------------------------------------------------------------*/
static int
UpdateEbNo(struct adapter *sc)
{

static struct EbNo_t {
        int32_t val;
        int32_t ebno;
} EbNo[] =
        {{9700,1000},
         {9600,1500},
         {9450,2000},
         {9260,2500},
         {9000,3000},
         {8760,3500},
         {8520,4000},
         {8250,4500},
         {7970,5000},
         {7690,5500},
         {7360,6000},
         {7080,6500},
         {6770,7000},
         {6470,7500},
         {6200,8000},
         {5900,8500},
         {5670,9000},
         {5420,9500},
         {5190,10000},
	{4960,10500},
	{4740,11000},
	{4550,11500},
	{4360,12000},
	{4170,12500},
	{4010,13000},
	{3860,13500},
	{3710,14000},
	{3580,14500},
	{3440,15000},
	{3300,15500}};

		int32_t i = 0, ebno = 0;

		u_int32_t snr = ((stv0299_readreg(sc, 0x24) << 8)
			        | stv0299_readreg(sc, 0x25));

		  DBG1 ("SNR: snr=0x%04x\n", snr);

		if (snr > 9700)
			return 1000;

		ebno = 15500;

		for (i = 0; i < sizeof(EbNo)/sizeof(EbNo[0]); i++)
		{
			if (snr >= (u_int32_t)EbNo[i].val)
			{
				ebno = EbNo[i].ebno;
				break;
			} 
		}

		return ebno;
}

/*----------------------------------------------------------------*/
int
stv0299_ioctl(struct dvb_frontend *fe, unsigned int cmd, void *arg)
{
	struct adapter *sc = (struct adapter *)fe->sc;
	struct stv0299_state *state = (struct stv0299_state *)fe->data;
	int ret, i;

	switch(cmd){

	case FE_GET_INFO:
		memcpy(arg, &samsung_tbmu24112_info, sizeof(struct dvb_frontend_info));
		break;

	case FE_READ_STATUS:
		{
		fe_status_t *status = (fe_status_t *)arg;
                u_int8_t signal = 0xff - stv0299_readreg(sc, 0x18);
                u_int8_t sync = stv0299_readreg(sc, 0x1b);
                	DBG1("VSTATUS: 0x%02x\n", sync);
                *status = 0;
                if (signal > 10)	*status |= FE_HAS_SIGNAL;

                if (sync & 0x80)	*status |= FE_HAS_CARRIER;
                if (sync & 0x10)	*status |= FE_HAS_VITERBI;
                if (sync & 0x08)	*status |= FE_HAS_SYNC;

                if ((sync & 0x98) == 0x98){
					*status |= FE_HAS_LOCK;
		/*			stv0299_writereg(sc, 0x12, 0x39); */
		}
		/* else			stv0299_writereg(sc, 0x12, 0xb9); */
		} break;

	case FE_READ_SIGNAL_STRENGTH:
		{
			int32_t i = 0, ebno = 0;
			int32_t fec_vl = 0;
			fe_code_rate_t fec;
			u_int32_t snr = 0;

			ebno = UpdateEbNo(sc);
			fec = stv0299_get_FEC(sc);

			switch(fec){
			case FEC_1_2:
				fec_vl = 1100; break;
			case FEC_2_3:
				fec_vl = 1300; break;
			case FEC_3_4:
				fec_vl = 1500; break;
			case FEC_5_6:
				fec_vl = 1800;  break;
			case FEC_7_8:
				fec_vl = 2200;  break;
			default:
				fec_vl = 2700;
			}

			DBG1("EbNo[%d] = [%d,%d] fec[%d] = %d\n", 
				i, snr, ebno, fec, (signed)fec_vl);

			snr = ((ebno - fec_vl) * 70)/(13500 - fec_vl) + 30;

			*((u_int32_t *)arg) = snr;
			DBG1("QUALITY=%d\n", snr);

		} break;

	case FE_READ_SNR:
		{
			int32_t i = 0, ebno = 0;
			int32_t fec_vl = 0;
			fe_code_rate_t fec;
			u_int32_t snr = 0;

			ebno = UpdateEbNo(sc);
			fec = stv0299_get_FEC(sc);

			switch(fec){
			case FEC_1_2:
				fec_vl = -3010; break;
			case FEC_2_3:
				fec_vl = -1761; break;
			case FEC_3_4:
				fec_vl = -1249; break;
			case FEC_5_6:
				fec_vl = -792;  break;
			case FEC_7_8:
				fec_vl = -580;  break;
			default:
				;
			}
			DBG1("EbNo[%d] = [%d,%d] fec[%d] = %d\n", 
				i, snr, ebno, fec, (signed)fec_vl);
			snr = (signed)((signed)ebno + (signed)fec_vl + 3053);
			*((u_int32_t *)arg) = snr;
			DBG1("SNR=%d\n", snr);
		} break;

	case FE_SET_FRONTEND:
			DBG1("FE_SET_FRONTEND\n");
		{
			struct dvb_frontend_parameters *p = (struct dvb_frontend_parameters *)arg;

			DBG1("set frequency %d\n", p->frequency);
			DBG1("set inversion %d\n", p->inversion);
			DBG1("set u.qpsk.symbol_rate %d\n", p->u.qpsk.symbol_rate);
			DBG1("set u.qpsk.fec_inner %d\n", p->u.qpsk.fec_inner);
#if 1
		if (p->inversion == INVERSION_OFF) {
			DBG("FE_SET_FRONTEND INV_OFF\n");
			stv0299_writereg(sc, 0x0c, stv0299_readreg(sc, 0x0c) | 0xfe);
		} else if (p->inversion == INVERSION_ON) {
			DBG("FE_SET_FRONTEND INV_ON\n");
			stv0299_writereg(sc, 0x0c, stv0299_readreg(sc, 0x0c) | 1);
		} else {
			DBG("FE_SET_FRONTEND INV_??\n");
			stv0299_check_inversion(sc);
		}
#else
			stv0299_writereg(sc, 0x0c, stv0299_readreg(sc, 0x0c) | 0);
#endif
			/* A "normal" tune is requested */
//			stv0299_writereg(sc, 0x32, 0x80);

			/* zap the derotator registers first */
//			stv0299_writereg(sc, 0x22, 0x00);
//			stv0299_writereg(sc, 0x23, 0x00);

//			stv0299_writereg(sc, 0x32, 0x19);

			pll_set_freq(sc, p->frequency);
			stv0299_set_FEC(sc, p->u.qpsk.fec_inner);
			stv0299_set_symbolrate(sc, p->u.qpsk.symbol_rate);

//			stv0299_writereg(sc, 0x22, 0x00);
//			stv0299_writereg(sc, 0x23, 0x00);
#if 1
			state->fe_freq = p->frequency;
			state->fec_inner = p->u.qpsk.fec_inner;
			state->sr = p->u.qpsk.symbol_rate;
#endif	
			DBG1("FE_SET_FRONEND -end\n");
		} break;

	case FE_GET_FRONTEND:
			DBG1("FE_GET_FRONTEND\n");
		{
			struct dvb_frontend_parameters *p = (struct dvb_frontend_parameters *)arg;
			int32_t derot_freq;
			int Mclk = M_CLK;

			derot_freq = (int32_t)(int16_t)
					 ((stv0299_readreg(sc, 0x22) << 8)
					 | stv0299_readreg(sc, 0x23));

			DBG1("get freq %d\n", derot_freq);

			derot_freq *= (Mclk >> 16);
			derot_freq += 500;
			derot_freq /= 1000;
			
			DBG1("get freq %d\n", derot_freq);
			DBG1("get0 frequency %d\n", p->frequency);

			p->frequency = state->fe_freq + (int32_t)derot_freq;

			DBG1("get1 frequency %d\n", p->frequency);

			p->inversion = (stv0299_readreg(sc, 0x0c) & 1) ?
					INVERSION_OFF : INVERSION_ON;
			p->u.qpsk.fec_inner = stv0299_get_FEC(sc);
			p->u.qpsk.symbol_rate = stv0299_get_symbolrate(sc);

			DBG1("get2 frequency %d\n", p->frequency);
			DBG1("get inversion %d\n", p->inversion);
			DBG1("get u.qpsk.symbol_rate %d\n", p->u.qpsk.symbol_rate);
			DBG1("get u.qpsk.fec_inner %d\n", p->u.qpsk.fec_inner);

		} break;

	case FE_SLEEP:
			stv0299_writereg(sc, 0x0c, 0x00);  /*  LNB power off! */
			stv0299_writereg(sc, 0x08, 0x00);  /*  LNB power off! */
			stv0299_writereg(sc, 0x02, 0x80);
			break;

	case FE_INIT:
			state->fe_freq = 0;
			stv0299_init(sc);
			stv0299_set_timing(sc, 4, 7);
			break;

	case FE_RESET:
			stv0299_writereg(sc, 0x22, 0x00);
			stv0299_writereg(sc, 0x23, 0x00);

			stv0299_readreg(sc, 0x23);
			stv0299_writereg(sc, 0x12, 0xb9);
			break;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}


/*----------------------------------------------------------------*/
int
stv0299_detach(struct adapter *sc)
{
	struct fe_data *fe = sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;

	if (frontend->data)
		free(frontend->data, M_DEVBUF);

	return 0;
}

/*----------------------------------------------------------------*/
int
stv0299_attach(struct adapter *sc)
{
	struct fe_data *fe = sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;
	struct stv0299_state *state;
	u_int8_t id;
	int ret;

	DBG("\n");

	stv0299_writereg(sc, 0x02, 0x00); /* standby off */ //0x02 , 0x034
	id = stv0299_readreg(sc, 0x00);
	DBG("id == 0x%02x\n", id);

	if (id != 0xa1 && id != 0xa2)	//0x80
		return -ENODEV;

	stv0299_writereg(sc, 0x01, 0x15);
	stv0299_writereg(sc, 0x02, 0x30); /* wakeup !!! from stand by */
	stv0299_writereg(sc, 0x03, 0x00);


	if ((state = (struct stv0299_state *)malloc(sizeof(struct stv0299_state), M_DEVBUF, M_NOWAIT | M_ZERO)) == NULL){
		DBG("can't alloc mem for state\n");
		return -ENOMEM;
	}

	fe->id = id;
	fe->info = &samsung_tbmu24112_info;

	frontend->data = (struct stv0299_state *)state;

        frontend->ioctl = stv0299_ioctl;
        frontend->detach = stv0299_detach;

	InitTunerWithLnb(sc);

	sc->diseqc_send_master_cmd = stv0299_send_diseqc_msg;
	sc->diseqc_send_burst = stv0299_send_diseqc_burst;
	sc->set_tone = stv0299_set_tone;

	state->fe_freq = 0;

	return 0;
}

