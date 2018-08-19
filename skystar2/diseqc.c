/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include "diseqc.h"
#include "sllutil.h"

/*----------------------------------------------------------------*/
static int
flexcop_set_tone(struct adapter *sc, fe_sec_tone_mode_t tone)
{
	u_int16_t wzHalfPeriodFor45MHz[] = { 0x01FF, 0x0154, 0x00FF, 0x00CC };
	u_int16_t ax;

	//printf("%s: %u\n", __FUNCTION__, tone);
#if 0
	switch(tone){
	case 1: ax = wzHalfPeriodFor45MHz[0]; break;	//ON
	case 2: ax = wzHalfPeriodFor45MHz[1]; break;
	case 3: ax = wzHalfPeriodFor45MHz[2]; break;
	case 4: ax = wzHalfPeriodFor45MHz[3]; break;
	default: ax = 0;				//OFF
	}
#endif
	switch(tone){
	case SEC_TONE_ON:	ax = 0x01FF; break;
	case SEC_TONE_OFF:	ax = 0; break;
	default: ax = 0;
	}

	if (ax != 0)
		write_reg(sc, 0x200, ((ax << 0x0F)+(ax & 0x7FFF))|0x40000000);
	else	write_reg(sc, 0x200, 0x40FF8000);

	return 0;
}

/*----------------------------------------------------------------*/
static int
flexcop_set_voltage(struct adapter *sc, fe_sec_voltage_t voltage)
{
	u_int32_t val;

	//printf("%s: %u\n", __FUNCTION__, voltage);

	val = read_reg(sc, 0x204);

	switch (voltage){
	case SEC_VOLTAGE_OFF: //0
		val = val | 1;
		break;
	case SEC_VOLTAGE_13: //1
		val = val & ~1;
		val = val & ~4;
		break;
	case SEC_VOLTAGE_18: //2
		val = val & ~1;
		val = val | 4;
		break;
	default:;
	}

	write_reg(sc, 0x204, val);
	return 0;
}


/*----------------------------------------------------------------*/
static void
diseqc_tone_period(struct adapter *sc, fe_sec_tone_mode_t tone, int delay)
{
	//printf("%s: %d\n", __FUNCTION__, delay);

	flexcop_set_tone(sc, tone);

	if (delay)
		DELAY(500 * delay);
}

/*----------------------------------------------------------------*/
static void
diseqc_send_bit(struct adapter *sc, int data)
{
	//printf("%s: %d\n", __FUNCTION__, data);

	diseqc_tone_period(sc, SEC_TONE_ON, data ? 1 : 2);	//1
	diseqc_tone_period(sc, SEC_TONE_OFF, data ? 2 : 1);	//0
}

/*----------------------------------------------------------------*/
static void
diseqc_send_byte(struct adapter *sc, int data)
{
	int i, par = 1, d;
	
	for (i = 7; i >= 0; i--){
		d = (data >> i) & 1;
		par ^= d;
		diseqc_send_bit(sc, d);
	}
	diseqc_send_bit(sc, par);
}

/*----------------------------------------------------------------*/
static int
send_diseqc_msg(struct adapter *sc, int len, u_int8_t *msg, u_int32_t burst)
{
	int i;

	//printf("%s\n", __FUNCTION__);

	flexcop_set_tone(sc, SEC_TONE_OFF); //0
	DELAY(16000);

	for (i = 0; i < len; i++)
		diseqc_send_byte(sc, msg[i]);

	DELAY(16000);
	if (burst != -1){
		if (burst)
			diseqc_send_byte(sc, 0xff);
		else {
			diseqc_tone_period(sc, SEC_TONE_ON, 25); //1
			diseqc_tone_period(sc, SEC_TONE_OFF, 0); //0
		}
		DELAY(20000);
	}
	return 0;
}

/*----------------------------------------------------------------*/
static int
flexcop_diseqc_send_master_cmd(struct adapter *sc, struct dvb_diseqc_master_cmd *cmd)
{
	//printf("%s\n", __FUNCTION__);
	return send_diseqc_msg(sc, cmd->msg_len, cmd->msg, 0);
}

static int
flexcop_diseqc_send_burst(struct adapter *sc, fe_sec_mini_cmd_t cmd)
{
	//printf("%s\n", __FUNCTION__);
	return send_diseqc_msg(sc, 0, NULL, cmd);
}

void
InitTunerWithLnb(struct adapter *sc)
{
        sc->diseqc_send_master_cmd = flexcop_diseqc_send_master_cmd;
        sc->diseqc_send_burst = flexcop_diseqc_send_burst;
        sc->set_tone = flexcop_set_tone;
        sc->set_voltage = flexcop_set_voltage;
}
