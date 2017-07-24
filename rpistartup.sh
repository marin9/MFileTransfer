#!/bin/bash

gpio mode 24 out
gpio mode 25 out
gpio mode 29 in
gpio mode 28 out
gpio write 28 1

sudo mount /dev/sda /mnt

S=$(echo $?)
if [ "$S" -eq 0 ]; then
	S=$(gpio read 29)
	if [ "$S" -eq 1 ]; then
		#write on
		gpio write 24 0
		gpio write 25 1
		/home/pi/MFileTransfer/mftserver -m 4 -d /mnt/server -w &
	else
		#read only
		gpio write 24 1
		gpio write 25 0
		/home/pi/MFileTransfer/mftserver -m 4 -d /mnt/server &
	fi
else
	echo "ERROR: Disc mount fail." > errlog.txt
fi

#Shutdown/Restart pin
gpio mode 	23 in
gpio mode 	22 out
gpio write 	22 1

while true; do
	S=$(gpio read 23)
	if [ "$S" -eq 1 ]; then
		sleep 2
		S=$(gpio read 23)
		if [ "$S" -eq 1 ]; then
			echo ""
			echo "Restart..."
			sync
			sudo umount /dev/sda
			sleep 1
			shutdown -r -t 00
		else
			echo ""
			echo "Shutdown..."
			sync
			sudo umount /dev/sda
			sleep 1
			shutdown -h -t 00
		fi
		break
	fi
	sleep .1
done
