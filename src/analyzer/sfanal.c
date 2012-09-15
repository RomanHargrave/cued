/*
** Copyright (C) 2008 Robert W. Fuller <hydrologiccycle@gmail.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "opt.h"
#include "macros.h"
#include "unix.h"

#ifdef __cplusplus
#define __STDC_LIMIT_MACROS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sndfile.h>


#define BUFFER_LEN          (64 * 1024)
#define SHORT_BUFFER_LEN    (BUFFER_LEN / sizeof(short))
#define DBL_BUFFER_LEN      (BUFFER_LEN / sizeof(double))
#define INT_BUFFER_LEN      (BUFFER_LEN / sizeof(int))

union _sfdata {

    short  h[SHORT_BUFFER_LEN];
    int    i[  INT_BUFFER_LEN];
    double d[  DBL_BUFFER_LEN];

} sfdata;


static void usage(char *progname)
{
    fprintf(stderr, "\nUsage : %s <infile> [infile2] [infile3...]\n", progname);
    exit(EXIT_FAILURE);
}


static int guard1;
static int amplitudeData[ UINT16_MAX + 1 ];
static int guard2;
static int *amplitudes = &amplitudeData[ -INT16_MIN ];
static int samples;

static void readSndfile(SNDFILE *infile, int channels)
{
    int frames, readcount, i;

    memset(amplitudeData, 0x00, sizeof(amplitudeData));
    samples = 0;

    readcount = frames = SHORT_BUFFER_LEN / channels;

    while (readcount > 0) {

        // TODO:  check return code?
        readcount = sf_readf_short(infile, sfdata.h, frames);
        for (i = 0;  i < readcount;  ++i) {
            ++(amplitudes[ sfdata.h[i] ]);
        }
        samples += readcount;
    }
}


int main(int argc, char *argv[])
{
    SF_INFO sfinfo;
    char *progname, *infilename;
    SNDFILE *infile;
    int infileminor;
    int i, j, c;
    double factor;

    progname = (char *) basename2(argv[0]);
    if (argc < 2) {
        usage(progname);
    }
    
    for (i = 1;  i < argc;  ++i) {

        infilename = argv[i];

        memset(&sfinfo, 0x00, sizeof(sfinfo));
        infile = sf_open(infilename, SFM_READ, &sfinfo);
        if (!infile) {
            fprintf(stderr, "fatal:  not able to open input file %s : %s\n", infilename, sf_strerror(NULL));
            return 2;
        }

        infileminor = sfinfo.format & SF_FORMAT_SUBMASK;
        if (SF_FORMAT_PCM_16 != infileminor) {
            fprintf(stderr, "fatal:  file %s is not 16-bit PCM\n", infilename);
            return 3;
        }

        readSndfile(infile, sfinfo.channels);
        if (guard1 || guard2) {
            fprintf(stderr, "fatal:  out of bounds writing data array while processing %s\n", infilename);
            return 4;
        }
        if (sf_close(infile)) {
            fprintf(stderr, "fatal:  could not close %s : %s\n", infilename, sf_strerror(NULL));
            return 5;
        }

    #if 0
        int min, max;

        for (min = INT16_MIN;  min <= INT16_MAX;  ++min) {
            if (amplitudes[min]) {
                printf("%d samples had minimum sample value of %d (%.4f%%)\n", amplitudes[min], min, amplitudes[min] * 100.0 / samples);
                break;
            }
        }

        for (max = INT16_MAX;  max >= INT16_MIN;  --max) {
            if (amplitudes[max]) {
                printf("%d samples had maximum sample value of %d (%.4f%%)\n", amplitudes[max], max, amplitudes[max] * 100.0 / samples);
                break;
            }
        }
    #endif

        c = 0;

        // 50 looked good;  100 is no different;  MACHINA
        for (j = INT16_MAX - 2500;  j <= INT16_MAX;  ++j) {
            c += amplitudes[j];
        }

        factor = c * 100.0 / samples;

        // .050 looked good;  .025 is winnowing the chaff
        if (factor > .0125) {
            printf("(factor=%.4f%%) %s is brickwall mastered\n", factor, infilename);
        }
    }

    return 0;
}
