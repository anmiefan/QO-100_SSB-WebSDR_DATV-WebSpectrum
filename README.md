# WebSDR Client specially for the es'hail-2 Satellite
a web based SDR program made for the SDRplay RSP1a and RTL-SDR

# made for LINUX ONLY ! 

actual version V1.3: August, 15  2019
by DJ0ABR
(many bug fixes, please replace your older versions !)

a PC runs the SDR software:
* capturing samples from the SDR receiver (SDRplay)
* creating one line of a waterfall over a range of 200kHz
* downmixing a selected QRG into baseband
* creating a line of baseband waterfall over 20 kHz
* SSB demodulation and playing to a soundcard
* AUTO Beacon Lock

the waterfall data are written into the Apache HTML directory.

User Interface:
the GUI runs in a browser
* receiving the single line of the waterfalls via WebSocket
* drawing the full waterfall
* creating the GUI
* sending user command via WebSocket to above SDR program

# this is WORK in PROGRESS
actual Status: 

1) build for the playSDR (the Raspberry or ARM / ARM64 library must be installed from the SDRplay website !)
execute the script:  ./build_SDRplay
or
build for the RTLSDR
execute the script:  ./build_RTLSDR
2) start the program:  sudo ./qo100websdr -f RXFREQUENCY
RXFREQUENCY is the frequency in Hz where you receive the QO100 beacon in Hz minus 25000 Hz
(Example: 739525000)
sudo ... this is required only for the first time ! (it copies files into the HTML folder). Then you can run it without sudo.
3) open a web browser and open the html web site
4) if all is ok then the waterfall will be running. Click into the waterfall the select the listening frequency.
5) click the "Audio ON" button

# NEW
Synchronisation between the waterfall and an ICOM transceiver.
* the CIV address is set to 0xA2 (IC9700), you can change it in civ.c
* The kHz must be equal i.e.: 10489.525 MHz must display in the transceiver as 144.525 MHz or similar.
* enter the Icoms receive frequency in playSDReshail2.h:  TUNED_FREQUENCY, this must be the frequency of the CW beacon minus 25 kHz

Frequency Adjustment:
=====================
first un-select "Sync ON/off"
now you can correct the SDR tuner frequency manually in the text box below. You can also use the mouse-wheel to set it in 100Hz steps, or enter a value in Hz.
When the beacon is close to the correct position then select "Sync ON/off" and the software will automatically correct the LNB drift.

This works with the SDRplay hardware because it has a resolution of 1 Hz. It does not work with the RTLsdr sticks (the frequency will jump up and down) but you can give it a try.

RTL-SDR:
========
the rtl sdr hardware is automatically detected (librtlsdr must be installed). All works fine, except the auto-beacon-lock which works not as good as with the SDRplay receiver.


Prerequisites:
==============
these libraries are required:

apt-get update
apt-get install libasound2-dev libfftw3-dev libgd3 libgd-dev apache2 sndfile-tools libsndfile1-dev php librtlsdr-dev

above libraries are installed if you execute ./prepare_ubuntu

(php ... sometimes this must be replaced with php5 or php7 depending on your linux distribution)

additionally the SDRplay driver from the SDRplay Webpage must be installed if you want to run the SDRplay receiver.

Make the software:
==================

make

Run the software:
=================

1) start the software  sudo ./playSDReshail2 -f 144525000 (your CW-beacon RX frequency minus 25 kHz) (sudo is only required for the first time).
2) open a browser and open the webpage "playSDRweb.html" on your webserver

ATTENTION: before you can run this website, the WebSocket IP address must be modified in the file playSDRweb.html
Search for the line: var sockurl = ....
and enter the IP address of the PC running this software.

to access the website from outside (from the internet)
======================================================
in your internet router you need to open two TCP ports for external access:
1) the port to your webserver (usually 80)
2) the port to the websocket: 8090 (can be changed in playSDReshail2.h)


​this is a short technical description:
=====================================

The SDRplay runs with a sample rate of 2.4Ms/s and is decimated by 4 by its driver. An RTLsdr runs with 1.2MS/s and is decimated by 2 in the software.
The resulting stream has 600 kS/s and is first processed by an FFT every 100ms which gives a resolution of 0-300kHz with 10 Hz/pixel.


There are 30.000 pixels available which are converted into two waterfall diagrams. The first with a range of 300kHz to show the complete NB transponder.
The second with a range of 20 kHz to show the signals around the actual listening frequency.


The the 600 kS/s stream is fed into a software down-mixer. The LO (reference frequency) is generated by a NCO software which works very similar to the well knows DDS. The mixing itself is done by a simple multiplication followed by an anti-aliasing filter.
This mixer shifts the listening frequency into the base band (0 Hz) so it can be directly used to generate the sample for the sound card (after an additional low pass filter).


The waterfall lines as well as the audio samples are sent via a WebSocket (port 8090) to the web browser. 
The web page uses javascript to build the waterfall picture and send the audio samples to the local soundcard.
