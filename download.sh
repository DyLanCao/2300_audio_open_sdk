#! /bin/bash

num=$(ls -l /dev/ttyUSB* | rev | cut -c 1)
#num=5
echo com is:$num
#sudo -S dldtool -c $num -f out/usb_audio_algorithms/usb_audio_algorithms.bin
#sudo -S dldtool -c $num -f out/wl_sv_230s_ep/wl_sv_230s_ep.bin
#sudo -S dldtool -c $num -f out/2300_codec/2300_codec.bin
sudo -S dldtool -c $num -f out/mic_uart/mic_uart.bin
#sudo -S dldtool -c $num -f /home/caoyin/doc/bes_code/i2s_codec/mcu-sw/out/b2000_loopback/b2000_loopback.bin
sudo minicom port$num

