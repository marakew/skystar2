
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/ioctl.h>

#include "../include/frontend.h"

//#define DEBUG_MODE
#ifdef DEBUG_MODE
#define DBG(fmt, args...) \
	do { if (1) printf("%s: " fmt, __FUNCTION__ , ## args); \
        } while(0);
#else
#define DBG(fmt, args...)       do { } while(0)
#endif


/*
 * name:frequency_MHz:polarization:fec:sat_no:symbolrate:inv:
 */

#define	CHANNEL_FILE		"channels.conf"
static char dvbDev[32]		= "/dev/dvb";
static int exit_after_tuning;

struct diseqc_cmd {
	struct dvb_diseqc_master_cmd msg;
	u_int32_t		timeout;
};


static void 
diseqc_send_msg(int fd, fe_sec_voltage_t v, struct diseqc_cmd *cmd,
		fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b)
{
	DBG("\n");

	if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
		printf("error FE_SET_TONE %d\n", SEC_TONE_OFF);

	if (ioctl(fd, FE_SET_VOLTAGE, v) == -1)
		printf("error FE_SET_VOLTAGE %d\n", v);

	usleep(15 * 1000);

	if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd->msg) == -1)
		printf("error FE_DISEQC_SEND_MASTER_CMD\n");

	usleep(cmd->timeout);
	usleep(15 * 1000);

	if (ioctl(fd, FE_DISEQC_SEND_BURST, b) == -1)
		printf("error FE_DISEQC_SEND_BURST %d\n", b);

	usleep(15 * 1000);

	if (ioctl(fd, FE_SET_TONE, t) == -1)
		printf("error FE_SET_TONE %d\n", t);
}

/* digital satellite equipment control
 * specification is available from http://www.eutelsat.com/
 */
static int 
diseqc(int secfd, int sat_no, int polar, int band)
{
	struct diseqc_cmd cmd =
		{ {{0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0};

	DBG("\n");

	cmd.msg.msg[3] = 
	0xf0 | (((sat_no * 4) & 0x0f) | (band ? 1:0) | (polar ? 0:2));

	diseqc_send_msg(secfd, polar ? SEC_VOLTAGE_13:SEC_VOLTAGE_18,
			&cmd, band ? SEC_TONE_ON : SEC_TONE_OFF,
			(sat_no / 4) % 2 ? SEC_MINI_B:SEC_MINI_A);
	return 1;
} 

static int 
do_tune(int fefd, unsigned int ifreq, fe_code_rate_t fec, unsigned int sr, fe_spectral_inversion_t inv)
{
	struct dvb_frontend_parameters tuneto;
	struct dvb_frontend_parameters tune;

	DBG("\n");

	tuneto.frequency = ifreq;
	tuneto.inversion = inv;
	tuneto.u.qpsk.symbol_rate = sr;
	tuneto.u.qpsk.fec_inner = fec;

	printf(">> freq = %u MHz, FEC = %u, symbolrate = %u, inv = %u\n",
		tuneto.frequency,
		tuneto.u.qpsk.fec_inner,
		tuneto.u.qpsk.symbol_rate, inv);

	if (ioctl(fefd, FE_SET_FRONTEND, &tuneto) == -1)
	{
		printf("error FE_SET_FRONTEND\n");
		return 0;
	}
#if 0
	if (ioctl(fefd, FE_GET_FRONTEND, &tune) == -1)
	{
		printf("error FE_GET_FRONTEND\n");
		return 0;
	}
	printf("<< freq = %u MHz, symbolrate %u\n",
		tune.frequency, tune.u.qpsk.symbol_rate);
#endif
	 return 1;
}

static int
check_fronted(int fefd)
{
	fe_status_t status;
	u_int16_t snr, signal;
	u_int32_t ber, uncorrected_blocks;
	int timeout = 0;

	DBG("\n");

	if (exit_after_tuning)
		return 0;

	do {
		if (ioctl(fefd, FE_READ_STATUS, &status) == -1)
			printf("error FE_READ_STATUS\n");

		if (ioctl(fefd, FE_READ_SIGNAL_STRENGTH, &signal) == -1)
			signal = -2;

		if (ioctl(fefd, FE_READ_SNR, &snr) == -1)
			snr = -2;

		printf ("status %02x | signal %04x | snr %04x | ",
			status, signal, snr);

		if (status & FE_HAS_LOCK)
			printf("LOCK | ");
		else	printf("UNLOCK | ");

		if (status & FE_HAS_SIGNAL)
			printf("SIGNAL | ");
		else	printf("NOSIGNAL | ");

		if (status & FE_HAS_CARRIER)
			printf("CR | ");
		else	printf("NOCR | ");

		if (status & FE_HAS_VITERBI)
			printf("VITERBI | ");
		else	printf("NOVITERBI | ");

		if (status & FE_HAS_SYNC)
			printf("SYNC |");
		else	printf("NOSYNC |");

		printf("\n");

		if ((status & FE_HAS_LOCK) || (++timeout >= 10))
			break;

		usleep(1000000);
	} while (1);

	return 0;
}

static int
tune(u_int32_t sat_no, unsigned int freq, unsigned int polar, fe_code_rate_t fec, unsigned int sr, int monitor, fe_spectral_inversion_t inv)
{
	int fefd;
	int error = 0;
	u_int32_t ifreq;
	int hiband, res;
	static struct dvb_frontend_info fe_info;

	DBG("\n");

	fefd = open(dvbDev, O_RDWR);
	if (fefd < 0)
	{
		printf("error open %s\n", dvbDev);
		return -1;
	}

	res = ioctl(fefd, FE_GET_INFO, &fe_info);
	if (res < 0)
	{
		printf("error FE_GET_INFO\n");
		return res;
	}

	if (monitor != 1)
	{

		if (freq >= 11700)	/* Ku hiBand */
		{
			hiband = 1;
			ifreq = freq - 10600;
		} else
		if (freq < 5150)	/* C band */
		{
			hiband = 0;
			ifreq = 5150 - freq;
		} else
		{			/* Ku lowBand */
			hiband = 0;
			if (freq < 9750)
				ifreq = 9750 - freq;
			else
				ifreq = freq - 9750;
		}

		res = 0;

		if (diseqc(fefd, sat_no, polar, hiband))
			if (do_tune(fefd, ifreq * 1000, fec, sr, inv))
				res = 1;
	}

	check_fronted(fefd);
	close(fefd);

	return res;
}


static int
read_channels(const char *filename, int chan_list, u_int32_t chan_no)
{
	FILE *chf;
	char buf[4096];
	char *field, *tmp, *name;
	unsigned int line;
	unsigned int freq, polar, sat_no, sr;
	fe_code_rate_t fec;
	fe_spectral_inversion_t inv;
	char fec_inner[6];
	char fe_inv[9];
	int ret;

	line = 0;
	if (!(chf = fopen(filename, "r"))) 
	{
		fprintf(stderr, "error open file '%s'\n", filename);
		return 0;
	}

	while (!feof(chf))
	{

		if (fgets(buf, sizeof(buf), chf))
		{
			line++;

			if (chan_no && chan_no != line)
				continue;

		tmp = buf;

			if (buf[0] == '#' || buf[0] == 10)
				continue;

			/* name */
			if (!(name = strsep(&tmp, ":")))
				goto syntax_err;

			/* freq */
			if (!(field = strsep(&tmp, ":")))
				goto syntax_err;
			freq = strtoul(field, NULL, 0);

			/* polarity */
			if (!(field = strsep(&tmp, ":")))
				goto syntax_err;
			polar = (field[0] == 'h' ? 0:1);

			/* fec */
			if (!(field = strsep(&tmp, ":")))
				goto syntax_err;
			strcpy(fec_inner, field);

					fec = FEC_AUTO;		/* by default */
				if (strcmp(field, "none") == 0){
					fec = FEC_NONE;
				} else
				if (strcmp(field, "1/2") == 0){
					fec = FEC_1_2;
				} else
				if (strcmp(field, "2/3") == 0){
					fec = FEC_2_3;
				} else
				if (strcmp(field, "3/4") == 0){
					fec = FEC_3_4;
				} else
				if (strcmp(field, "5/6") == 0){
					fec = FEC_5_6;
				} else
				if (strcmp(field, "7/8") == 0){
					fec = FEC_7_8;
				} else
				if (strcmp(field, "auto") == 0){
					fec = FEC_AUTO;
				} 

			/* sat no */
			if (!(field = strsep(&tmp, ":")))
				goto syntax_err;
			sat_no = strtoul(field, NULL, 0);

			/* sr */
			if (!(field = strsep(&tmp, ":")))
				goto syntax_err;
			sr = strtoul(field, NULL, 0) * 1000;

			if (!(field = strsep(&tmp, ":")))
				goto syntax_err;
			strcpy(fe_inv, field);

					inv = INVERSION_AUTO;	/* by default */
				if (strcmp(field, "auto") == 0){
					inv = INVERSION_AUTO;
				} else
				if (strcmp(field, "off") == 0){
					inv = INVERSION_OFF;
				} else
				if (strcmp(field, "on") == 0){
					inv = INVERSION_ON;
				} 

			if (!chan_list)
				printf("tuning to ");
			printf("%03u %-20s \n", line, name);

	printf("Name: %+s | lnb: %-u | Freq: %+u MHz | Polarity: %+s | FEC: %+s[%d] | SR: %+5u Kb/s | inv: %+s\n",
		name, sat_no, freq, polar ? "V":"H", fec_inner, fec, sr/1000, fe_inv);

			if (chan_list) 
				continue;

			fclose(chf);

			ret = tune(sat_no, freq, polar, fec, sr, 0, inv);
			if (ret)
				return 1;
			return 0;

syntax_err:
			fprintf(stderr, "syntax error in line %u: '%s'\n", line, buf);
		} else
		if (ferror(chf))
		{
			fprintf(stderr, "error reading channel list '%s': %d %m\n",
				filename, errno);
			fclose(chf);
			return 0;
		} else
		{
			fclose(chf);
			return 1;
		}

	}
	fclose(chf);		/* never be ...*/

	return 1;
}

/* ================================================================== */
int main(int argc,char **argv)
{

	int nfopt = -1;
	char chanfile[2 * 256];
	int chan_list = 0, cfile = 0, fmon = 0;
	u_int32_t chan_no = 0;
	int ifnum = 0;
	

	while ((nfopt = getopt(argc, argv, "hlmxc:n:a:")) != -1){
		switch (nfopt){
		case '?':
		case 'h':
			printf("help config ;-)\n");
			printf("-a[num]        adapter\n");
			printf("-l             list channel\n");
			printf("-n[num]        channel number\n");
			printf("-c[file name]  config file\n");
			printf("-m             monitor\n");
			printf("-x             exit after tuning\n");
			exit(0);
			break;
		case 'l':
			chan_list = 1;
			break;
		case 'a':
			ifnum = strtoul(optarg, NULL, 0);
			break;
		case 'n':
			chan_no = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			cfile = 1;
			strncpy(chanfile, optarg, sizeof(chanfile));
			break;
		case 'm':
			fmon = 1;
			break;
		case 'x':
			exit_after_tuning = 1;
			break;
		default:
			break;
		}
	}	

	sprintf(dvbDev, "/dev/dvb%d", ifnum);

	if (fmon == 1)
		tune(0, 0, 0, 0, 0, 1, 0);
	else
	{	
		if (!cfile)
			strncpy(chanfile, CHANNEL_FILE, sizeof(chanfile));

		printf("reading channel from file '%s'\n", chanfile);

		if (!read_channels(chanfile, chan_list, chan_no))
			return 1;
	}

	return 0;

}
