#make T=wl_sv_230s_ep DEBUG=1 -j8 > log.txt 2>&1
make T=2300_codec DEBUG=1 -j8 > log.txt 2>&1

if [ $? -eq 0 ];then
	echo "build success"
else
	echo "build failed and call log.txt"
	cat log.txt | grep "error:*"
fi
