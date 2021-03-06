/* cobra_reg.c */ 
 
#include "cobra.h"                     /* Cobra include files, ordered */ 
#include "cobra_regs.h"                /* Cobra Internal */ 
 
#if INCLUDE_DEBUG 
//static
BOOL  RegisterVerifyRegRW(NIM *nim,int); 
//static
BOOL  RegisterVerifyRegFilter(NIM *nim,int); 
//static
BOOL  RegisterVerifyRegDataType(NIM *nim,int); 
//static
BOOL  RegisterVerifyRegDefault(NIM *nim,int); 
//static
BOOL  RegisterVerifyRegBitCount(NIM *nim,int); 
//static
BOOL  RegisterOneOf(char,char*,int); 
#endif /* INCLUDE_DEBUG */ 
 
 
#ifndef TONE22K_AMP 
#define TONE22K_AMP 3 
#endif 
 
/* performs Cobra register I/O using the cobra_reg.h REGISTER structure to determine        */ 
/* how register data is read and written to the Demod.                                      */ 
 
/******************************************************************************************************* 
 *                 R E G I S T E R    I N T E R F A C E    F U N C T I O N S 
 *******************************************************************************************************/ 
/* ===                                Register Functions                                            ===*/ 
/******************************************************************************************************* 
 * Name: RegisterWrite 
 * 
 * Description: 
 * 	This function writes to a bit-field or a byte in the special control area. This function is NOT  
 *     applicable for Page0 address space since it is read-only. 
 * Return Value: 
 * 	TRUE - operation successful; FALSE - otherwise. 
 * 
 * I/O	Parameters	                         Descriptions 
 * IN	NIM*               p_nim	     Pointer to NIM structure allocated by application 
 * IN	REGISTER_BIT_FIELD reg_field_index	 Index that uniquely identifies the value within a register  
 *                                           (or spanning multiple 8-bit registers while reading from Page0).  
 * IN	unsigned long      value	         Value to be written. 
 *******************************************************************************************************/ 
BOOL  
RegisterWrite(NIM*       p_nim, 
	unsigned short   reg_field_index,  
	unsigned long    value, 
	    IO_METHOD  io_method) 
{ 
	int            bytes      = 0; 
	unsigned char  offset     = 0; 
	unsigned char  index      = 0; 
	unsigned char  data_write = 0x00U; 
	unsigned char  data_read  = 0x00U; 
	unsigned char  sub_address = 0; 
 
	unsigned long              handle; 
	const REGISTER_MAP*  p_register_map; 
 
    /* Validation */ 
    if (p_nim == 0) /* bad pointer */ 
    { 
        return (False); 
    } 
 
	/* Initialization */ 
	switch (io_method) 
	{ 
		case DEMOD_I2C_IO: 
			if (reg_field_index == CX24123_TUNI2CRPTSTART)
			{
				if ( (p_nim->demod_handle >> 16) & 0xff != 0)
					handle = (p_nim->demod_handle >> 16) & 0xff;
				else
					handle = p_nim->demod_handle;
			} else
			{
				handle = p_nim->demod_handle;
			}
			p_register_map = demod_register_map; 
			break; 
 
#if INCLUDE_VIPER 
		case VIPER_I2C_IO: 
			handle = p_nim->tuner.cx24128.tuner_handle; 
			p_register_map = viper_register_map; 
			break; 
#endif /* INCLUDE_VIPER */ 

#if INCLUDE_VIPER_BTI
		case VIPER_BTI_IO: 
			handle = p_nim->demod_handle; 
			p_register_map = viper_bti_register_map; 
			break; 
#endif /* INCLUDE_VIPER_BTI */ 

		default: 
			return (False); 
	} 
     
    /* Verify that index passed points to the correct register record */ 
    if (p_register_map[reg_field_index].bit_field != reg_field_index) 
    { 
        DRIVER_SET_ERROR (p_nim, API_REG_MATCH_IDX); 
        return (False); 
    } 
     
    /* Register read-only */ 
    if (p_register_map[reg_field_index].access_level == REG_RO || 
        p_register_map[reg_field_index].access_level == REG_UNUSED) 
    { 
        DRIVER_SET_ERROR(p_nim, API_REG_HDWR_REGRDO); 
        return (False); 
    } 
     
	sub_address = (unsigned char)(p_register_map[reg_field_index].address); 

#if INCLUDE_VIPER_BTI
	if (io_method == VIPER_BTI_IO) 
	{ 
		sub_address = (unsigned char)(sub_address + p_nim->tuner.cx24128.register_offset); 
	} else
#endif /* INCLUDE_VIPER_BTI */ 
 
	if (io_method == DEMOD_I2C_IO) 
	{ 
		sub_address = (unsigned char)(sub_address + p_nim->register_offset); 
	} 
#if INCLUDE_VIPER 
	else if (io_method == VIPER_I2C_IO) 
	{ 
		sub_address = (unsigned char)(sub_address + p_nim->tuner.cx24128.register_offset); 
	} 
#endif /* INCLUDE_VIPER */ 
 
    /* de-translate from variable to hardware bit positioned value and write to hardware */ 
    switch (p_register_map[reg_field_index].reg_type) 
    { 
        case REGT_BIT:    /* 1 to 7 bit(s) wide */ 
	case REGT_NULL: 
        case REGT_BYTE:   /* 1 byte wide */ 
	case REGT_BITHL:  /* 1 bit wide. Toggle */ 
        { 
		unsigned char startbit = p_register_map[reg_field_index].start_bit; 
 
            if (p_register_map[reg_field_index].bit_count == 0x01) /* single bit */ 
            { 
                if (value) 
                { 
                    data_write = (unsigned char)(p_register_map[reg_field_index].p_hw_mask[0]); 
                } 
                else 
                { 
                    data_write = 0x00; 
                } 
            } 
            else /* Minimum 2 bits, max 8 bits */ 
            {                 
                unsigned long temp; 
 
#if INCLUDE_VIPER_BTI
		if (io_method == VIPER_BTI_IO) 
		{ 
			//??...
		} 
#endif /* INCLUDE_VIPER_BTI */ 

                /* mask useful bits from raw, shift left (make range 0..2^bit_count) */ 
                temp = value & ((0x01UL<<(p_register_map[reg_field_index].bit_count))-1UL); 
                temp = (temp << (startbit-(p_register_map[reg_field_index].bit_count-1))); 
                data_write = (unsigned char)temp; 
            } 
             
            /* clear io error return value */ 
            p_nim->iostatus = 0UL; 
 
#if INCLUDE_VIPER 
		if (io_method == VIPER_I2C_IO) 
		{ 
			/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
			if (TUNER_CX24128_RepeaterModeReadData(p_nim, handle, sub_address, &data_read) == False) 
			{ 
				return (False); 
			} 
		} 
#endif /* INCLUDE_VIPER */ 
 
#if INCLUDE_VIPER_BTI
		if (io_method == VIPER_BTI_IO) 
		{ 
			/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
			if (TUNER_CX24128_BTIRepeaterModeReadData(p_nim, handle, sub_address, &data_read) == False) 
			{ 
				return (False); 
			} 
			//??...
		} 
#endif /* INCLUDE_VIPER_BTI */ 
 
		if (io_method == DEMOD_I2C_IO) 
		{ 
			/* Byte must be read, modified, written */ 
			data_read = (unsigned char)((*p_nim->SBRead)(p_nim->ptuner, handle, sub_address, &p_nim->iostatus));  
			/* Watch for special-case error, read error during write */ 
			if (p_nim->iostatus != 0UL) 
			{ 
				/* Hardware write error */ 
				DRIVER_SET_ERROR(p_nim, API_REG_HDWR_REWTERR); 
				return (False); 
			} 
		} 
 
            /* mask data that must be retained */ 
            data_read = (unsigned char)(data_read & (unsigned char)(~p_register_map[reg_field_index].p_hw_mask[0])); 
            data_read |= data_write; 
 
            if ((p_register_map[reg_field_index].regfilter & REGF_ZEROB6) != 0) /* zero bit 6 */ 
            { 
                data_read &= (unsigned char)0xBF; 
            } 
 
            /* clear io error return value */ 
            p_nim->iostatus = 0UL; 
 
            /* write byte to hardware */ 
#if INCLUDE_VIPER 
		if (io_method == VIPER_I2C_IO) 
		{ 
			/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
			if (TUNER_CX24128_RepeaterModeWriteData(p_nim, handle, sub_address, data_read) == False) 
			{ 
				return (False); 
			} 
			/* add toggle code here when regmap of that type defined for viper */ 
		} 
#endif /* INCLUDE_VIPER */ 

#if INCLUDE_VIPER_BTI
		if (io_method == VIPER_BTI_IO) 
		{ 
			//??..
			/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
			if (TUNER_CX24128_BTIRepeaterModeWriteData(p_nim, handle, sub_address, data_read) == False) 
			{ 
				return (False); 
			} 
			/* add toggle code here when regmap of that type defined for viper */ 
		} 
#endif /* INCLUDE_VIPER */ 
 
		if (io_method == DEMOD_I2C_IO) 
		{ 
			(*p_nim->SBWrite)(p_nim->ptuner, handle, sub_address, data_read, &p_nim->iostatus);  
			if (p_register_map[reg_field_index].reg_type == REGT_BITHL) /* toggle */ 
			{ 
				(*p_nim->SBWrite)(p_nim->ptuner, handle, sub_address, 0x00, &p_nim->iostatus);  
			} 
		} 
        } 
        break; 
 
	case REGT_MINT: /* > 1 to 4 byte(s) wide */ 
		{ 
			unsigned char startbit = p_register_map[reg_field_index].start_bit; 
			signed char bitcnt = (signed char)(p_register_map[reg_field_index].bit_count); 
			unsigned char mask; 
 
#if INCLUDE_VIPER_BTI
			if (io_method == VIPER_BTI_IO) 
			{ 
				//??...
			}
#endif /* INCLUDE_VIPER_BTI */ 

			if (startbit > 7) 
			{ 
				startbit -= 8; 
			} 
 
			/* Read write the multiple bit (> 8 bits) value */ 
			while (bytes < p_register_map[reg_field_index].bit_count) 
			{ 
#if INCLUDE_VIPER 
				if (io_method == VIPER_I2C_IO) 
				{ 
					/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
					if (TUNER_CX24128_RepeaterModeReadData(p_nim, handle, (unsigned char)(sub_address+offset), &data_read) == False) 
					{ 
						return (False); 
					} 
				} 
#endif /* INCLUDE_VIPER */ 

#if INCLUDE_VIPER_BTI
				if (io_method == VIPER_BTI_IO) 
				{ 
					/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
					if (TUNER_CX24128_BTIRepeaterModeReadData(p_nim, handle, (unsigned char)(sub_address+offset), &data_read) == False) 
					{ 
						return (False); 
					} 
				} 
#endif /* INCLUDE_VIPER_BTI */ 

				if (io_method == DEMOD_I2C_IO) 
				{ 
					data_read = (unsigned char)((*p_nim->SBRead)(p_nim->ptuner, handle, (unsigned char)(sub_address+offset), &p_nim->iostatus));  
 
					/* Watch for special-case error, read error during write */ 
					if (p_nim->iostatus != 0UL) 
					{ 
						/* Hardware write error */ 
						DRIVER_SET_ERROR(p_nim, API_REG_HDWR_REWTERR); 
						return (False); 
					} 
				} 
 
				/* mask data that must be retained */ 
				data_read = (unsigned char)(data_read & (unsigned char)(~p_register_map[reg_field_index].p_hw_mask[index])); 
				if ((long)value < 0)  
				{ 
					/* Take the 2's complement. */ 
					value = value + (1UL << bitcnt); 
				} 
 
				bitcnt = (signed char)(bitcnt - (startbit + 1)); 
 
				if (bitcnt > 0) 
				{ 
					data_write = (unsigned char)(value >> bitcnt); 
					mask = (unsigned char)(p_register_map[reg_field_index].p_hw_mask[index]); 
					data_write &= p_register_map[reg_field_index].p_hw_mask[index]; 
				} 
				else 
				{ 
					mask = (unsigned char)(p_register_map[reg_field_index].p_hw_mask[index]); 
					data_write = (unsigned char)((value << (0 - bitcnt)) & p_register_map[reg_field_index].p_hw_mask[index]); 
				} 
				startbit = 7; 
				data_read |= data_write; 
 
#if INCLUDE_VIPER 
				if (io_method == VIPER_I2C_IO) 
				{ 
					/* write back the modified value. */ 
					/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
					if (TUNER_CX24128_RepeaterModeWriteData(p_nim, handle, (unsigned char)(sub_address+offset), data_read) == False) 
					{ 
						return (False); 
					} 
				} 
#endif /* INCLUDE_VIPER */ 
 
#if INCLUDE_VIPER_BTI
				if (io_method == VIPER_BTI_IO) 
				{ 
					/* write back the modified value. */ 
					/* Set special control register to repeater mode. It will go off after one serial bus transaction.*/ 
					if (TUNER_CX24128_BTIRepeaterModeWriteData(p_nim, handle, (unsigned char)(sub_address+offset), data_read) == False) 
					{ 
						return (False); 
					} 
				} 
#endif /* INCLUDE_VIPER_BTI */ 

				if (io_method == DEMOD_I2C_IO) 
				{ 
					(*p_nim->SBWrite)(p_nim->ptuner, handle, (unsigned char)(sub_address+offset), data_read, &p_nim->iostatus);  
					 
					/* Watch for special-case error, read error during write */ 
					if (p_nim->iostatus != 0UL) 
					{ 
						/* Hardware write error */ 
						DRIVER_SET_ERROR(p_nim, API_REG_HDWR_REWTERR); 
						return (False); 
					} 
				} 
 
				offset++; 
				index++; 
				if (index == MAX_REGISTER_LEN) 
				{ 
					break; 
				} 
				bytes += 8; 
			} 
		} 
		break; 
 
        case REGT_EOLIST: 
        default: 
        break; 
    } 
    /* SB Write error */ 
    if (p_nim->iostatus != 0UL) 
    { 
      /* write error writing to hardware */ 
      DRIVER_SET_ERROR(p_nim, API_IO_WRITERR); 
      return (False); 
    } 
 
    return (True); 
} /* RegisterWrite */ 
 
/******************************************************************************************************* 
 * Name: RegisterRead 
 * Description: 
 * 	This function reads from a bit-field or a byte in the special control area. This function is NOT  
 *   applicable for Page0 address space since it is read-only. 
 * Return Value: 
 * 	TRUE - operation successful; FALSE - otherwise. 
 * 
 * I/O	Parameters	                        Descriptions 
 * IN	NIM*               p_nim	        Pointer to NIM structure allocated by application 
 * IN	unsigned short     reg_field_index	Index that uniquely identifies the value within a register  
 *                                          or one that spans multiple 8-bit registers beginning form  
 *                                          the address that corresponds to this reg_field_index.  
 * IN	unsigned long*     p_value	        Value read from the Page0 or Special control address. 
 * IN   IO_METHOD          io_method		Use specified io method for serial read/write. 
 *    
 *******************************************************************************************************/ 
BOOL  
RegisterRead(NIM*         p_nim, 
	unsigned short     reg_field_index,  
	unsigned long*     p_value, 
	     IO_METHOD    io_method) 
{ 
	register int   bytes  = 0;    /* counts 8-bits read */ 
	unsigned char  offset = 0;   
	unsigned char  index  = 0; 
	unsigned char  reg_buffer[MAX_REGISTER_LEN]; 
	unsigned char  sub_address = 0; 
	unsigned char  bits_per_read; 
	unsigned char  bytes_per_read; 
 
	unsigned long  handle; 
	const REGISTER_MAP*  p_register_map; 
	unsigned char startbit = 0;  
 
    /* Validation */ 
    if (p_nim == 0) /* bad pointer */ 
    { 
        return (False); 
    } 
 
    if (p_value == 0) 
    { 
        return (False); 
    } 
    *p_value = 0; 
 
	/* Initialization */ 
	switch (io_method) 
	{ 
		case DEMOD_I2C_IO: 
			handle = p_nim->demod_handle; 
			p_register_map = demod_register_map; 
			bits_per_read = 8; 
			bytes_per_read = 1; 
			break; 
 
#if INCLUDE_VIPER 
		case VIPER_I2C_IO: 
			handle = p_nim->tuner.cx24128.tuner_handle; 
			p_register_map = viper_register_map; 
			bits_per_read = 8; 
			bytes_per_read = 1; 
			break; 
#endif /* INCLUDE_VIPER */ 
#if INCLUDE_VIPER_BTI
		case VIPER_BTI_IO: 
			handle = p_nim->demod_handle; 
			p_register_map = viper_bti_register_map; 
			bits_per_read = 16; 
			bytes_per_read = 2; 
			//??...
			break; 
#endif /* INCLUDE_VIPER_BTI */ 

		default: 
			return (False); 
	}	 
 
    /* Verify that index passed points to the correct register record */ 
    if (p_register_map[reg_field_index].bit_field != reg_field_index) 
    { 
        DRIVER_SET_ERROR (p_nim, API_REG_MATCH_IDX); 
        return (False); 
    } 
 
    /* Register write-only */ 
    if (p_register_map[reg_field_index].access_level == REG_WO || 
        p_register_map[reg_field_index].access_level == REG_UNUSED) 
    { 
        DRIVER_SET_ERROR(p_nim, API_REG_HDWR_REGWTO); 
        return (False); 
    } 
 
    /* Read the register(s) */ 
    p_nim->iostatus = 0UL; /* clear io error return value */ 
 
	sub_address = (unsigned char)(p_register_map[reg_field_index].address); 

#if INCLUDE_VIPER_BTI
	if (io_method == VIPER_BTI_IO) 
	{ 
		sub_address = (unsigned char)(sub_address + p_nim->tuner.cx24128.register_offset); 
	} else
#endif /* INCLUDE_VIPER_BTI */ 
 
	if (io_method == DEMOD_I2C_IO) 
	{ 
		sub_address = (unsigned char)(sub_address + p_nim->register_offset); 
	} 
#if INCLUDE_VIPER 
	else if (io_method == VIPER_I2C_IO) 
	{ 
		sub_address = (unsigned char)(sub_address + p_nim->tuner.cx24128.register_offset); 
	} 
#endif /* INCLUDE_VIPER */ 
 
	startbit = p_register_map[reg_field_index].start_bit; 
 
    /* Read the multiple bit (> bits_per_read bits) value */ 
    while (bytes < p_register_map[reg_field_index].bit_count && p_nim->iostatus != 1UL) 
    { 
#if INCLUDE_VIPER 
		if (io_method == VIPER_I2C_IO) 
		{ 
			/* Enable Repeater mode again */ 
			if (TUNER_CX24128_RepeaterModeReadData(p_nim, handle, (unsigned char)(sub_address+offset), &reg_buffer[index]) == False) 
			{ 
				return (False); 
			} 
		} 
#endif /* INCLUDE_VIPER */ 
#if INCLUDE_VIPER_BTI
		if (io_method == VIPER_BTI_IO) 
		{ 
			/* Enable Repeater mode again */ 
			if (TUNER_CX24128_BTIRepeaterModeReadData(p_nim, handle, (unsigned char)(sub_address+offset), &reg_buffer[index]) == False) 
			{ 
				return (False); 
			} 
		} 
#endif /* INCLUDE_VIPER_BTI */ 
 
		if (io_method == DEMOD_I2C_IO) 
		{ 
			reg_buffer[index] = (unsigned char)((*p_nim->SBRead)(p_nim->ptuner, handle, (unsigned char)(sub_address+offset), &p_nim->iostatus));  
 
			if (p_nim->iostatus != 0UL) 
			{ 
				/* hardware read error */ 
				DRIVER_SET_ERROR(p_nim, API_IO_READERR); 
				return (False); 
			} 
		} 
 
        offset++; 
	index = (unsigned char)(index + bytes_per_read); 
        if (index == MAX_REGISTER_LEN) 
        { 
            break; 
        } 
        bytes += bits_per_read; 

#if INCLUDE_VIPER_BTI
		if (io_method == VIPER_BTI_IO) 
		{ 
			//??? ...
		} 
#endif /* INCLUDE_VIPER_BTI */ 

    } 
     
    /* Translate: raw data read from the hardware registers to data used by the driver*/   
    switch(p_register_map[reg_field_index].reg_type) 
    { 
	case REGT_BITHL: /* Latch -> this bit is latched hi, then immediately low */ 
		{ 
			*p_value = (unsigned char)p_register_map[reg_field_index].p_hw_mask[0]; 
			break; 
		} 
        case REGT_BIT: 
	case REGT_NULL: 
        case REGT_BYTE: 
		{ 
#if INCLUDE_VIPER_BTI
			if (io_method == VIPER_BTI_IO) 
			{ 
				//??? ...
			} 
#endif /* INCLUDE_VIPER_BTI */ 

			if (p_register_map[reg_field_index].bit_count == 0x01) /* single bit */ 
			{ 
				/* Mask the bit to 1 or 0 */ 
				if ((reg_buffer[0] & p_register_map[reg_field_index].p_hw_mask[0]) != 0)             
				{ 
					*p_value = 0x01; 
				} 
				else 
				{ 
					*p_value = 0x00; 
				} 
				break; 
			} 
			else 
			{ 
#if INCLUDE_VIPER_BTI
				if (io_method == VIPER_BTI_IO) 
				{ 
					//??? ...
				} 
#endif /* INCLUDE_VIPER_BTI */ 

				/* Min 2 bits, max 8 bits */ 
				unsigned char temp; 
 
				/* mask useful bits from raw, shift rt (make range 0..2^bit_count) */ 
				temp = (unsigned char)(reg_buffer[0] & (unsigned char)p_register_map[reg_field_index].p_hw_mask[0]);             
				temp = (unsigned char)(temp >> (startbit-(p_register_map[reg_field_index].bit_count-1))); 
				*p_value = (unsigned char)temp; 
				break; 
			} 
		} 
        case  REGT_MINT: 
        { 
            unsigned short byte_count; 
            unsigned char  bit_value;  
            unsigned char  byte_index;  
            unsigned char  bit_mask; 
 
		/* count the number of hardware reg bytes that comprise the register in question */ 
            for (byte_count = 0; byte_count < MAX_REGISTER_LEN; byte_count++)   
            { 
                if (p_register_map[reg_field_index].p_hw_mask[byte_count] == 0x00)
                { 
                    break; 
                } 
            } 
 
            /* Convert multiple byte value from spread bits to compressed */ 
            for (byte_index = 0; byte_index < byte_count; byte_index++) 
            { 
                for (bit_mask = 0x80; bit_mask != 0; bit_mask >>= 1) 
                { 
                    if ((p_register_map[reg_field_index].p_hw_mask[byte_index]  & bit_mask) != 0) 
                    { 
                        bit_value = (unsigned char)((bit_mask & reg_buffer[byte_index]) != 0 ? 1UL : 0UL); 
                        *p_value  = ((*p_value << 1) | bit_value); 
                    } 
                } 
            } 
            break; 
        } 
 
        default: 
			DRIVER_SET_ERROR(p_nim, API_REG_VERFY_REGDTP); 
			return(False); 
    }  /* switch(... */ 
 
    return (True); 
} /* RegisterRead */ 
#if INCLUDE_RATTLER 
/******************************************************************************************************* 
 *                 R E G I S T E R    I N T E R F A C E    F U N C T I O N S 
 *******************************************************************************************************/ 
/* ===                                Register Functions                                            ===*/ 
/******************************************************************************************************* 
 * Name: TunerRegisterWrite 
 * 
 * Description: 
 * 	This function writes to a bit-field or a byte in the special control area. This function is NOT  
 *     applicable for Page0 address space since it is read-only. 
 * Return Value: 
 * 	TRUE - operation successful; FALSE - otherwise. 
 * 
 * I/O	Parameters	                         Descriptions 
 * IN	NIM*               p_nim	     Pointer to NIM structure allocated by application 
 * IN	REGISTER_BIT_FIELD reg_field_index	 Index that uniquely identifies the value within a register  
 *                                           (or spanning multiple 8-bit registers while reading from Page0).  
 * IN	unsigned long      value	         Value to be written. 
 *******************************************************************************************************/ 
BOOL  
TunerRegisterWrite(NIM*       p_nim, 
	unsigned short   reg_field_index,  
	unsigned long    value, 
	IO_METHOD  io_method) 
{ 
    if(TUNER_CX24128_RepeaterModeWriteData(p_nim, p_nim->tuner.cx24128.tuner_handle, reg_field_index, value) == False) 
     { 
        return(False); 
     } 
     return (True); 
} 
/******************************************************************************************************* 
 * Name: TunerRegisterRead 
 * Description: 
 * 	This function reads from a bit-field or a byte in the special control area. This function is NOT  
 *   applicable for Page0 address space since it is read-only. 
 * Return Value: 
 * 	TRUE - operation successful; FALSE - otherwise. 
 * 
 * I/O	Parameters	                        Descriptions 
 * IN	NIM*               p_nim	        Pointer to NIM structure allocated by application 
 * IN	unsigned short     reg_field_index	Index that uniquely identifies the value within a register  
 *                                          or one that spans multiple 8-bit registers beginning form  
 *                                          the address that corresponds to this reg_field_index.  
 * IN	unsigned long*     p_value	        Value read from the Page0 or Special control address. 
 * IN   IO_METHOD          io_method		Use specified io method for serial read/write. 
 *    
 *******************************************************************************************************/ 
BOOL  
TunerRegisterRead(NIM*         p_nim, 
	unsigned short     reg_field_index,  
	unsigned long*     p_value, 
	IO_METHOD    io_method) 
{ 
    if(TUNER_CX24128_RepeaterModeReadData(p_nim, p_nim->tuner.cx24128.tuner_handle, reg_field_index, (unsigned char *)p_value)== False) 
     { 
        return(False); 
     } 
     return (True); 
} 
#endif 
#if INCLUDE_DEBUG 
/*******************************************************************************************************/ 
/* RegisterVerifyMap() */ 
/*******************************************************************************************************/ 
//static
BOOL  
RegisterVerifyMap(NIM *nim)   /* function to verify register map (run at start of driver) */ 
{ 
  /* performs initialization-time checking of register structure */ 
  int  i; 
  static char  *rvm_msg = "RegisterVerifyMap()"; 
 
  for (i = 0 ; i < CX24130_REG_COUNT; i++) 
  { 
    /* test that index number matches the count */ 
    if (demod_register_map[i].bit_field != (REGIDX)i) 
    { 
        /* read error reading hardware */ 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,i); 
        return(False); 
    } 
 
    /* test that address is within valid range */ 
    if (demod_register_map[i].address > MAX_COBRA_ADDR) 
    { 
        /* read error reading hardware */ 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,i); 
        return(False); 
    } 
 
    /* test that the various enum flags are valid */ 
    if (RegisterVerifyRegRW(nim,i) == False)
	return(False); 

    if (RegisterVerifyRegFilter(nim,i) == False)
	return(False); 

    if (RegisterVerifyRegDataType(nim,i) == False)
	return(False); 
 
    /* test that starting bit number is valid in conjunction with bit_count */ 
    if (RegisterVerifyRegBitCount(nim,i) == False)
	return(False);
 
    /* test that default_value and testsetting are within the range of pow(2,bit-length) */ 
    if (RegisterVerifyRegDefault(nim,i) == False)
	return(False);
  } 
 
  return(True); 
 
}  /* RegisterVerifyMap() */ 
 
 
/*******************************************************************************************************/ 
/* RegMapTest() - verifies the register map once */ 
/*******************************************************************************************************/ 
BOOL RegMapTest (NIM *nim) 
{ 
	static int regmap_test = False;    /* allows test code to be run once at start-up */ 
 
	/* verify register map once at start-up */ 
	if (regmap_test == False) 
	{ 
		regmap_test = True; 
 
		if (RegisterVerifyMap(nim) != True) 
		{ 
			/* tell exactly where the error occurred! */ 
			//nim->errfname = (char*)demod_register_map[nim->errline].regname; 
			return(False); 
		} 
	} 
	return (True); 
} 
 
 
/*******************************************************************************************************/ 
/* RegisterVerifyRegRW() */ 
/*******************************************************************************************************/ 
//static
BOOL  RegisterVerifyRegRW(             /* function to verify register settings */ 
	NIM   *nim,                            /* pointer to nim */ 
	int   idx)                             /* register to test */ 
{ 
  static char  *rvm_msg = "RegisterVerifyRegRW()"; 
 
  switch (demod_register_map[idx].access_level) 
  { 
    case  REG_RW: 
    case  REG_RO: 
    case  REG_WO: 
    { 
      /* above constitute valid values for regrw in reg.map */ 
      break; 
    } 
    case  REG_UNUSED: 
    default: 
    { 
      /* invalid setting in reg.map, report to app */ 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
        return(False); 
    } 
  }  /* switch(... */ 
 
  return(True); 
 
}  /* RegisterVerifyRegRW() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterVerifyRegFilter() */ 
/*******************************************************************************************************/ 
//static
BOOL  RegisterVerifyRegFilter(         /* function to verify reg.map filter setting */ 
	NIM   *nim,                            /* pointer to nim */ 
	int   idx)                             /* register to test */ 
{ 
  static char  *rvm_msg = "RegisterVerifyRegFilter()"; 
 
  if ((demod_register_map[idx].regfilter&(REGF_COBRA|REGF_CAM_DEF|REGF_CAM_EXT|REGF_CAM_ONLY|REGF_CAM_RED|REGF_ZEROB6)) == 0) 
  { 
    /* invalid setting in reg.map, report to app */ 
    DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
    return(False); 
  } 
 
  return(True); 
 
}  /* RegisterVerifyRegFilter() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterVerifyRegDataType() */ 
/*******************************************************************************************************/ 
//static
BOOL  RegisterVerifyRegDataType(       /* function to verify register data type value in reg.map */ 
	NIM   *nim,                            /* pointer to nim */ 
	int   idx)                             /* register to test */ 
{ 
  static char  *rvm_msg = "RegisterVerifyRegDataType()"; 
 
  /* test data-type contained in reg.map against 1) valid type; 2) bit_count must be consistant */ 
  switch (demod_register_map[idx].reg_type) 
  { 
    case  REGT_INT: 
    case  REGT_UINT: 
    case  REGT_LONG: 
    case  REGT_ULONG: 
    case  REGT_EOLIST: 
    { 
      /* above constitute valid values in reg.map */ 
      break; 
    } 
    case  REGT_MINT: 
    { 
      /* length must be > 8 */ 
      if (demod_register_map[idx].bit_count < 9) 
      { 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
        return(False); 
      } 
      break; 
    } 
    case  REGT_BYTE: 
    { 
      /* length must be <= 8 */ 
      if (demod_register_map[idx].bit_count > 8) 
      { 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
        return(False); 
      } 
      break; 
    } 
    case  REGT_BITHL: 
    case  REGT_BIT: 
    { 
      /* length must be == 1 */ 
      if (demod_register_map[idx].bit_count != 1) 
      { 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
        return(False); 
      } 
      break; 
    } 
    case  REGT_NULL: 
    { 
      /* perform no special tests on register designated as NULL */ 
      break; 
    } 
    default: 
    { 
      /* invalid setting in reg.map, report to app */ 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
        return(False); 
    } 
  }  /* switch(... */ 
 
  return(True); 
 
}  /* RegisterVerifyRegDataType() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterVerifyRegDefault() */ 
/*******************************************************************************************************/ 
//static
BOOL  RegisterVerifyRegDefault(        /* function to verify range of default setting in reg.map */ 
	NIM   *nim,                            /* pointer to nim */ 
	int   idx)                             /* register to test */ 
{ 
  static char  *rvm_msg = "RegisterVerifyRegDefault()"; 
  unsigned long  ulTemp; 
 
  /* create max bit pattern to test against */ 
  ulTemp = (demod_register_map[idx].bit_count == 0 ? 0x01UL : (0x01UL<<demod_register_map[idx].bit_count)); 
   
  /* bitmap in l will be pow(2,bit_count)+1, so values below this are valid */ 
  if (demod_register_map[idx].default_value >= ulTemp && demod_register_map[idx].default_value != 0xffffffffUL) 
  { 
    if (demod_register_map[idx].reg_type != REGT_NULL) 
    { 
      /* invalid setting in reg.map, report to app */ 
        DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
        return(False); 
    } 
  }  /* switch(... */ 
 
  return(True); 
 
}  /* RegisterVerifyRegDefault() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterVerifyRegBitCount() */ 
/*******************************************************************************************************/ 
//static
BOOL  RegisterVerifyRegBitCount(       /* function to verify that bit count rules are adhered to */ 
	NIM   *nim,                            /* pointer to nim */ 
	int   idx)                             /* register to test */ 
{ 
  static char  *rvm_msg = "RegisterVerifyRegBitCount()"; 
 
  int  len; 
 
  /* bit count of zero is an error! */ 
  if (demod_register_map[idx].bit_count != 0) 
  { 
    /* test for bit counts of less-eq 8 */ 
    if (demod_register_map[idx].bit_count <= 8) 
    { 
      /* calc the number of valid positions starting pt. can be */ 
      len = (8 - demod_register_map[idx].bit_count); 
      if (RegisterOneOf((signed char)(demod_register_map[idx].start_bit+'0'),"76543210",len+1) == True) return(True); 
    } 
    else 
    { 
      /* if bit-count > 8, then start can be anywhere --> look at reg.map SYSSymbolRate[21:0] */ 
      return(True); 
    } 
  } 
 
  /* invalid setting in reg.map, report to app */ 
  DRIVER_SET_ERROR_MSG(nim,API_REG_VERFY_IDX,rvm_msg,idx); 
   
  return(False); 
 
}  /* RegisterVerifyBitCount() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterOneOf()  */ 
/*******************************************************************************************************/ 
BOOL  RegisterOneOf(                   /* function to find match in match_list for len.  */ 
char  match,                           /* char to match */ 
char  *match_list,                     /* list to match against */ 
int   len)                             /* max length of match_list */ 
{ 
  register  int  i; 
 
  /* try to match 'match' with 'match_list[0..n] */ 
  for (i = 0 ; match_list[i] != CNULL ; i++) 
  { 
    if (match == match_list[i])
	return(True); 

    if (i > 255)
	break; 

    if (i >= len)
	break; 
  } 
   
  return(False); 
 
}  /* RegisterOneOf() */ 
#endif /* INCLUDE_DEBUG */ 
 
 
/*******************************************************************************************************/ 
/* Register_bitlength() */ 
/*******************************************************************************************************/ 
int    Register_bitlength(             /* returns the length in bit of a register */ 
REGIDX regidx)                         /* regidx of register number to read bitlength of */ 
{ 
  return(demod_register_map[regidx].bit_count); 
 
}  /* Register_bitlength() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterWritePLLMult() */ 
/*******************************************************************************************************/ 
BOOL           RegisterWritePLLMult(   /* function to write PLLMult register */ 
NIM            *nim,                   /* pointer to nim */ 
unsigned long  ulRegVal)               /* value to write to PLLMult register */ 
{ 
   /* only write the PLLMult value if it is different from the last value 
    * programmed to the PLLMult register. 
    */ 
   if (nim->ucPLLMult_shadow != (unsigned char)ulRegVal) 
   { 
      if (RegisterWrite(nim,CX24130_PLLMULT,ulRegVal, DEMOD_I2C_IO) == False) 
      { 
         return (False); 
      } 
      nim->ucPLLMult_shadow = (unsigned char)ulRegVal; 
   } 
 
   return (True); 
 
}  /* RegisterWritePLLMult() */ 
 
/*******************************************************************************************************/ 
/* RegisterReadCentralFreq() */ 
/*******************************************************************************************************/ 
BOOL           RegisterReadCentralFreq(  /* function to read current central freq register for camaric */ 
NIM            *nim,                     /* pointer to nim */ 
unsigned long  *pFreq)                   /* pointer to returned frequency value */ 
{ 
    unsigned long ulRegVal = 0UL; 

    if (DRIVER_Camaric(nim) == 1)
    {
	/* initialize return value */ 
	*pFreq = 0UL; 
 
	/* read the sign bit and shift it to bit 12 */ 
	if (RegisterRead(nim,CX24123_ACQPRFREQCURRSIGN,&ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return(False); 
	} 
	*pFreq |= ulRegVal << 12; 
 
	/* read the MSB of mantissa and shift it to bit 11 */ 
	if (RegisterRead(nim,CX24123_ACQPRFREQCURRMSB,&ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return(False); 
	} 
	*pFreq |= ulRegVal << 4; 
 
	/* read the LSB of mantissa */ 
	if (RegisterRead(nim,CX24123_ACQPRFREQCURRLSB,&ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return(False); 
	} 
	*pFreq |= ulRegVal; 
    } else
	return (False);
 
  return (True); 
}  /* RegisterReadCentralFreq() */ 
 
 
/*******************************************************************************************************/ 
/* RegisterWriteCentralFreq() */ 
/*******************************************************************************************************/ 
BOOL           RegisterWriteCentralFreq(  /* function to write nominal central freq register for camaric */ 
NIM            *nim,                      /* pointer to nim */ 
unsigned long  ulFreq)                    /* frequency value to write */ 
{ 
   unsigned long ulRegVal;

    if (DRIVER_Camaric(nim) == 1)
    {
	/* write the sign bit at bit 12 */ 
	ulRegVal = (ulFreq & 0x1000UL) >> 12; 
	if (RegisterWrite(nim,CX24123_ACQPRFREQNOMSIGN,ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return(False); 
	} 
 
	/* write the MSB of mantissa at bits 11:4 */ 
	ulRegVal = (ulFreq & 0xff0UL) >> 4; 
	if (RegisterWrite(nim,CX24123_ACQPRFREQNOMMSB,ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return(False); 
	} 
 
	/* write the LSB of mantissa at bits 3:0 */ 
	ulRegVal = ulFreq & 0xfUL; 
	if (RegisterWrite(nim,CX24123_ACQPRFREQNOMLSB,ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return(False); 
	} 
    } else
	return (False);

  return (True); 
}  /* RegisterWriteCentralFreq() */ 
 
 
 
/*******************************************************************************************************/ 
/* RegisterWriteClkSmoothDiv() */ 
/*******************************************************************************************************/ 
BOOL           RegisterWriteClkSmoothDiv( /* function to write mpeg clk smoother freq divider */ 
NIM            *nim,                      /* pointer to nim */ 
unsigned long  ulClkSmoothDiv)            /* clk smoother freq divider value to write */ 
{ 
   unsigned long ulRegVal; 
 
    if (DRIVER_Cobra(nim) == 1)
    {
	ulRegVal = ulClkSmoothDiv; 
	if (RegisterWrite(nim, 0xA2,ulRegVal, DEMOD_I2C_IO) != True)
	{ 
		return (False); 
	} 

    } else
    if (DRIVER_Camaric(nim) == 1)
    {
	/* write 8 LSB bits */ 
	ulRegVal = ulClkSmoothDiv & 0xFFUL; 
	if (RegisterWrite(nim,CX24123_MPGCLKSMFREQDIVLSB,ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return (False); 
	} 
 
	/* write 1 middle bit */ 
	ulRegVal = (ulClkSmoothDiv & 0x100UL) >> 8; 
	if (RegisterWrite(nim,CX24123_MPGCLKSMFREQDIVMID,ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
		return (False); 
	} 
 
	/* write 2 MSB bits */ 
	ulRegVal = (ulClkSmoothDiv & 0x600UL) >> 9; 
	if (RegisterWrite(nim,CX24123_MPGCLKSMFREQDIVMSB,ulRegVal, DEMOD_I2C_IO) != True) 
	{ 
	return (False); 
	} 
   }
   return (True); 
 
}  /* RegisterWriteClkSmoothDiv() */ 
 
 
/*******************************************************************************************************/ 
/* REG.MAP (reg.map -- register map) Register descriptions */ 
const REGISTER_MAP demod_register_map[] = {          /* REG.MAP (reg.map -- register map) Register descriptions */ 
#define StrIp(s)       (s)             /* allow driver to contain register names */ 
#ifdef STRIP_REGNAMES                  /* if defined, then register str name is discarded */ 
#undef  StrIp                          /* at compile time (size of .obj, .lib is reduced) */ 
#define StrIp(s)       (NULL)          /* */ 
#endif  /* STRIP_REGNAMES */           /* */ 
  /* Regname                      RegIndex#                    Addr  Start  Bits  RegRW        Filter       RegType        Default       Bit-Mask */
  {StrIp("RstCobra"),             CX24130_RSTCOBRA,              0x00, 7,     8 ,   REG_WO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("RstCTL"),               CX24130_RSTCTL,                0x00, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("RstBTL"),               CX24130_RSTBTL,                0x00, 6,     1 ,   REG_RO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("RstDemod"),             CX24130_RSTDEMOD,              0x00, 5,     1 ,   REG_RO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("ACQReAcquire"),         CX24130_ACQREACQUIRE,          0x00, 4,     1 ,   REG_WO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("RstViterbi"),           CX24130_RSTVITERBI,            0x00, 3,     1 ,   REG_RO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("RstRS"),                CX24130_RSTRS,                 0x00, 2,     1 ,   REG_RO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("SYSPReset"),            CX24130_SYSPRESET,             0x00, 1,     1 ,   REG_RO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x02\x00\x00\x00"}, /* Made RO per CR 11434 */
  {StrIp("SYSReset"),             CX24130_SYSRESET,              0x00, 0,     1 ,   REG_WO,      REGF_COBRA,  REGT_BITHL,    0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("SYSVersion[7:0]"),      CX24130_SYSVERSION,            0x00, 7,     8 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x8d      ,  "\xff\x00\x00\x00"}, 
  {StrIp("PLLMult[5:0]"),         CX24130_PLLMULT,               0x01, 5,     6 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x3a      ,  "\x3f\x00\x00\x00"}, 
  {StrIp("PLLGain[5:0]"),         CX24130_PLLGAIN,               0x02, 5,     6 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x20      ,  "\x3f\x00\x00\x00"}, 
  {StrIp("ModulationReg"),        CX24130_MODULATIONREG,         0x03, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x07      ,  "\xff\x00\x00\x00"}, 
  {StrIp("SYSModType"),           CX24130_SYSMODTYPE,            0x03, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("SYSTranStd[1:0]"),      CX24130_SYSTRANSTD,            0x03, 6,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x60\x00\x00\x00"}, 
  {StrIp("DC2Mode[1:0]"),         CX24130_DC2MODE,               0x03, 4,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x18\x00\x00\x00"}, 
  {StrIp("ACQAutoAcqEn[2:0]"),    CX24130_ACQAUTOACQEN,          0x03, 2,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x7       ,  "\x07\x00\x00\x00"}, 
  {StrIp("MPGDataWidth"),         CX24130_MPGDATAWIDTH,          0x04, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x1       ,  "\x80\x00\x00\x00"}, 
  {StrIp("MPGGapClk"),            CX24130_MPGGAPCLK,             0x04, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x1       ,  "\x40\x00\x00\x00"}, 
  {StrIp("MPGClkPos[1:0]"),       CX24130_MPGCLKPOS,             0x04, 5,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\x30\x00\x00\x00"}, 
  {StrIp("*IdleClkDis"),          CX24130_IDLECLKDIS,            0x04, 3,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("MPGSyncPunc"),          CX24130_MPGSYNCPUNC,           0x04, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x1       ,  "\x01\x00\x00\x00"}, 
  {StrIp("MPGFailMode"),          CX24130_MPGFAILMODE,           0x05, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("MPGFailPol"),           CX24130_MPGFAILPOL,            0x05, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("MPGValidMode"),         CX24130_MPGVALIDMODE,          0x05, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("MPGValidPol"),          CX24130_MPGVALIDPOL,           0x05, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("MPGStartMode"),         CX24130_MPGSTARTMODE,          0x05, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("MPGStartPol"),          CX24130_MPGSTARTPOL,           0x05, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("MPGTeiDis"),            CX24130_MPGTEIDIS,             0x05, 1,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("MPGFailNSVal"),         CX24130_MPGFAILNSVAL,          0x05, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("MPGCntl1sel[1:0]"),     CX24130_MPGCNTL1SEL,           0x06, 3,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x0c\x00\x00\x00"}, 
  {StrIp("MPGCntl2sel[1:0]"),     CX24130_MPGCNTL2SEL,           0x06, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\x03\x00\x00\x00"}, 
  {StrIp("MPGCntl3Sel[1:0]"),     CX24130_MPGCNTL3SEL,           0x06, 5,     2,    REG_RW,      REGF_COBRA,  REGT_BYTE,     0x2       ,  "\x30\x00\x00\x00"}, /* kir++ */
  {StrIp("SYSSymbolRate[21:0]"),  CX24130_SYSSYMBOLRATE,         0x08, 5,     22,   REG_RW,      REGF_COBRA,  REGT_MINT,     0x1a3128  ,  "\x3f\xff\xff\x00"}, 
  {StrIp("ACQPRFreqNom[11:0]"),   CX24130_ACQPRFREQNOM,          0x0b, 7,     12,   REG_RW,      REGF_COBRA|REGF_CAM_EXT,  REGT_MINT,     0x0       ,  "\xff\x0f\x00\x00"}, 
  {StrIp("ACQFreqRange[2:0]"),    CX24130_ACQFREQRANGE,          0x0d, 2,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x2       ,  "\x07\x00\x00\x00"}, 
  {StrIp("ACQVitSINom"),          CX24130_ACQVITSINOM,           0x0e, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("DC2ModeSel"),           CX24130_DC2MODESEL,            0x0e, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("ACQVitCRNom[2:0]"),     CX24130_ACQVITCRNOM,           0x0e, 2,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x3       ,  "\x07\x00\x00\x00"}, 
  {StrIp("ACQCREn[7:0]"),         CX24130_ACQCREN,               0x0f, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xae      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQSISearchDis"),       CX24130_ACQSISEARCHDIS,        0x10, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("ACQRepeatCR[3:0]"),     CX24130_ACQREPEATCR,           0x10, 3,     4 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\x0f\x00\x00\x00"}, 
  {StrIp("ACQFullSync"),          CX24130_ACQFULLSYNC,           0x14, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("ACQDeIntSync"),         CX24130_ACQDEINTSYNC,          0x14, 4,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("ACQSyncByteSync"),      CX24130_ACQSYNCBYTESYNC,       0x14, 3,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("ACQVitSync"),           CX24130_ACQVITSYNC,            0x14, 2,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("ACQDmdSync"),           CX24130_ACQDMDSYNC,            0x14, 1,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("SyncStatus"),           CX24130_SYNCSTATUS,            0x14, 7,     8 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"},  
  {StrIp("PLLLock"),              CX24130_PLLLOCK,               0x14, 0,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("ACQPRFreqCurr[11:0]"),  CX24130_ACQPRFREQCURR,         0x15, 7,     12,   REG_RO,      REGF_COBRA|REGF_CAM_EXT,  REGT_MINT,     0x0       ,  "\xff\x0f\x00\x00"}, 
  {StrIp("ACQPRFreqRdSel"),       CX24130_ACQPRFREQRDSEL,        0x16, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("ESNOStart"),            CX24130_ESNOSTART,             0x17, 0,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("ESNORdy[7:0]"),         CX24130_ESNORDY,               0x17, 7,     8 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("ESNOCount[15:0]"),      CX24130_ESNOCOUNT,             0x18, 7,     16,   REG_RO,      REGF_COBRA,  REGT_MINT,     0x0       ,  "\xff\xff\x00\x00"}, 
  {StrIp("ACQVitNormCount[7:0]"), CX24130_ACQVITNORMCOUNT,       0x1a, 7,     8 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitCurrSI"),         CX24130_ACQVITCURRSI,          0x1b, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("ACQVitCurrCR[2:0]"),    CX24130_ACQVITCURRCR,          0x1b, 2,     3 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x07\x00\x00\x00"}, 
  {StrIp("BERStart"),             CX24130_BERSTART,              0x1c, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("StartPnBer"),           CX24130_STARTPNBER,            0x1c, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("BERReady"),             CX24130_BERREADY,              0x1c, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("BERCountRS[21:0]"),     CX24130_BERCOUNT_RS,           0x1c, 5,     22,   REG_RO,      REGF_COBRA,  REGT_MINT,     0x0       ,  "\x3f\xff\xff\x00"}, 
  {StrIp("BERCount[21:0]"),       CX24130_BERCOUNT,              0x1c, 5,     22,   REG_RO,      REGF_COBRA,  REGT_MINT,     0x0       ,  "\x3f\xff\xff\x00"}, 
  {StrIp("TUNBurstBusy"),         CX24130_TUNBURSTBUSY,          0x20, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("TUNBurstRdy"),          CX24130_TUNBURSTRDY,           0x20, 6,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("TUNBurstClkRate[1:0]"), CX24130_TUNBURSTCLKRATE,       0x20, 5,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x30\x00\x00\x00"}, 
  {StrIp("TUNDataBit"),           CX24130_TUNDATABIT,            0x20, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("TUNClkBit"),            CX24130_TUNCLKBIT,             0x20, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("TUNEnBit"),             CX24130_TUNENBIT,              0x20, 1,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("TUNPllLock"),           CX24130_TUNPLLLOCK,            0x20, 0,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("TUNBurstDis"),          CX24130_TUNBURSTDIS,           0x21, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("TUNBurstClkPol"),       CX24130_TUNBURSTCLKPOL,        0x21, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("TUNBurstLength[5:0]"),  CX24130_TUNBURSTLENGTH,        0x21, 5,     6 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x15      ,  "\x3f\x00\x00\x00"}, 
  {StrIp("TUNBurstData[7:0]"),    CX24130_TUNBURSTDATA,          0x22, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xFFFFFFFFUL,"\xff\x00\x00\x00"}, 
  {StrIp("TUNBTIEn"),             CX24130_TUNBTIEN,              0x23, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("TUNBTIStart"),          CX24130_TUNBTISTART,           0x23, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("TUNBTIData[23:0]"),     CX24130_TUNBTIDATA,            0x24, 5,     22,   REG_WO,      REGF_COBRA,  REGT_MINT,     0xFFFFFFFFUL,"\x3f\xff\xff\x00"}, 
  {StrIp("FILValue[9:0]"),        CX24130_FILVALUE,              0x27, 7,     10,   REG_RW,      REGF_COBRA,  REGT_MINT,     0x30      ,  "\xff\x03\x00\x00"}, 
  {StrIp("FILValue[9:2]"),        CX24130_FILVALUE9_2,           0x27, 7,     8,    REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0C      ,  "\xff\x00\x00\x00"}, /* kir++ */
  {StrIp("FILValue[1:0]"),        CX24130_FILVALUE1_0,           0x28, 1,     2,    REG_RW,      REGF_COBRA,  REGT_BYTE,     0x00      ,  "\x03\x00\x00\x00"}, /* kir++ */
  {StrIp("FILDis"),               CX24130_FILDIS,                0x28, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
#ifdef DEBUG_SCAN_LOSE_CHANNEL
  {StrIp("FILPol"),               CX24130_FILPOL,                0x28, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x1       ,  "\x10\x00\x00\x00"}, 
#else
  {StrIp("FILPol"),               CX24130_FILPOL,                0x28, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
#endif
  {StrIp("LNBDC"),                CX24130_LNBDC,                 0x29, 7,     1 ,   REG_RW,      REGF_COBRA|REGF_ZEROB6, REGT_BIT,      0x1       ,  "\x80\x00\x00\x00"}, 
  {StrIp("LNBSendMsg"),           CX24130_LNBSENDMSG,            0x29, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("LNBLongMsg"),           CX24130_LNBLONGMSG,            0x29, 5,     1 ,   REG_RW,      REGF_COBRA|REGF_ZEROB6, REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("LNBTone"),              CX24130_LNBTONE,               0x29, 4,     1 ,   REG_RW,      REGF_COBRA|REGF_ZEROB6, REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("LNBBurstModSel"),       CX24130_LNBBURSTMODSEL,        0x29, 3,     1 ,   REG_RW,      REGF_COBRA|REGF_ZEROB6, REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("LNBMoreMsg"),           CX24130_LNBMOREMSG,            0x29, 2,     1 ,   REG_RW,      REGF_COBRA|REGF_ZEROB6, REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("LNBMsgLength[1:0]"),    CX24130_LNBMSGLENGTH,          0x29, 1,     2 ,   REG_RW,      REGF_COBRA|REGF_ZEROB6, REGT_BYTE,     0x0       ,  "\x03\x00\x00\x00"}, 
  {StrIp("LNBBurstLength[4:0]"),  CX24130_LNBBURSTLENGTH,        0x2a, 7,     5 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x16      ,  "\xf8\x00\x00\x00"}, 
  {StrIp("LNBDiseqcDis"),         CX24130_LNBDISEQCDIS,          0x2a, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("LNBMode[1:0]"),         CX24130_LNBMODE,               0x2a, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x03\x00\x00\x00"}, 
  {StrIp("LNBToneClk[7:0]"),      CX24130_LNBTONECLK,            0x2b, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x73      ,  "\xff\x00\x00\x00"}, 
  {StrIp("LNBMsg1[7:0]"),         CX24130_LNBMSG1,               0x2c, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("LNBMsg2[7:0]"),         CX24130_LNBMSG2,               0x2d, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("LNBMsg3[7:0]"),         CX24130_LNBMSG3,               0x2e, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("LNBMsg4[7:0]"),         CX24130_LNBMSG4,               0x2f, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("LNBMsg5[7:0]"),         CX24130_LNBMSG5,               0x30, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("LNBMsg6[7:0]"),         CX24130_LNBMSG6,               0x31, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("IntrEnable"),           CX24130_INTRENABLE,            0x33, 6,     7 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x7f\x00\x00\x00"}, 
  {StrIp("INTSyncEn"),            CX24130_INTSYNCEN,             0x33, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("INTAcqFailEn"),         CX24130_INTACQFAILEN,          0x33, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("INTVitUnLockEn"),       CX24130_INTVITUNLOCKEN,        0x33, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("INTVitLockEn"),         CX24130_INTVITLOCKEN,          0x33, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("INTDmdUnlockEn"),       CX24130_INTDMDUNLOCKEN,        0x33, 1,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("INTDmdLockEn"),         CX24130_INTDMDLOCKEN,          0x33, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("IntrPending"),          CX24130_INTRPENDING,           0x34, 6,     7 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x7f\x00\x00\x00"}, 
  {StrIp("INTSyncRd"),            CX24130_INTSYNCRD,             0x34, 5,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("INTAcqFailRd"),         CX24130_INTACQFAILRD,          0x34, 4,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("INTVitUnLockRd"),       CX24130_INTVITUNLOCKRD,        0x34, 3,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("INTVitLockRd"),         CX24130_INTVITLOCKRD,          0x34, 2,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("INTDmdUnLockRd"),       CX24130_INTDMDUNLOCKRD,        0x34, 1,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("INTDmdLockRd"),         CX24130_INTDMDLOCKRD,          0x34, 0,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("INTSync"),              CX24130_INTSYNC,               0x34, 5,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("INTAcqFail"),           CX24130_INTACQFAIL,            0x34, 4,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("INTVitUnLock"),         CX24130_INTVITUNLOCK,          0x34, 3,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("INTVitLock"),           CX24130_INTVITLOCK,            0x34, 2,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("INTDmdUnLock"),         CX24130_INTDMDUNLOCK,          0x34, 1,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("INTDmdLock"),           CX24130_INTDMDLOCK,            0x34, 0,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("DMDAccumSel[2:0]"),     CX24130_DMDACCUMSEL,           0x3a, 4,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x2       ,  "\x1c\x00\x00\x00"}, 
  {StrIp("DMDSubAcmSel[1:0]"),    CX24130_DMDSUBACMSEL,          0x3a, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x03\x00\x00\x00"}, 
  {StrIp("DMDAccumVal[7:0]"),     CX24130_DMDACCUMVAL,           0x3b, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("DMDAccumRst[7:0]"),     CX24130_DMDACCUMRST,           0x3c, 7,     8 ,   REG_WO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\xff\x00\x00\x00"}, 
  {StrIp("AGCThresh[4:0]"),       CX24130_AGCTHRESH,             0x3d, 4,     5 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x16      ,  "\x1f\x00\x00\x00"}, 
  {StrIp("AGCPol"),               CX24130_AGCPOL,                0x3e, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("AGCBW[1:0]"),           CX24130_AGCBW,                 0x3e, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x00      ,  "\x03\x00\x00\x00"}, 
  {StrIp("CTLAfcThresh[4:0]"),            CX24130_CTLAFCTHRESH,          0x3f, 4,     5 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xb       ,  "\x1f\x00\x00\x00"}, 
  {StrIp("CTLInSel"),             CX24130_CTLINSEL,              0x40, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("CTLAfcGain[1:0]"),      CX24130_CTLAFCGAIN,            0x40, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\x03\x00\x00\x00"}, 
  {StrIp("CTLAcqBW[2:0]"),        CX24130_CTLACQBW,              0x41, 6,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x4       ,  "\x70\x00\x00\x00"}, 
  {StrIp("CTLTrackBW[2:0]"),      CX24130_CTLTRACKBW,            0x41, 2,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\x07\x00\x00\x00"}, 
  {StrIp("DMDLDGain[1:0]"),       CX24130_DMDLDGAIN,             0x42, 5,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x3       ,  "\x30\x00\x00\x00"}, 
  {StrIp("BTLBW[1:0]"),           CX24130_BTLBW,                 0x42, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x03\x00\x00\x00"}, 
  {StrIp("ESNOThresh[2:0]"),      CX24130_ESNOTHRESH,            0x43, 6,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x4       ,  "\x70\x00\x00\x00"}, 
  {StrIp("DMDSDThresh"),          CX24130_DMDSDTHRESH,           0x43, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("ConstIQ"),              CX24130_CONSTIQ,               0x44, 7,     8 ,   REG_RO,      REGF_COBRA,  REGT_NULL,     0xff76ff12UL,"\xFF\x00\x00\x00"}, 
  {StrIp("CSTPrTag"),             CX24130_CSTPRTAG,              0x44, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("CSTTag"),               CX24130_CSTTAG,                0x44, 6,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("CSTVal[5:0]"),          CX24130_CSTVAL,                0x44, 5,     6 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x3f\x00\x00\x00"}, 
  {StrIp("ACQVitNormThresh[7:0]"),CX24130_ACQVITNORMTHRESH,      0x48, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xfe      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin12[7:0]"), CX24130_ACQVITNORMWIN12,       0x4a, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x9       ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin23[7:0]"), CX24130_ACQVITNORMWIN23,       0x4b, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x15      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin34[7:0]"), CX24130_ACQVITNORMWIN34,       0x4c, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x24      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin45[7:0]"), CX24130_ACQVITNORMWIN45,       0x4d, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x34      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin56[7:0]"), CX24130_ACQVITNORMWIN56,       0x4e, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x44      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin67[7:0]"), CX24130_ACQVITNORMWIN67,       0x4f, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x56      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin78[7:0]"), CX24130_ACQVITNORMWIN78,       0x50, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x64      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin511[7:0]"),CX24130_ACQVITNORMWIN511,      0x51, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x7       ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitNormWin35[7:0]"), CX24130_ACQVITNORMWIN35,       0x52, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x11      ,  "\xff\x00\x00\x00"}, 
  {StrIp("RSDeRandEn"),           CX24130_RSDERANDEN,            0x55, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("RSFECDis"),             CX24130_RSFECDIS,              0x55, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("MPGInvSyncMode"),       CX24130_MPGINVSYNCMODE,        0x55, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("ACQRSSyncThresh[2:0]"), CX24130_ACQRSSYNCTHRESH,       0x55, 2,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x5       ,  "\x07\x00\x00\x00"}, 
  {StrIp("BERRSSelect[1:0]"),     CX24130_BERRSSELECT,           0x56, 7,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\xc0\x00\x00\x00"}, 
  {StrIp("BERErrorSel"),          CX24130_BERERRORSEL,           0x56, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("BERRSInfWinEn"),        CX24130_BERRSINFWINEN,         0x56, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("BERPnPol"),             CX24130_BERPNPOL,              0x56, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("BERPnLock"),            CX24130_BERPNLOCK,             0x56, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("BERPnErrWin[1:0]"),     CX24130_BERPNERRWIN,           0x56, 1,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1       ,  "\x03\x00\x00\x00"}, 
  {StrIp("BERRSErrWin[7:0]"),     CX24130_BERRSERRWIN,           0x57, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xff      ,  "\xff\x00\x00\x00"}, 
  {StrIp("MPGClkHold"),           CX24130_MPGCLKHOLD,            0x58, 5,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x2       ,  "\x30\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothGap"),      CX24130_MPGCLKSMOOTHGAP,       0x58, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothEn"),       CX24130_MPGCLKSMOOTHEN,        0x58, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothFreqDiv"),  CX24130_MPGCLKSMOOTHFREQDIV,   0x59, 5,     6 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x3f\x00\x00\x00"}, 
  {StrIp("ACQLockThresh[2:0]"),   CX24130_ACQLOCKTHRESH,         0x5a, 7,     3 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x3       ,  "\xe0\x00\x00\x00"}, 
  {StrIp("ACQUnlockThresh[4:0]"), CX24130_ACQUNLOCKTHRESH,       0x5a, 4,     5 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x1e      ,  "\x1f\x00\x00\x00"}, 
  {StrIp("ACQAccClrEn[7:0]"),     CX24130_ACQACCCLREN,           0x5b, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x3c      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQAfcWin[7:0]"),       CX24130_ACQAFCWIN,             0x5c, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x10      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQDmdWin[7:0]"),       CX24130_ACQDMDWINDOW,          0x5d, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xff      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQVitExpWin[7:0]"),    CX24130_ACQVITEXPWIN,          0x5e, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x20      ,  "\xff\x00\x00\x00"}, 
  {StrIp("ACQSyncByteWin[7:0]"),  CX24130_ACQSYNCBYTEWIN,        0x5f, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xFFFFFFFFUL,"\xff\x00\x00\x00"}, 
  {StrIp("ACQFullSyncWin[7:0]"),  CX24130_ACQFULLSYNCWIN,        0x60, 7,     8 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x10      ,  "\xff\x00\x00\x00"}, 
  {StrIp("*AcqLockMode"),         CX24130_ACQLOCKMODE,           0x61, 0,     1 ,   REG_WO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("GPIO3RdVal"),           CX24130_GPIO3RDVAL,            0x62, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("GPIO2RdVal"),           CX24130_GPIO2RDVAL,            0x62, 6,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("GPIO1RdVal"),           CX24130_GPIO1RDVAL,            0x62, 5,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("GPIO0RdVal"),           CX24130_GPIO0RDVAL,            0x62, 4,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("GPIO3Val"),             CX24130_GPIO3VAL,              0x62, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("GPIO2Val"),             CX24130_GPIO2VAL,              0x62, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("GPIO1Val"),             CX24130_GPIO1VAL,              0x62, 1,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("GPIO0Val"),             CX24130_GPIO0VAL,              0x62, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("GPIO3Dir"),             CX24130_GPIO3DIR,              0x63, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x1       ,  "\x80\x00\x00\x00"}, 
  {StrIp("GPIO2Dir"),             CX24130_GPIO2DIR,              0x63, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("GPIO1Dir"),             CX24130_GPIO1DIR,              0x63, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("GPIO0Dir"),             CX24130_GPIO0DIR,              0x63, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("GPIO3Sel"),             CX24130_GPIO3SEL,              0x63, 3,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x08\x00\x00\x00"}, 
  {StrIp("GPIO2Sel"),             CX24130_GPIO2SEL,              0x63, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("GPIO1Sel"),             CX24130_GPIO1SEL,              0x63, 1,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("GPIO0Sel"),             CX24130_GPIO0SEL,              0x63, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("GPIO4RdVal"),           CX24130_GPIO4RDVAL,            0x64, 7,     1 ,   REG_RO,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x80\x00\x00\x00"}, 
  {StrIp("GPIO4Val"),             CX24130_GPIO4VAL,              0x64, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("GPIO4Dir"),             CX24130_GPIO4DIR,              0x64, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("MPGCntl1HiZ"),          CX24130_MPGCNTL1_HIZ,          0x65, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("MPGCntl2HiZ"),          CX24130_MPGCNTL2_HIZ,          0x65, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, 
  {StrIp("MPGClkHiZ"),            CX24130_MPGCLKHIZ,             0x65, 4,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x10\x00\x00\x00"}, 
  {StrIp("MPGDataHiZ"),           CX24130_MPGDATA_HIZ,           0x65, 2,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x04\x00\x00\x00"}, 
  {StrIp("MPGData1HiZ"),          CX24130_MPGDATA1_HIZ,          0x65, 1,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x02\x00\x00\x00"}, 
  {StrIp("MPGData0HiZ"),          CX24130_MPGDATA0_HIZ,          0x65, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, 
  {StrIp("DC2ClkDis"),            CX24130_DC2CLKDIS,             0x66, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0xFFFFFFFFUL,"\x80\x00\x00\x00"}, 
  {StrIp("DCIIClkDir"),           CX24130_DC2CLKDIR,             0x66, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0xFFFFFFFFUL,"\x40\x00\x00\x00"}, 
  {StrIp("DC2ClkFreq[5:0]"),      CX24130_DC2CLKFREQ,            0x66, 5,     6 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0xFFFFFFFFUL,"\x3f\x00\x00\x00"}, 
  {StrIp("PLLEn"),                CX24130_PLLEN,                 0x67, 7,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x1       ,  "\x80\x00\x00\x00"}, 
  {StrIp("SYSSleep"),             CX24130_SYSSLEEP,              0x67, 6,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x40\x00\x00\x00"}, 
  {StrIp("INTRSPinSel[1:0]"),     CX24130_INTRSPINSEL,           0x67, 5,     2 ,   REG_RW,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x30\x00\x00\x00"}, 
  {StrIp("SYSBoardVer[1:0]"),     CX24130_SYSBOARDVER,           0x67, 1,     2 ,   REG_RO,      REGF_COBRA,  REGT_BYTE,     0x0       ,  "\x03\x00\x00\x00"}, 
/*  {StrIp("RSParityDis"),          CX24130_RSPARITYDIS,           0xea, 0,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x01\x00\x00\x00"}, */
/*  {StrIp("LOCK_ZeroEM"),          CX24130_LOCK_ZEROEM,           0x72, 5,     1 ,   REG_RW,      REGF_COBRA,  REGT_BIT,      0x0       ,  "\x20\x00\x00\x00"}, */

#ifdef CAMARIC_FEATURES
  /*******************************************************************************************************/
  /* additional Camaric registers (est. 43) */
  /*******************************************************************************************************/
  {StrIp("DC2Mode[1:0]"),         CX24123_SYSTRANAUTO,           0x03, 4,     2 ,   REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x18\x00\x00\x00"},
  {StrIp("MPGNullDataVal"),       CX24123_MPGNULLDATAVAL,        0x04, 2,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x04\x00\x00\x00"}, 
  {StrIp("MPGFixNullDataEn"),     CX24123_MPGFIXNULLDATAEN,      0x04, 1,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x02\x00\x00\x00"}, 
  {StrIp("MPGParSel[1:0]"),       CX24123_MPGPARSEL,             0x06, 7,     2,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xC0\x00\x00\x00"}, 
  {StrIp("MPGCntl3Sel[1:0]"),     CX24123_MPGCNTL3SEL,           0x06, 5,     2,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x03,        "\x30\x00\x00\x00"}, 
  {StrIp("ACQPRFreqNom[11:4]"),   CX24123_ACQPRFREQNOMMSB,       0x0b, 7,     8,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("ACQPRFreqNom[12]"),     CX24123_ACQPRFREQNOMSIGN,      0x0c, 4,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x10\x00\x00\x00"}, 
  {StrIp("ACQPRFreqNom[3:0]"),    CX24123_ACQPRFREQNOMLSB,       0x0c, 3,     4,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x0f\x00\x00\x00"}, 
  {StrIp("DMDSampleGain[2:0]"),   CX24123_DMDSAMPLEGAIN,         0x0c, 7,     3,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x02,        "\xe0\x00\x00\x00"}, 
  {StrIp("ACQFreqRange[6:0]"),    CX24123_ACQFREQRANGE,          0x0d, 6,     7 ,   REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x02,        "\x7f\x00\x00\x00"}, 
  {StrIp("ACQPRFreqCurr[11:4]"),  CX24123_ACQPRFREQCURRMSB,      0x15, 7,     8,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("ACQPRFreqCurr[12]"),    CX24123_ACQPRFREQCURRSIGN,     0x16, 4,     1,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x10\x00\x00\x00"}, 
  {StrIp("ACQPRFreqCurr[3:0]"),   CX24123_ACQPRFREQCURRLSB,      0x16, 3,     4,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x0f\x00\x00\x00"}, 
  {StrIp("TUNI2CRptEn"),          CX24123_TUNI2CRPTEN,           0x23, 6,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x40\x00\x00\x00"}, 
  {StrIp("TUNI2CRptStart"),	  CX24123_TUNI2CRPTSTART,         0x23, 5,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x20\x00\x00\x00"}, /* kir++ */
  {StrIp("LNBDi2RxSel[1:0]"),     CX24123_LNBDI2RXSEL,           0x28, 6,     2,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x02,        "\x60\x00\x00\x00"}, 
  {StrIp("LNBSendMsg"),           CX24123_LNBSENDMSG,            0x29, 6,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x40\x00\x00\x00"}, 
  {StrIp("LNBMsg1"),              CX24123_LNBMSG1,               0x2c, 7,     8,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("LNBSMCntlBits[3:0]"),   CX24123_LNBSMCNTLBITS,         0x2c, 7,     4,    REG_WO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xf0\x00\x00\x00"}, 
  {StrIp("LNBSMECBits[2:0]"),     CX24123_LNBSMECBITS,           0x2c, 3,     3,    REG_WO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x0e\x00\x00\x00"}, 
  {StrIp("LNBSMPol"),             CX24123_LNBSMPOL,              0x2c, 0,     1,    REG_WO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x01\x00\x00\x00"}, 
  {StrIp("LNBSMDelay[4:0]"),      CX24123_LNBSMDELAY,            0x2c, 4,     5,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x1f\x00\x00\x00"}, 
  {StrIp("LNBDi2RxErrorLoc[7:0]"),CX24123_LNBDI2RXERRORLOC,      0x2d, 7,     8,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("LNBDi2RxLength[3:0]"),  CX24123_LNBDI2RXLENGTH,        0x2e, 7,     4,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xf0\x00\x00\x00"}, 
  {StrIp("LNBDi2RxError"),        CX24123_LNBDI2RXERROR,         0x2e, 3,     1,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x08\x00\x00\x00"}, 
  {StrIp("LNBDi2RxTimeout"),      CX24123_LNBDI2RXTIMEOUT,       0x2e, 2,     1,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x04\x00\x00\x00"}, 
  {StrIp("LNBDi2RxAutoRdEn"),     CX24123_LNBDI2RXAUTORDEN,      0x32, 7,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x01,        "\x80\x00\x00\x00"}, 
  {StrIp("LNBDi2RxTag[2:0]"),     CX24123_LNBDI2RXTAG,           0x32, 6,     3,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x70\x00\x00\x00"}, 
  {StrIp("LNBDi2RxExpWin[1:0]"),  CX24123_LNBDI2RXEXPWIN,        0x32, 3,     2,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x03,        "\x0c\x00\x00\x00"}, 
  {StrIp("LNBDCPol"),             CX24123_LNBDCPOL,              0x32, 1,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x02\x00\x00\x00"}, 
  {StrIp("LNBDi2En"),             CX24123_LNBDI2EN,              0x32, 0,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x01\x00\x00\x00"}, 
  {StrIp("INTLNBMsgRdyEn"),       CX24123_INTLNBMSGRDYEN,        0x33, 6,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x40\x00\x00\x00"}, 
  {StrIp("INTLNBMsgRdy"),         CX24123_INTLNBMSGRDY,          0x34, 6,     1,    REG_WO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x40\x00\x00\x00"}, 
  {StrIp("LNBToneAmp[5:0]"),      CX24123_LNBTONEAMP,            0x35, 5,     6,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x06,        "\x3f\x00\x00\x00"}, 
  {StrIp("LNBDCODEn"),            CX24123_LNBDCODEN,             0x36, 5,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x20\x00\x00\x00"}, 
  {StrIp("LNBSMEn"),              CX24123_LNBSMEN,               0x36, 4,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x10\x00\x00\x00"}, 
  {StrIp("LNBDi2ToneFreq[11:8]"), CX24123_LNBDI2TONEFREQMSB,     0x36, 3,     4,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x02,        "\x0f\x00\x00\x00"}, 
  {StrIp("LNBDi2ToneFreq[7:0]"),  CX24123_LNBDI2TONEFREQLSB,     0x37, 7,     8,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x3a,        "\xff\x00\x00\x00"}, 
  {StrIp("DMDSymValue[7:0]"),     CX24123_DMDSYMVALUE,           0x45, 7,     8,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("DMDSymUpdate"),         CX24123_DMDSYMUPDATE,          0x46, 3,     1,    REG_WO,      REGF_CAM_ONLY, REGT_BYTE,   0x01,        "\x08\x00\x00\x00"}, 
  {StrIp("DMDSymWin[2:0]"),       CX24123_DMDSYMWIN,             0x46, 2,     3,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x05,        "\x07\x00\x00\x00"}, 
  {StrIp("DMDSymReady[7:0]"),     CX24123_DMDSYMREADY,           0x47, 7,     8,    REG_RO,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("MPGFailValidDis"),      CX24123_MPGFAILVALIDDIS,       0x55, 6,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x40\x00\x00\x00"}, 
  {StrIp("MPGFailStartDis"),      CX24123_MPGFAILSTARTDIS,       0x55, 5,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x20\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothFreqDiv[10:9]"),
                                  CX24123_MPGCLKSMFREQDIVMSB,    0x58, 7,     2,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xc0\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothFreqDiv[8]"),
                                  CX24123_MPGCLKSMFREQDIVMID,    0x58, 2,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x04\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothSel[1:0]"), CX24123_MPGCLKSMOOTHSEL,       0x58, 1,     2,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x03\x00\x00\x00"}, 
  {StrIp("MPGClkSmoothFreqDiv[7:0]"),
                                  CX24123_MPGCLKSMFREQDIVLSB,    0x59, 7,     8,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\xff\x00\x00\x00"}, 
  {StrIp("DC2ClkSel"),            CX24123_DC2CLKSEL,             0x67, 2,     1,    REG_RW,      REGF_CAM_ONLY, REGT_BYTE,   0x00,        "\x04\x00\x00\x00"}, 

#endif  /* #ifdef CAMARIC_FEATURES */
#undef StrIp
/*******************************************************************************************************/
/* end-of-Register map indicator -- must be the last record in list */
  {"",REGID_EOL,0xff,CNULL,CNULL,REG_UNUSED,REGF_NONE,REGT_EOLIST,0UL,"\x00\x00\x00\x00"}     
}; 
 
#if INCLUDE_VIPER 
/******************************************************************************************************* 
 |              -- M O N G O O S E / V I P E R   I 2 C   R E G I S T E R   M A P --                    | 
 *******************************************************************************************************/ 
const REGISTER_MAP viper_register_map[] = {   
/* ---------------------------------------------------------------------------------------------------------------------------------* 
 |Name,[bit_field]                [address]                      [start_bit][bits] [RegRW] [filter]    [reg_type] default [hw_mask] | 
 * ---------------------------------------------------------------------------------------------------------------------------------*/ 
  /* ---- TUNER (2x for Viper) ---- */  
  {0, CX24128_DSM_CLK,        CX24128_LO_TEST,                   7,		 1,     REG_RW, REGF_VIPER, REGT_BIT,  0,      "\x80"}, 
  {0, CX24128_PS_TEST,        CX24128_LO_TEST,                   6,		 1,     REG_RW, REGF_VIPER, REGT_BIT,  0,      "\x40"},               
  {0, CX24128_ICP_MAN,        CX24128_LO_TEST,                   5,		 2,     REG_RW, REGF_VIPER, REGT_BIT,  0,      "\x30"}, 
  {0, CX24128_IDIG_SEL,       CX24128_LO_TEST,                   3,		 2,     REG_RW, REGF_VIPER, REGT_BIT,  0,      "\x0C"},             
  {0, CX24128_LOCK_DET,       CX24128_LO_TEST,                   1,		 1,     REG_RO, REGF_VIPER, REGT_BIT,  0,      "\x02"}, 
  {0, CX24128_MOR,            CX24128_LO_TEST,                   0,		 1,     REG_RW, REGF_VIPER, REGT_BIT,  0,      "\x01"},              
  {0, CX24128_ICP_LEVEL,      CX24128_LO_CTL1,		         7,		 8,     REG_RW, REGF_VIPER, REGT_BYTE, 	0,      "\xFF"}, 
  {0, CX24128_BS_DELAY,       CX24128_LO_CTL2,                   7,		 4,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\xF0"}, 
  {0, CX24128_ACP_ON_ALW,     CX24128_LO_CTL2,                   2,		 1,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x04"}, 
  {0, CX24128_ICP_SEL,        CX24128_LO_CTL2,                   1,		 2,     REG_RO, REGF_VIPER, REGT_BIT,	0,      "\x03"},   
  {0, CX24128_BS_VCOMT,       CX24128_BAND_SEL2_14,              7,		 2,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\xC0"},  
  {0, CX24128_BS_FREQ,        CX24128_BAND_SEL2_14,              4,		 13,    REG_RW, REGF_VIPER, REGT_MINT, 	0,      "\x1F\xFF\x00\x00"}, 
  {0, CX24128_BS_DIV_CNT,     CX24128_BAND_SEL4_16,              7,		 12,    REG_RW, REGF_VIPER, REGT_MINT, 	0,      "\xFF\xF0\x00\x00"}, 
#if 0	/*kir*/
  {0, CX24128_BS_???,         CX24128_BAND_SEL4_17,              1,		 1,	REG_RO, REGF_VIPER, REGT_BIT, 	0,      "\x02"}, 
#endif
  {0, CX24128_BS_ERR,         CX24128_BAND_SEL5_17,              0,		 1,     REG_RO, REGF_VIPER, REGT_BIT,	0,      "\x01"}, 
  {0, CX24128_DIV24_SEL,      CX24128_VCO_18,                    6,		 1,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x40"}, 
  {0, CX24128_VCO_SEL_SPI,    CX24128_VCO_18,                    5,		 5,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x3E"},  
  {0, CX24128_VCO6_SEL_SPI,   CX24128_VCO_18,                    7,		 1,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x80"}, 
  {0, CX24128_VCO_BSH_SPI,    CX24128_VCO_18,                    0,		 1,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x01"}, 
  {0, CX24128_INT,            CX24128_LO1_19,                    7,		 9,     REG_RW, REGF_VIPER, REGT_MINT, 	0,      "\xFF\x80\x00\x00"},        
  {0, CX24128_FRACTN,         CX24128_LO2_1A,                    6,		 18,    REG_RW, REGF_VIPER, REGT_MINT, 	0,      "\x7F\xFF\xE0\x00"},  
  {0, CX24128_SYS_RESET,      CX24128_LO4_1C,                    4,		 1,     REG_WO, REGF_VIPER, REGT_BIT,	0,      "\x10"},    
  {0, CX24128_AMP_OUT,        CX24128_BB1_1D,                    3,		 4,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x0F"}, 
  {0, CX24128_FILTER_BW,      CX24128_BB2_1E,                    7,		 2,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\xC0"}, 
  {0, CX24128_GMC_BW,         CX24128_BB2_1E,                    5,		 6,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x3F"}, 
  {0, CX24128_VGA2_OFFSET,    CX24128_BB3_1F,                    5,		 3,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x38"}, 
  {0, CX24128_VGA1_OFFSET,    CX24128_BB3_1F,                    2,		 3,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x07"},  
  {0, CX24128_RFBC_DISABLE,   CX24128_RF_20,                     4,		 1,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x10"},  
  {0, CX24128_RF_OFFSET,      CX24128_RF_20,                     3,		 2,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x0C"}, 
  {0, CX24128_LNA_GC,         CX24128_RF_20,                     1,		 2,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x03"}, 
  {0, CX24128_EN,             CX24128_ENABLE_21,                 5,		 6,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x3F"}, 
  {0, CX24128_LNA_EN,         CX24128_ENABLE_21,                 1,		 1,     REG_RW, REGF_VIPER, REGT_BIT,	0,      "\x02"} 
};                             
#endif 

#if INCLUDE_VIPER_BTI
/******************************************************************************************************* 
 |              -- M O N G O O S E / V I P E R   I 2 C   R E G I S T E R   M A P --                    | 
 *******************************************************************************************************/ 
const REGISTER_MAP viper_bti_register_map[] = {   
/* ---------------------------------------------------------------------------------------------------------------------------------* 
 |Name,[bit_field]                [address]                      [start_bit][bits] [RegRW] [filter]    [reg_type] default [hw_mask] | 
 * ---------------------------------------------------------------------------------------------------------------------------------*/ 
  /* ---- TUNER (2x for Viper) ---- */  
  {0, BTI_CX24128_DSM_CLK,		2,			15,		1, 	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x80"},
  {0, BTI_CX24128_PS_TEST,		2,			14,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x40"},
  {0, BTI_CX24128_ICP_MAN,		2,			13,		2,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x30"},
  {0, BTI_CX24128_IDIG_SEL,		2,			11,		2,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x0C"},
  {0, BTI_CX24128_LOCK_DET,		2,			9,		1,	REG_RO, REGF_VIPER, REGT_BIT, 0,	"\x02"},
  {0, BTI_CX24128_MOR,			2,			8,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x01"},
  {0, BTI_CX24128_ICP_LEVEL,		2,			7,		8,	REG_RW, REGF_VIPER, REGT_BYTE, 0,	"\xFF"},

  {0, BTI_CX24128_BS_DELAY,		3,			15,		4,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\xF0"},
  {0, BTI_CX24128_ACP_ON_ALW,		3,			10,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x04"},
  {0, BTI_CX24128_ICP_SEL,		3,			9,		2,	REG_RO, REGF_VIPER, REGT_BIT, 0,	"\x03"},

  {0, BTI_CX24128_BS_VCOMT,		4,			15,		2,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\xC0"},
  {0, BTI_CX24128_BS_FREQ,		4,			12,		13,	REG_RW, REGF_VIPER, REGT_MINT, 0,	"\x1F\xFF\x00\x00"},

  {0, BTI_CX24128_BS_DIV_CNT,		5,			15,		12,	REG_RW, REGF_VIPER, REGT_MINT, 0,	"\xFF\xF0\x00\x00"},
  {0, BTI_CX24128_BS_ERR,		5,			0,		1,	REG_RO, REGF_VIPER, REGT_BIT, 0,	"\x01"},

  {0, BTI_CX24128_DIV24_SEL,		6,			14,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x40"},
  {0, BTI_CX24128_VCO_SEL_SPI,		6,			13,		5,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x3E"},
  {0, BTI_CX24128_VCO6_SEL_SPI,		6,			15,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x80"},
  {0, BTI_CX24128_VCO_BSH_SPI,		6,			8,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x01"},
  {0, BTI_CX24128_INT,			6,			7,		9,	REG_RW, REGF_VIPER, REGT_MINT, 0,	"\xFF\x80\x00\x00"},

  {0, BTI_CX24128_FRACTN,		7,			14,		18,	REG_RW, REGF_VIPER, REGT_MINT, 0,	"\x7F\xFF\xE0\x00"},

  {0, BTI_CX24128_SYS_RESET,		8,			12,		1,	REG_WO, REGF_VIPER, REGT_BIT, 0,	"\x10"},
  {0, BTI_CX24128_AMP_OUT,		8,			3,		4,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x0F"},

  {0, BTI_CX24128_FILTER_BW,		9,			15,		2,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\xC0"},
  {0, BTI_CX24128_GMC_BW,		9,			13,		6,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x3F"},
  {0, BTI_CX24128_VGA2_OFFSET,		9,			5,		3,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x38"},
  {0, BTI_CX24128_VGA1_OFFSET,		9,			2,		3,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x07"},

  {0, BTI_CX24128_RFBC_DISABLE,		0x0A,			12,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x10"},
  {0, BTI_CX24128_RF_OFFSET,		0x0A,			11,		2,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x0C"},
  {0, BTI_CX24128_LNA_GC,		0x0A,			9,		2,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x03"},
  {0, BTI_CX24128_EN,			0x0A,			5,		6,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x3F"},
  {0, BTI_CX24128_LNA_EN,		0x0A,			1,		1,	REG_RW, REGF_VIPER, REGT_BIT, 0,	"\x02"},

};                             
#endif 

/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/*******************************************************************************************************/ 
/* CR 9509 : Add an extra newline */
