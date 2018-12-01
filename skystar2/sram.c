/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include "sram.h"
#include "sllutil.h"

/*=================================================================
= SRAM memory is accessed through a buffer register in the FlexCop
= chip (0x700). This register has the following structure:
=  bits 0-14  : address
=  bit  15    : read/write flag
=  bits 16-23 : 8-bit word to write
=  bits 24-27 : = 4
=  bits 28-29 : memory bank selector
=  bit  31    : busy flag
==================================================================*/

/*----------------------------------------------------------------*/
static void
FlexSramWrite(struct adapter *sc, u_int32_t bank, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t	i, cmd, retr;

	for (i = 0; i < len; i++)
	{
		cmd = bank | addr | 0x04000000 | ( *buf << 0x10 );
		retr = 2;
		while ( ((read_reg(sc, 0x700) & 0x80000000) != 0) && 
			(retr > 0)){
			DELAY(1000);
			retr --;
		}
		if (retr == 0)
			printf("SRAM timeout\n");

		write_reg(sc, 0x700, cmd);
		buf++;
		addr++;
	}
}

/*----------------------------------------------------------------*/
static void
FlexSramRead(struct adapter *sc, u_int32_t bank, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t	i, cmd, val, retr;

	for (i = 0; i < len; i++)
	{
		cmd = bank | addr | 0x04008000;
		retr = 10000;
		while ( ((read_reg(sc, 0x700) & 0x80000000) != 0) && 
			(retr > 0)){
			DELAY(1000);
			retr --;
		}
		if (retr == 0)
			printf("SRAM timeout\n");

		write_reg(sc, 0x700, cmd);

		retr = 10000;
		while ( ((read_reg(sc, 0x700) & 0x80000000) != 0) && 
			(retr > 0)){
			DELAY(1000);
			retr --;
		}
		if (retr == 0)
			printf("SRAM timeout\n");

		val = read_reg(sc, 0x700) >> 0x10;
		*buf = (val & 0xff);
		buf++;
		addr++;
	}
}

/*----------------------------------------------------------------*/
static void
SRAM_writeChunk(struct adapter *sc, u_int32_t addr, u_int8_t *buf, u_int16_t len)
{
	u_int32_t bank;

	bank = 0;

	if (sc->dwSramType == 0x20000 || sc->dwSramType == 0x30000)
	{
		bank = (addr & 0x18000) << 0x0D;
	} else
	if (sc->dwSramType == 0x00000)
	{
		if ((addr >> 0x0F) == 0)
			bank = 0x20000000;
		else
			bank = 0x10000000;
	}

	FlexSramWrite(sc, bank, addr & 0x7FFF, buf, len);
}

/*----------------------------------------------------------------*/
static void
SRAM_readChunk(struct adapter *sc, u_int32_t addr, u_int8_t *buf, u_int16_t len)
{
	u_int32_t bank;

	bank = 0;
	if (sc->dwSramType == 0x20000 || sc->dwSramType == 0x30000)
	{
		bank = (addr & 0x18000) << 0x0D;
	} else
	if (sc->dwSramType == 0x00000)
	{
		if ((addr >> 0x0F) == 0)
			bank = 0x20000000;
		else
			bank = 0x10000000;
	}

	FlexSramRead(sc, bank, addr & 0x7FFF, buf, len);
}

/*----------------------------------------------------------------*/
static void
SRAM_read(struct adapter *sc, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t leng;

	while (len != 0)
	{
		leng = len;
		/* check if the address range belongs to the same
		 * 32K memory chip. If not, the data is read from
		 * one chip at a time.
		 */
		if ((addr >> 0x0F) != ((addr+len-1) >> 0x0F))
		{
			leng = (((addr >> 0x0F) + 1) << 0x0F) - addr;
		}

		SRAM_readChunk(sc, addr, buf, leng);
		len -= leng;
		addr += leng;
		buf += leng;
	}
}

/*----------------------------------------------------------------*/
static void
SRAM_write(struct adapter *sc, u_int32_t addr, u_int8_t *buf, u_int32_t len)
{
	u_int32_t leng;

	while (len != 0)
	{
		leng = len;
		/* check if the address range belongs to the same
		 * 32K memory chip. If not, the data is read from
		 * one chip at a time.
		 */
		if ((addr >> 0x0F) != ((addr+len-1) >> 0x0F))
		{
			leng = (((addr >> 0x0F) + 1) << 0x0F) - addr;
		}

		SRAM_writeChunk(sc, addr, buf, leng);
		len -= leng;
		addr += leng;
		buf += leng;
	}
}



/*----------------------------------------------------------------*/
static void
SRAM_setSize(struct adapter *sc, u_int32_t mask)
{
	write_reg(sc, 0x71C, (mask | (~0x30000 & read_reg(sc, 0x71C))));
}

/*----------------------------------------------------------------*/
static u_int32_t
SRAM_init(struct adapter *sc)
{
	u_int32_t tmp;

	tmp = read_reg(sc, 0x71C);
	write_reg(sc, 0x71C, 1);

	if (read_reg(sc, 0x71C) != 0)
	{
		write_reg(sc, 0x71C, tmp);
		sc->dwSramType = tmp & 0x30000;
	} else
	{
		sc->dwSramType = 0x10000;
	}	

	printf("dwSramType=%x\n" , sc->dwSramType);

	return sc->dwSramType;
}

/*----------------------------------------------------------------*/
static int
SRAM_testLocation(struct adapter *sc, u_int32_t mask, u_int32_t addr)
{
	u_int8_t tmp1, tmp2;

	SRAM_setSize(sc, mask);
	SRAM_init(sc);

	tmp2 = 0xa5;
	tmp1 = 0x4f;

	SRAM_write(sc, addr,	 &tmp2, 1);
	SRAM_write(sc, addr + 4, &tmp1, 1);
	
	tmp2 = 0;
	DELAY(20000);
	
	SRAM_read(sc, addr, &tmp2, 1);
	SRAM_read(sc, addr, &tmp2, 1);
	
	if (tmp2 != 0xa5)
		return 0;

	tmp2 = 0x5a;
	tmp1 = 0xf4;

	SRAM_write(sc, addr,	 &tmp2, 1);
	SRAM_write(sc, addr + 4, &tmp1, 1);
	
	tmp2 = 0;
	DELAY(20000);
	
	SRAM_read(sc, addr, &tmp2, 1);
	SRAM_read(sc, addr, &tmp2, 1);
	
	if (tmp2 != 0x5a)
		return 0;

	return 1;
}


/*=================================================================
  = FlexcopII can work with 32K, 64K or 128K of external SRAM memory.
  =  - for 128K there are 4x32K chips at bank 0,1,2,3.
  =  - for  64K there are 2x32K chips at bank 1,2.
  =  - for  32K there is one 32K chip at bank 0.
  =
  = FlexCop works only with one bank at a time. The bank is selected
  = by bits 28-29 of the 0x700 register.
  = 
  = bank 0 covers addresses 0x00000-0x07FFF
  = bank 1 covers addresses 0x08000-0x0FFFF
  = bank 2 covers addresses 0x10000-0x17FFF
  = bank 3 covers addresses 0x18000-0x1FFFF

  = FlexcopIII have 48K of external SRAM memory.
==================================================================*/

/*----------------------------------------------------------------*/
u_int32_t
SRAM_length(struct adapter *sc)
{
        if (sc->dwSramType == 0x10000)
                return 32768;

	if (sc->dwSramType == 0x30000)
		return 49152;

        if (sc->dwSramType == 0x00000)
                return 65536;

        if (sc->dwSramType == 0x20000)
                return 131072;

        return 32768;
}

/*----------------------------------------------------------------*/
int
SramDetectForFlex2(struct adapter *sc)
{
	u_int32_t tmp, tmp2, tmp3;

	tmp = read_reg(sc, 0x208);
	write_reg(sc, 0x208, 0);

	tmp2 = read_reg(sc, 0x71C);

	write_reg(sc, 0x71C, 1);

	tmp3 = read_reg(sc, 0x71C);

	write_reg(sc, 0x71C, tmp2);

	if (tmp3 != 1)
	{
		SRAM_setSize(sc, 0x10000);
		SRAM_init(sc);
		write_reg(sc, 0x208, tmp);
		return 32;
	}	

	if (SRAM_testLocation(sc, 0x20000, 0x18000) != 0)
	{
		SRAM_setSize(sc, 0x20000);
		SRAM_init(sc);
		write_reg(sc, 0x208, tmp);
		return 128;
	}		
        
	if (SRAM_testLocation(sc, 0x00000, 0x10000) != 0)
	{
		SRAM_setSize(sc, 0x00000);
		SRAM_init(sc);
		write_reg(sc, 0x208, tmp);
		return 64;
	}		

	if (SRAM_testLocation(sc, 0x10000, 0x00000) != 0)
	{
		SRAM_setSize(sc, 0x10000);
		SRAM_init(sc);
		write_reg(sc, 0x208, tmp);
		return 32;
	}		

	SRAM_setSize(sc, 0x10000);
	SRAM_init(sc);
	write_reg(sc, 0x208, tmp);

	return 0;
}


/*----------------------------------------------------------------*/
int
SramDetectForFlex3(struct adapter *sc)
{
	SRAM_setSize(sc, 0x30000);
	SRAM_init(sc);
	return 48;
}
	
int
SLL_detectSramSize(struct adapter *sc)
{

//	if (SllGetFlexRevision(sc, NULL, NULL) == 192)
//		return SramDetectForFlex3(sc);
//	else
		return SramDetectForFlex2(sc);
}
