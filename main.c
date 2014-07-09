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
#define PROGRAM_VERSION "0.4.5"

#include <stdio.h>
#include <string.h>
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
extern const unsigned short _usPIDS;
extern const unsigned short _usPIDP;
extern const unsigned short _usPIDO;
extern int whichdongle;

void print_list(void)
{
    // based on code from fcd.c
    struct hid_device_info *phdi=NULL;
    //hid_device *phd=NULL;
    //char *pszPath=NULL;

    // look for all FCDs
    phdi=hid_enumerate(_usVID,_usPIDS);
    if (!phdi)
        phdi=hid_enumerate(_usVID,_usPIDP);
    if (!phdi)
        phdi=hid_enumerate(_usVID,_usPIDO);
    if (phdi==NULL)
    {
        puts("No FCD found.\n");
        return;
    }

    puts("  nr   USB path       firmware   frequency         LNA gain   audio device");
    //      0    0004:0006:02   18.09      1234.567890 MHz   +20 dB     card2

    // iterate over all FCDs found
    int idx=0;
    while (phdi) {
        whichdongle=idx;
        printf("  %-3i  %-12s  ",idx, phdi->path);
        int stat = fcdGetMode();
        int hwvr = fcdGetVersion();
        if (stat == FCD_MODE_NONE) printf("No FCD Detected.\n");
        else if (stat == FCD_MODE_BL) printf("In bootloader mode.\n");
        else {
            uint8_t lnagain;
            uint8_t freq[4];
            char version[20];

            // read version, frequency, gain
            fcdGetFwVerStr(version);
            fcdAppGetParam(FCD_CMD_APP_GET_FREQ_HZ,freq,4);
            fcdAppGetParam(FCD_CMD_APP_GET_LNA_GAIN,&lnagain,1);

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
                    sscanf(s,"%d/%d",&u1,&u2);
                    fclose(f);
                    if (u1==usb1 && u2==usb2) { sprintf(audiopath,"card%i",i); break; }
                }
            }

            // print our findings
            if (FCD_VERSION_2==hwvr)
                printf(" %-8s   %11.6f MHz   %s    %s\n", version, (*(int *)freq)/1e6, lnagain ? "enabled" : "disabled", audiopath);
            else
                printf(" %-8s   %11.6f MHz   %4g dB     %s\n", version, (*(int *)freq)/1e6, lnagainvalues[lnagain], audiopath);
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
    printf("  -l, --list             List all FCDs in the system\n");
    printf("  -s, --status           Gets FCD current status\n");
    printf("  -f, --frequency <freq> Sets FCD frequency in MHz\n");
    printf("  -g, --gain <gain>      Sets LNA gain in dB (V1.x) or enable/disable LNA gain [0/1] (V2.x)\n");
    printf("  -g, --gain <gain>      Sets LNA gain in dB\n");
    printf("  -c, --correction <cor> Sets frequency correction in ppm\n");
    printf("  -d, --dump <file>      Saves existing FCD firmware to <file>\n");
    printf("  -u, --update <file>    Updates FCD firmware from <file>\n");
    printf("  -i, --index <index>    Which dongle to show/set (default: 0)\n");
    printf("  -h, --help             Shows this help\n");
}

void print_status()
{
    FCD_MODE_ENUM stat;
    FCD_VERSION_ENUM hwvr;
    char *hwstr, version[40];
    unsigned char info[4];

    stat = fcdGetMode();
    hwvr = fcdGetVersion();
    if (hwvr == FCD_VERSION_NONE)
        hwstr = "Not detected";
    else if (hwvr == FCD_VERSION_1)
        hwstr = "V1.0";
    else if (hwvr == FCD_VERSION_1_1)
        hwstr = "V1.1";
    else if (hwvr == FCD_VERSION_2)
        hwstr = "V2.0";
    else
        hwstr = "Unknown";

    if (stat == FCD_MODE_NONE)
    {
        printf("No FCD Detected.\n");
        return;
    }
    else if (stat == FCD_MODE_BL)
    {
        printf("FCD present in bootloader mode.\n");
        printf("FCD hardware version: %s.\n", hwstr);
        stat = fcdGetDeviceInfo(info);
        if (FCD_MODE_BL == stat)
            printf("FCD device info bytes: %02x %02x %02x %02x\n", info[0], info[1], info[2], info[3]);
        else
            printf("Unable to read device info bytes\n");
        return;
    }
    else	
    {
        printf("FCD present in application mode.\n");
        stat = fcdGetFwVerStr(version);
        printf("FCD hardware version: %s.\n", hwstr);
        printf("FCD firmware version: %s.\n", version);
        unsigned char b[8];
        stat = fcdAppGetParam(FCD_CMD_APP_GET_FREQ_HZ,b,8);
        printf("FCD frequency: %.6f MHz.\n", (*(int *)b)/1e6);
        stat = fcdAppGetParam(FCD_CMD_APP_GET_LNA_GAIN,b,1);
        if (FCD_VERSION_2 == hwvr)
            printf("FCD LNA gain: %s.\n", b[0] == 1 ? "enabled" : "disabled");
        else
            printf("FCD LNA gain: %g dB.\n", lnagainvalues[b[0]]);
        return;
    }
}

FCD_API_CALL void progress(uint32_t start, uint32_t end, uint32_t position, int err)
{
    uint32_t len = end - start;
    uint32_t pos = position - start;
    printf("\r%08x-%08x: %08x: %d%% (%d) ", start, end, position, (pos*100)/len, err);
    fflush(stdout);
}

void update_firm(char *firm)
{
    int stat;
    long fwsiz;
    FILE *fp = NULL;
    char *fwbuf = NULL, resp[10];

    fp = fopen(firm, "rb");
    if (!fp)
    {
        printf("Unable to open firmware file: %s\n", firm);
        goto done;
    }
    stat = fseek(fp, 0, SEEK_END);
    if (stat)
    {
        printf("Unable to seek to end of firmware\n");
        goto done;
    }
    fwsiz = ftell(fp);
    if (fwsiz<0)
    {
        printf("Unable to read firmware size\n");
        goto done;
    }
    fwbuf = malloc(fwsiz);
    if (!fwbuf)
    {
        printf("Unable to allocate memory for firmware buffer\n");
        goto done;
    }
    if (fseek(fp, SEEK_SET, 0)<0 || fread(fwbuf, fwsiz, 1, fp)!=1)
    {
        printf("Unable to read firmware into buffer\n");
        goto done;
    }
    fclose(fp);
    fp = NULL;

    stat = fcdGetMode();

    if (stat == FCD_MODE_NONE)
    {
        printf("No FCD Detected.\n");
    }
    else if (stat == FCD_MODE_APP)
    {
        printf("FCD present in application mode - resetting to bootloader..\n");
        stat = fcdAppReset();
        printf("Please check /var/log/[messages|syslog] to confirm mode change, then re-run update.\n");
    }
    else if (stat == FCD_MODE_BL)
    {
        printf("FCD present in bootloader mode - type 'yes' to confirm update, or 'no' to verify existing Flash: ");
        fflush(stdout);
        fflush(stdin);
        if (fgets(resp, sizeof(resp), stdin) && strncmp(resp, "yes", 3)==0)
        {
            printf("erasing..\n");
            stat = fcdBlErase();
            if (stat != FCD_MODE_BL)
            {
                printf("Unable to erase existing firmware.\n");
                goto done;
            }
            printf("writing..\n");
            stat = fcdBlWriteFirmwareProg(fwbuf, fwsiz, progress);
            if (stat != FCD_MODE_BL)
            {
                printf("Unable to write firmware to FCD.\n");
                goto done;
            }
        }
        printf("verifying..\n");
        stat = fcdBlVerifyFirmwareProg(fwbuf, fwsiz, progress);
        if (stat != FCD_MODE_BL)
        {
                printf("Unable to verify firmware on FCD.\n");
                goto done;
        }
        printf("\ndone.\n");
    }
done:
    if (fwbuf) free(fwbuf);
    if (fp) fclose(fp);
}

void dump_firm(char *dump)
{
    int stat;
    FILE *saveFile = fopen(dump, "wb");
    if (!saveFile)
    {
        printf("Unable to open dump file: %s\n", dump);
        return;
    }
    stat = fcdGetMode();
    if (stat == FCD_MODE_NONE)
    {
        printf("No FCD detected.\n");
    }
    else if (stat == FCD_MODE_APP)
    {
        printf("FCD detceted in application mode, switching to bootloader..\n");
        fcdAppReset();
        printf("Please check /var/log/[messages|syslog] to confirm mode change, then re-run dump\n");
    }
    else if (stat == FCD_MODE_BL)
    {
        stat = fcdBlSaveFirmwareProg(saveFile, progress);
        if (stat == FCD_MODE_BL)
            printf("Firmware saved to: %s, FCD remains in bootloader mode (use -r to reset).\n", dump);
        else
            printf("Unable to save firmware to: %s, (is it writeable?)\n", dump);
    }
    fclose(saveFile);
}

void reset_fcd()
{
    int stat = fcdGetMode();
    if (stat == FCD_MODE_NONE)
    {
        printf("No FCD detected.\n");
    }
    else if (stat == FCD_MODE_APP)
    {
        printf("FCD in application mode, resetting to bootloader.\n");
        fcdAppReset();
    }
    else if (stat == FCD_MODE_BL)
    {
        printf("FCD in bootloader mode, resetting to application.\n");
        fcdBlReset();
    }
    printf("Reset completed, please check /var/log/[message|syslog] to confirm.\n");
}

int main(int argc, char* argv[])
{
    int stat;
    int freq = 0;
    double freqf = 0;
    int gain = -999;
    int corr = 0;
    int dolist = 0;
    int dostatus = 0;
    int doreset = 0;
    char *firm = NULL;
    char *dump = NULL;

    /* getopt infrastructure */
    int next_option;
    const char* const short_options = "slrg:f:c:i:d:u:h";
    const struct option long_options[] =
    {
        { "status", 0, NULL, 's' },
        { "list", 0, NULL, 'l' },
        { "reset", 0, NULL, 'r' },
        { "frequency", 1, NULL, 'f' },
        { "index", 1, NULL, 'i' },
        { "gain", 1, NULL, 'g' },
        { "correction", 1, NULL, 'c' },
        { "dump", 1, NULL, 'd' },
        { "update", 1, NULL, 'u' },
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
            case 'r' :
                doreset=1;
                break;
            case 'f' :
                freqf = atof(optarg);
                break;
            case 'g' :
                gain = atoi(optarg);
                break;
            case 'i' :
                whichdongle = atoi(optarg);
                break;
            case 'c' :
                corr = atoi(optarg);
                break;
            case 'd' :
                dump = optarg;
                break;
            case 'u' :
                firm = optarg;
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
        int hwvr = fcdGetVersion();
        unsigned char b=0;
        if (FCD_VERSION_2 == hwvr) {
            b = gain ? 1 : 0;
        } else {
            while (b<sizeof(lnagainvalues)/sizeof(lnagainvalues[0]) && gain>lnagainvalues[b]+1) b++;
        }
        stat = fcdAppSetParam(FCD_CMD_APP_SET_LNA_GAIN,&b,1);
        if (stat == FCD_MODE_NONE) { printf("No FCD Detected.\n"); return 1; }
        else if (stat == FCD_MODE_BL) { printf("FCD in bootloader mode.\n"); return 1; }
        else if (FCD_VERSION_2 == hwvr)
            printf("LNA gain %s.\n", b ? "enabled" : "disabled");
        else
            printf("LNA gain set to %g dB.\n",lnagainvalues[b]);
    }

    if (dump) dump_firm(dump);

    if (firm) update_firm(firm);

    if (dolist) print_list();

    if (dostatus) print_status();

    if (doreset) reset_fcd();

    return EXIT_SUCCESS;
}
