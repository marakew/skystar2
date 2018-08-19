#!/bin/sh

DRIVER="skystar2"
PID="666"
DVB="dvb0"
DVBNET="/usr/local/bin/dvbnet"
DVBTUNE="/usr/local/bin/dvbtune"
ifconfig="/sbin/ifconfig"

case "$1" in
start)
	if [ -f /modules/${DRIVER}.ko ]; then
	kldload ${DRIVER}
	if [ -f ${DVBNET} -a -x ${DVBNET} ]; then
		${DVBNET} -p ${PID}
		${ifconfig} ${DVB} inet 192.168.238.238 up
		sh -c "${DVBTUNE} -c /etc/channels.conf -x > /dev/null 2>&1 &"
		echo -n ' dvbnet'
	fi
	fi
	;;
stop)

#	if [ -f ${DVBNET} -a -x ${DVBNET} ]; then
#		${ifconfig} ${DVB} down
#		${DVBNET} -d 1
#	fi

	kldunload ${DRIVER}
	;;
restart)
	$0 stop
	sleep 2
	$0 start
	;;
*)
	echo "usage: {start|stop|restart}" >&2
	;;
esac
