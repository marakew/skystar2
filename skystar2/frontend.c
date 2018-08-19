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

#include "skystar2.h"
#include "frontend.h"
#include "samsung_tbdu18132.h"
#include "samsung_tbmu24112.h"
#include "debug.h"

static int debug = 0;
SYSCTL_INT(_debug, OID_AUTO, debug_fe, CTLFLAG_RW, &debug, 0,
	"enable verbose debug messages: 0-9");

static	struct proc *feproc;

static struct dvb_frontend_chip dvb_frontend_list[] = 
{ {mt312_attach}, {stv0299_attach}, {NULL} };

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
static void
dvb_frontend_swzigzag_update_delay(int *quality, int *delay, int min_delay, int locked)
{
	int q2;
	if (locked)
		(*quality) = (*quality * 220 + 36*256) / 256;
	else	(*quality) = (*quality * 220 + 0) / 256;

	q2 = *quality - 128;
	q2 *= q2;
	*delay = min_delay + q2 * hz / (128*128);
}

/*----------------------------------------------------*/
static int
dvb_frontend_swzigzag_autotune(struct fe_data *fe)
{
	struct dvb_frontend *frontend = &fe->frontend;
	struct adapter *sc = frontend->sc;
	int stepsize;
	int maxdrift;
	int autoinversion;
	int ready = 0;
	int wrapped = 0;
	int original_inversion = fe->parameters.inversion;
	u_int32_t original_frequency = fe->parameters.frequency;

	DBG1("\n");

	if (fe->parameters.u.qpsk.symbol_rate < 10000000){
		stepsize = fe->parameters.u.qpsk.symbol_rate / 32000;
		maxdrift = 5000;
	} else {
		stepsize = fe->parameters.u.qpsk.symbol_rate / 16000;
		maxdrift = fe->parameters.u.qpsk.symbol_rate / 2000;
	}

	autoinversion = ((!(fe->info->caps & FE_CAN_INVERSION_AUTO)) &&
			 (fe->parameters.inversion == INVERSION_AUTO));

	while(!ready){
		/* wrap the count if we've reached the maximum drift */
		fe->lnb_drift = (fe->auto_step / 4) * stepsize;
		if (fe->lnb_drift >= maxdrift){
			fe->auto_step = 0;
			fe->lnb_drift = 0;
			wrapped = 1;
		}

		switch(fe->auto_step % 4){
		case 0:
			fe->inversion = INVERSION_OFF;
			ready = 1;
			break;
		case 1:
			if (!autoinversion)
				break;
			fe->inversion = INVERSION_ON;
			ready = 1;
			break;
		case 2:
			if (fe->lnb_drift == 0)
				break;
			fe->inversion = INVERSION_OFF;
			fe->lnb_drift = -fe->lnb_drift;
			ready = 1;
			break;
		case 3:
			if (fe->lnb_drift == 0)
				break;
			if (!autoinversion)
				break;
			fe->inversion = INVERSION_ON;
			fe->lnb_drift = -fe->lnb_drift;
			ready = 1;
			break;
		}
		if (!ready)
			fe->auto_step++;
	}

	fe->parameters.frequency += fe->lnb_drift + fe->bending;
	if (autoinversion)
		fe->parameters.inversion = fe->inversion;

	fe->frontend.ioctl(&fe->frontend, FE_SET_FRONTEND, (void *)&fe->parameters);

	fe->parameters.frequency = original_frequency;
	fe->parameters.inversion = original_inversion;
	
	fe->frontend.ioctl(&fe->frontend, FE_RESET, NULL);

	/* if we've hit where we started from, indicate a complete iteration
	 * has occurred
	 */
	fe->auto_step++;

	if ((fe->auto_step == fe->started_auto_step) ||
	    (fe->started_auto_step == 0 && wrapped))
		return 1;

	return 0;
}

/*----------------------------------------------------*/
static	void 
fe_proc(void *arg)
{
	struct fe_data *fe = (struct fe_data *)arg;
	struct dvb_frontend *frontend = &fe->frontend;
	struct adapter *sc = frontend->sc;
	int quality = 0, delay = 3*hz;
	int clean_setup_count = 0;
	fe_status_t s;
	int ss;

	printf("Initializing frontend '%s'...\n", fe->info->name);

	fe->frontend.ioctl(&fe->frontend, FE_INIT, NULL);

	for(;;){
		DBG1("tsleep %d\n", delay);
		tsleep(fe, PWAIT, "fe-sleep", delay * 10);
		DBG1("tsleep wakeup\n");

		if (fe->exit)
			break;

		/* if we've got no parameters, just keep idling */
		if (fe->state & FESTATE_IDLE)
		{
			DBG1("FESTATE_IDLE\n");
			delay = 3*hz;
			quality = 0;
			continue;
		}

		/* get the frontend status */
		fe->frontend.ioctl(&fe->frontend, FE_READ_STATUS, (void *)&s);


		/* if we're not tuned, and we have a lock,
		   move to the TUNED state */
		if ((fe->state & FESTATE_SEARCHING) && (s & FE_HAS_LOCK))
		{
			dvb_frontend_swzigzag_update_delay(&quality, &delay, fe->min_delay,
					s & FE_HAS_LOCK);
			DBG1("%d: q %d, d %d, m_d %d, s %x\n", __LINE__,
				quality, delay, fe->min_delay, s & FE_HAS_LOCK);
			fe->state = FESTATE_TUNED;
			/* if we're tuned,
			   then we have determined the correct inversion */	
			if ((!(fe->info->caps & FE_CAN_INVERSION_AUTO)) &&
				(fe->parameters.inversion == INVERSION_AUTO)){
				fe->parameters.inversion = fe->inversion;
			}
			continue;
		}

		/* if we are tuned already, check we're still locked */
		if (fe->state & FESTATE_TUNED)
		{
			dvb_frontend_swzigzag_update_delay(&quality, &delay, fe->min_delay,
					 s & FE_HAS_LOCK);
			DBG1("%d: q %d, d %d, m_d %d, s %x\n", __LINE__,
				quality, delay, fe->min_delay, s & FE_HAS_LOCK);

			/* we're tuned, and the lock is still good... */
			if (s & FE_HAS_LOCK)
			{
				continue;
			} else {
				/* if we _WERE_ tuned,
				   but now don't have a lock,
				   need to zigzag */
				fe->state = FESTATE_ZIGZAG_FAST;
				fe->started_auto_step = fe->auto_step;
				/* fallthrough */
			}
		}

		/* don't actually do anything if we're in the LOSTLOCK state */
		/* and the frontend can recover automatically */
		if ((fe->state & FESTATE_LOSTLOCK) &&
			(fe->info->caps & FE_CAN_RECOVER)){
			dvb_frontend_swzigzag_update_delay(&quality, &delay, fe->min_delay,
					s & FE_HAS_LOCK);
			DBG1("%d: q %d, d %d, m_d %d, s %x\n", __LINE__,
				quality, delay, fe->min_delay, s & FE_HAS_LOCK);
			continue;
		}

		//my
#if 0
		if (fe->state & FESTATE_DISEQC)
		{
			dvb_frontend_swzigzag_update_delay(&quality, &delay, fe->min_delay,
					s & FE_HAS_LOCK);
			DBG1("%d: q %d, d %d, m_d %d, s %x\n", __LINE__,
				quality, delay, fe->min_delay, s & FE_HAS_LOCK);
			continue;
		}
#endif			
		/* if we're in the RETUNE state,
		   set everything up for a brand new scan */
		if (fe->state & FESTATE_RETUNE)
		{
			DBG1("RETUNE\n");
			fe->lnb_drift = 0;
			fe->inversion = INVERSION_OFF;
			fe->auto_step = 0;
			fe->started_auto_step = 0;
			clean_setup_count = 0;
		}

		/* fast zigzag */
		if ((fe->state & FESTATE_SEARCHING_FAST) ||
		    (fe->state & FESTATE_RETUNE)){
			DBG1("fast-zigzag\n");
			delay = fe->min_delay;

			/* OK, if we've run out of trials at the fast speed.
			   Drop back to */
			/* slow for the _next_ attempt */
			if (dvb_frontend_swzigzag_autotune(fe))
			{
				fe->state = FESTATE_SEARCHING_SLOW;
				fe->started_auto_step = fe->auto_step;
				continue;
			}
#if 1
			/* enter clean setup state after the first tune 
			   if necessary. yeuch */
			if ((!(fe->info->caps & FE_CAN_CLEAN_SETUP)) &&
				(clean_setup_count == 0)){
				fe->state = FESTATE_CLEAN_SETUP;
			}
#endif	
			/* if we've just retuned, enter the ZIGZAG_FAST state.
			   This ensures */
			/* we cannot return from an FE_SET_FRONTEND before
			   the return occurs. */
			if (fe->state & FESTATE_RETUNE)
			{
				fe->state = FESTATE_TUNING_FAST;
			}
		}

		/* slow zigzag */
		if (fe->state & FESTATE_SEARCHING_SLOW)
		{
			DBG1("slow-zigzag\n");
			dvb_frontend_swzigzag_update_delay(&quality, &delay, fe->min_delay,
					 s & FE_HAS_LOCK);
			DBG1("%d: q %d, d %d, m_d %d, s %x\n", __LINE__,
				quality, delay, fe->min_delay, s & FE_HAS_LOCK);
			dvb_frontend_swzigzag_autotune(fe);
		}
#if 1
		/* clean setup */
		if (fe->state & FESTATE_CLEAN_SETUP)
		{
			DBG1("clean-setup\n");
			if ((clean_setup_count < 10) && (!(s & FE_HAS_LOCK))){
				fe->frontend.ioctl(&fe->frontend, FE_RESET, NULL);
			} else {
				fe->state = FESTATE_TUNING_FAST;
			}
			clean_setup_count ++;
		}
#endif
	}

	printf("shutdown frontend...\n");
	fe->frontend.ioctl(&fe->frontend, FE_SLEEP, NULL);

	wakeup(fe); //wait for fe_stop tsleep

	frontend->detach(sc);
	free(fe, M_DEVBUF);

#if __FreeBSD_version > 800000
	kproc_exit(0);
#else
	kthread_exit(0);
#endif
}

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

	/* Find DVB Frontend and attach */
        frontend_list = dvb_frontend_list;
        while (frontend_list->attach != NULL)
	{
                if (frontend_list->attach(sc) == 0)
                        break;
                frontend_list++;
                if (frontend_list->attach == NULL)
		{
			free(fe, M_DEVBUF);
                        return -ENOMEM;
		}
        }

	fe->exit = 0;
	fe->state = FESTATE_IDLE;

	snprintf(name, sizeof(name), "dvbtuner:%d", fe->id);
#if __FreeBSD_version > 800000
	err = kproc_create((void (*)(void *)) fe_proc, fe,
			 &feproc, 0, 0, name);
#elif __FreeBSD_version > 500005
	err = kthread_create((void (*)(void *)) fe_proc, fe,
			 &feproc, 0, 0, name);
#else
	err = kthread_create((void (*)(void *)) fe_proc, fe,
			 &feproc, name);
#endif
	if (err)
	{
		free(fe, M_DEVBUF);
		printf("can't create dvbtuner '%s', error %d", name, err);
	}
	return err;

}

/*----------------------------------------------------*/
void
fe_stop(struct adapter *sc)
{
	struct fe_data *fe = (struct fe_data *)sc->fe;
	struct dvb_frontend *frontend = &fe->frontend;

	fe->exit = 1;
	fe->state = FESTATE_IDLE;
	wakeup(fe);
	tsleep(fe, PWAIT, "dvbtuner-sleep", 0);
}

/*----------------------------------------------------*/
int
fe_ioctl(struct fe_data *fe, u_long cmd, caddr_t arg)
{
	struct dvb_frontend *frontend =  &fe->frontend;
	struct adapter *sc = (struct adapter *)frontend->sc;
	int err = -EOPNOTSUPP;
	int delay_ms = 0;
	int s;

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
                	fe->state = FESTATE_IDLE;
		}
		break;

        case FE_DISEQC_SEND_BURST:
		if (sc->diseqc_send_burst)
		{
                	err = sc->diseqc_send_burst(sc, *(fe_sec_mini_cmd_t *) arg);
                	fe->state = FESTATE_IDLE;
		}
		break;
        case FE_SET_TONE:
		if (sc->set_tone)
		{
			err = sc->set_tone(sc, *(fe_sec_tone_mode_t *) arg);
			fe->state = FESTATE_IDLE;
		}
		break;

        case FE_SET_VOLTAGE:
		if (sc->set_voltage)
		{
                	err = sc->set_voltage(sc, *(fe_sec_voltage_t *) arg);
                	fe->state = FESTATE_IDLE;
		}
		break;

        case FE_SET_FRONTEND:
                fe->state = FESTATE_RETUNE;
		memcpy(&fe->parameters, (void *)arg,
			sizeof(struct dvb_frontend_parameters));

		delay_ms = 600;
		if (delay_ms >= 0){
			fe->min_delay = (delay_ms * hz) / 1000;
		} else {
			fe->min_delay = hz/20; /* 50 ms */
		}
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
