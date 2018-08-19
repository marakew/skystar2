
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <sys/bus.h>

#include <machine/bus.h>

#include "skystar2.h"
#include "sllutil.h"

void
write_reg(struct adapter *sc, u_int32_t reg, u_int32_t val)
{
	bus_space_write_4(rman_get_bustag(sc->res_mem), rman_get_bushandle(sc->res_mem), reg, val);
}

u_int32_t
read_reg(struct adapter *sc, u_int32_t reg)
{
	return bus_space_read_4(rman_get_bustag(sc->res_mem), rman_get_bushandle(sc->res_mem), reg);
}

