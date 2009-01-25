/*
** Copyright (C) 1999-2005 Erik de Castro Lopo <erikd@mega-nerd.com>
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
**
** TODO:  always check return code of sf_command, sf_close, and sf_write*
**
*/

#include "opt.h"
#include "macros.h"
#include "unix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> /* optind */

#include <sndfile.h>


static void copy_metadata(SNDFILE *outfile, SNDFILE *infile);
static void copy_data_fp (SNDFILE *outfile, SNDFILE *infile, int channels);
static void copy_data_int(SNDFILE *outfile, SNDFILE *infile, int channels);


#define BUFFER_LEN      (64 * 1024)
#define DBL_BUFFER_LEN  (BUFFER_LEN / sizeof(double))
#define INT_BUFFER_LEN  (BUFFER_LEN / sizeof(int))

union _data {

    double d[DBL_BUFFER_LEN];
    int    i[INT_BUFFER_LEN];

} data;


typedef struct {
    const char *ext;
    int         len;
    int         format;
} OUTPUT_FORMAT_MAP;

static OUTPUT_FORMAT_MAP format_map[] = {
    {   "aif",      3,  SF_FORMAT_AIFF  },
    {   "wav",      0,  SF_FORMAT_WAV   },
    {   "au",       0,  SF_FORMAT_AU    },
    {   "caf",      0,  SF_FORMAT_CAF   },
    {   "flac",     0,  SF_FORMAT_FLAC  },
    {   "snd",      0,  SF_FORMAT_AU    },
    {   "svx",      0,  SF_FORMAT_SVX   },
    {   "paf",      0,  SF_ENDIAN_BIG    | SF_FORMAT_PAF },
    {   "fap",      0,  SF_ENDIAN_LITTLE | SF_FORMAT_PAF },
    {   "gsm",      0,  SF_FORMAT_RAW   },
    {   "nist",     0,  SF_FORMAT_NIST  },
    {   "ircam",    0,  SF_FORMAT_IRCAM },
    {   "sf",       0,  SF_FORMAT_IRCAM },
    {   "voc",      0,  SF_FORMAT_VOC   },
    {   "w64",      0,  SF_FORMAT_W64   },
    {   "raw",      0,  SF_FORMAT_RAW   },
    {   "mat4",     0,  SF_FORMAT_MAT4  },
    {   "mat5",     0,  SF_FORMAT_MAT5  },
    {   "mat",      0,  SF_FORMAT_MAT4  },
    {   "pvf",      0,  SF_FORMAT_PVF   },
    {   "sds",      0,  SF_FORMAT_SDS   },
    {   "sd2",      0,  SF_FORMAT_SD2   },
    {   "vox",      0,  SF_FORMAT_RAW   },
    {   "xi",       0,  SF_FORMAT_XI    },
    {   "bin",      0,  SF_FORMAT_RAW   }

}; /* format_map */


static int guess_output_file_type(char *str, int format)
{
    char buffer[16], *cptr;
    int i;

    format &= SF_FORMAT_SUBMASK;

    if (!(cptr = strrchr(str, '.'))) {
        return 0;
    }

    strncpy(buffer, cptr + 1, sizeof(buffer));
    buffer[ sizeof(buffer) - 1 ] = 0;

    for (i = 0;  buffer[i];  ++i) {
        buffer[i] = tolower((buffer[i]));
    }

    if (!(strcmp(buffer, "gsm"))) {
        return SF_FORMAT_RAW | SF_FORMAT_GSM610;
    }

    if (!(strcmp(buffer, "vox"))) {
        return SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM;
    }

    for (i = 0;  i < SNELEMS(format_map);  ++i) {
        if (format_map[i].len > 0 && !(strncmp(buffer, format_map[i].ext, format_map[i].len))) {
            return format_map[i].format | format;
        }
        else if (!(strcmp(buffer, format_map[i].ext))) {
            return format_map[i].format | format;
        }
    }

    return 0;

} /* guess_output_file_type */


static void print_usage(char *progname)
{
    SF_FORMAT_INFO info;
    int i;

    fprintf(stderr, "\nUsage : %s [options] <infile1> [infile2] [infile3...] <output file>\n", progname);
    fprintf(stderr,
       "    Force the <output file> with the following [options]:\n"
       "        --pcms8,     --pcmu8             : 8 bit signed, unsigned pcm\n"
       "        --pcm16,     --pcm24,   --pcm32  : 16, 24, 32 bit signed pcm\n"
       "        --float32,   --float64           : 32, 64 bit floating point pcm\n"
       "        --ulaw,      --alaw              : ULAW, ALAW\n"
       "        --ima-adpcm, --ms-adpcm          : IMA/MS ADPCM (WAV only)\n"
       "        --gsm610                         : GSM6.10 (WAV only)\n"
       "        --dwvw12,    --dwvw16,  --dwvw24 : 12, 16, 24 bit DWVW (AIFF only)\n"
       "    Force the <infile> with the following [options]:\n"
       "        --in-pcms8,   --in-pcmu8,   --in-pcm16,  --in-pcm24, --in-pcm32\n"
       "        --in-float32, --in-float64, --in-gsm610\n"
       "        --in-channels=number of channels,   --in-rate=sample rate (Hz)\n"
       "        --in-cd : shorthand for --in-pcm16, --in-rate=44100, --in-channels=2\n"
       "    Miscellaneous [options]:\n"
       "        --silent : do not display progress messages\n"
       "    The format of the <output file> is determined by the file extension of the\n"
       "    <output file> name. The following extensions are currently understood:\n"
       );

    for (i = 0;  i < SNELEMS(format_map);  ++i) {
        info.format = format_map[i].format;
        sf_command(NULL, SFC_GET_FORMAT_INFO, &info, sizeof(info));
        fprintf(stderr, "        %-5s : %s\n", format_map[i].ext, info.name);
    }

    fprintf(stderr, "\n");

} /* print_usage */


static int outfileminor;

static void set_outfileminor(void *context, char *optarg, const char *optname)
{
    outfileminor = (intptr_t) context;
}


static SF_INFO proto;

static void set_rawfileminor(void *context, char *optarg, const char *optname)
{
    int minor = (intptr_t) context;
    proto.format = minor | SF_FORMAT_RAW;
}

static void set_in_cd(void *context, char *optarg, const char *optname)
{
    proto.format     = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
    proto.samplerate = 44100;
    proto.channels   = 2;
}


int main(int argc, char * argv[])
{
    SF_INFO outinfo, ininfo;
    char *progname, *infilename, *outfilename;
    SNDFILE *infile, *outfile;
    int i, outfilemajor, infileminor, silent;
    opt_param_t opts[] = {
        { "pcms8",       (void *) SF_FORMAT_PCM_S8,    set_outfileminor, OPT_NONE },
        { "pcmu8",       (void *) SF_FORMAT_PCM_U8,    set_outfileminor, OPT_NONE },
        { "pcm16",       (void *) SF_FORMAT_PCM_16,    set_outfileminor, OPT_NONE },
        { "pcm24",       (void *) SF_FORMAT_PCM_24,    set_outfileminor, OPT_NONE },
        { "pcm32",       (void *) SF_FORMAT_PCM_32,    set_outfileminor, OPT_NONE },
        { "float32",     (void *) SF_FORMAT_FLOAT,     set_outfileminor, OPT_NONE },
        { "float64",     (void *) SF_FORMAT_DOUBLE,    set_outfileminor, OPT_NONE },

        { "ulaw",        (void *) SF_FORMAT_ULAW,      set_outfileminor, OPT_NONE },
        { "alaw",        (void *) SF_FORMAT_ALAW,      set_outfileminor, OPT_NONE },
        { "ima-adpcm",   (void *) SF_FORMAT_IMA_ADPCM, set_outfileminor, OPT_NONE },    
        { "ms-adpcm",    (void *) SF_FORMAT_MS_ADPCM,  set_outfileminor, OPT_NONE },
        { "gsm610",      (void *) SF_FORMAT_GSM610,    set_outfileminor, OPT_NONE },
        { "dwvw12",      (void *) SF_FORMAT_DWVW_12,   set_outfileminor, OPT_NONE },
        { "dwvw16",      (void *) SF_FORMAT_DWVW_16,   set_outfileminor, OPT_NONE },
        { "dwvw24",      (void *) SF_FORMAT_DWVW_24,   set_outfileminor, OPT_NONE },

        { "in-pcms8",    (void *) SF_FORMAT_PCM_S8,    set_rawfileminor, OPT_NONE },
        { "in-pcmu8",    (void *) SF_FORMAT_PCM_U8,    set_rawfileminor, OPT_NONE },
        { "in-pcm16",    (void *) SF_FORMAT_PCM_16,    set_rawfileminor, OPT_NONE },
        { "in-pcm24",    (void *) SF_FORMAT_PCM_24,    set_rawfileminor, OPT_NONE },
        { "in-pcm32",    (void *) SF_FORMAT_PCM_32,    set_rawfileminor, OPT_NONE },
        { "in-float32",  (void *) SF_FORMAT_FLOAT,     set_rawfileminor, OPT_NONE },
        { "in-float64",  (void *) SF_FORMAT_DOUBLE,    set_rawfileminor, OPT_NONE },
        { "in-gsm610",   (void *) SF_FORMAT_GSM610,    set_rawfileminor, OPT_NONE },

        { "in-channels", (void *) &proto.channels,     opt_set_nat_no,   OPT_REQUIRED },
        { "in-rate",     (void *) &proto.samplerate,   opt_set_nat_no,   OPT_REQUIRED },
        { "in-cd",       NULL,                         set_in_cd,        OPT_NONE },

        { "silent",      &silent,                      opt_set_flag,     OPT_NONE }
    };

    silent = 0;

    progname = (char *) basename2(argv[0]);
    opt_register_params(opts, NELEMS(opts), 0, 0);
    switch (opt_parse_args(argc, argv)) {

        case OPT_SUCCESS:
            break;

        case OPT_INVALID:
        case OPT_NOT_FOUND:
        case OPT_FAILURE:
        default:
            print_usage(progname);
            return 1;
    }

    /* there should be at least two arguments after the options */
    if (optind > argc - 2) {
        print_usage(progname);
        return 1;
    }

    infilename  = argv[ optind ];
    outfilename = argv[ argc - 1 ];

    if (!(infile = sf_open(infilename, SFM_READ, &proto))) {
        fprintf(stderr, "Not able to open input file %s : %s\n", infilename, sf_strerror(NULL));
        return 1;
    }

    infileminor = proto.format & SF_FORMAT_SUBMASK;
    outinfo = proto;

    if (!(outinfo.format = guess_output_file_type(outfilename, proto.format))) {
        fprintf(stderr, "Error : Not able to determine output file type for %s.\n", outfilename);
        return 1;
    }

    outfilemajor = outinfo.format & (SF_FORMAT_TYPEMASK | SF_FORMAT_ENDMASK);
    if (!outfileminor) {
        outfileminor = outinfo.format & SF_FORMAT_SUBMASK;
    }
    outinfo.format = outfilemajor | outfileminor;

    if (SF_FORMAT_XI == (outinfo.format & SF_FORMAT_TYPEMASK)) {
        switch (outinfo.format & SF_FORMAT_SUBMASK) {
            case SF_FORMAT_PCM_16:
                outinfo.format = outfilemajor | SF_FORMAT_DPCM_16;
                break;

            case SF_FORMAT_PCM_S8:
            case SF_FORMAT_PCM_U8:
                outinfo.format = outfilemajor | SF_FORMAT_DPCM_8;
                break;
        }
    }

    if (!(sf_format_check(&outinfo))) {
        fprintf(stderr, "Error : output file format is invalid (0x%08X).\n", outinfo.format);
        return 1;
    }

    /* Open the output file. */
    if (!(outfile = sf_open(outfilename, SFM_WRITE, &outinfo))) {
        fprintf(stderr, "Not able to open output file %s : %s\n", outfilename, sf_strerror(NULL));
        return 1;
    }

    copy_metadata(outfile, infile);

    for (i = optind;  ;  ) {

        if (strcmp(infilename, outfilename)) {
            if (!silent) {
                printf("Copying file \"%s.\"\n", infilename);
            }

            if (   (SF_FORMAT_DOUBLE == outfileminor) || (SF_FORMAT_FLOAT == outfileminor)
                || (SF_FORMAT_DOUBLE == infileminor)  || (SF_FORMAT_FLOAT == infileminor))
            {
                copy_data_fp (outfile, infile, outinfo.channels);
            }
            else
            {
                copy_data_int(outfile, infile, outinfo.channels);
            }
        } else {
            fprintf(stderr, "Skipping an input file with the same name as the output file.\n");
        }

        sf_close(infile);

        if (argc - 1 == ++i) {
            break;
        }

        infilename = argv[i];
        ininfo = proto;

        if (!(infile = sf_open(infilename, SFM_READ, &ininfo))) {
            fprintf(stderr, "Not able to open input file %s : %s\n", infilename, sf_strerror(NULL));
            return 1;
        }

        if (ininfo.samplerate != outinfo.samplerate) {
            fprintf(stderr, "Sample rate of file %s is incompatible with other files.\n", infilename);
            return 1;
        }

        if (ininfo.channels   != outinfo.channels) {
            fprintf(stderr, "Number of channels in file %s is incompatible with other files.\n", infilename);
            return 1;
        }
    }

    if (!silent) {
        printf("Done.\n");
    }

    sf_close(outfile);

    return 0;

} /* main */


static void copy_metadata(SNDFILE *outfile, SNDFILE *infile)
{
    SF_INSTRUMENT inst;
    const char *str;
    int i, err = 0;

    for (i = SF_STR_FIRST;  i <= SF_STR_LAST;  ++i) {
        str = sf_get_string(infile, i);
        if (str) {
            err = sf_set_string(outfile, i, str);
        }
    }

    memset(&inst, 0x00, sizeof(inst));
    if (SF_TRUE == sf_command(infile, SFC_GET_INSTRUMENT, &inst, sizeof(inst))) {
        sf_command(outfile, SFC_SET_INSTRUMENT, &inst, sizeof(inst));
    }

} /* copy_metadata */


static void copy_data_fp(SNDFILE *outfile, SNDFILE *infile, int channels)
{
    double max;
    int frames, readcount, i;

    readcount = frames = DBL_BUFFER_LEN / channels;

    sf_command(infile, SFC_CALC_SIGNAL_MAX, &max, sizeof(max));

    /* RWF: range is inclusive of 1.0, not exclusive;  changed < to <= */
    if (max <= 1.0) {
        while (readcount > 0) {
            readcount = sf_readf_double(infile, data.d, frames);
            sf_writef_double(outfile, data.d, readcount);
        }
    } else {
        sf_command(infile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);

        while (readcount > 0) {
            readcount = sf_readf_double(infile, data.d, frames);
            for (i = 0;  i < readcount * channels;  ++i) {
                data.d[i] /= max;
            }
            sf_writef_double(outfile, data.d, readcount);
        }
    }

} /* copy_data_fp */


static void copy_data_int(SNDFILE *outfile, SNDFILE *infile, int channels)
{
    int frames, readcount;

    readcount = frames = INT_BUFFER_LEN / channels;

    while (readcount > 0) {
        readcount = sf_readf_int(infile, data.i, frames);
        sf_writef_int(outfile, data.i, readcount);
    }

} /* copy_data_int */
