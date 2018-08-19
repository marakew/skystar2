#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <term.h>
#include <curses.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <sys/time.h>

#include "../include/frontend.h"

static char dvbDev[32]          = "/dev/dvb";
static int fefd;

static struct termios init_settings, new_settings;
static int peek_char = -1;


void
init_kb(void)
{
	tcgetattr(0, &init_settings);
	new_settings = init_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
}

void
close_kb(void)
{
	tcsetattr(0, TCSANOW, &init_settings);
}

int
hit_kb(void)
{
	char ch;
	int nread;

	if(peek_char != -1)
		return -1;
	
	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
	
	if(nread == 1){
		peek_char = ch;
		return 1;
	}

	return 0;
}

int
readch(void)
{
	char ch;
	
	if(peek_char != -1){
		ch = peek_char;
		peek_char = -1;
		return ch;
	}
	read(0, &ch, 1);
	return ch;
}


void
show_info(void)
{
	static struct dvb_frontend_info fe_info;
	struct dvb_frontend_parameters fe;

	fe_status_t status;
	fe_sec_voltage_t voltage;
	u_int32_t snr;
	u_int32_t signal;
	u_int32_t ber, unc_blocks;
	u_int8_t mac_addr[8];
	int32_t fec_cr = 0;
	int i;


	move(0, 0);
	attron(A_BOLD);
	attrset(COLOR_PAIR(2));
	printw("\n ------------------------- \n");
	printw(" Monitor for B2C2 SkyStar2 \n");
	printw(" ------------------------- \n\n");
	attroff(A_BOLD);
	attrset(COLOR_PAIR(1));
	refresh();

	move(5, 0);
	attrset(COLOR_PAIR(4));
	printw("[Driver/Device Info]\n");
	attrset(COLOR_PAIR(1));
	refresh();

	ioctl(fefd, FE_GET_INFO, &fe_info);

	move(5+2, 0);
	attrset(COLOR_PAIR(5));
	printw("# Frontend : '%s'", fe_info.name);
	attrset(COLOR_PAIR(1));
	refresh();

	/* -------- [ MAC ] --------- */
	memset(&mac_addr, 0xef, sizeof(mac_addr));

	ioctl(fefd, DVB_GET_MAC, &mac_addr);

	move(5+3, 0);
	attrset(COLOR_PAIR(5));
#if 1
	printw("# MAC address : %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac_addr[0], mac_addr[1],
			mac_addr[2], mac_addr[3],
			mac_addr[4], mac_addr[5]);
#else
	printw("# MAC address : %6D\n", mac_addr, ":");
#endif
	attrset(COLOR_PAIR(1));
	refresh();


	if (ioctl(fefd, FE_GET_FRONTEND, &fe) == -1)
	{
		fe.frequency = 1;
		fe.u.qpsk.symbol_rate = 1;
	}

	move(5+4, 0);
	attrset(COLOR_PAIR(5));
	printw("# Tuner Frequency: %u [Mhz]\n", fe.frequency);
	attrset(COLOR_PAIR(1));
	refresh();


	move(5+6, 0);
	attrset(COLOR_PAIR(5));
	printw("# Symbol Rate:     %u [kS/s]\n", fe.u.qpsk.symbol_rate/1000);
	attrset(COLOR_PAIR(1));
	refresh();


	move(5+7, 0);
	attrset(COLOR_PAIR(5));
	printw("# FEC:             %s\n", 
		fe.u.qpsk.fec_inner == FEC_1_2 ? "1/2":
		fe.u.qpsk.fec_inner == FEC_2_3 ? "2/3":
		fe.u.qpsk.fec_inner == FEC_3_4 ? "3/4":
		fe.u.qpsk.fec_inner == FEC_5_6 ? "5/6":
		fe.u.qpsk.fec_inner == FEC_6_7 ? "6/7":
		fe.u.qpsk.fec_inner == FEC_7_8 ? "7/8":
		fe.u.qpsk.fec_inner == FEC_AUTO ? "Auto":
		fe.u.qpsk.fec_inner == FEC_NONE ? "None": "??");
	attrset(COLOR_PAIR(1));
	refresh();



	/* [ Info ] */
	move(5+10, 0);
	attrset(COLOR_PAIR(4));
	printw("[Tuner Info]\n");
	attrset(COLOR_PAIR(1));
	refresh();

	ioctl(fefd, FE_READ_STATUS, &status);

	if (ioctl(fefd, FE_READ_SIGNAL_STRENGTH, &signal) == -1)
		signal = -2;

	if (ioctl(fefd, FE_READ_SNR, &snr) == -1)
		snr = -2;

	move(5+11, 0);
	attrset(COLOR_PAIR(3));
	printw("# LNB Lock status: ");
	if (status & FE_HAS_LOCK)
	{
		attrset(COLOR_PAIR(2));
		printw("Lock:Yes ");
	} else {
		attrset(COLOR_PAIR(6));
		printw("Lock:No  ");
	}

	if (status & FE_HAS_SIGNAL)  printw("Signal:Yes ");
			else	     printw("Signal:No  ");
	if (status & FE_HAS_CARRIER) printw("Carrier:Yes ");
			else	     printw("Carrier:No  ");
	if (status & FE_HAS_VITERBI) printw("Viterbi:Yes ");
			else	     printw("Viterbi:No  ");
	if (status & FE_HAS_SYNC) printw("Sync:Yes ");
			else	  printw("Sync:No  ");

	printw("\n");
	attrset(COLOR_PAIR(1));
	refresh();

	/* Signal Quality SNR[dB] */
	move(5+13, 0);
	attrset(COLOR_PAIR(3));
	printw("# Signal to Noise Ratio: %.3f [dB]\n", (float)(snr)/1000);
	attrset(COLOR_PAIR(1));
	refresh();

	move(5+14, 0);
	attrset(COLOR_PAIR(3));
	printw("# Signal Quality: %2d%%", signal);
	printw(" [");
	for (i = 0; i < (100/2); i++)
		if (i < (signal/2))
			printw("%c", 146);
		else	printw("%c", 144);
	printw("]\n");
	attrset(COLOR_PAIR(1));
	refresh();


	move(5+20, 0);
	attrset(COLOR_PAIR(5));
	printw("Press the 'q' key to finish it!\n");
	attrset(COLOR_PAIR(1));
	refresh();

}


void
timer_refresh(int signum)
{
	show_info();
}


int	
main(int argc, char **argv)
{
	int ch;
	struct itimerval itv;
	int opt;
	int nfopt = -1;
	int ifnum = 0;

	while ((nfopt = getopt(argc, argv, "ha:")) != -1)
	{
		switch (nfopt){
                case '?':
                case 'h':
                        printf("help config ;-)\n");
                        printf("-a[num]        adapter\n");
                        exit(0);
                        break;
                case 'a':
			ifnum = strtol(optarg, NULL, 0);
                        break;
		default:
                        break;
                }
	}

	sprintf(dvbDev, "/dev/dvb%d", ifnum);

	if ((fefd = open(dvbDev, O_RDWR)) < 0)
	{
		printf("error open %s\n", dvbDev);
		exit(1);
	}

	initscr();
	if(!has_colors())
	{
		endwin();
		exit(1);
	}

	if(start_color() != OK)
	{
		endwin();
		exit(1);
	}

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);
	init_pair(4, COLOR_BLUE, COLOR_GREEN);
	init_pair(5, COLOR_RED, COLOR_BLACK);
	init_pair(6, COLOR_WHITE, COLOR_RED);

	clear();

	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	itv.it_interval.tv_sec = 1;
	itv.it_interval.tv_usec = 0;
	signal(SIGPROF, timer_refresh);
	setitimer(ITIMER_PROF, &itv, NULL);

	ch = 0;
	init_kb();
	while(ch != 'q')
	{
		if (hit_kb())
			ch = readch();
	}
	close_kb();

	endwin();
	close(fefd);

	exit(0);
}
