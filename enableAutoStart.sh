if [ ! -f /etc/init.d/startCamera ]
then 
	cp /home/root/startCamera /etc/init.d/startCamera
fi

if [ ! -h /etc/rc5.d/S99zstartCamera ]
then
	cd /etc/rc5.d
	ln -s -r ../init.d/startCamera S99zstartCamera
fi
