/*
* Web based SDR Client for SDRplay
* =============================================================
* Author: DJ0ABR
*
*   (c) DJ0ABR
*   www.dj0abr.de
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
* 
* sdrplay.c ... handles the SDRplay hardware
* 
*/

#ifdef SDR_PLAY

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "qo100websdr.h"
#include "sdrplay.h"
#include "ssbfft.h"
#include "wb_fft.h"
#include "setup.h"
#include "mirsdrapi-rsp.h"  // the SDRplay driver must be installed !

int device = 0;     // device number. If we have one SDRplay device connected: device=0
int devModel = 1;
float ppmOffset = 0.0;
int antenna = 0; // 0 = Ant A, 1 = Ant B, 2 = HiZ
mir_sdr_RSPII_AntennaSelectT ant = mir_sdr_RSPII_ANTENNA_A;
int refClk = 0;
int notchEnable = 0;
int biasT = 0;
mir_sdr_SetGrModeT grMode = mir_sdr_USE_SET_GR_ALT_MODE;
int gainR = 50;
#ifdef WIDEBAND
    int bwkHz = 8000;   // default BW, possible values: 200,300,600,1536,5000,6000,7000,8000
#else
    int bwkHz = 5000;   // default BW, possible values: 200,300,600,1536,5000,6000,7000,8000
#endif // WIDEBAND

int ifkHz = 0;      // 0 is used in this software
int rspLNA = 0;
void *cbContext = NULL;
int agcControl = 1;
long setPoint = -30;

/*
 * init the SDRplay hardware
 * when this function suceeds, then
 * streamCallback
 * will be called delivering new samples
 */
int init_SDRplay()
{
    printf("Initialize SDRplay hardware\n");
    mir_sdr_DeviceT devices[4];
    unsigned int numDevs;
    int devAvail = 0;
    mir_sdr_ErrT r;
    int gRdBsystem;

    // scan for SDRplay devices
    mir_sdr_GetDevices(&devices[0], &numDevs, 4);
    
    // is an SDRplay device available ?
    for(int i = 0; i < numDevs; i++) 
    {
        if(devices[i].devAvail == 1)
            devAvail++;
    }

    if (devAvail == 0) {
        printf("ERROR: No RSP devices available.\n");
        return 0;
    }

    if (devices[device].devAvail != 1) {
        printf("ERROR: RSP selected (%d) is not available.\n", (device + 1));
        return 0;
    }
    
    // read device information and initialize
    mir_sdr_SetDeviceIdx(device);
    devModel = devices[device].hwVer;
    mir_sdr_SetPpm(ppmOffset);

    // set model specific parameters
    if (devModel == 2) {
        if (antenna == 1) {
            ant = mir_sdr_RSPII_ANTENNA_A;
            mir_sdr_RSPII_AntennaControl(ant);
            mir_sdr_AmPortSelect(0);
        } else {
            ant = mir_sdr_RSPII_ANTENNA_B;
            mir_sdr_RSPII_AntennaControl(ant);
            mir_sdr_AmPortSelect(0);
        }

        if (antenna == 2) {
            mir_sdr_AmPortSelect(1);
        }
    }
	
    grMode = mir_sdr_USE_RSP_SET_GR;
    if(devModel == 1) grMode = mir_sdr_USE_SET_GR_ALT_MODE;

    double dqrg = tuned_frequency;
    r = mir_sdr_StreamInit(&gainR, ((double)SDR_SAMPLE_RATE/1e6), dqrg/1e6,
        (mir_sdr_Bw_MHzT)bwkHz, (mir_sdr_If_kHzT)ifkHz, rspLNA, &gRdBsystem,
        grMode, &samplesPerPacket, streamCallback, gainCallback, &cbContext);

	if (r != mir_sdr_Success) {
		printf("Failed to start SDRplay RSP device.\n");
		return 0;
	}
	
	#ifndef WIDEBAND
	// narrow band
	// reduce the sample rate
	mir_sdr_DecimateControl(1,SR_MULTIPLIER,0);
    #endif

    mir_sdr_AgcControl(agcControl, setPoint, 0, 0, 0, 0, rspLNA);
    
    if (devModel == 2) {
        if (refClk > 0) {
            mir_sdr_RSPII_ExternalReferenceControl(1);
        } else {
            mir_sdr_RSPII_ExternalReferenceControl(0);
        }

        if (notchEnable > 0) {
            mir_sdr_RSPII_RfNotchEnable(1);
        } else {
            mir_sdr_RSPII_RfNotchEnable(0);
        }

        if (biasT > 0) {
            mir_sdr_RSPII_BiasTControl(1);
        } else {
            mir_sdr_RSPII_BiasTControl(0);
        }
    }
    
    return 1;
}

// clean up SDRplay device
void remove_SDRplay()
{
    mir_sdr_Uninit();
}

extern double lastsdrqrg;
void setTunedQrgOffset(int hz)
{
    if(lastsdrqrg == 0) lastsdrqrg = tuned_frequency;
    
    double off = (double)hz;
    lastsdrqrg = lastsdrqrg - off;
    printf("set tuner: new:%f offset:%f\n",lastsdrqrg,off);
    mir_sdr_SetRf(lastsdrqrg,1,0);
    printf("set tuner : %.6f MHz\n",lastsdrqrg/1e6);
}

void reset_Qrg_SDRplay()
{
    int gRdBsystem;
    mir_sdr_ErrT r;
    lastsdrqrg = tuned_frequency;
    
    printf("re-tune: %.6f MHz\n",lastsdrqrg/1e6);
    
    r = mir_sdr_Reinit(&gainR, 0, lastsdrqrg/1e6, 0, 0, 0, 0,&gRdBsystem, 0, &samplesPerPacket,mir_sdr_CHANGE_RF_FREQ);

	if (r != mir_sdr_Success) {
		printf("Failed to restart SDRplay RSP device: %d\n",r);
		return;
	}
}

/*
 * the SDRplay driver calls this function
 * numSamples ... number of samples included
 * xi and xq ... samples
 * */

void streamCallback(short *xi, short *xq, unsigned int firstSampleNum,
    int grChanged, int rfChanged, int fsChanged, unsigned int numSamples,
    unsigned int reset, unsigned int hwRemoved, void *cbContext)
{   
    /*
     * Testfunction to measure the sample rate 
     * give it one minute for measurement
     * 
     * uncomment this function to check if the SDR hardware 
     * delivers samples in the right speed
     */
    /*
    struct timeval  tv;
    static unsigned long lastus=0;
    static unsigned long samples = 0;
    static int f=1;
	gettimeofday(&tv, NULL);
    if(f)
    {
        f = 0;
        lastus = tv.tv_sec * 1000000 + tv.tv_usec;
    }
    samples += numSamples;
    unsigned long tnow = tv.tv_sec * 1000000 + tv.tv_usec;
    printf("%.2f\n",((samples*1e6)/(tnow-lastus))/1e6);
    //return;
    */
    
    // call the function to process the new samples (file: sampleprocessing.c)
#ifdef WIDEBAND
	wb_sample_processing(xi, xq, numSamples);
#else
    fssb_sample_processing(xi, xq, numSamples);
#endif // WIDEBAND
}

// currently not used
void gainCallback(unsigned int gRdB, unsigned int lnaGRdB, void *cbContext)
{
    return;
}

#endif // SDR_PLAY
