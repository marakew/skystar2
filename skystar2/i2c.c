/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include "i2c.h"
#include "sllutil.h"

/*---------------------------------------------------------------*/
static u_int32_t
i2cMainIO(struct adapter *sc, u_int32_t cmd, u_int8_t *buf, u_int32_t retr)
{
	u_int32_t i, val;
	
	write_reg(sc, 0x100, 0);
	write_reg(sc, 0x100, cmd);

	for (i = 0; i < retr; i++)
	{
		val = read_reg(sc, 0x100);
		if ( (val & 0x40000000) == 0)
		{
			if ( (val & 0x81000000) == 0x80000000)
			{
			 if (buf != 0) *buf = (val >> 0x10) & 0xff;
				return 1;
			}
		} else {
			write_reg(sc, 0x100, 0);
			write_reg(sc, 0x100, cmd);
		}
	}
	return 0;
}

/*================================================================
	device = 0x10000000 for tuner
		 0x20000000 for eeprom
  ================================================================*/
/*----------------------------------------------------------------*/
static u_int32_t
i2cMainSetup(u_int32_t device, u_int32_t chip_addr, u_int8_t op, u_int8_t addr, u_int32_t val, u_int32_t len)
{
	u_int32_t cmd;

	cmd = device | ((len-1)<<26) | (val<<16) | (addr<<8) | chip_addr;
	if (op != 0) cmd |= 0x03000000;
		else cmd |= 0x01000000;
	return cmd;
}


/*----------------------------------------------------------------*/
static u_int32_t
FlexI2cRead4(struct adapter *sc, u_int32_t device, u_int32_t chip_addr, u_int16_t addr, u_int8_t *buf, u_int8_t len)
{
	u_int32_t cmd,val;
	int res, i;

	cmd = i2cMainSetup(device, chip_addr, 1, addr, 0, len);
	res = i2cMainIO(sc, cmd, buf, 100000);

	if ( (res & 0xff) != 0)
	{
		if (len > 1)
		{
			val = read_reg(sc, 0x104);
			for (i = 1; i < len; i++)
			{
				buf[i] = val & 0xff;
				val = val >> 8;
			}
		}
	}
	return res;
}

/*----------------------------------------------------------------*/
static u_int32_t
FlexI2cWrite4(struct adapter *sc, u_int32_t device, u_int32_t chip_addr, u_int16_t addr, u_int8_t *buf, u_int8_t len)
{
	u_int32_t cmd,val;
	int res, i;

	if (len > 1)
	{
		val = 0;
		for (i = len; i > 1; i--)
		{
			val = val << 8;
			val = val | buf[i-1];
		}
		write_reg(sc, 0x104, val);
	}

	cmd = i2cMainSetup(device, chip_addr, 0, addr, buf[0], len);
	res = i2cMainIO(sc, cmd, 0 , 100000);
	
	return res;
}


/*----------------------------------------------------------------*/
static u_int32_t
fixChipAddr(u_int32_t device, u_int32_t bus, u_int32_t addr)
{
	if (device == 0x20000000) 
		return bus | ((addr >> 8) & 3);
	return bus;
}


/*----------------------------------------------------------------*/
u_int32_t
FLEXI2C_read(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t ChipAddr;
	u_int32_t bytes2transfer;
	u_int8_t *start;

	start = buf;
	while (len != 0)
	{
		bytes2transfer = len;

		if (bytes2transfer > 4)
			bytes2transfer = 4;

		ChipAddr = fixChipAddr(device, bus, addr);

		if (FlexI2cRead4(sc, device, ChipAddr, addr, buf, bytes2transfer) == 0)
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
	u_int32_t ChipAddr;
	u_int32_t bytes2transfer;
	u_int8_t *start;

	start = buf;
	while (len != 0)
	{
		bytes2transfer = len;

		if (bytes2transfer > 4)
			bytes2transfer = 4;

		ChipAddr = fixChipAddr(device, bus, addr);

		if (FlexI2cWrite4(sc, device, ChipAddr, addr, buf, bytes2transfer) == 0)
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

