#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "../include/net.h"

static char dvbDev[32]          = "/dev/dvb";

enum mode {
	UNKNOWN,
	LST_INTERFACE,
	ADD_INTERFACE,
	DEL_INTERFACE
} op_mode;

static struct dvb_net_if net_data;

int query_if(int fd, int dev)
{
	struct dvb_net_if data;
	int i, nIFaces = 0, ret = -1;

	printf("Query DVB network interfaces:\n");
	printf("-----------------------------\n");

	for (i = 0; i < DVB_NET_DEVICES_MAX; i++)
	{
		data.if_num = i;
		if (ioctl(fd, NET_GET_IF, &data))
			continue;
	
		if (dev == data.if_num)
			ret = 0;

		printf("Found device %d: interface dvb%d, "
		       "listening on PID %d\n",
			i, data.if_num, data.pid);
		nIFaces++;
	}

	printf("-----------------------------\n");
	printf("Found %d interface(s).\n\n", nIFaces);
	return ret;
}

int main(int argc, char **argv)
{

	int dvbfd = -1;
	int nfopt = -1;
	int ifnum = 0;

	while ((nfopt = getopt(argc, argv, "lhd:p:a:")) != -1)
	{
		switch(nfopt){
		case '?':
		case 'h':
			fprintf(stderr, "-a[num]     adapter\n");
			fprintf(stderr, "-p[pid]     create IF with pid\n");
			fprintf(stderr, "-d[num]     del IF by number\n");
			fprintf(stderr, "-l          show list IF\n");
			exit(0);
			break;
		case 'a':
			ifnum = strtol(optarg, NULL, 0);
			break;
		case 'p':
			net_data.pid = strtol(optarg, NULL, 0);
			op_mode = ADD_INTERFACE;
			break;
		case 'l':
			op_mode = LST_INTERFACE;
			break;
		case 'd':
			net_data.if_num = strtol(optarg, NULL, 0);
			op_mode = DEL_INTERFACE;
			break;
		default:
			break;
		}
	}

	sprintf(dvbDev, "/dev/dvb%d", ifnum);
	dvbfd = open(dvbDev, O_RDWR);
	if (dvbfd < 0)
	{
                printf("error open %s\n", dvbDev);
                return -1;
        }

	switch (op_mode){

	case DEL_INTERFACE:
		if (ioctl(dvbfd, NET_REMOVE_IF, net_data.if_num))
			printf(
				"Error: could't remove interface %d: %d %m\n",
					net_data.if_num, errno);
		else
			printf("Status: device %d removed successfuly\n",
					net_data.if_num);
		break;
	case ADD_INTERFACE:
		if (ioctl(dvbfd, NET_ADD_IF, &net_data))
			fprintf(stderr,
				"Error: could't add interface for pid %d: %d %m\n",
					net_data.pid, errno);
		else
			printf("Status: device dvb%d for pid %d created successfuly\n",
					net_data.if_num, net_data.pid);
		break;
	case LST_INTERFACE:
		query_if(dvbfd, 0);
		break;
	default:
		;
	}

	close(dvbfd);
	return 0;
}
