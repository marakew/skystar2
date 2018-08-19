/*
 *	DVB-S SAMSUNG TBDU18132 Frontend vp310/mt312 PLL synth tsa5059
 */

#include <sys/types.h>

#include "../include/frontend.h"
#include "skystar2.h"
#include "i2c.h"
#include "frontend.h"
#include "samsung_tbdu18132.h"
#include "diseqc.h"
#include "debug.h"

static int debug = 0;

#define MT312_SYS_CLK           90000000UL      /* 90 MHz */
#define MT312_LPOWER_SYS_CLK    60000000UL      /* 60 MHz */
#define MT312_PLL_CLK           10000000UL      /* 10 MHz */


static struct dvb_frontend_info samsung_tbdu18132_info = {
	name: "Samsung TBDU18132(PLL TSA5059)",
        type: FE_QPSK,
        frequency_min: 950000,
        frequency_max: 2150000,
        frequency_stepsize: (MT312_PLL_CLK / 1000) / 128,
        /*frequency_tolerance: 29500,         FIXME: */
        symbol_rate_min: MT312_SYS_CLK / 128,
        symbol_rate_max: MT312_SYS_CLK / 2,
        /*symbol_rate_tolerance: 500,         FIXME: */
        notifier_delay: 0,
        caps:   FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
                FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 |
                FE_CAN_QPSK |
                FE_CAN_FEC_AUTO | FE_CAN_INVERSION_AUTO |
                /* FE_CAN_RECOVER | */
                FE_CAN_CLEAN_SETUP | FE_CAN_MUTE_TS
};

/*----------------------------------------------------------------*/
static int
mt312_read(struct adapter *sc, const enum mt312_reg_addr reg, void *buf, const size_t count)
{
	int ret;
	struct i2c_msg msg[2];
	u_int8_t regbuf[1] = { reg };

	DBG1("\n");

	msg[0].addr = 0x0e;
	msg[0].flags = 0;
	msg[0].buf = regbuf;
	msg[0].len = 1;
	msg[1].addr = 0x0e;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = count;
	
	ret = master_xfer(sc, msg, 2);

	if (ret != 2) {
		DBG1("ret == %d\n", ret);
		return -2;
	}

#ifdef MT312_DEBUG
		int i;
		//DBG("R(%d):", reg & 0x7f);
		printf("read R(%d):", reg & 0x7f);
		for (i = 0; i < count; i++)
			//DBG(" %02x", ((const u_int8_t *) buf)[i]);
			printf(" %02x", ((const u_int8_t *) buf)[i]);
		//DBG("\n");
		printf("\n");
#endif
	return 0;
}

/*----------------------------------------------------------------*/
static int
mt312_write(struct adapter *sc, const enum mt312_reg_addr reg, const void *src, const size_t count)
{
	int ret;
	struct i2c_msg msg;
	u_int8_t buf[count+1];

	DBG1("\n");

#ifdef MT312_DEBUG
		int i;
		//DBG("R(%d):", reg & 0x7f);
		printf("write R(%d):", reg & 0x7f);
		for (i = 0; i < count; i++)
			//DBG(" %02x", ((const u_int8_t *) buf)[i]);
			printf(" %02x", ((const u_int8_t *) buf)[i]);
		//DBG("\n");
		printf("\n");
#endif

	buf[0] = reg;
	memcpy(&buf[1], src, count);
	
	msg.addr = 0x0e;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = count + 1;

	ret = master_xfer(sc, &msg, 1);

	if (ret != 1) {
		DBG1("ret == %d\n", ret);
		return -2;
	}
	return 0;
}

/*----------------------------------------------------------------*/
static int
mt312_readreg(struct adapter *sc, const enum mt312_reg_addr reg, u_int8_t *val)
{
	return mt312_read(sc, reg, val, 1);
}

/*----------------------------------------------------------------*/
static int
mt312_writereg(struct adapter *sc, const enum mt312_reg_addr reg, const u_int8_t val)
{
	return mt312_write(sc, reg, &val, 1);
}

/*----------------------------------------------------------------*/
static int
mt312_pll_write(struct adapter *sc, const u_int8_t addr, const u_int8_t *buf, const u_int8_t len)
{
	int ret;
	struct i2c_msg msg;

	DBG1("\n");

	msg.addr = addr;
	msg.flags = 0;
	msg.buf = (u_int8_t *)buf;
	msg.len = len;

	if ((ret = mt312_writereg(sc, GPP_CTRL, 0x40)) < 0)
		return ret;

	if ((ret = master_xfer(sc, &msg, 1)) != 1)
		DBG1("ret == %d\n", ret);

	if ((ret = mt312_writereg(sc, GPP_CTRL, 0x00)) < 0)
		return ret;	
	return 0;
}

static u_int32_t 
mt312_div(u_int32_t a, u_int32_t b)
{
	return (a + (b / 2)) / b;
}

static int
tsa5059_set_tv_freq(struct adapter *sc, u_int32_t freq, u_int32_t sr)
{
	u_int8_t buf[4];

	u_int32_t ref = mt312_div(freq, 125);

	DBG1("\n");
	//printf("tsa5059_set_tv_freq: %u, %u\n", freq, sr);

	buf[0] = (ref >> 8) & 0x7f;
        buf[1] = (ref >> 0) & 0xff;
        buf[2] = 0x84 | ((ref >> 10) & 0x60);
        buf[3] = 0x80;

        if (freq < 1550000)
                buf[3] |= 0x02;

	return mt312_pll_write(sc, 0x61, buf, sizeof(buf));
}

static int
mt312_reset(struct adapter *sc, const u_int8_t full)
{
	return mt312_writereg(sc, RESET, full ? 0x80 : 0x40);
}

static int
mt312_init(struct adapter *sc/*, const long id*/, u_int8_t pll)
{
	int ret;
	u_int8_t buf[2];

	DBG1("\n");

	/* wake up */
        if ((ret = mt312_writereg(sc, CONFIG, (pll == 60 ? 0x88 : 0x8c))) < 0)
                return ret;

	DELAY(150);

	/* full reset */
        if ((ret = mt312_reset(sc, 1)) < 0)
                return ret;

	u_int8_t buf_def[8]={0x14, 0x12, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00};
	if ((ret = mt312_write(sc, VIT_SETUP, buf_def, sizeof(buf_def))) < 0)
		return ret;

	/* SYS_CLK */
        buf[0] = mt312_div((pll == 60 ? MT312_LPOWER_SYS_CLK : MT312_SYS_CLK) * 2, 1000000);

	/* DISEQC_RATIO */
        buf[1] = mt312_div(MT312_PLL_CLK, 15000 * 4);

        if ((ret = mt312_write(sc, SYS_CLK, buf, sizeof(buf))) < 0)
                return ret;

        if ((ret = mt312_writereg(sc, SNR_THS_HIGH, 0x32)) < 0)
                return ret;

        if ((ret = mt312_writereg(sc, OP_CTRL, 0x53)) < 0)
                return ret;

	/* TS_SW_LIM */
        buf[0] = 0x8c;
        buf[1] = 0x98;

        if ((ret = mt312_write(sc, TS_SW_LIM_L, buf, sizeof(buf))) < 0)
                return ret;

        if ((ret = mt312_writereg(sc, CS_SW_LIM, 0x69)) < 0)
                return ret;

        return 0;
}

static int
mt312_read_status(struct adapter *sc, fe_status_t *s/*, const long id*/)
{
	int ret;
	u_int8_t status[3], vit_mode;

	DBG1("\n");

	*s = 0;

	if ((ret = mt312_read(sc, QPSK_STAT_H, status, sizeof(status))) < 0)
			return ret;

	if (status[0] & 0xc0)
                *s |= FE_HAS_SIGNAL;    /* signal noise ratio */
        if (status[0] & 0x04)
                *s |= FE_HAS_CARRIER;   /* qpsk carrier lock */
        if (status[2] & 0x02)
                *s |= FE_HAS_VITERBI;   /* viterbi lock */
        if (status[2] & 0x04)
                *s |= FE_HAS_SYNC;      /* byte align lock */
        if (status[0] & 0x01)
                *s |= FE_HAS_LOCK;      /* qpsk lock */

	/* VP310 doesn't have AUTO, so we "implement it here" ACCJr */
        if (!(status[0] & 0x01)) {
                if ((ret = mt312_readreg(sc, VIT_MODE, &vit_mode)) < 0)
                        return ret;
                vit_mode ^= 0x40;
                if ((ret = mt312_writereg(sc, VIT_MODE, vit_mode)) < 0)
                        return ret;
                if ((ret = mt312_writereg(sc, GO, 0x01)) < 0)
                        return ret;
        }

        return 0;
}

static int
mt312_get_code_rate(struct adapter *sc, fe_code_rate_t *cr)
{
	const fe_code_rate_t fec_tab[8] =
		{ FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_6_7, FEC_7_8,
		FEC_AUTO, FEC_AUTO };

	int ret;
	u_int8_t fec_status;

	if ((ret = mt312_readreg(sc, FEC_STATUS, &fec_status)) < 0)
		return ret;

	*cr = fec_tab[(fec_status >> 4) & 0x07];
	return 0;
}

static int
mt312_read_agc(struct adapter *sc, u_int32_t *snr)
{
	int ret;
	u_int8_t buf[2];
	int32_t fec_vl = 0;
	fe_code_rate_t fec;
	int32_t ebno = 0;
	u_int32_t m_snr = 0;

	DBG1("\n");

	if ((ret = mt312_read(sc, M_SNR_H, &buf, sizeof(buf))) < 0)
		return ret;

	ebno = 13312 - ((((buf[0] & 0x7f) << 8) | buf[1]));
	ebno *= 12000;
	ebno /= 8192;

	mt312_get_code_rate(sc, &fec);

	switch(fec){
	case FEC_1_2:
		//printf("FEC_1_2\n");
		fec_vl = 1100; break;
	case FEC_2_3:
		//printf("FEC_2_3\n");
		fec_vl = 1300; break;
	case FEC_3_4:
		//printf("FEC_3_4\n");
		fec_vl = 1500; break;
	case FEC_5_6:
		//printf("FEC_5_6\n");
		fec_vl = 1800;  break;
	case FEC_7_8:
		//printf("FEC_7_8\n");
		fec_vl = 2200;  break;
	default:
		//printf("FEC_DEFAULT\n");
		fec_vl = 2700;
	}
		
	DBG1("EbNo = [%d] fec[%d] = %d\n",
                                ebno, fec, (signed)fec_vl);

	m_snr = ((ebno - fec_vl) * 70)/(13500 - fec_vl) + 30;

	DBG1("QUALITY=%d\n", m_snr);
	//printf("QUALITY=%d\n", m_snr);

	*snr = m_snr;

	return 0;

}

static int
mt312_read_snr(struct adapter *sc, u_int32_t *snr)
{
	int ret;
	u_int8_t buf[2];
	int32_t fec_vl = 0;
	fe_code_rate_t fec;
	int32_t ebno = 0;
	u_int32_t m_snr = 0;

	DBG1("\n");
	//printf("%s\n", __FUNCTION__);

	if ((ret = mt312_read(sc, M_SNR_H, &buf, sizeof(buf))) < 0)
		return ret;

	ebno = 13312 - ((((buf[0] & 0x7f) << 8) | buf[1]));
	ebno *= 12000;
	ebno /= 8192;

	mt312_get_code_rate(sc, &fec);

	switch(fec){
	case FEC_1_2:
		//printf("FEC_1_2\n");
		fec_vl = -3010; break;
	case FEC_2_3:
		//printf("FEC_2_3\n");
		fec_vl = -1761; break;
	case FEC_3_4:
		//printf("FEC_3_4\n");
		fec_vl = -1249; break;
	case FEC_5_6:
		//printf("FEC_5_6\n");
		fec_vl = -792;  break;
	case FEC_7_8:
		//printf("FEC_7_8\n");
		fec_vl = -580;  break;
	default:
		//printf("FEC_DEFAULT\n");
		;
	}
		
	DBG1("EbNo = [%d] fec[%d] = %d\n",
                                ebno, fec, (signed)fec_vl);

	m_snr = (signed)((signed)ebno + (signed)fec_vl + 3053);  

	DBG1("SNR=%d\n", m_snr);
	//printf("SNR=%d\n", m_snr);

	*snr = m_snr;

	return 0;
}

static int
mt312_set_frontend(struct adapter *sc, const struct dvb_frontend_parameters *p)
{
	int ret;
	u_int8_t buf[5], config_val;
	u_int16_t sr;

	const u_int8_t fec_tab[10] =
		{ 0x00, 0x01, 0x02, 0x04, 0x3f, 0x08, 0x10, 0x20, 0x3f, 0x3f };
	const u_int8_t inv_tab[3] = { 0x00, 0x40, 0x80 };

	DBG1("\n");
	//printf("mt set begin\n");
#if 0
	if ((p->frequency < samsung_tbdu18132_info.frequency_min)
            || (p->frequency > samsung_tbdu18132_info.frequency_max))
	{
		//printf("mt freq error\n");
                return -EINVAL;
	}

	if ((p->inversion < INVERSION_OFF)
            || (p->inversion > INVERSION_AUTO))
	{
		//printf("mt inv\n");
                return -EINVAL;
	}

	if ((p->u.qpsk.symbol_rate < samsung_tbdu18132_info.symbol_rate_min)
            || (p->u.qpsk.symbol_rate > samsung_tbdu18132_info.symbol_rate_max))
	{
		//printf("mt rate\n");
                return -EINVAL;
	}

	if ((p->u.qpsk.fec_inner < FEC_NONE)
            || (p->u.qpsk.fec_inner > FEC_AUTO))
	{
		//printf("mt fec_inner\n");
                return -EINVAL;
	}

	if ((p->u.qpsk.fec_inner == FEC_4_5)
            || (p->u.qpsk.fec_inner == FEC_8_9))
	{
		//printf("mt fec_inner 4_5 8_9\n");
                return -EINVAL;
	}
#endif
	if ((ret = mt312_readreg(sc, CONFIG, &config_val) < 0))
		return ret;

	if (p->u.qpsk.symbol_rate >= 30000000){
		if ((config_val & 0x0c) == 0x08)
			if ((ret = mt312_init(sc, /*id,*/ (u_int8_t)90)) < 0)
				return ret;
	} else {
		if ((config_val & 0x0c) == 0x0C)
			if ((ret = mt312_init(sc, /*id,*/ (u_int8_t)60)) < 0)
				return ret;
	}

	if ((ret = tsa5059_set_tv_freq(sc, p->frequency, p->u.qpsk.symbol_rate)) < 0)
                return ret;

	/* sr = (u16)(sr * 256.0 / 1000000.0) */
        sr = mt312_div(p->u.qpsk.symbol_rate * 4, 15625);

        /* SYM_RATE */
        buf[0] = (sr >> 8) & 0x3f;
        buf[1] = (sr >> 0) & 0xff;

	/* VIT_MODE */
        buf[2] = inv_tab[p->inversion] | fec_tab[p->u.qpsk.fec_inner];

        /* QPSK_CTRL */
        buf[3] = 0x40;          /* swap I and Q before QPSK demodulation */

        if (p->u.qpsk.symbol_rate < 10000000)
                buf[3] |= 0x04; /* use afc mode */

        /* GO */
        buf[4] = 0x01;

        if ((ret = mt312_write(sc, SYM_RATE_H, buf, sizeof(buf))) < 0)
                return ret;

	//printf("set_mt ret 0\n");
        return 0;
}

static int
mt312_get_inversion(struct adapter *sc, fe_spectral_inversion_t *i)
{
	int ret;
	u_int8_t vit_mode;

	if ((ret = mt312_readreg(sc, VIT_MODE, &vit_mode)) < 0)
		return ret;

	if (vit_mode & 0x80)    /* auto inversion was used */
		*i = (vit_mode & 0x40) ? INVERSION_ON : INVERSION_OFF;

	return 0;
}

static int
mt312_get_symbol_rate(struct adapter *sc, u_int32_t *sr)
{
	int ret;
	u_int8_t sym_rate_h;
	u_int8_t dec_ratio;
	u_int16_t sym_rat_op;
	u_int16_t monitor;
	u_int8_t buf[2];


	DBG1("\n");

	if ((ret = mt312_readreg(sc, SYM_RATE_H, &sym_rate_h)) < 0)
                return ret;

	if (sym_rate_h & 0x80) {
		if ((ret = mt312_writereg(sc, MON_CTRL, 0x03)) < 0)
                        return ret;

                if ((ret = mt312_read(sc, MONITOR_H, buf, sizeof(buf))) < 0)
                        return ret;

                monitor = (buf[0] << 8) | buf[1];

	} else {

		if ((ret = mt312_writereg(sc, MON_CTRL, 0x05)) < 0)
                        return ret;

                if ((ret = mt312_read(sc, MONITOR_H, buf, sizeof(buf))) < 0)
                        return ret;

                dec_ratio = ((buf[0] >> 5) & 0x07) * 32;

                if ((ret = mt312_read(sc, SYM_RAT_OP_H, buf, sizeof(buf))) < 0)
                        return ret;

                sym_rat_op = (buf[0] << 8) | buf[1];
	}

	return 0;
}

static int
mt312_get_frontend(struct adapter *sc, struct dvb_frontend_parameters *p)
{
	int ret;

	DBG1("\n");

	if ((ret = mt312_get_inversion(sc, &p->inversion)) < 0)
		return ret;

	if ((ret = mt312_get_symbol_rate(sc, &p->u.qpsk.symbol_rate)) < 0)
		return ret;

	if ((ret = mt312_get_code_rate(sc, &p->u.qpsk.fec_inner)) < 0)
		return ret;
}

static int 
mt312_sleep(struct adapter *sc)
{
	int ret;
	u_int8_t config;

	DBG1("\n");

	/* reset all registers to defaults */
	if ((ret = mt312_reset(sc, 1)) < 0)
		return ret;

	if ((ret = mt312_readreg(sc, CONFIG, &config)) < 0)
		return ret;

	/* enter standby */
	if ((ret = mt312_writereg(sc, CONFIG, config & 0x7f)) < 0)
		return ret;

	return 0;
}


/*----------------------------------------------------------------*/
int
mt312_ioctl(struct dvb_frontend *fe, unsigned int cmd, void *arg)
{
	struct adapter *sc = (struct adapter *)fe->sc;

	DBG1("\n");
	//printf("mt ioctl\n");

	switch(cmd){

	case FE_GET_INFO:
		memcpy(arg, &samsung_tbdu18132_info, sizeof(struct dvb_frontend_info));
		break;

	case FE_READ_STATUS:
		return mt312_read_status(sc, arg);

	case FE_READ_SIGNAL_STRENGTH:
		return mt312_read_agc(sc, arg);

	case FE_READ_SNR:
		return mt312_read_snr(sc, arg);

	case FE_SET_FRONTEND:
		return mt312_set_frontend(sc, arg);

	case FE_GET_FRONTEND:
		return mt312_get_frontend(sc, arg);

	case FE_SLEEP:
		return mt312_sleep(sc);

	case FE_INIT:
			return mt312_init(sc, (u_int8_t) 60);

	case FE_RESET:
		return mt312_reset(sc, 0);

	default:
		return -EOPNOTSUPP;
		//return -ENOSYS;	
	}
	return 0;
}

/*----------------------------------------------------------------*/
int
mt312_detach(struct adapter *sc)
{
	struct fe_data *fe = sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;

}

/*----------------------------------------------------------------*/
int
mt312_attach(struct adapter *sc)
{
	struct fe_data *fe = sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;
	int ret;
	u_int8_t id;

	DBG1("\n");

	if ((ret = mt312_readreg(sc, ID, &id)) < 0)
		return ret;
	
        if (id != 1)
                return -ENODEV;

	fe->id = id;
	fe->info = &samsung_tbdu18132_info;

	frontend->data = (void *)(long)id;

	frontend->ioctl = mt312_ioctl;
	frontend->detach = mt312_detach;

	InitTunerWithLnb(sc);

	return 0;
}

