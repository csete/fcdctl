/* 
 * Funcube Dongle command line interface
 * Copyright 2011 David Pello EA1IDZ
 * Copyright 2011 Pieter-Tjerk de Boer PA3FWM
 * Copyright 2012-2014 Alexandru Csete OZ9AEC
 *
 * This code is licensed under a GNU GPL licensed
 * See LICENSE for information
 *
 */
#ifdef FCDPP
#define PROGRAM_VERSION "0.4.4-fcdpp"
#else
#define PROGRAM_VERSION "0.4.4"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include "fcd.h"
#include "fcdhidcmd.h"

// based on the #defines for TLGE_P10_0DB etc. from fcdhidcmd.h :
double lnagainvalues[]={-5.0,-2.5,-999,-999,0,2.5,5,7.5,10,12.5,15,17.5,20,25,30};

#include "hidapi.h"
extern const unsigned short _usVID;
extern const unsigned short _usPID;
extern int whichdongle;

void print_list(void)
{
    // based on code from fcd.c
    struct hid_device_info *phdi=NULL;
    //hid_device *phd=NULL;
    //char *pszPath=NULL;

    // look for all FCDs
    phdi=hid_enumerate(_usVID,_usPID);
    if (phdi==NULL)
    {
        puts("No FCD found.\n");
        return;
    }
#ifdef FCDPP
    puts("nr  USB path      firmware  frequency        gain (lna/mixer/if)  bias-t  audio device");
    //    0   0004:0006:02  18.09     1234.567890 MHz  on / on / +20 dB     on      card2
#else
    puts("nr  USB path      firmware  frequency        LNA gain  audio device");
    //    0   0004:0006:02  18.09     1234.567890 MHz  +20 dB    card2
#endif

    // iterate over all FCDs found
    int idx=0;
    while (phdi) {
        whichdongle=idx;
        printf("%-3i %-12s  ",idx, phdi->path);
        int stat = fcdGetMode();
        if (stat == FCD_MODE_NONE) printf("No FCD Detected.\n");
        else if (stat == FCD_MODE_BL) printf("In bootloader mode.\n");
        else {
            uint8_t lnagain;
#ifdef FCDPP
            uint8_t mixergain;
            uint8_t ifgain;
            char sfreq[128];
            uint8_t biast;
#endif
            uint8_t freq[4];
            char version[20];

            // read version, frequency, gain
            fcdGetFwVerStr(version);
            fcdAppGetParam(FCD_CMD_APP_GET_FREQ_HZ,freq,4);
            fcdAppGetParam(FCD_CMD_APP_GET_LNA_GAIN,&lnagain,1);
#ifdef FCDPP
            fcdAppGetParam(FCD_CMD_APP_GET_MIXER_GAIN,&mixergain,1);
            fcdAppGetParam(FCD_CMD_APP_GET_IF_GAIN1,&ifgain,1);
            fcdAppGetParam(FCD_CMD_APP_GET_BIAS_TEE,&biast,1);
#endif

            // try to find the corresponding audio device, by comparing the USB path to USB paths found under /proc/asound
            char audiopath[16]="(not found)";
            int usb1=-1,usb2=-1;
            sscanf(phdi->path,"%x:%x",&usb1,&usb2);

            int i;
            for (i=0;i<16;i++) {
                char s[32];
                sprintf(s,"/proc/asound/card%i/usbbus",i);
                FILE *f;
                f=fopen(s,"r");
                if (f) {
                    fgets(s,32,f);
                    int u1=0,u2=0;
                    sscanf(s,"%u/%u",&u1,&u2);
                    fclose(f);
                    if (u1==usb1 && u2==usb2) { sprintf(audiopath,"card%i",i); break; }
                }
            }

            // print our findings
#ifdef FCDPP
            sprintf(sfreq, "%.6f MHz", (*(int *)freq)/1e6);
            printf("%-8s  %-16s %s / %s / %d dB%s%s%s    %s     %s\n", version, sfreq, lnagain ? "on" : "off", mixergain ? "on" : "off", ifgain, lnagain ? " " : "", mixergain ? " "  : "", (ifgain>=10) ? ""  : " ", biast ? "on " : "off", audiopath);
#else
            printf("%-8s %11.6f MHz %4g dB     %s\n", version, (*(int *)freq)/1e6, lnagainvalues[lnagain], audiopath);
#endif
        }
        idx++;
        phdi = phdi->next;
    }
    hid_free_enumeration(phdi);
}


const char* program_name;

void print_help()
{
    printf("\n");
    printf("This is fcdctl version %s\n", PROGRAM_VERSION);
    printf("\n");
    printf("Usage: %s options [arguments]\n", program_name);
    printf("  -l, --list              List all FCDs in the system\n");
    printf("  -s, --status            Gets FCD current status\n");
    printf("  -f, --frequency <freq>  Sets FCD frequency in MHz\n");
#ifdef FCDPP
    printf("  -g, --lna-gain <gain>   Enable/disable LNA gain (0 or 1)\n");
    printf("  -m, --mixer-gain <gain> Enable/disable Mixer gain (0 or 1)\n");
    printf("  -i, --if-gain <gain>    Sets IF gain (0 to 59) in dB\n");
    printf("  -b, --bias-t <value>    Sets Bias-T power ((0 or 1)\n");
#else
    printf("  -g, --lna-gain <gain>   Sets LNA gain in dB\n");
    printf("  -c, --correction <cor>  Sets frequency correction in ppm\n");
#endif
    printf("  -n, --number <number>   Which dongle to show/set (default: 0)\n");
    printf("  -h, --help              Shows this help\n");
}

void print_status()
{
    int stat;
    char version[20];

    stat = fcdGetMode();

    if (stat == FCD_MODE_NONE)
    {
        printf("No FCD Detected.\n");
        return;
    }
    else if (stat == FCD_MODE_BL)
    {
        printf("FCD present in bootloader mode.\n");
        return;
    }
    else	
    {
        printf("FCD present in application mode.\n");
        stat = fcdGetFwVerStr(version);
        printf("FCD firmware version: %s.\n", version);
        unsigned char b[8];
        stat = fcdAppGetParam(FCD_CMD_APP_GET_FREQ_HZ,b,8);
        printf("FCD frequency: %.6f MHz.\n", (*(int *)b)/1e6);
        stat = fcdAppGetParam(FCD_CMD_APP_GET_LNA_GAIN,b,1);
#ifdef FCDPP
        printf("FCD LNA gain: %s.\n", b[0] == 1 ? "enabled" : "disabled");
        stat = fcdAppGetParam(FCD_CMD_APP_GET_MIXER_GAIN,b,1);
        printf("FCD Mixer gain: %s.\n", b[0] == 1 ? "enabled" : "disabled");
        stat = fcdAppGetParam(FCD_CMD_APP_GET_IF_GAIN1,b,1);
        printf("FCD IF gain: %u dB.\n", b[0]);
#else
        printf("FCD LNA gain: %g dB.\n", lnagainvalues[b[0]]);
#endif
        return;
    }
}

int main(int argc, char* argv[])
{
    int stat;
    int freq = 0;
    double freqf = 0;
    int gain = -999;
#ifdef FCDPP
    int mixergain = -999;
    int ifgain = -999;
    int biast = -999;
#endif
    int corr = 0;
    int dolist = 0;
    int dostatus = 0;

    /* getopt infrastructure */
    int next_option;
    const char* const short_options = "slg:f:m:i:b:c:n:h";
    const struct option long_options[] =
    {
        { "status", 0, NULL, 's' },
        { "list", 0, NULL, 'l' },
        { "frequency", 1, NULL, 'f' },
        { "number", 1, NULL, 'n' },
        { "lna-gain", 1, NULL, 'g' },
#ifdef FCDPP
        { "mixer-gain", 1, NULL, 'm' },
        { "if-gain", 0, NULL, 'i' },
        { "bias-t", 0, NULL, 'b' },
#endif
        { "correction", 1, NULL, 'c' },
        { "help", 0, NULL, 'h' }
    };


    /* save program name */
    program_name = argv[0];

    if (argc == 1)
    {
        print_help();
        exit(EXIT_SUCCESS);
    }

    while(1)
    {
        /* call getopt */
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);

        /* end of the options */
        if (next_option == -1)
            break;

        switch (next_option)
        {
            case 'h' :
                print_help();
                exit(EXIT_SUCCESS);
            case 's' :
                dostatus=1;
                break;
            case 'l' :
                dolist=1;
                break;
            case 'f' :
                freqf = atof(optarg);
                break;
            case 'g' :
                gain = atoi(optarg);
                break;
#ifdef FCDPP
            case 'm' :
                mixergain = atoi(optarg);
                break;
            case 'i' :
                ifgain = atoi(optarg);
                break;
            case 'b' :
                biast = atoi(optarg);
                break;
#endif
            case 'n' :
                whichdongle = atoi(optarg);
                break;
            case 'c' :
                corr = atoi(optarg);
                break;
            case '?' :
                print_help();
                exit(1);
            default :
                abort();
        }	
    }

    if (freqf>0) {
        /* MHz -> Hz */
        freq = (int)(freqf * 1.0e6f);

        /* calculate frequency */
        freq *= 1.0 + corr / 1000000.0;

        /* set it */
        stat = fcdAppSetFreq(freq);
        if (stat == FCD_MODE_NONE)
        {
            printf("No FCD Detected.\n");
            return 1;
        }
        else if (stat == FCD_MODE_BL)
        {
            printf("FCD in bootloader mode.\n");
            return 1;
        }
        else	
        {
            printf("Freq set to %.6f MHz.\n", freq/1e6);
        }
    }

    if (gain>-999) {
        unsigned char b=0;
#ifdef FCDPP
        b = gain ? 1 : 0;
#else
        while (b<sizeof(lnagainvalues)/sizeof(lnagainvalues[0]) && gain>lnagainvalues[b]+1) b++;
#endif
        stat = fcdAppSetParam(FCD_CMD_APP_SET_LNA_GAIN,&b,1);
        if (stat == FCD_MODE_NONE) { printf("No FCD Detected.\n"); return 1; }
        else if (stat == FCD_MODE_BL) { printf("FCD in bootloader mode.\n"); return 1; }
#ifdef FCDPP
        else printf("LNA gain %s.\n", b ? "enabled" : "disabled");
#else
        else printf("LNA gain set to %g dB.\n",lnagainvalues[b]);
#endif

    }

#ifdef FCDPP
    if (mixergain>-999) {
        unsigned char b=0;
        b = mixergain ? 1 : 0;
        stat = fcdAppSetParam(FCD_CMD_APP_SET_MIXER_GAIN,&b,1);
        if (stat == FCD_MODE_NONE) { printf("No FCD Detected.\n"); return 1; }
        else if (stat == FCD_MODE_BL) { printf("FCD in bootloader mode.\n"); return 1; }
        else printf("Mixer gain %s.\n", b ? "enabled" : "disabled");
    }

    if (ifgain>-999) {
        unsigned char b=0;
        b = ifgain;
        stat = fcdAppSetParam(FCD_CMD_APP_SET_IF_GAIN1,&b,1);
        if (stat == FCD_MODE_NONE) { printf("No FCD Detected.\n"); return 1; }
        else if (stat == FCD_MODE_BL) { printf("FCD in bootloader mode.\n"); return 1; }
        else printf("IF gain set to %u dB.\n", b);
    }

    if (biast>-999) {
        unsigned char b=0;
        b = biast ? 1 : 0;
        stat = fcdAppSetParam(FCD_CMD_APP_SET_BIAS_TEE,&b,1);
        if (stat == FCD_MODE_NONE) { printf("No FCD Detected.\n"); return 1; }
        else if (stat == FCD_MODE_BL) { printf("FCD in bootloader mode.\n"); return 1; }
        else printf("Bias-T %s.\n", b ? "enabled" : "disabled");
    }

#endif

    if (dolist) print_list();

    if (dostatus) print_status();

    return EXIT_SUCCESS;
}
