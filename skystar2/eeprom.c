/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include "eeprom.h"
#include "sllutil.h"

/*----------------------------------------------------------------*/
static int
EEPROM_write(struct adapter *sc, u_int16_t addr, u_int8_t *buf, u_int16_t len)
{
	return FLEXI2C_write(sc, 0x20000000, 0x50, addr, buf, len);
}

/*----------------------------------------------------------------*/
static int
EEPROM_read(struct adapter *sc, u_int16_t addr, u_int8_t *buf, u_int16_t len)
{
	return FLEXI2C_read(sc, 0x20000000, 0x50, addr, buf, len);
}

/*----------------------------------------------------------------*/
static u_int8_t
calc_LRC(u_int8_t *buf, int len)
{
	int i;
	u_int8_t sum;

	sum = 0;
	for (i = 0; i < len; i++) 
		sum = sum ^ buf[i];
	return sum;
}

/*----------------------------------------------------------------*/
static int
EEPROM_LRC_read(struct adapter *sc, u_int32_t addr, u_int32_t len, u_int8_t *buf, int retr)
{
	int i;

	for (i = 0; i < retr; i++)
	{
		if (EEPROM_read(sc, addr, buf, len) == len)
		{
			if (calc_LRC(buf, len-1) == buf[len-1])
			 return 1;
		}
	}
	return 0;
}

/*----------------------------------------------------------------*/
static int
EEPROM_LRC_write(struct adapter *sc, u_int32_t addr, u_int32_t len, u_int8_t *wbuf, u_int8_t *rbuf, int retr)
{
	int i;

	for (i = 0; i < retr; i++)
	{
		if (EEPROM_write(sc, addr, wbuf, len) == len)
		{
			if (EEPROM_LRC_read(sc, addr, len, rbuf, retr) == 1)
				return 1;
		}
	}
	return 0;
}

/*----------------------------------------------------------------*/
int
EEPROM_getMacAddr(struct adapter *sc, char type, u_int8_t *mac)
{
	u_int8_t tmp[8];

	if (EEPROM_LRC_read(sc, 0x3F8, 8, tmp, 4) != 0)
	{
		if (type != 0)
		{
			mac[0] = tmp[0];
			mac[1] = tmp[1];
			mac[2] = tmp[2];
			mac[3] = 0xFE;
			mac[4] = 0xFF;
			mac[5] = tmp[3];
			mac[6] = tmp[4];
			mac[7] = tmp[5];
		} else {
			mac[0] = tmp[0];
			mac[1] = tmp[1];
			mac[2] = tmp[2];
			mac[3] = tmp[3];
			mac[4] = tmp[4];
			mac[5] = tmp[5];
		}
		return 1;
	} else {
		if (type != 0) memset(mac, 0, 8);
	 		else   memset(mac, 0, 6);
		return 0;
	}
}

/*----------------------------------------------------------------*/
int
EEPROM_setMacAddr(struct adapter *sc, char type, u_int8_t *mac)
{
	u_int8_t tmp[8];

	if (type != 0)
	{
		tmp[0] = mac[0];
		tmp[1] = mac[1];
		tmp[2] = mac[2];
		tmp[3] = mac[5];
		tmp[4] = mac[6];
		tmp[5] = mac[7];
	} else {
		tmp[0] = mac[0];
		tmp[1] = mac[1];
		tmp[2] = mac[2];
		tmp[3] = mac[3];
		tmp[4] = mac[4];
		tmp[5] = mac[5];
	}

	tmp[6] = 0;
	tmp[7] = calc_LRC(tmp, 7);

	if (EEPROM_write(sc, 0x3F8, tmp, 8) == 8)
	{
		return 1;
	}
	return 0;
}

/*----------------------------------------------------------------*/
int
EEPROM_writeTunerInfo(struct adapter *sc, unsigned char *info)
{
	unsigned char tmp1[68];
	unsigned char tmp2[68];
	int res;

	tmp1[0] = 0xB2;
	memcpy(&tmp1[1], info, 64);
	tmp1[65] = 0xC2;
	tmp1[66] = calc_LRC(tmp1, 66);

	res = 1;

	return res;
}

/*----------------------------------------------------------------*/
int
EEPROM_readTunerInfo(struct adapter *sc, unsigned char *info)
{
	unsigned char tmp[80];
	int res;

	res = EEPROM_LRC_read(sc, 0x3B5, 67, tmp, 4);

	if (tmp[0] == 0xB2 && tmp[65] == 0xC2)
	{
		if (info != NULL)
		{
			memcpy(info, &tmp[1], 64);
		}
	}
	return res;
}

