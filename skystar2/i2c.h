/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _i2c_h_
#define _i2c_h_

#include <sys/_null.h>
#include <sys/types.h>
#include <sys/errno.h>

#include "skystar2.h"

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
FLEXI2C_read(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len);

extern u_int32_t
FLEXI2C_write(struct adapter *sc, u_int32_t device, u_int32_t bus, u_int32_t addr, u_int8_t *buf, u_int32_t len);

extern int
master_xfer(struct adapter *sc, const struct i2c_msg *msgs, int num);

#endif

