/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include "cobra.h"

#include "skystar2.h"
#include "frontend.h"
#include "i2c.h"
#include "cx24123_24113.h"

#include "debug.h"

static int debug = 0;
SYSCTL_INT(_debug, OID_AUTO, debug_fe, CTLFLAG_RW, &debug, 0,
	"enable verbose debug messages: 0-9");

/*----------------------------------------------------*/
#if 0
struct fe_ioctl_tab_t {
        u_int32_t       cmd;
        char            *desc;
} fe_ioctl_tab[] = {
{FE_GET_INFO,                   "FE_GET_INFO:"},
{FE_READ_STATUS,                "FE_READ_STATUS:"},
{FE_READ_SIGNAL_STRENGTH,       "FE_READ_SIGNAL_STRENGTH:"},
{FE_READ_SNR,                   "FE_READ_SNR:"},
{FE_SET_FRONTEND,               "FE_SET_FRONTEND:"},
{FE_GET_FRONTEND,               "FE_GET_FRONTEND:"},
{FE_SLEEP,                      "FE_SLEEP:"},
{FE_INIT,                       "FE_INIT:"},
{FE_RESET,                      "FE_RESET:"},
{FE_DISEQC_SEND_MASTER_CMD,     "FE_DISEQC_SEND_MASTER_CMD:"},
{FE_DISEQC_SEND_BURST,          "FE_DISEQC_SEND_BURST:"},
{FE_SET_TONE,                   "FE_SET_TONE:"},
{FE_SET_VOLTAGE,                "FE_SET_VOLTAGE:"}
};
#endif

/*----------------------------------------------------*/
int
fe_start(struct adapter *sc)
{
	struct fe_data *fe;
	struct dvb_frontend *frontend;
	struct dvb_frontend_chip *frontend_list;
	char name[15];
	int err;

	DBG("\n");

	if ((fe = (struct fe_data *)malloc(sizeof(struct fe_data),
				M_DEVBUF, M_NOWAIT)) == NULL){
		printf("error: can't alloc mem for fe_data\n");
		return -ENOMEM;
	}

	bzero(fe, sizeof(struct fe_data));

	sc->fe = fe;

	frontend = &fe->frontend;
	frontend->sc = sc;

	if (cx24123_attach(sc) != 0)
	{
		free(fe, M_DEVBUF);
		return -ENOMEM;
	}

	fe->exit = 0;

	fe->frontend.ioctl(&fe->frontend, FE_INIT, NULL);

	return 0;
}

/*----------------------------------------------------*/
void
fe_stop(struct adapter *sc)
{
	struct fe_data *fe = (struct fe_data *)sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;

	fe->exit = 1;
	frontend->detach(sc);
	free(fe, M_DEVBUF);
}

/*----------------------------------------------------*/
int
fe_ioctl(struct fe_data *fe, u_long cmd, caddr_t arg)
{
	struct dvb_frontend *frontend =  &fe->frontend;
	struct adapter *sc = (struct adapter *)frontend->sc;
	int err = -EOPNOTSUPP;
	int s;
	int i;

	DBG1("\n");

	if (!fe || fe->exit || !fe->frontend.ioctl)
		return -ENODEV;

#if 0
        char *name;
        name = "FE_UNSUPP:";
        for (i = 0; i < sizeof(fe_ioctl_tab) / sizeof(fe_ioctl_tab[0]); i++)
	{
                if (fe_ioctl_tab[i].cmd == cmd)
		{
                        name = fe_ioctl_tab[i].desc;
                        break;
                }
        }
        printf("%s: cmd %s (0x%x)\n", __FUNCTION__, name, cmd);
#endif

/*	s = splhigh();*/

        switch(cmd){

        case FE_DISEQC_SEND_MASTER_CMD:
		if (sc->diseqc_send_master_cmd)
		{
                	err = sc->diseqc_send_master_cmd(sc, (struct dvb_diseqc_master_cmd *) arg);
		}
		break;

        case FE_DISEQC_SEND_BURST:
		if (sc->diseqc_send_burst)
		{
                	err = sc->diseqc_send_burst(sc, *(fe_sec_mini_cmd_t *) arg);
		}
		break;
        case FE_SET_TONE:
		if (sc->set_tone)
		{
			err = sc->set_tone(sc, *(fe_sec_tone_mode_t *) arg);
		}
		break;

        case FE_SET_VOLTAGE:
		if (sc->set_voltage)
		{
                	err = sc->set_voltage(sc, *(fe_sec_voltage_t *) arg);
		}
		break;

        case FE_SET_FRONTEND:
		memcpy(&fe->parameters, (void *)arg,
			sizeof(struct dvb_frontend_parameters));

		err = fe->frontend.ioctl(&fe->frontend, FE_SET_FRONTEND, arg);

                break;

        case FE_GET_FRONTEND:
		memcpy((void *)arg, &fe->parameters,
			sizeof(struct dvb_frontend_parameters));
		err = 0;
		break;

        default:
		err = fe->frontend.ioctl(&fe->frontend, cmd, arg);
        }

/*	splx(s);*/
	return err;
}
