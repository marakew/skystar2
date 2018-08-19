/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _eeprom_h_
#define _eeprom_h_

#include <sys/types.h>

#include "i2c.h"
#include "skystar2.h"

/*
 * EEPROM (Skystar2 has one "24LC08B" chip on board)
 */

extern int
EEPROM_getMacAddr(struct adapter *sc, char type, u_int8_t *mac);

extern int
EEPROM_setMacAddr(struct adapter *sc, char type, u_int8_t *mac);

extern int
EEPROM_writeTunerInfo(struct adapter *sc, unsigned char *info);

extern int
EEPROM_readTunerInfo(struct adapter *sc, unsigned char *info);

#endif
