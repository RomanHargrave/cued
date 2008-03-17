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

#define BUFFER_LEN      (64 * 1024)
#define FP_BUFFER_LEN   (BUFFER_LEN / sizeof(double))
#define INT_BUFFER_LEN  (BUFFER_LEN / sizeof(int))


typedef struct
{   char    *infilename, *outfilename ;
    SF_INFO  infileinfo,  outfileinfo ;
} OptionData ;

typedef struct
{   const char  *ext ;
    int         len ;
    int         format ;
} OUTPUT_FORMAT_MAP ;

static void copy_metadata (SNDFILE *outfile, SNDFILE *infile) ;
static void copy_data_fp  (SNDFILE *outfile, SNDFILE *infile, int channels) ;
static void copy_data_int (SNDFILE *outfile, SNDFILE *infile, int channels) ;

static OUTPUT_FORMAT_MAP format_map [] =
{
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

} ; /* format_map */

static int
guess_output_file_type (char *str, int format)
{   char    buffer [16], *cptr ;
    int     k ;

    format &= SF_FORMAT_SUBMASK ;

    if ((cptr = strrchr (str, '.')) == NULL)
        return 0 ;

    strncpy (buffer, cptr + 1, 15) ;
    buffer [15] = 0 ;

    for (k = 0 ; buffer [k] ; k++)
        buffer [k] = tolower ((buffer [k])) ;

    if (strcmp (buffer, "gsm") == 0)
        return SF_FORMAT_RAW | SF_FORMAT_GSM610 ;

    if (strcmp (buffer, "vox") == 0)
        return SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM ;

    for (k = 0 ; k < SNELEMS(format_map) ; k++)
    {   if (format_map [k].len > 0 && strncmp (buffer, format_map [k].ext, format_map [k].len) == 0)
            return format_map [k].format | format ;
        else if (strcmp (buffer, format_map [k].ext) == 0)
            return format_map [k].format | format ;
        } ;

    return  0 ;
} /* guess_output_file_type */


static void
print_usage (char *progname)
{   SF_FORMAT_INFO  info ;

    int k ;

    printf ("\nUsage : %s [options] <infile1> [infile2] [infile3...] <output file>\n", progname) ;
    puts ("\n"
        "    where [options] may be one of the following:\n\n"
        "        --pcms8     : force the output to signed 8 bit pcm\n"
        "        --pcmu8     : force the output to unsigned 8 bit pcm\n"
        "        --pcm16     : force the output to 16 bit pcm\n"
        "        --pcm24     : force the output to 24 bit pcm\n"
        "        --pcm32     : force the output to 32 bit pcm\n"
        "        --float32   : force the output to 32 bit floating point"
        ) ;
    puts (
        "        --ulaw      : force the output ULAW\n"
        "        --alaw      : force the output ALAW\n"
        "        --ima-adpcm : force the output to IMA ADPCM (WAV only)\n"
        "        --ms-adpcm  : force the output to MS ADPCM (WAV only)\n"
        "        --gsm610    : force the GSM6.10 (WAV only)\n"
        "        --dwvw12    : force the output to 12 bit DWVW (AIFF only)\n"
        "        --dwvw16    : force the output to 16 bit DWVW (AIFF only)\n"
        "        --dwvw24    : force the output to 24 bit DWVW (AIFF only)\n"
        ) ;

    puts (
        "    The format of the output file is determined by the file extension of the\n"
        "    output file name. The following extensions are currently understood:\n"
        ) ;

    for (k = 0 ; k < SNELEMS(format_map) ; k++)
    {   info.format = format_map [k].format ;
        sf_command (NULL, SFC_GET_FORMAT_INFO, &info, sizeof (info)) ;
        printf ("         %-10s : %s\n", format_map [k].ext, info.name) ;
        } ;

    puts ("") ;
} /* print_usage */


static int outfileminor;

static void set_outfileminor(void *context, char *optarg, char *optname)
{
    outfileminor = (intptr_t) context;
}


int
main (int argc, char * argv [])
{   char        *progname, *infilename, *outfilename ;
    SNDFILE     *infile = NULL, *outfile = NULL ;
    SF_INFO     sfinfo1, sfinfo2 ;
    int         i, outfilemajor, outfileminor = 0, infileminor ;

    progname = (char *) basename2(argv[0]);

    opt_param_t opts[] = {
        { "pcms8",     (void *) SF_FORMAT_PCM_S8,    set_outfileminor, OPT_NONE },
        { "pcmu8",     (void *) SF_FORMAT_PCM_U8,    set_outfileminor, OPT_NONE },
        { "pcm16",     (void *) SF_FORMAT_PCM_16,    set_outfileminor, OPT_NONE },
        { "pcm24",     (void *) SF_FORMAT_PCM_24,    set_outfileminor, OPT_NONE },
        { "pcm32",     (void *) SF_FORMAT_PCM_32,    set_outfileminor, OPT_NONE },
        { "float32",   (void *) SF_FORMAT_FLOAT,     set_outfileminor, OPT_NONE },
        { "ulaw",      (void *) SF_FORMAT_ULAW,      set_outfileminor, OPT_NONE },
        { "alaw",      (void *) SF_FORMAT_ALAW,      set_outfileminor, OPT_NONE },
        { "ima-adpcm", (void *) SF_FORMAT_IMA_ADPCM, set_outfileminor, OPT_NONE },    
        { "ms-adpcm",  (void *) SF_FORMAT_MS_ADPCM,  set_outfileminor, OPT_NONE },
        { "gsm610",    (void *) SF_FORMAT_GSM610,    set_outfileminor, OPT_NONE },
        { "dwvw12",    (void *) SF_FORMAT_DWVW_12,   set_outfileminor, OPT_NONE },
        { "dwvw16",    (void *) SF_FORMAT_DWVW_16,   set_outfileminor, OPT_NONE },
        { "dwvw24",    (void *) SF_FORMAT_DWVW_24,   set_outfileminor, OPT_NONE }
    };

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

    infilename = argv[optind];
    outfilename = argv[argc - 1];

    if ((infile = sf_open (infilename, SFM_READ, &sfinfo1)) == NULL)
    {   printf ("Not able to open input file %s.\n", infilename) ;
        puts (sf_strerror (NULL)) ;
        return 1 ;
        } ;

    infileminor = sfinfo1.format & SF_FORMAT_SUBMASK ;

    if ((sfinfo1.format = guess_output_file_type (outfilename, sfinfo1.format)) == 0)
    {   printf ("Error : Not able to determine output file type for %s.\n", outfilename) ;
        return 1 ;
        } ;

    outfilemajor = sfinfo1.format & (SF_FORMAT_TYPEMASK | SF_FORMAT_ENDMASK) ;

    if (outfileminor == 0)
        outfileminor = sfinfo1.format & SF_FORMAT_SUBMASK ;

    if (outfileminor != 0)
        sfinfo1.format = outfilemajor | outfileminor ;
    else
        sfinfo1.format = outfilemajor | (sfinfo1.format & SF_FORMAT_SUBMASK) ;

    if ((sfinfo1.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_XI)
        switch (sfinfo1.format & SF_FORMAT_SUBMASK)
        {   case SF_FORMAT_PCM_16 :
                    sfinfo1.format = outfilemajor | SF_FORMAT_DPCM_16 ;
                    break ;

            case SF_FORMAT_PCM_S8 :
            case SF_FORMAT_PCM_U8 :
                    sfinfo1.format = outfilemajor | SF_FORMAT_DPCM_8 ;
                    break ;
            } ;

    if (sf_format_check (&sfinfo1) == 0)
    {   printf ("Error : output file format is invalid (0x%08X).\n", sfinfo1.format) ;
        return 1 ;
        } ;

    /* Open the output file. */
    if ((outfile = sf_open (outfilename, SFM_WRITE, &sfinfo1)) == NULL)
    {   printf ("Not able to open output file %s : %s\n", outfilename, sf_strerror (NULL)) ;
        return 1 ;
        } ;

    /* Copy the metadata */
    copy_metadata (outfile, infile) ;

    for (i = optind ; ; ) {

        if (strcmp(infilename, outfilename)) {
            printf("Copying file \"%s.\"\n", infilename);

            if ((outfileminor == SF_FORMAT_DOUBLE) || (outfileminor == SF_FORMAT_FLOAT) ||
                        (infileminor == SF_FORMAT_DOUBLE) || (infileminor == SF_FORMAT_FLOAT))
                copy_data_fp (outfile, infile, sfinfo1.channels) ;
            else
                copy_data_int (outfile, infile, sfinfo1.channels) ;
        } else {
            printf("Skipping an input file with the same name as the output file.\n") ;
        }

        sf_close (infile) ;

        if (++i == argc - 1) {
            break;
        }

        infilename = argv[i];

        if ((infile = sf_open (infilename, SFM_READ, &sfinfo2)) == NULL)
        {   printf ("Not able to open input file %s.\n", infilename) ;
            puts (sf_strerror (NULL)) ;
            return 1 ;
            }

        if (sfinfo2.samplerate != sfinfo1.samplerate) {
            printf("Sample rate of file %s is incompatible with other files.\n", infilename);
            return 1;
        }

        if (sfinfo2.channels != sfinfo1.channels) {
            printf("Number of channels in file %s is incompatible with other files.\n", infilename);
            return 1;
        }
    }

    printf("Done.\n");

    sf_close (outfile) ;

    return 0 ;
} /* main */

static void
copy_metadata (SNDFILE *outfile, SNDFILE *infile)
{   SF_INSTRUMENT inst ;
    const char *str ;
    int k, err = 0 ;

    for (k = SF_STR_FIRST ; k <= SF_STR_LAST ; k++)
    {   str = sf_get_string (infile, k) ;
        if (str != NULL)
            err = sf_set_string (outfile, k, str) ;
        } ;

    memset (&inst, 0, sizeof (inst)) ;
    if (sf_command (infile, SFC_GET_INSTRUMENT, &inst, sizeof (inst)) == SF_TRUE)
        sf_command (outfile, SFC_SET_INSTRUMENT, &inst, sizeof (inst)) ;

} /* copy_metadata */

static void
copy_data_fp (SNDFILE *outfile, SNDFILE *infile, int channels)
{   static double   data [FP_BUFFER_LEN], max ;
    int     frames, readcount, k ;

    frames = FP_BUFFER_LEN / channels ;
    readcount = frames ;

    sf_command (infile, SFC_CALC_SIGNAL_MAX, &max, sizeof (max)) ;

    /* RWF: range is inclusive of 1.0, not exclusive;  changed < to <= */
    if (max <= 1.0)
    {   while (readcount > 0)
        {   readcount = sf_readf_double (infile, data, frames) ;
            sf_writef_double (outfile, data, readcount) ;
            } ;
        }
    else
    {   sf_command (infile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE) ;

        while (readcount > 0)
        {   readcount = sf_readf_double (infile, data, frames) ;
            for (k = 0 ; k < readcount * channels ; k++)
                data [k] /= max ;
            sf_writef_double (outfile, data, readcount) ;
            } ;
        } ;

    return ;
} /* copy_data_fp */

static void
copy_data_int (SNDFILE *outfile, SNDFILE *infile, int channels)
{   static int  data [INT_BUFFER_LEN] ;
    int     frames, readcount ;

    frames = INT_BUFFER_LEN / channels ;
    readcount = frames ;

    while (readcount > 0)
    {   readcount = sf_readf_int (infile, data, frames) ;
        sf_writef_int (outfile, data, readcount) ;
        } ;

    return ;
} /* copy_data_int */

/*
** Do not edit or modify anything in this comment block.
** The arch-tag line is a file identity tag for the GNU Arch
** revision control system.
**
** arch-tag: 259682b3-2887-48a6-b5bb-3cde00521ba3
*/
