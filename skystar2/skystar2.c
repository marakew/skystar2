/*
 *	SkyStar2 driver based on chip FlexCopII
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/signalvar.h>
#include <sys/mman.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>
#include <vm/vm_extern.h>

//-6.0
//#include <machine/bus_memio.h>
#include <machine/bus.h>
#include <machine/resource.h>
#include <machine/stdarg.h>
#include <sys/rman.h>

#if defined(__FreeBSD__) && (__FreeBSD_version < 500000)
#include <machine/clock.h>
#include <pci/pcivar.h>
#include <pci/pcireg.h>
#else
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#endif

#include <sys/sysctl.h>

#include "skystar2.h"
#include "sllutil.h"
#include "eeprom.h"
#include "sram.h"

#include "dvbdmx.h"
#include "dvbdev.h"
#include "debug.h"

static int debug = 0;

SYSCTL_INT(_debug, OID_AUTO, debug_hw, CTLFLAG_RW, &debug, 0, 
	"enable verbose debug messages: 0-9");

static int skystar2_probe(device_t dev);
static int skystar2_attach(device_t dev);
static int skystar2_detach(device_t dev);
static int skystar2_shutdown(device_t dev);
static int skystar2_suspend(device_t dev);
static int skystar2_resume(device_t dev);

static device_method_t skystar2_methods[] = {
	/* Device Interface */
	DEVMETHOD(device_probe,	skystar2_probe),
	DEVMETHOD(device_attach, skystar2_attach),
	DEVMETHOD(device_detach, skystar2_detach),
	DEVMETHOD(device_shutdown, skystar2_shutdown),
/* may be crush
	DEVMETHOD(device_suspend, skystar2_suspend),
	DEVMETHOD(device_resume, skystar2_resume),
*/	
	{0, 0}
};

static driver_t  skystar2_driver = {
	"skystar2",
	skystar2_methods,
	sizeof(struct adapter)
};

static void isr(void *arg);

devclass_t skystar2_devclass;
DRIVER_MODULE(skystar2, pci, skystar2_driver, skystar2_devclass, 0, 0);


/*============================================================================*/

static void
WriteRegBitField(struct adapter *sc, u_int32_t reg, u_int32_t zeromask, u_int32_t orval)
{
#if 1
	u_int32_t tmp;
	tmp = read_reg(sc, reg);
	tmp = (tmp & ~zeromask) | orval;
	write_reg(sc, reg, tmp);
#else
	write_reg(sc, reg, (read_reg(sc, reg) & ~zeromask) | orval);
#endif
}

/*-------------------------------------------------------------------------*/
static u_int32_t
WriteRegOp(struct adapter *sc, u_int32_t reg, int op, u_int32_t andval, u_int32_t orval)
{
	u_int32_t tmp;

	tmp = read_reg(sc, reg);
	if (op == 1) tmp = tmp | orval;
	if (op == 2) tmp = tmp & andval;
	if (op == 3) tmp = (tmp & andval) | orval;
	write_reg(sc, reg, tmp);
	return tmp;
}

/*================================================================
    SRAM (Skystar2 rev2.3 has one "ISSI IS61LV256" chip on board,
    but it seems that FlexCopII can work with more than one chip)
==================================================================*/

/*----------------------------------------------------------------*/
static u_int32_t
SRAMSetNetDest(struct adapter *sc, u_int8_t dest)
{
	u_int32_t tmp;

	DELAY(1000);
	tmp = (read_reg(sc, 0x714) & 0xfffffffc) | (dest & 3);

	DELAY(1000);
	DELAY(1000);
	write_reg(sc, 0x714, tmp);
	write_reg(sc, 0x714, tmp);

	DELAY(1000);

	return tmp;
}

/*----------------------------------------------------------------*/
static u_int32_t
SRAMSetCaiDest(struct adapter *sc, u_int8_t dest)
{
	u_int32_t tmp;

	DELAY(1000);
	tmp = (read_reg(sc, 0x714) & 0xfffffff3) | ((dest & 3) << 2);

	DELAY(1000);
	DELAY(1000);
	write_reg(sc, 0x714, tmp);
	write_reg(sc, 0x714, tmp);

	DELAY(1000);

	return tmp;
}

/*----------------------------------------------------------------*/
static u_int32_t
SRAMSetCaoDest(struct adapter *sc, u_int8_t dest)
{
	u_int32_t tmp;

	DELAY(1000);
	tmp = (read_reg(sc, 0x714) & 0xffffffcf) | ((dest & 3) << 4);

	DELAY(1000);
	DELAY(1000);
	write_reg(sc, 0x714, tmp);
	write_reg(sc, 0x714, tmp);

	DELAY(1000);

	return tmp;
}

/*----------------------------------------------------------------*/
static u_int32_t
SRAMSetMediaDest(struct adapter *sc, u_int8_t dest)
{
	u_int32_t tmp;

	DELAY(1000);
	tmp = (read_reg(sc, 0x714) & 0xffffff3f) | ((dest & 3) << 6);

	DELAY(1000);
	DELAY(1000);
	write_reg(sc, 0x714, tmp);
	write_reg(sc, 0x714, tmp);

	DELAY(1000);

	return tmp;
}

/*================================================================
=	PID Filter
= every flexcop has 6 "lower" hw PID filters
= 0(0x01) Stream1Filter
= 1(0x02) Stream2Filter
= 2(0x04) Pcr
= 3(0x08) Pmt
= 4(0x10) Emm
= 5(0x20) Ecm
= 
= these are enabled by setting bits 0-5 of 0x208
= for the 32 additional filters we have to select one
= of them through 0x310 and modify through 0x314
= op: 0=disable, 1=enable
==================================================================*/

/*----------------------------------------------------------------*/
static void
FilterEnableHwFilter(struct adapter *sc, int pid, u_int8_t op)
{
	DBG("pid=%d op=%d\n", pid, op);
	if (pid <= 5)
	{
		u_int32_t mask = (0x00000001 << pid);
		WriteRegBitField(sc, 0x208, mask, op ? mask : 0);
	} else {
		/* select */
		WriteRegBitField(sc, 0x310, 0x1f, (pid - 6) & 0x1f);
		/* modify */
		WriteRegBitField(sc, 0x314, 0x00006000, op ? 0x00004000 : 0);
	}
}

/*----------------------------------------------------------------*/
static void
PidSetHwPid(struct adapter *sc, int id, u_int16_t pid)
{
	DBG("id=%d pid=%d\n", id, pid);
	if (id <= 5)
	{
		u_int32_t adr = 0x300 + ((id & 6) << 1);
		int shift = (id & 1) ? 16 : 0;
		DBG("id=%d addr=%x %c pid=%d\n", id, adr, (id&1)?'h':'l', pid);
		WriteRegBitField(sc, adr, (0x7fff) << shift, (pid & 0x1fff) << shift);
	} else {
		/* select */
		WriteRegBitField(sc, 0x310, 0x1f, (id - 6) & 0x1f);
		/* modify */
		WriteRegBitField(sc, 0x314, 0x1fff, pid & 0x1fff);
	}
}


/*----------------------------------------------------------------*/
static void 
FilterEnableStream1Filter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000001, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000001);
}
/*----------------------------------------------------------------*/
static void
FilterEnableStream2Filter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000002, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000002);
}
/*----------------------------------------------------------------*/
static void 
FilterEnablePcrFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000004, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000004);
}
/*----------------------------------------------------------------*/
static void 
FilterEnablePmtFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000008, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000008);
}
/*----------------------------------------------------------------*/
static void 
FilterEnableEmmFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000010, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000010);
}

/*----------------------------------------------------------------*/
static void 
FilterEnableEcmFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000020, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000020);
}

/*----------------------------------------------------------------*/
static void 
FilterEnableNullFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000040, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000040);
}

/*----------------------------------------------------------------*/
static void 
FilterEnableMaskFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000080, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000080);
}

/*----------------------------------------------------------------*/
static void 
CtrlEnableMAC(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00004000, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00004000);
}

/*----------------------------------------------------------------*/
static int 
CASetMacDstAddrFilter(struct adapter *sc, u_int8_t *mac)
{
	u_int32_t tmp1,tmp2;

	DBG("\n" );
	tmp1 = (mac[3] << 0x18) | (mac[2] << 0x10) | (mac[1] << 0x08) | mac[0];
	tmp2 = (mac[5] << 0x08) | mac[4];

	write_reg(sc, 0x418, tmp1);
	write_reg(sc, 0x41C, tmp2);
	return 0;
}

/*----------------------------------------------------------------*/
static void 
SetIgnoreMACFilter(struct adapter *sc, int op)
{
	DBG("op=%x\n" , op);
	if (op != 0)
	{
		WriteRegOp(sc, 0x208, 2, ~0x00004000, 0);
		sc->mac_filter = 1;
	} else {
		if (sc->mac_filter != 0){
		    sc->mac_filter = 0;
		     WriteRegOp(sc, 0x208, 1, 0, 0x00004000);
		}
	}
}

/*----------------------------------------------------------------*/
static void 
CheckNullFilterEnable(struct adapter *sc)
{
	DBG("\n" );
	FilterEnableNullFilter(sc, 1);
	FilterEnableMaskFilter(sc, 1);
}

/*----------------------------------------------------------------*/
static void 
InitPIDsInfo(struct adapter *sc)
{
	u_int32_t i;
	for (i = 0; i < 0x27; i++) sc->pids[i] = 0x1FFF;
}

/*----------------------------------------------------------------*/
static u_int32_t
CheckPID(struct adapter *sc, u_int16_t pid)
{
	u_int32_t i;

	DBG("pid=%x\n" , pid);
	if (pid == 0x1FFF) return 0;
	for (i = 0; i < 0x27; i++)
		if (sc->pids[i] == pid) return 1;

	return 0;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetStream1PID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);
	
	val = (pid & 0x3FFF) | (read_reg(sc, 0x300) & 0xFFFFC000);
	write_reg(sc, 0x300, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetStream2PID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = ((pid & 0x3FFF) << 0x10) | (read_reg(sc, 0x300) & 0xFFFF);
	write_reg(sc, 0x300, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetPcrPID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = (pid & 0x3FFF) | (read_reg(sc, 0x304) & 0xFFFFC000);
	write_reg(sc, 0x304, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetPmtPID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = ((pid & 0x3FFF) << 0x10) | (read_reg(sc, 0x304) & 0xFFFF);
	write_reg(sc, 0x304, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetEmmPID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = (pid & 0xFFFF) | (read_reg(sc, 0x308) & 0xFFFF0000);
	write_reg(sc, 0x308, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetEcmPID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = (pid << 0x10) | (read_reg(sc, 0x308) & 0xFFFF);
	write_reg(sc, 0x308, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetGroupPID(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = (pid & 0x3FFF) | (read_reg(sc, 0x30C) & 0xFFFF0000);
	write_reg(sc, 0x30C, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidSetGroupMASK(struct adapter *sc, u_int32_t pid)
{
	u_int32_t val;
	DBG("pid=%x\n" , pid);

	val = ((pid & 0x3FFF) << 0x10) | (read_reg(sc, 0x30C) & 0xFFFF);
	write_reg(sc, 0x30C, val);
	return val;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetStream1PID(struct adapter *sc)
{
	return read_reg(sc, 0x300) & 0x0000FFFF;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetStream2PID(struct adapter *sc)
{
	return read_reg(sc, 0x300) >> 0x10;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetPcrPID(struct adapter *sc)
{
	return read_reg(sc, 0x304) & 0x0000FFFF;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetPmtPID(struct adapter *sc)
{
	return read_reg(sc, 0x304) >> 0x10;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetEmmPID(struct adapter *sc)
{
	return read_reg(sc, 0x308) & 0x0000FFFF;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetEcmPID(struct adapter *sc)
{
	return read_reg(sc, 0x308) >> 0x10;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetGroupPID(struct adapter *sc)
{
	return read_reg(sc, 0x30C) & 0x0000FFFF;
}

/*----------------------------------------------------------------*/
static u_int32_t
PidGetGroupMASK(struct adapter *sc)
{
	return read_reg(sc, 0x30C) >> 0x10;
}

/*----------------------------------------------------------------*/
static void 
ResetHardwarePIDFilter(struct adapter *sc)
{
	DBG("\n" );
	PidSetStream1PID(sc, 0x1FFF);	

	PidSetStream2PID(sc, 0x1FFF);
	FilterEnableStream2Filter(sc, 0);

	PidSetPcrPID(sc, 0x1FFF);
	FilterEnablePcrFilter(sc, 0);

	PidSetPmtPID(sc, 0x1FFF);
	FilterEnablePmtFilter(sc, 0);

	PidSetEcmPID(sc, 0x1FFF);
	FilterEnableEcmFilter(sc, 0);

	PidSetEmmPID(sc, 0x1FFF);
	FilterEnableEmmFilter(sc, 0);
}

/*----------------------------------------------------------------*/
static void
OpenWholeBandwidth(struct adapter *sc)
{
	PidSetGroupPID(sc, 0);
	PidSetGroupMASK(sc, 0);
#if 0
	FilterEnableMaskFilter(sc, 1);
#endif
}
	
/*----------------------------------------------------------------*/
static void
CloseWholeBandwidth(struct adapter *sc)
{
	PidSetGroupPID(sc, 0);
	PidSetGroupMASK(sc, 0x1fe0);
#if 0
	FilterEnableMaskFilter(sc, 1);
#endif
}
/*----------------------------------------------------------------*/
static void
WholeBandwidthInc(struct adapter *sc)
{
	if(sc->WholeBandWidthCount++ == 0)
		OpenWholeBandwidth(sc);
}

/*----------------------------------------------------------------*/
static void
WholeBandwidthDec(struct adapter *sc)
{
	if(--sc->WholeBandWidthCount <= 0)
		CloseWholeBandwidth(sc);
}


/*----------------------------------------------------------------*/
static int 
AddHwPID(struct adapter *sc, u_int32_t pid)
{
#if 1
	int i;
	DBG("pid=%d\n", pid);

	if (pid <= 0x1f)
			return 1;

	if (pid != 0x2000)
		for (i = 0; i < sc->UseableHwFilters; i++)
		{
			DBG("pid=%d searching slot=%d\n", pid, i);
			if (sc->HwPids[i] == 0x1fff)
			{
				DBG("pid=%d slot=%d\n", pid, i);
				sc->HwPids[i] = pid;
				PidSetHwPid(sc, i, pid);
				FilterEnableHwFilter(sc, i, 1);
				return 1;
			}
		}

	DBG("pid=%d whole bandwidth\n", pid);
	WholeBandwidthInc(sc);
	return 1;

#else
	DBG("pid=%d\n" , pid);

	if ( (PidGetGroupMASK(sc) == 0) &&
	     (PidGetGroupPID(sc) == 0) )
		return 0;

	if ( (PidGetStream1PID(sc) & 0x1FFF) == 0x1FFF)
	{
		PidSetStream1PID(sc, (pid & 0x1FFF));
		FilterEnableStream1Filter(sc, 1);
		return 1;
	}

	if ( (PidGetStream2PID(sc) & 0x1FFF) == 0x1FFF)
	{
		PidSetStream2PID(sc, (pid & 0x1FFF));
		FilterEnableStream2Filter(sc, 1);
		return 1;
	}
	
	if ( (PidGetPcrPID(sc) & 0x1FFF) == 0x1FFF)
	{
		PidSetPcrPID(sc, (pid & 0x1FFF));
		FilterEnablePcrFilter(sc, 1);
		return 1;
	}

	if ( (PidGetPmtPID(sc) & 0x1FFF) == 0x1FFF)
	{
		PidSetPmtPID(sc, (pid & 0x1FFF));
		FilterEnablePmtFilter(sc, 1);
		return 1;
	}

	if ( (PidGetEmmPID(sc) & 0x1FFF) == 0x1FFF)
	{
		PidSetEmmPID(sc, (pid & 0x1FFF));
		FilterEnableEmmFilter(sc, 1);
		return 1;
	}

	if ( (PidGetEcmPID(sc) & 0x1FFF) == 0x1FFF)
	{
		PidSetEcmPID(sc, (pid & 0x1FFF));
		FilterEnableEcmFilter(sc, 1);
		return 1;
	}
	return -1;
#endif
}

/*----------------------------------------------------------------*/
static int 
RemoveHwPID(struct adapter *sc, u_int32_t pid)
{
#if 1
	int i;

	DBG("pid=%d\n", pid);

	if (pid <= 0x1f)
			return 1;
	
	if (pid != 0x2000)
		for (i = 0; i < sc->UseableHwFilters; i++)
		{
			DBG("pid=%d searching slot=%d\n", pid, i);
			if (sc->HwPids[i] == pid)
			{
				DBG("pid=%d slot=%d\n", pid, i);
				sc->HwPids[i] = 0x1fff;
				PidSetHwPid(sc, i, 0x1fff);
				FilterEnableHwFilter(sc, i, 0);
				return 1;
			}
		}

	DBG("pid=%d whole bandwidth\n", pid);
	WholeBandwidthDec(sc);
	return 1;
			

#else
	DBG("pid=%d\n" , pid);
	if (pid < 0x1F) return 1;

	if ( (PidGetStream1PID(sc) & 0x1FFF) == pid)
	{
		PidSetStream1PID(sc, 0x1FFF);
		return 1;
	}

	if ( (PidGetStream2PID(sc) & 0x1FFF) == pid)
	{
		PidSetStream2PID(sc, 0x1FFF);
		FilterEnableStream2Filter(sc, 0);
		return 1;
	}
	
	if ( (PidGetPcrPID(sc) & 0x1FFF) == pid)
	{
		PidSetPcrPID(sc, 0x1FFF);
		FilterEnablePcrFilter(sc, 0);
		return 1;
	}

	if ( (PidGetPmtPID(sc) & 0x1FFF) == pid)
	{
		PidSetPmtPID(sc, 0x1FFF);
		FilterEnablePmtFilter(sc, 0);
		return 1;
	}

	if ( (PidGetEmmPID(sc) & 0x1FFF) == pid)
	{
		PidSetEmmPID(sc, 0x1FFF);
		FilterEnableEmmFilter(sc, 0);
		return 1;
	}

	if ( (PidGetEcmPID(sc) & 0x1FFF) == pid)
	{
		PidSetEcmPID(sc, 0x1FFF);
		FilterEnableEcmFilter(sc, 0);
		return 1;
	}

	return -1;
#endif
}

/*----------------------------------------------------------------*/
static int 
AddPID(struct adapter *sc, u_int32_t pid)
{
	int i;

	DBG("pid=%d\n" , pid);
#if 1
	if (pid > 0x1ffe && pid != 0x2000)
			return -1;

	for (i = 0; i < sc->PidCount; i++)
		if (sc->PidList[i] == pid)
		{
			sc->PidRc[i]++;
			return 1;
		}

	if (sc->PidCount == nPidSlots)
		return -1;

	sc->PidList[sc->PidCount] = pid;
	sc->PidRc[sc->PidCount] = 1;
	sc->PidCount++;
	AddHwPID(sc, pid);
	return 1;
#else
	if (pid > 0x1FFE) return -1;

	if (CheckPID(sc, pid) == 1) return 1;

	for (i = 0; i < 0x27; i++)
	{
		if (sc->pids[i] == 0x1FFE)
		{
		    sc->pids[i] = pid;
		    if (AddHwPID(sc, pid) < 0) OpenWholeBandwidth(sc);
		    return 1;
		}
	}
	return -1;
#endif
}

/*----------------------------------------------------------------*/
static int 
RemovePID(struct adapter *sc, u_int32_t pid)
{
	int i;

	DBG("pid=%d\n" , pid);
#if 1
	if (pid > 0x1ffe && pid != 0x2000)
			return -1;

	for (i = 0; i < sc->PidCount; i++)
		if (sc->PidList[i] == pid)
		{
			sc->PidRc[i]--;
			if (sc->PidRc[i] <= 0)
			{
				sc->PidCount--;
				sc->PidList[i] = sc->PidList[sc->PidCount];
				sc->PidRc[i] = sc->PidRc[sc->PidCount];
				RemoveHwPID(sc, pid);
			}
			return 1;
		}

	return -1;
#else
	if (pid > 0x1FFE) return -1;
	
	for (i = 0; i < 0x27; i++)
	{
		if (sc->pids[i] == pid)
		{
		    sc->pids[i] = 0x1FFE;
		    RemoveHwPID(sc, pid);
		    return 1;
		}
	}
	return -1;
#endif
}


/*-----------------------------------------------------------------
-		new PIDs
-------------------------------------------------------------------*/
static void
InitPids(struct adapter *sc)
{
	int i;

	sc->PidCount = 0;
	sc->WholeBandWidthCount = 0;
	for (i = 0; i < sc->UseableHwFilters; i++)
	{
		DBG("setting filter %d to 0x1fff\n", i);
		sc->HwPids[i] = 0x1fff;
		PidSetHwPid(sc, i, 0x1fff);
	}

	PidSetGroupPID(sc, 0);
	PidSetGroupMASK(sc, 0x1fe0);
}


/* ===============================================================
			DMA & IRQ
   ===============================================================*/

/*----------------------------------------------------------------*/
static void 
CtrlEnableSmc(struct adapter *sc, int op)
{
	if (op == 0)
		WriteRegOp(sc, 0x208, 2, ~0x00000800, 0);
	else
		WriteRegOp(sc, 0x208, 1, 0, 0x00000800);
}

/*----------------------------------------------------------------*/
static u_int32_t
DmaEnableDisableIrq(struct adapter *sc, int flag1, int flag2, int flag3)
{
	sc->dma_ctrl &= 0x000F0000;

	if (flag1 == 0)
	{
		if (flag2 == 0)
			sc->dma_ctrl &= ~0x00010000;
		else	sc->dma_ctrl |=  0x00010000;

		if (flag3 == 0)
			sc->dma_ctrl &= ~0x00020000;
		else	sc->dma_ctrl |=  0x00020000;
	} else {
		if (flag2 == 0)
			sc->dma_ctrl &= ~0x00040000;
		else	sc->dma_ctrl |=  0x00040000;

		if (flag3 == 0)
			sc->dma_ctrl &= ~0x00080000;
		else	sc->dma_ctrl |=  0x00080000;
	}
	return sc->dma_ctrl; 
}

/*----------------------------------------------------------------*/
static u_int32_t
IrqDmaEnableDisableIrq(struct adapter *sc, int op)
{
	u_int32_t val;

	DBG("\n" );
	val = read_reg(sc, 0x208) & 0xFFF0FFFF;
	if (op != 0) val = val | (sc->dma_ctrl & 0x000F0000);
	write_reg(sc, 0x208, val);
	return val;
}

/*=============================================================================
= FlexCopII has 2 dma channels. DMA1 is used to transfer TS data to
= system memory.
=
= The DMA1 buffer is divided in 2 subbuffers of equal size.
= FlexCopII will transfer TS data to one subbuffer, signal an interrupt
= when the subbuffer is full and continue fillig the second subbuffer.
=
= For DMA1:
=	subbuffer size in 32-bit words is stored in the first 24 bits of
=	register 0x004. The last 8 bits of register 0x004 contain the number
=	of subbuffers.
=
=	the first 30 bits of register 0x000 contain the address of the first
=	subbuffer. The last 2 bits contain 0, when dma1 is disabled and 1,
=	when dma1 is enabled.
=
=	the first 30 bits of register 0x00C contain the address of the second
=	subbuffer. the last 2 bits contain 1.
=
=	register 0x008 will contain the address of the subbuffer that was filled
=	with TS data, when FlexCopII will generate an interrupt.
=
= For DMA2:
=	subbuffer size in 32-bit words is stored in the first 24 bits of
=	register 0x014. The last 8 bits of register 0x004 contain the number
=	of subbuffers.
=
=	the first 30 bits of register 0x010 contain the address of the first
=	subbuffer. The last 2 bits contain 0, when dma1 is disabled and 1,
=	when dma1 is enabled.
=
=	the first 30 bits of register 0x01C contain the address of the second
=	subbuffer. the last 2 bits contain 1.
=
=	register 0x018 will contain the address of the subbuffer that was filled
=	with TS data, when FlexCopII will generate an interrupt.
=
==============================================================================*/

/*----------------------------------------------------------------*/
static int
DmaInitDMA(struct adapter *sc, u_int32_t dma_channel)
{
	u_int32_t subbuff, subbufsize, subbuf0, subbuf1;

	DBG("\n" );
	if (dma_channel == 0)
	{
		DBG("Init DMA1 channel\n" );
	
		subbuff = 2;
		subbufsize = ( ((sc->DmaQ1.size/2)/4) << 8) | subbuff;
		subbuf0 = sc->DmaQ1.addr & 0xFFFFFFFC;
		subbuf1 = ((sc->DmaQ1.addr + sc->DmaQ1.size/2) & 0xFFFFFFFC ) | 1;
		DBG("first subbuff addr=0x%x\n" , subbuf0);
		DELAY(1000);
		write_reg(sc, 0x000, subbuf0);

		DBG("subbuff size = 0x%x\n", (subbufsize >> 8)*4 );
		DELAY(1000);
		write_reg(sc, 0x004, subbufsize);
		
		DBG("second subbuff addr=0x%x\n", subbuf1);
		DELAY(1000);
		write_reg(sc, 0x00C, subbuf1);
		
		DBG("counter = 0x%x\n", sc->DmaQ1.addr & 0xFFFFFFFC );
		write_reg(sc, 0x008, sc->DmaQ1.addr & 0xFFFFFFFC);
		DELAY(1000);

		if (subbuff == 0) DmaEnableDisableIrq(sc, 0, 1, 0);
			     else DmaEnableDisableIrq(sc, 0, 1, 1);	

		IrqDmaEnableDisableIrq(sc, 1);

		SRAMSetMediaDest(sc, 1);
		SRAMSetNetDest(sc, 1);
		SRAMSetCaiDest(sc, 2);
		SRAMSetCaoDest(sc, 2);
	}

	if (dma_channel == 1)
	{
		DBG("Init DMA2 channel\n" );
	
		subbuff = 2;
		subbufsize = ( ((sc->DmaQ2.size/2)/4) << 8) | subbuff;
		subbuf0 = sc->DmaQ2.addr & 0xFFFFFFFC;
		subbuf1 = ((sc->DmaQ2.addr + sc->DmaQ2.size/2) & 0xFFFFFFFC ) | 1;
		DBG("first subbuff addr=0x%x\n", subbuf0);
		DELAY(1000);
		write_reg(sc, 0x010, subbuf0);
		DBG("subbuff size = 0x%x\n", (subbufsize >> 8)*4 );

		DELAY(1000);
		write_reg(sc, 0x014, subbufsize);
		
		DBG("second subbuff addr=0x%x\n", subbuf1);
		DELAY(1000);
		write_reg(sc, 0x01C, subbuf1);

		DBG("counter = 0x%x\n", sc->DmaQ2.addr & 0xFFFFFFFC );
		write_reg(sc, 0x018, sc->DmaQ2.addr & 0xFFFFFFFC);
		DELAY(1000);

		SRAMSetCaiDest(sc, 2);
	}
	return 0;
}

/*----------------------------------------------------------------*/
static void 
CtrlEnableReceiveData(struct adapter *sc, u_int32_t op)
{
	if (op == 0)
	{
		WriteRegOp(sc, 0x208, 2, ~0x00008000, 0);
		sc->dma_status = sc->dma_status & ~0x00000004;
	} else {
		WriteRegOp(sc, 0x208, 1, 0, 0x00008000);
		sc->dma_status = sc->dma_status | 0x00000004;
	}
}

/*========================================================================
= bit 0 of dma_mask is set to 1 if dma1 channel has to be enabled/disabled
= bit 1 of dma_mask is set to 1 if dma2 channel has to be enabled/disabled
=========================================================================*/
/*----------------------------------------------------------------*/
static void 
DmaStartStop0x2102(struct adapter *sc, u_int32_t dma_mask, u_int32_t start_stop)
{
	int dma_enable, dma1_enable, dma2_enable;

	DBG("dma_mask=%x\n", dma_mask);

	if (start_stop == 1)
	{
		DBG("Starting dma\n");
		dma1_enable = 0;
		dma2_enable = 0;
		
		if ( ((dma_mask & 1) != 0) && 
		     ((sc->dma_status & 1) == 0) && (sc->DmaQ1.addr != 0) )
		{
			sc->dma_status = sc->dma_status | 1;
			dma1_enable = 1;
		}

		if ( ((dma_mask & 2) != 0) && 
		     ((sc->dma_status & 2) == 0) && (sc->DmaQ2.addr != 0) )
		{
			sc->dma_status = sc->dma_status | 2;
			dma2_enable = 1;
		}

		if ( (dma1_enable == 1) && (dma2_enable == 1))
		{
			write_reg(sc, 0x000, sc->DmaQ1.addr | 1);
			write_reg(sc, 0x00C, (sc->DmaQ1.addr + sc->DmaQ1.size/2) | 1);
			write_reg(sc, 0x010, sc->DmaQ2.addr | 1);
			/* */
			CtrlEnableReceiveData(sc, 1);
			return;
		}

		if ( (dma1_enable == 1) && (dma2_enable == 0))
		{
			write_reg(sc, 0x000, sc->DmaQ1.addr | 1);
			write_reg(sc, 0x00C, (sc->DmaQ1.addr + sc->DmaQ1.size/2) | 1);
			CtrlEnableReceiveData(sc, 1);
			return;
		}

		if ( (dma1_enable == 0) && (dma2_enable == 1))
		{
			write_reg(sc, 0x010, sc->DmaQ2.addr | 1);
			/* */
			CtrlEnableReceiveData(sc, 1);
			return;
		}

		if ( (dma1_enable == 0) && (dma2_enable == 0))
		{
			CtrlEnableReceiveData(sc, 1);
			return;
		}
	} else {
		DBG("Stoping dma\n");

		dma_enable = sc->dma_status & 0x00000003;

		if ( ((dma_mask & 1) != 0) && ((sc->dma_status & 1) != 0) )
			dma_enable = dma_enable & 0xFFFFFFFE;
		if ( ((dma_mask & 2) != 0) && ((sc->dma_status & 2) != 0) )
			dma_enable = dma_enable & 0xFFFFFFFD;

		if ( (dma_enable == 0) && ((sc->dma_status & 4) != 0) )
		{
			CtrlEnableReceiveData(sc, 0);
			DELAY(3000);
		}

		if ( ((dma_mask & 1) != 0) && 
		     ((sc->dma_status & 1) != 0) && (sc->DmaQ1.addr != 0) ){
		write_reg(sc, 0x000, sc->DmaQ1.addr);
		write_reg(sc, 0x00C, (sc->DmaQ1.addr + sc->DmaQ1.size/2) | 1);
		sc->dma_status = sc->dma_status & ~0x00000001;
		}

		if ( ((dma_mask & 2) != 0) && 
		     ((sc->dma_status & 2) != 0) && (sc->DmaQ2.addr != 0) ){
		write_reg(sc, 0x010, sc->DmaQ2.addr);
		/* */
		sc->dma_status = sc->dma_status & ~0x00000002;
		}
	}
}

/*----------------------------------------------------------------*/
static void 
OpenStream(struct adapter *sc, u_int32_t pid)
{
	u_int32_t dma_mask;

	++sc->capturing;

	FilterEnableMaskFilter(sc, 1);

	AddPID(sc, pid);

	DBG("dma_status=%x\n", sc->dma_status);

	if ( (sc->dma_status & 7) != 7 )
	{
		dma_mask = 0;
		if ( ((sc->dma_status & 0x10000000) != 0) &&
		     ((sc->dma_status & 1) == 0) ){
			dma_mask |= 1;
			sc->DmaQ1.head  = 0;
			sc->DmaQ1.tail  = 0;
			memset(sc->DmaQ1.buf, 0, sc->DmaQ1.size);
		}
		
		if ( ((sc->dma_status & 0x20000000) != 0) &&
		     ((sc->dma_status & 2) == 0) ){
			dma_mask |= 2;
			sc->DmaQ2.head  = 0;
			sc->DmaQ2.tail  = 0;
			memset(sc->DmaQ2.buf, 0, sc->DmaQ2.size);
		}
		
		if (dma_mask != 0)
		{
			IrqDmaEnableDisableIrq(sc, 1);
			DmaStartStop0x2102(sc, dma_mask, 1);
		}		
	}
}

/*----------------------------------------------------------------*/
static void 
CloseStream(struct adapter *sc, u_int32_t pid)
{
	u_int32_t dma_mask;

	if (sc->capturing > 0) --sc->capturing;
	DBG("dma_status=%x cap=%d\n", sc->dma_status, sc->capturing);

	if (sc->capturing == 0)
	{
		dma_mask = 0;
		if ((sc->dma_status & 1) != 0) dma_mask |= 0x00000001;
		if ((sc->dma_status & 2) != 0) dma_mask |= 0x00000002;

		if (dma_mask != 0)
			DmaStartStop0x2102(sc, dma_mask, 0);
	}
	RemovePID(sc, pid);
}

/*----------------------------------------------------------------*/
static void
reset300Block(struct adapter *sc)
{
	u_int32_t reg_300, reg_304, reg_308, reg_30C, reg_310, reg_208;
	DBG2("\n");

	reg_300 = read_reg(sc, 0x300);
	reg_304 = read_reg(sc, 0x304);
	reg_308 = read_reg(sc, 0x308);
	reg_30C = read_reg(sc, 0x30C);
	reg_310 = read_reg(sc, 0x310);

	/* ... */

	reg_208 = read_reg(sc, 0x208);
	write_reg(sc, 0x208, 0);
	write_reg(sc, 0x210, 0xB208);

	/* ... */

	write_reg(sc, 0x300, reg_300);
	write_reg(sc, 0x304, reg_304);
	write_reg(sc, 0x308, reg_308);
	write_reg(sc, 0x30C, reg_30C);
	write_reg(sc, 0x310, reg_310);

	write_reg(sc, 0x208, reg_208);
	
}

/*----------------------------------------------------------------*/
static void
HandleDataFreeze(struct adapter *sc)
{
	u_int32_t val_208, val_710n, val_710o;


	val_208 = read_reg(sc, 0x208);

	val_710o = read_reg(sc, 0x710);
	write_reg(sc, 0x208, 0x8000);
	val_710n = read_reg(sc, 0x710);

	write_reg(sc, 0x208, val_208);

	sc->HandleFreeze = 0;

	DBG2("0x710  (%d,%x =?= %d,%x)\n", 
			val_710o, val_710o, val_710n, val_710n);

	if (val_710o == val_710n)
	{ 
		DBG2("0x710  (%x =?= %x)\n", val_710o, val_710n);
		reset300Block(sc);
	}

}

/*----------------------------------------------------------------*/
static void
DetectAndHandleDataFreeze(struct adapter *sc, int DmaCounter)
{

	if (sc->DmaCounter == DmaCounter)
	{
		sc->HandleFreeze ++;
		if (sc->HandleFreeze > 3000)
			HandleDataFreeze(sc);
	} else {
		sc->DmaCounter = DmaCounter;
		sc->HandleFreeze = 0;
	}
}

/*----------------------------------------------------------------*/
static void
ResetComplete(void *xsc)
{
	struct adapter *sc;
	int s, nsec;

	sc = xsc;
	s = splhigh();

	sc->ResetTimeout++; 
	if (sc->ResetTimeout == 4)
	{
		HandleDataFreeze(sc);
		sc->ResetTimeout = 0;
		nsec = 5000;	/* 50sec */
	} else {
		nsec = 5000/(sc->ResetTimeout + 1);
	}

	DBG2("%d\n", nsec);
	sc->timeout_isr = timeout(ResetComplete, sc, nsec); /* sec */

	splx(s);
}

/*----------------------------------------------------------------*/
static int
GetDmaCounter(struct adapter *sc, int ndma)
{
	int counter;

	counter = read_reg(sc, 0x008) - sc->DmaQ1.addr;

	return counter;
}

/*----------------------------------------------------------------*/
static int
InterruptServiceDMA1(struct adapter *sc)
{
	struct dvb_demux *dvbdmx = &sc->demux;

	int nCurDmaCounter;
	u_int32_t nNumBytesParsed;
	u_int32_t nNumNewBytesTransferred;
	u_int32_t dwDefaultPacketSize = 188;
	u_int8_t gbTmpBuffer[188];
	u_int8_t *pbDMABufCurPos;

	DBG2("\n");

	sc->ResetTimeout = 0;

	nCurDmaCounter = GetDmaCounter(sc, 1);

	DetectAndHandleDataFreeze(sc, nCurDmaCounter);

	nCurDmaCounter = (nCurDmaCounter / dwDefaultPacketSize) * dwDefaultPacketSize;
	if ((nCurDmaCounter < 0) || (nCurDmaCounter > sc->DmaQ1.size)){
		DBG("dma counter outside dma buffer!!!\n");
		return 1;
	}

	sc->DmaQ1.head = nCurDmaCounter;

	if (sc->DmaQ1.tail <= nCurDmaCounter){
		nNumNewBytesTransferred = nCurDmaCounter - sc->DmaQ1.tail;
	} else {
		nNumNewBytesTransferred = (sc->DmaQ1.size - sc->DmaQ1.tail) + nCurDmaCounter;
	}
#if 0
	DBG2("nCurDmaCounter   = %d\n"  , nCurDmaCounter);
	DBG2("DmaQ1.tail       = %d\n"  , sc->DmaQ1.tail);
	DBG2("BytesTransferred = %d\n"  , nNumNewBytesTransferred);
#endif
	if (nNumNewBytesTransferred <  dwDefaultPacketSize)
		return 0;

	nNumBytesParsed  = 0;

	while (nNumBytesParsed < nNumNewBytesTransferred)
	{
		pbDMABufCurPos  = sc->DmaQ1.buf + sc->DmaQ1.tail;	
	 	if (sc->DmaQ1.buf + sc->DmaQ1.size < 
		    sc->DmaQ1.buf + sc->DmaQ1.tail + 188)
		{

			memcpy(gbTmpBuffer, 
				sc->DmaQ1.buf + sc->DmaQ1.tail, 
				sc->DmaQ1.size - sc->DmaQ1.tail);

		 	memcpy(gbTmpBuffer +
			(sc->DmaQ1.size - sc->DmaQ1.tail),
			 sc->DmaQ1.buf,
			(188 - (sc->DmaQ1.size - sc->DmaQ1.tail)) );	

			pbDMABufCurPos  = gbTmpBuffer;
		}

		if (sc->capturing != 0)
		{
			u_int8_t *dq = (u_int8_t *)pbDMABufCurPos;
			size_t dwCount = dwDefaultPacketSize / 188;
			dvb_dmx_swfilter_packets(dvbdmx, dq, dwCount);
		}

		nNumBytesParsed += dwDefaultPacketSize;
		sc->DmaQ1.tail += dwDefaultPacketSize;

		if (sc->DmaQ1.tail >= sc->DmaQ1.size)
			sc->DmaQ1.tail -= sc->DmaQ1.size;
	}	
	return 1;
}

/*----------------------------------------------------------------*/
static void
InterruptServiceDMA2(struct adapter *sc)
{
	DBG2("\n");
}

/*----------------------------------------------------------------*/
static void
isr(void *arg)
{
	struct adapter *sc = (struct adapter *)arg;
	u_int32_t val;
	
	DBG2("\n");

	while (((val = read_reg(sc, 0x20C)) & 0x0F) != 0)
	{
		if ((val & 0x03) != 0)
		{
			 InterruptServiceDMA1(sc);
		}
		if ((val & 0x0C) != 0)
		{
			 InterruptServiceDMA2(sc);
		}
	}
}

/*----------------------------------------------------------------*/
static void 
InitDmaQueue(struct adapter *sc)
{
	
	sc->dma_status = 0;

	if (sc->DmaQ1.buf != 0)
		return;

	sc->DmaQ1.head	 = 0;
	sc->DmaQ1.tail	 = 0;
	sc->DmaQ1.buf	 = 0;
	
	sc->DmaQ1.buf = contigmalloc(SizeOfBufDMA1+0x80, 
			M_DEVBUF, M_NOWAIT /*| M_ZERO*/, 0, ~0, PAGE_SIZE, 0); 

	if (sc->DmaQ1.buf != 0){
		bzero(sc->DmaQ1.buf, SizeOfBufDMA1);

		sc->DmaQ1.addr = vtophys((vm_offset_t)sc->DmaQ1.buf);
		sc->DmaQ1.size = SizeOfBufDMA1;

		DmaInitDMA(sc, 0);

		sc->dma_status = sc->dma_status | 0x10000000;

		DBG("allocated dma buffer at 0x%x, length=%d\n",
			(int)sc->DmaQ1.buf, SizeOfBufDMA1); 
	} else {
		DBG("error alloc dma1 buffer\n");
		sc->dma_status = sc->dma_status & ~0x10000000;
	}

	if (sc->DmaQ2.buf != 0)
		return;

	sc->DmaQ2.head	 = 0;
	sc->DmaQ2.tail	 = 0;
	sc->DmaQ2.buf	 = 0;
	
	sc->DmaQ2.buf = contigmalloc(SizeOfBufDMA2 + 0x80, 
			M_DEVBUF, M_NOWAIT /*| M_ZERO*/, 0, ~0, PAGE_SIZE, 0);

	if (sc->DmaQ2.buf != 0){
		bzero(sc->DmaQ2.buf, SizeOfBufDMA2);

		sc->DmaQ2.addr = vtophys((vm_offset_t)sc->DmaQ2.buf);
		sc->DmaQ2.size = SizeOfBufDMA2;

		DmaInitDMA(sc, 1);

		sc->dma_status = sc->dma_status | 0x20000000;

		DBG("allocated dma buffer at 0x%x, length=%d\n",
			(int)sc->DmaQ2.buf, SizeOfBufDMA2); 
	} else {
		DBG("error alloc dma2 buffer\n");
		sc->dma_status = sc->dma_status & ~0x20000000;
	}
}

/*----------------------------------------------------------------*/
static void 
FreeDmaQueue(struct adapter *sc)
{

	if (sc->DmaQ1.buf != 0){
		contigfree(sc->DmaQ1.buf, 
			SizeOfBufDMA1+0x80, M_DEVBUF);
		sc->DmaQ1.addr		= 0;
		sc->DmaQ1.head		= 0;
		sc->DmaQ1.tail		= 0;
		sc->DmaQ1.size		= 0;
		sc->DmaQ1.buf		= 0;
	}

	if (sc->DmaQ2.buf != 0){
		contigfree(sc->DmaQ1.buf, 
			SizeOfBufDMA2+0x80, M_DEVBUF);
		sc->DmaQ2.addr		= 0;
		sc->DmaQ2.head		= 0;
		sc->DmaQ2.tail		= 0;
		sc->DmaQ2.size		= 0;
		sc->DmaQ2.buf		= 0;
	}
}

/* ----------------------------------------------------------------*/
static int
dvb_start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;
	struct adapter *sc = (struct adapter *)dvbdmx->priv;

	DBG("PID=%d type=%d\n", dvbdmxfeed->pid, dvbdmxfeed->type);
	OpenStream(sc, dvbdmxfeed->pid);
	return 0;
}

/* ----------------------------------------------------------------*/
static int
dvb_stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;
	struct adapter *sc = (struct adapter *)dvbdmx->priv;

	DBG("PID=%d type=%d\n", dvbdmxfeed->pid, dvbdmxfeed->type);
	CloseStream(sc, dvbdmxfeed->pid);
	return 0;
}

/* --------------------------------------------------------------- */
char *
GetTunerName(unsigned char Id)
{
        switch (Id)
        {
        case 150:
                return "CX24113_24123";

        case 430:
        case 431:
        case 432:
        case 433:
                return "ATSC TDVS_H061F NIM";

        case 140:
        case 141:
        case 142:
        case 143:
                return "ITD1000_PN1010";

        case 420:
                return "ATSC Samsung Nim witn Nxt2002 Demod";

        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
                return "Mitel QPSK Tuner";

        case 200:
        case 201:
        case 202:
        case 210:
        case 211:
                return "Grundig LSI64781 NIM";

        case 310:
        case 311:
        case 312:
        case 320:
        case 321:
                return "QAM ST-STV0297 with MicroTune-MT2030";


        case 330:
        case 331:
        case 332:
        case 333:
                return "QAM ST-STV0297 with Alps Tuner";

        case 334:
                return "QAM ST-STV0297 with Towner TNC5803A Tuner";

        case 110:
        case 111:
        case 112:
        case 113:
        case 114:
        case 115:
        case 120:
        case 121:
        case 122:
        case 123:
        case 130:
        case 131:
        case 132:
        case 133:
        case 134:
        case 135:
        case 136:
        case 137:
                return "SamsungSTV0299NIM";

        case 400:
        case 410:
                return "Broadcom BCM3510 with Panasonic Tuner";

        case 300:
                return "SAMSUNG TCMU30311PSB Tuner";

        case 220:
        case 221:
        case 222:
        case 230:
        case 231:
        case 232:
        case 233:

        case 224:
        case 225:
                return "Samsung MT352 NIM 78";

        default:
                return "Unknown Tuner!!!";
        }
}

/*================================================================
=
=
=
==================================================================*/

/*----------------------------------------------------------------*/
static int
skystar2_probe(device_t dev)
{
	if ((pci_get_vendor(dev) == 0x13D0) && (pci_get_device(dev) == 0x2103))
	{
		device_set_desc(dev, "B2C2 Broadband Receiver PCI Adapter (FCII)");
		return 0;
	}

#if (__FreeBSD_version < 500000)
	return ENXIO;
#else
	return EIO;
#endif
}

/*----------------------------------------------------------------*/
static int
skystar2_attach(device_t dev)
{
 	int s;
	u_int32_t val;
	struct adapter *sc;
	struct dvb_demux	*dvbdemux;
	unsigned char tunerInfo[64];
	int 	rid, unit, error = 0;

	DBG("\n");
	s = splimp();
	unit = device_get_unit(dev);
	sc = device_get_softc(dev);

	if (sc == NULL)
	{
		device_printf(dev, "no memory for softc struct!\n");
		error = ENXIO;
		goto fail;
	}

	bzero(sc, sizeof(struct adapter));
	sc->unit = unit;

	/*
	 * Map Control/Status register 
	*/
#if 1
	val = pci_read_config(dev, PCIR_COMMAND, 4);
	val |= (PCIM_CMD_MEMEN  | PCIM_CMD_BUSMASTEREN);
	pci_write_config(dev, PCIR_COMMAND, val /* & 0x000000ff*/, 4);
#else
	pci_enable_busmaster(dev);
	//pci_enable_io(dev, SYS_RES_IOPORT);
	pci_enable_io(dev, SYS_RES_MEMORY);
#endif
	val = pci_read_config(dev, PCIR_COMMAND, 4);

	if (!(val & PCIM_CMD_PORTEN) && !(val & PCIM_CMD_MEMEN))
	{
		device_printf(dev, "failed to enable I/O ports and memory mapping!\n");
		error = ENXIO;
		goto fail_mem;
	}

#if (__FreeBSD_version < 500000)
	rid = PCI_MAP_REG_START; /*0x14*/
#else
	rid = PCIR_BAR(0);
#endif
	sc->res_mem = bus_alloc_resource(dev, SYS_RES_MEMORY, &rid, 
					0, ~0, 1, RF_ACTIVE);

	if (sc->res_mem == NULL)
	{
		device_printf(dev, "failed to alloc resource ports\n");
		error = ENXIO;
		goto fail_mem;
	}

	IrqDmaEnableDisableIrq(sc, 0);

	rid = 0;
	sc->res_irq = bus_alloc_resource(dev, SYS_RES_IRQ, &rid, 
					0 ,~0, 1, RF_SHAREABLE | RF_ACTIVE);
	if (sc->res_irq == NULL)
	{
		device_printf(dev, "failed to alloc resource interrupt\n");
		error = ENXIO;
		goto fail_irq;
	}
	
	error = bus_setup_intr(dev, sc->res_irq, INTR_TYPE_NET,
#if (__FreeBSD_version >= 700000)
			NULL,
#endif
			isr, sc, &sc->res_ih);
	if (error)
	{
		device_printf(dev, "filed setup isr handler\n");
		goto fail_isr;
	}

	
	// >= 0x2103
	write_reg(sc, 0x208, 0);	/* disable Irq */
	write_reg(sc, 0x210, 0xB2FF); /* reset FlexCop chip */

	// SLL_get_FLEX_rev  read_reg	
	sc->rev = (read_reg(sc, 0x204) >> 0x18);

	switch(sc->rev)
	{
	case 0x82:	printf("FlexCopII(rev.130) chip found\n"); break;
	case 0xC3:	printf("FlexCopIIB(rev.195) chip found\n"); break;
	default:	printf("FlexCop unknown chip rev %d\n", sc->rev);
			error = ENXIO;
			goto fail_init;
	}

	/* begin SLL_PLUS_init */

	/* begin SSL_SMC_init */	
	/* end SSL_SMC_init */

	/* begin SLL_Tuner_Init */
	val = read_reg(sc, 0x204);
	
	write_reg(sc, 0x204, 0);
	DELAY(20000);

	write_reg(sc, 0x204, val);
	DELAY(10000);
	/* end SLL_Tuner_Init */


	sc->dwSramType = 0x10000;

	setb_reg(sc, 0x308, 0x4000);

	SLL_detectSramSize(sc);

	/* end SLL_PLUS_init */

#if 1
	sc->UseableHwFilters = 6;

	InitPids(sc);
#else
	InitPIDsInfo(sc);

	PidSetGroupPID(sc, 0);
	PidSetGroupMASK(sc, 0x1FE0);

	PidSetStream1PID(sc, 0x1FFF);
	PidSetStream2PID(sc, 0x1FFF);

	PidSetPmtPID(sc, 0x1FFF);
	PidSetPcrPID(sc, 0x1FFF);
	PidSetEcmPID(sc, 0x1FFF);
	PidSetEmmPID(sc, 0x1FFF);

	//FilterEnableNullFilter(sc, 1);
	//FilterEnableStream1Filter(sc, 1);
#endif

	InitDmaQueue(sc);

	if ((sc->dma_status & 0x30000000) != 0x30000000)
	{
		printf("can't alloc DMA memory\n");
		error = ENXIO;
		goto fail_dma;
	}

	//SllHwGetMAC   - EEPROM_getMacAddr(sc, 0, sc->eaddr)

	//InitializeTuner

	// CtrlEnableSmc(sc, x);

	SRAMSetMediaDest(sc, 1);
	SRAMSetNetDest(sc, 1);
	
	CtrlEnableSmc(sc, 0); 

	SRAMSetCaiDest(sc, 2);
	SRAMSetCaoDest(sc, 2);

	DmaEnableDisableIrq(sc, 1, 0, 0);

	if (EEPROM_getMacAddr(sc, 0, sc->eaddr) != 0)
	{
		printf("MAC address : %6D\n", sc->eaddr, ":");

		CASetMacDstAddrFilter(sc, sc->eaddr);
		CtrlEnableMAC(sc, 1);
	}

	memset(tunerInfo, 0, 64);

	EEPROM_readTunerInfo(sc, tunerInfo);

	printf("Detected tuner: id=%x, '%s'\n", tunerInfo[0], GetTunerName(tunerInfo[0]));

	if (fe_start(sc) < 0)
	{
		printf("Unsupported frontend !!!\n");
		error = ENXIO;
		goto fail_frontend;
	}


	dvbdemux = &sc->demux;
	dvbdemux->priv = (void *)sc;
	dvbdemux->nfilter = nPidSlots;
	dvbdemux->nfeed = nPidSlots;
	dvbdemux->start_feed = dvb_start_feed;
	dvbdemux->stop_feed = dvb_stop_feed;

	dvb_dmx_init(&sc->demux);

	dvb_net_init(sc, &sc->dvbnet, &dvbdemux->dmx);

	dvb_dev_init(&sc->dvbdev, device_get_unit(dev));

	/* .... */

	callout_handle_init(&sc->timeout_isr);
	sc->timeout_isr = timeout(ResetComplete, sc, 3000); /* 30sec */

	splx(s);
	return 0;

fail_frontend:
	/* ... */
	CloseStream(sc, 0);

fail_dma:
	FreeDmaQueue(sc);

fail_init:
	if (sc->res_ih)
	bus_teardown_intr(dev, sc->res_irq, sc->res_ih);

fail_isr:
	if (sc->res_irq)
	bus_release_resource(dev, SYS_RES_IRQ, 0, sc->res_irq);

fail_irq:
	if (sc->res_mem)
#if (__FreeBSD_version < 500000)
	bus_release_resource(dev, SYS_RES_MEMORY, PCI_MAP_REG_START, sc->res_mem);
#else
	rid = PCIR_BAR(0);
	bus_release_resource(dev, SYS_RES_MEMORY, rid, sc->res_mem);
#endif

fail_mem:
fail:
	splx(s);
	return (error);

}

/*----------------------------------------------------------------*/
static int
skystar2_detach(device_t dev)
{
	struct adapter *sc;
	int	s;
	int	rid;

	DBG("\n");

	s = splimp();
	sc = device_get_softc(dev);

	untimeout(ResetComplete, sc, sc->timeout_isr);

 	IrqDmaEnableDisableIrq(sc, 0);
	CtrlEnableReceiveData(sc, 0);

	fe_stop(sc);


	dvb_dev_release(&sc->dvbdev);
	dvb_net_release(&sc->dvbnet);
	dvb_dmx_release(&sc->demux);

	CloseStream(sc, 0);
	FreeDmaQueue(sc);

	if (sc->res_ih)
	bus_teardown_intr(dev, sc->res_irq, sc->res_ih);

	if (sc->res_irq)
	bus_release_resource(dev, SYS_RES_IRQ, 0, sc->res_irq);

	if (sc->res_mem)
#if (__FreeBSD_version < 500000)
	bus_release_resource(dev, SYS_RES_MEMORY, PCI_MAP_REG_START, sc->res_mem);
#else
	rid = PCIR_BAR(0);
	bus_release_resource(dev, SYS_RES_MEMORY, rid, sc->res_mem);
#endif
	splx(s);

	return 0;
}

/*----------------------------------------------------------------*/
static int
skystar2_shutdown(device_t dev)
{
	struct adapter *sc;
	
	DBG("\n");
	sc = device_get_softc(dev);

 	IrqDmaEnableDisableIrq(sc, 0);
	CtrlEnableReceiveData(sc, 0);

	fe_stop(sc);

	/* ...*/

	return 0;
}

/*----------------------------------------------------------------*/
static int
skystar2_suspend(device_t dev)
{
	struct adapter *sc;

	DBG("\n");

	return 0;
}

/*----------------------------------------------------------------*/
static int
skystar2_resume(device_t dev)
{
	struct adapter *sc;

	DBG("\n");

	return 0;
}

