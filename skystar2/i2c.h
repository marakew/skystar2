/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _i2c_h_
#define _i2c_h_

#include <sys/_null.h>
#include <sys/types.h>
#include <sys/errno.h>

#include "skystar2.h"

struct _I2CBUS
{
	unsigned int	device;	//device	unk0
	unsigned int	unk4;	//bus
	unsigned int	retr;	//retr	unk8

	unsigned char	unkC;	
	unsigned char	unkD;	//state
	unsigned char	unkE;
	unsigned char	unkF;		//base_addr

	unsigned int	eeprom;	//eeprom	//unk10
	unsigned int	unk14;		//chip_addr
	unsigned int	unk18;	//sm
	unsigned int	wait;	//delay	//ukn1C
};

struct i2c_msg {
        u_int16_t       addr;
        u_int16_t       flags;
#define I2C_M_TEN               0x10
#define I2C_M_RD                0x01
#define I2C_M_NONSTART          0x4000
#define I2C_M_REV_DIR_ADDR      0x2000
        u_int32_t       len;
        u_int8_t        *buf;
};

extern u_int32_t
FLEXI2C_busRead(struct adapter *sc, struct _I2CBUS *bus, u_int32_t addr, u_int8_t *buf, u_int32_t len);

extern u_int32_t
FLEXI2C_busWrite(struct adapter *sc, struct _I2CBUS *bus, u_int32_t addr, u_int8_t *buf, u_int32_t len);

extern u_int32_t
FLEXI2C_read(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len);

extern u_int32_t
FLEXI2C_write(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len);

extern int
master_xfer(struct adapter *sc, const struct i2c_msg *msgs, int num);

#endif

