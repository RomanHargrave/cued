/*
** Copyright (C) 2008, 2009 Robert W. Fuller <hydrologiccycle@gmail.com>
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

//#define USE_BOYER

#if !defined(USE_BOYER) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "opt.h"
#include "macros.h"
#include "unix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h> /* optind */

#include <sndfile.h>


static ssize_t countLeadingZeros(char *a, ssize_t bytes, ssize_t granularity)
{
    ssize_t i;

    for (i = 0;  i < bytes;  ++i) {
        if (a[i]) {
            break;
        }
    }

    return i - i % granularity;
}


static ssize_t countTrailingZeros(char *a, ssize_t bytes, ssize_t granularity)
{
    ssize_t i, n;

    for (i = bytes - 1;  i >= 0;  --i) {
        if (a[i]) {
            break;
        }
    }

    n = bytes - 1 - i;

    return n - n % granularity;
}


#if 0

// working, but dead code
static inline ssize_t matchStrG(char *a, char *b, ssize_t bytes, ssize_t granularity)
{
    ssize_t i;

    for (i = 0;  i < bytes;  i += granularity) {
        if (memcmp(&a[i], &b[i], granularity)) {
            break;
        }
    }

    return i;
}

#endif


static inline ssize_t matchStr(char *a, char *b, ssize_t bytes)
{
    ssize_t i;

    for (i = 0;  i < bytes;  ++i) {
        if (a[i] != b[i]) {
            break;
        }
    }

    return i;
}


typedef struct _sndfile_data {

    char *filename;

    SF_INFO sfinfo;
    SNDFILE *sndfile;

    char *mapStart;
    ssize_t mappedSize, headerSize;

    char *audioDataStart, *audioStart;
    ssize_t audioDataBytes, audioBytes;
    ssize_t frameSize, trailingSilence;

    int fd;

} sndfile_data;


#define SFCMP_MIN(a, b) ((a) < (b) ? (a) : (b))

static int cmpSndFiles(sndfile_data *files, int initWindow, int resyncWindow, int initThreshold, int resyncThreshold)
{
    char *sw1, *sw2, *ew1, *ew2, *e1, *e2, *m1, *m2, *mw2;
    ssize_t n, threshold;

    // convert from seconds to bytes
    initWindow   *= files[0].sfinfo.samplerate * files[0].frameSize;
    resyncWindow *= files[0].sfinfo.samplerate * files[0].frameSize;

    threshold = initThreshold;

    if (   files[0].audioStart != files[0].audioDataStart 
        || files[1].audioStart != files[1].audioDataStart)
    {
        printf("[0, 0] - (%ld, %ld):  ignored leading silence\n",
            files[0].audioStart - files[0].audioDataStart,
            files[1].audioStart - files[1].audioDataStart);
    }

    // set end of audio pointers
    //

    e1 = files[0].audioStart + files[0].audioBytes;
    e2 = files[1].audioStart + files[1].audioBytes;

    sw1 = m1 = files[0].audioStart;
    ew1 = SFCMP_MIN(sw1 + initWindow, e1);

    sw2 = m2 = files[1].audioStart;
    ew2 = SFCMP_MIN(sw2 + initWindow, e2);

    while (sw1 < ew1) {

#ifdef USE_BOYER
        mw2 = bmh_memmem(sw2, ew2 - sw2, sw1, SFCMP_MIN(threshold, e1 - sw1));
#else
        mw2 = (char *) memmem(sw2, ew2 - sw2, sw1, SFCMP_MIN(threshold, e1 - sw1));
#endif
        if (mw2) {

            if (mw2[0] != sw1[0] && ew2 - sw2 > 0 && SFCMP_MIN(threshold, e1 - sw1) > 0) {
                printf("SPURIOUS MATCH FROM memmem");
                exit(EXIT_FAILURE);
            }

            // mw2 is where the match starts
            n = matchStr(mw2, sw1, SFCMP_MIN(e2 - mw2, e1 - sw1));

            if (m1 != sw1 || m2 != mw2) {
                printf("[%ld, %ld] - (%ld, %ld):  did not match [%ld, %ld] bytes\n",
                    m1  - files[0].audioDataStart,
                    m2  - files[1].audioDataStart,
                    sw1 - files[0].audioDataStart,
                    mw2 - files[1].audioDataStart,
                    sw1 - m1,
                    mw2 - m2
                    );
            }

            if (n == countLeadingZeros(sw1, n, 1)) {

                printf("[%ld, %ld] - (%ld, %ld):  %ld bytes of silence ignored\n",
                    sw1 - files[0].audioDataStart,
                    mw2 - files[1].audioDataStart,
                    sw1 - files[0].audioDataStart + n,
                    mw2 - files[1].audioDataStart + n,
                    n);

                //// don't match silence because it could get us out of sync?
                //goto next;

            } else {

                printf("[%ld, %ld] - (%ld, %ld):  %ld bytes matched\n",
                    sw1 - files[0].audioDataStart,
                    mw2 - files[1].audioDataStart,
                    sw1 - files[0].audioDataStart + n,
                    mw2 - files[1].audioDataStart + n,
                    n);

                threshold = resyncThreshold;
            }

            // skip over matched section
            //

            sw1 += n;
            ew1 = SFCMP_MIN(sw1 + resyncWindow, e1);

            sw2 = mw2 + n;
            ew2 = SFCMP_MIN(sw2 + resyncWindow, e2);

            // save match location
            //
            m1 = sw1;
            m2 = sw2;

            if (sw2 >= e2) {
                break;
            }

            continue;
        }

    //next:

        ++sw1;
    }

    if (m1 != e1 || m2 != e2) {
        printf("[%ld, %ld] - (%ld, %ld):  did not match [%ld, %ld] bytes\n",
            m1 - files[0].audioDataStart,
            m2 - files[1].audioDataStart,
            files[0].audioDataBytes - files[0].trailingSilence,
            files[1].audioDataBytes - files[1].trailingSilence,
            (files[0].audioDataBytes - files[0].trailingSilence) - (m1 - files[0].audioDataStart),
            (files[1].audioDataBytes - files[1].trailingSilence) - (m2 - files[1].audioDataStart)
            );
    }

    if (files[0].trailingSilence || files[1].trailingSilence) {
        printf("[%ld, %ld] - (%ld, %ld):  ignored trailing silence of [%ld, %ld] bytes\n",
            files[0].audioDataBytes - files[0].trailingSilence,
            files[1].audioDataBytes - files[1].trailingSilence,
            files[0].audioDataBytes,
            files[1].audioDataBytes,
            files[0].trailingSilence,
            files[1].trailingSilence
            );
    }

    return 0;
}


typedef sf_count_t (*sf_readf_fn)(SNDFILE *, char *, sf_count_t);

static int openSndFiles(sndfile_data *files[], int count, char *filenames[])
{
    sndfile_data *cmp;
    sf_readf_fn readfn;
    ssize_t s;
    SF_EMBED_FILE_INFO embedInfo;
    int i;

    cmp = (sndfile_data *) calloc(count, sizeof(sndfile_data));
    if (!cmp) {
        return 1;
    }

    // TODO:  error handling:  free memory, close files, etc.

    for (i = 0;  i < count;  ++i) {


        // open sound file
        //

        cmp[i].filename = filenames[i];

        cmp[i].fd = open(cmp[i].filename, O_RDONLY, 0666);
        if (cmp[i].fd < 0) {
            fprintf(stderr, "fatal:  %s could not be opened\n", cmp[i].filename);
            return 2;
        }


        cmp[i].sndfile = sf_open_fd(cmp[i].fd, SFM_READ, &cmp[i].sfinfo, SF_TRUE);
        if (!cmp[i].sndfile) {
            fprintf(stderr, "fatal:  %s could not be opened as a sound file : %s\n", cmp[i].filename, sf_strerror(NULL));
            return 3;
        }


        // determine frameSize
        //

        switch (cmp[i].sfinfo.format & SF_FORMAT_SUBMASK) {

            case SF_FORMAT_PCM_S8:
            case SF_FORMAT_PCM_U8:
                cmp[i].frameSize = 1;
                readfn = NULL;
                break;

            case SF_FORMAT_PCM_16:
                cmp[i].frameSize = 2;
                readfn = (sf_readf_fn) sf_readf_short;
                break;

            case SF_FORMAT_PCM_24:
                cmp[i].frameSize = 3;
                readfn = NULL;
                break;

            case SF_FORMAT_PCM_32:
                cmp[i].frameSize = 4;
                readfn = (sf_readf_fn) sf_readf_int;
                break;

            case SF_FORMAT_FLOAT:
                cmp[i].frameSize = 4;
                readfn = (sf_readf_fn) sf_readf_float;
                break;

            case SF_FORMAT_DOUBLE:
                cmp[i].frameSize = 8;
                readfn = (sf_readf_fn) sf_readf_double;
                break;

            default:
                fprintf(stderr, "fatal:  %s has unknown sample size\n", cmp[i].filename);
                return 4;
                break;
        }

        cmp[i].frameSize *= cmp[i].sfinfo.channels;

        // this is for convenience
        cmp[i].audioDataBytes = cmp[i].frameSize * cmp[i].sfinfo.frames;


        // determine header size of container
        //

        if (sf_command(cmp[i].sndfile, SFC_GET_EMBED_FILE_INFO, &embedInfo, sizeof(embedInfo))) {
            fprintf(stderr, "fatal:  unable to get container information for %s : %s\n", cmp[i].filename, sf_strerror(NULL));
            return 5;
        }

        cmp[i].mappedSize = embedInfo.offset + embedInfo.length;
        cmp[i].headerSize = cmp[i].mappedSize - cmp[i].audioDataBytes;

        // check for compression;  if it's compressed, mmap will not work;
        // this test isn't foolproof if the header is large relative to the size of the audio data
        //
        if (cmp[i].headerSize < 0) {

            if (!readfn) {
                fprintf(stderr, "fatal:  %s is compressed and libsndfile lacks appropriate read function\n", cmp[i].filename);
                return 6;
            }

            cmp[i].mapStart = (char *) mmap(NULL, cmp[i].audioDataBytes, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
            if (!cmp[i].mapStart) {
                fprintf(stderr, "fatal:  %s could not allocate %ld bytes for compressed file\n", cmp[i].filename, cmp[i].audioDataBytes);
                return 7;
            }

            if (cmp[i].sfinfo.frames != readfn(cmp[i].sndfile, cmp[i].mapStart, cmp[i].sfinfo.frames)) {
                fprintf(stderr, "fatal:  %s could not be read : %s\n", cmp[i].filename, sf_strerror(NULL));
                return 8;
            }

            cmp[i].audioDataStart = cmp[i].mapStart;
            cmp[i].mappedSize = cmp[i].audioDataBytes;
            cmp[i].headerSize = 0;

        } else {

            // memory map the audio samples
            //

            cmp[i].mapStart = (char *) mmap(NULL, cmp[i].mappedSize, PROT_READ, MAP_SHARED, cmp[i].fd, 0);
            if (MAP_FAILED == cmp[i].mapStart) {
                fprintf(stderr, "fatal:  %s could not be mapped\n", cmp[i].filename);
                return 9;
            }

            cmp[i].audioDataStart = cmp[i].mapStart + cmp[i].headerSize;
        }


        // identify leading and trailing silence
        //

        s = countLeadingZeros(cmp[i].audioDataStart, cmp[i].audioDataBytes, cmp[i].frameSize);
        cmp[i].audioStart = cmp[i].audioDataStart + s;
        cmp[i].audioBytes = cmp[i].audioDataBytes - s;

        cmp[i].trailingSilence = countTrailingZeros(cmp[i].audioDataStart, cmp[i].audioDataBytes, cmp[i].frameSize);
        cmp[i].audioBytes -= cmp[i].trailingSilence;
    }

    *files = cmp;

    return 0;
}


static int closeSndFiles(sndfile_data *files, int count)
{
    int i, rc;

    i = 0, rc = 0;
    for (i = 0;  i < count;  ++i) {

        rc <<= 2;
        
        if (sf_close(files[i].sndfile)) {
            fprintf(stderr, "error:  %s could not be closed\n", files[i].filename);
            rc += 1;
        }

        if (munmap(files[i].mapStart, files[i].mappedSize)) {
            fprintf(stderr, "error:  %s could not be unmapped\n", files[i].filename);
            rc += 2;
        }
    }

    return rc;
}


// match at least one sector (1/75th of a second)
#define INITIAL_THRESHOLD  (2352)
#define RESYNC_THRESHOLD   (INITIAL_THRESHOLD / 4)


// execution times for mismatch with CD-DA:
//
//      window  1 ==  0m28.788s
//      window  3 ==  0m58.146s
//      window  5 ==  1m40.410s
//      window  7 ==  2m29.845s
//      window  9 ==  3m42.397s
//      window 11 ==  5m11.618s
//      window 13 ==  8m51.871s
//      window 15 == 14m29.715s
//
// L2 cache = 512 KB per core
// L3 cache =   2 MB total
//
#define INITIAL_WINDOW 9
#define RESYNC_WINDOW  5


static void usage(const char *exeName)
{
    fprintf(stderr,
        "\n%s [options] <infile1> <infile2>\n"
            "\twhere options are:\n"
                "\t\t-i <seconds> set initial search window (defaults to %d)\n"
                "\t\t-r <seconds> set search window during re-sync (defaults to %d)\n"
                "\t\t-t <bytes> set initial match length threshold (defaults to %d)\n"
                "\t\t-u <bytes> set match length during re-sync (defaults to %d)\n"
                , exeName
                , INITIAL_WINDOW, RESYNC_WINDOW, INITIAL_THRESHOLD, RESYNC_THRESHOLD
                );

    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    char *progname;
    sndfile_data *cmp;
    int rc;

    // set defaults before option parsing
    //
    int optInitWindow = INITIAL_WINDOW;
    int optResyncWindow = RESYNC_WINDOW;
    int optInitThreshold = INITIAL_THRESHOLD;
    int optResyncThreshold = RESYNC_THRESHOLD;

    opt_param_t opts[] = {
        { "i", &optInitWindow,          opt_set_nat_no,     OPT_REQUIRED },
        { "r", &optResyncWindow,        opt_set_nat_no,     OPT_REQUIRED },
        { "t", &optInitThreshold,       opt_set_nat_no,     OPT_REQUIRED },
        { "u", &optResyncThreshold,     opt_set_nat_no,     OPT_REQUIRED },
    };

    progname = (char *) basename2(argv[0]);
    opt_register_params(opts, NELEMS(opts), 0, 0);
    switch (opt_parse_args(argc, argv)) {

        case OPT_SUCCESS:
            break;

        case OPT_INVALID:
        case OPT_NOT_FOUND:
        case OPT_FAILURE:
        default:
            usage(progname);
            return 1;
    }

    // there should be two arguments after the options
    if (optind != argc - 2) {
        usage(progname);
        return 1;
    }

    rc = openSndFiles(&cmp, 2, &argv[optind]);
    if (rc) {
        return rc;
    }
    rc = cmpSndFiles(cmp, optInitWindow, optResyncWindow, optInitThreshold, optResyncThreshold);
    (void) closeSndFiles(cmp, 2);

    return rc;
}
