/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include "i2c.h"
#include "sllutil.h"

/*----------------------------------------------------------------*/
static u_int32_t
FlexI2cRW(struct adapter *sc,	//0
	u_int32_t device,	//4
	u_int32_t chip_addr,	//8
	u_int32_t dir,	//C
	u_int16_t addr,	//10
	u_int8_t *buf,	//14
	u_int32_t len,	//18
	unsigned int unkC,	//1C
	unsigned int retr,	//20
	unsigned int unkD,	//24
	unsigned int unkE,	//28
	unsigned int unkF,	//2C
	unsigned int unk14,	//30
	unsigned int eeprom,	//34
	unsigned int bank)	//38
{
	u_int32_t cmd;
	u_int32_t val;
	u_int32_t bankaddr;
	u_int32_t bankval;
	int i;
	int r;
	int res;
	int state;
	int done;

	res = 0;

	if (bank)
	{
		if (device == 0x20000000)
			bankaddr = 0x10C;
		else
		if (device == 0x30000000)
			bankaddr = 0x110;
		else
			bankaddr = 0x108;

		bankval = read_reg(sc, bankaddr);

		write_reg(sc, bankaddr, bank);
	}

	state = unkD ? 3 : 0;

	if (addr > 0xFF && eeprom == 1)
	{
		chip_addr |= ((addr >> 8) & 3);
	}

	cmd = device | ((len - 1) << 26) | (1 << 16) | (addr << 8) | chip_addr;

	if (dir != 0)
	{
		cmd |= 0x2000000;
	} else
	{
		cmd |= (buf[0] << 16);

		if (len > 1)
		{
			val = 0;
			for (i = len; i > 1; i--)
			{
				val <<= 8;
				val |= buf[i-1];
			}

			write_reg(sc, 0x104, val);
		}
	}

	done = 0;

	for (r = 0; r < 100000; r++)
	{
		if (done != 0)
			break;

		switch (state)
		{
		case 3:
			write_reg(sc, 0x100, 0);
			write_reg(sc, 0x100, device | ((unkE | unkD | 0x100) << 16) | (unkF << 8) | unk14);
			state = 5;
			break;

		case 5:
			val = read_reg(sc, 0x100);
			if (val & 0x40000000)
			{
				if (retr--)
					state = 3;
				else
					done = 1;
			} else
			{
				if ((val & 0x81000000) == 0x80000000)
					state = 0;
			}
			break;

		case 0:
			write_reg(sc, 0x100, 0);
			write_reg(sc, 0x100, cmd);

			if ((unkC != 0) && (cmd & 0x2000000))
				state = 1;
			else
				state = 2;
			break;

		case 1:
			val = read_reg(sc, 0x100);
		
			if (! (val &  0x40000000) )
				break;

			write_reg(sc, 0x100, 0);

			if (unkD == 0)
			{
				write_reg(sc, 0x100, cmd | 0x40000000);
				state = 2;
			} else
			{
				write_reg(sc, 0x100, device | ((unkE | unkD | 0x100) << 16) | (unkF << 8) | unk14);
				state = 6;
			}
			break;

		case 6:	
			val = read_reg(sc, 0x100);

			if (val & 0x40000000)
			{
				if (retr --)
					state = 1;
				else
					done = 1;
			} else
			{
				if ((val & 0x81000000) == 0x80000000)
				{
					write_reg(sc, 0x100, 0);
					write_reg(sc, 0x100, cmd | 0x40000000);
					state = 2;
				}
			}
			break;

		case 2:
			val = read_reg(sc, 0x100);

			if (val & 0x40000000)
			{
				if (retr --)
				{
					state = unkD ? 3 : 0;
				} else
				{
					if (dir != 0 && len > 0)
						memset(&buf[0], 0xFF, len);
				}
			} else
			{
				if ((val & 0x81000000) != 0x80000000)
					break;

				if (dir != 0)
				{
					buf[0] = (val >> 16) & 0xFF;

					if (len > 1)
					{
						val = read_reg(sc, 0x104);

						for (i = 1; i < len; i++)
						{
							buf[i] = val & 0xFF;
							val >>= 8;
						}
					}
				}
				res = 1;
			}
			done = 1;
			break;

		} //switch

	} // for

	if (bankval)
	{
		write_reg(sc, bankaddr, bankval);
	}

	return res;
}
		
/*----------------------------------------------------------------*/
static u_int32_t
FlexI2cRead4(struct adapter *sc,
		unsigned int device,
		unsigned int chip_addr,
		unsigned short addr,
		unsigned char *buf,
		unsigned int len,
		unsigned int unkC,
		unsigned int retr,	//unk8
		unsigned int unkD,
		unsigned int unkE,
		unsigned int unkF,
		unsigned int unk14,
		unsigned int eeprom,	//unk10
		unsigned int bank)	//unk18
{
	return FlexI2cRW(sc,		//0
			device,		//4
			chip_addr,	//8
			1,		//C
			addr,		//10
			buf,		//14
			len,		//18
			unkC,		//1C
			retr,		//20
			unkD,		//24
			unkE,		//28
			unkF,		//2C
			unk14,		//30
			eeprom,		//34
			bank);		//38
}

/*----------------------------------------------------------------*/
static u_int32_t
FlexI2cWrite4(struct adapter *sc,
		unsigned int device,
		unsigned int chip_addr,
		unsigned short addr,
		unsigned char *buf,
		unsigned int len,
		unsigned int unkC,
		unsigned int retr,	//unk8
		unsigned int unkD,
		unsigned int unkE,
		unsigned int unkF,
		unsigned int unk14,
		unsigned int eeprom,	//unk10
		unsigned int bank)	//unk18
{
	return FlexI2cRW(sc,
			device,
			chip_addr,
			0,
			addr,
			buf,
			len,
			unkC,
			retr,
			unkD,
			unkE,
			unkF,
			unk14,
			eeprom,
			bank);
}


/*----------------------------------------------------------------*/
u_int32_t
FLEXI2C_busRead(struct adapter *sc, struct _I2CBUS *bus, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t bytes2transfer;
	u_int8_t *start;

	if (bus == NULL)
		return 0;

	start = buf;
	while (len != 0)
	{
		bytes2transfer = len;

		if (bytes2transfer > 4)
			bytes2transfer = 4;

		if (FlexI2cRead4(sc,
				bus->device,
				bus->unk4,
				addr,
				buf,
				bytes2transfer,
				bus->unkC,
				bus->retr,
				bus->unkD,
				bus->unkE,
				bus->unkF,
				bus->unk14,
				bus->eeprom,
				bus->unk18) == 0)
			return (buf - start);

		buf += bytes2transfer;
		addr += bytes2transfer;
		len -= bytes2transfer;
	}
	return (buf - start);
}

/*----------------------------------------------------------------*/
u_int32_t
FLEXI2C_busWrite(struct adapter *sc, struct _I2CBUS *bus, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t bytes2transfer;
	u_int8_t *start;

	if (bus == NULL)
		return 0;

	start = buf;
	while (len != 0)
	{
		bytes2transfer = len;

		if (bytes2transfer > 4)
			bytes2transfer = 4;

		if (FlexI2cWrite4(sc,
				bus->device,
				bus->unk4,
				addr,
				buf,
				bytes2transfer,
				bus->unkC,
				bus->retr,
				bus->unkD,
				bus->unkE,
				bus->unkF,
				bus->unk14,
				bus->eeprom,
				bus->unk18) == 0)
			return (buf - start);

		if (len > bytes2transfer && bus->wait)
			DELAY(bus->wait);

		buf += bytes2transfer;
		addr += bytes2transfer;
		len -= bytes2transfer;
	}
	return (buf - start);
}

/*----------------------------------------------------------------*/
u_int32_t
FLEXI2C_read(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t bytes2transfer;
	u_int8_t *start;

	start = buf;
	while (len != 0)
	{
		bytes2transfer = len;

		if (bytes2transfer > 4)
			bytes2transfer = 4;


		if (FlexI2cRead4(sc,
				device,
				bus,
				addr,
				buf,
				bytes2transfer,
				0,
				1,
				0,
				0,
				0,
				0,
				0,
				0) == 0)
			return (buf - start);

		buf += bytes2transfer;
		addr += bytes2transfer;
		len -= bytes2transfer;
	}
	return (buf - start);
}

/*----------------------------------------------------------------*/
u_int32_t
FLEXI2C_write(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t bytes2transfer;
	u_int8_t *start;

	start = buf;
	while (len != 0)
	{
		bytes2transfer = len;

		if (bytes2transfer > 4)
			bytes2transfer = 4;

		if (FlexI2cWrite4(sc,
				device,
				bus,
				addr,
				buf,
				bytes2transfer,
				0,
				1,
				0,
				0,
				0,
				0,
				0,
				0) == 0)
			return (buf - start);

		buf += bytes2transfer;
		addr += bytes2transfer;
		len -= bytes2transfer;
	}
	return (buf - start);
}

/*----------------------------------------------------------------*/
int
master_xfer(struct adapter *sc, const struct i2c_msg *msgs, int num)
{
	int i, ret = 0;
	int s;

	/* read */
	if ((num == 2) &&
	    (msgs[0].flags == 0) && 
	    (msgs[1].flags == I2C_M_RD) &&
	    (msgs[0].buf != NULL) && (msgs[1].buf != NULL))
	{

		ret = FLEXI2C_read(sc, 0x10000000, msgs[0].addr, msgs[0].buf[0], msgs[1].buf, msgs[1].len);

		if (ret != msgs[1].len)
			ret =  -EIO;
		ret = num;
	} else
	/* write */
	for (i = 0; i < num; i++){
		if ((msgs[i].flags != 0) || 
		    (msgs[i].buf == NULL) || 
		    (msgs[i].len < 2))
		{
			ret =  -EINVAL;
			break;
		}

		ret = FLEXI2C_write(sc, 0x10000000, msgs[i].addr, msgs[i].buf[0], &msgs[i].buf[1], msgs[i].len - 1);
		if (ret != msgs[0].len - 1)
			ret = -EIO;
		ret = num;
	}

	return ret;
}

