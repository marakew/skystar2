/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _sram_h_
#define _sram_h_

#include <sys/types.h>
#include "skystar2.h"

int
SLL_detectSramSize(struct adapter *sc);

#endif
