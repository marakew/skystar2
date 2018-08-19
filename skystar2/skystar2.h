/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _skystar2_h_
#define _skystar2_h_

#include "../include/frontend.h"
#include "dvbdmx.h"
#include "dvbnet.h"
#include "dvbdev.h"
#include "frontend.h"


#define MaxHwFilters	(6+32)
#define nPidSlots	256

#define SizeOfBufDMA1   1280*188
#define SizeOfBufDMA2   10*188

struct DmaQ {
	u_int32_t	addr;
	u_int32_t	head;
	u_int32_t	tail;
	u_int32_t	size;
	u_int8_t	*buf;
};

struct adapter {

	u_int32_t		rev;

	struct resource		*res_mem;
	struct resource 	*res_irq;
	void			*res_ih;

	struct callout_handle	timeout_isr;

	u_int8_t		unit;
	u_int8_t		eaddr[6/*ETHER_ADDR_LEN*/];
	u_int32_t		dwSramType;	


	struct dvb_demux	demux;
	struct dvb_net		dvbnet;
	struct dvb_dev		dvbdev;

	struct DmaQ		DmaQ1;
	struct DmaQ		DmaQ2;
	u_int32_t		dma_ctrl;
	u_int32_t		dma_status;

	int			DmaCounter;
	int			ResetTimeout;
	u_int32_t		HandleFreeze;

	u_int32_t		capturing;


	int			UseableHwFilters;
	u_int16_t		HwPids[MaxHwFilters];
	u_int16_t		PidList[nPidSlots];
	int			PidRc[nPidSlots];
	int			PidCount;
	int			WholeBandWidthCount;

	u_int16_t		pids[0x27];
	u_int32_t		mac_filter;


	struct fe_data		*fe;

	int (*diseqc_send_master_cmd)(struct adapter *sc, struct dvb_diseqc_master_cmd *cmd);
	int (*diseqc_send_burst)(struct adapter *sc, fe_sec_mini_cmd_t cmd);
	int (*set_tone)(struct adapter *sc, fe_sec_tone_mode_t tone);
	int (*set_voltage)(struct adapter *sc, fe_sec_voltage_t voltage);
		
};

#endif
