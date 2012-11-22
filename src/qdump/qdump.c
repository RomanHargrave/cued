//
// qdump.c
//
// Copyright (C) 2007,2008 Robert William Fuller <hydrologiccycle@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  run:
//      qdump -q qchannel >qchannel.txt
//

#include "unix.h"
#include "macros.h"
#include "qsc.h"
#include "opt.h"

#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // *opt*
#include <errno.h>


//#define DEBUG_MCN
//#define DEBUG_ISRC
//#define DEBUG_CRC
#define DEBUG_INDEX


void unixError(const char *fn, const char *arg, int isFatal)
{
    int quoted = strlen(arg);

    fprintf(stderr,
        "%s(%s%s%s) returned error \"%s\" (errno = %d)\n",
        fn, quoted ? "\"" : "", arg, quoted ? "\"" : "", strerror(errno), errno);

    if (isFatal) {
        exit(EXIT_FAILURE);
    }
}

void usage(const char *exeName)
{
    fprintf(stderr,
        "\n%s [options]\n"
            "\twhere options are:\n"
                "\t\t-q <filename> reads Q sub-channel from file\n"
                , exeName
                );

    exit(EXIT_FAILURE);
}


// procedural model fits too nicely;  no object disorientation needed
//

int main(int argc, char *const argv[])
{
    // things that will need to be freed
    //

    PIT(FILE, qSubChannelFile);

    // things that need default values
    //
    const char *qSubChannelFileName;

    // things that do not need to be freed and will be initialized on first use
    //

    PIT(const char, exeName);
    int psc, notpsc, crc_fail;
    qsc_file_buffer_t qsc;

    opt_param_t opts[] = {
        { "q", &qSubChannelFileName, opt_set_string, OPT_REQUIRED }
    };

    qSubChannelFileName = NULL;

    exeName = basename2(argv[0]);
    opt_register_params(opts, NELEMS(opts), 0, 0);
    switch (opt_parse_args(argc, argv)) {

        case OPT_SUCCESS:
            break;

        case OPT_INVALID:
            fprintf(stderr, "unknown option \"%c\" specified\n", optopt);
            usage(exeName);
            break;

        case OPT_NOT_FOUND:
            unixError("getopt", "", 1);
            break;

        case OPT_FAILURE:
            exit(EXIT_FAILURE);
            break;

        default:
            fprintf(stderr, "unexpected result from opt_parse_args (internal error)");
            exit(EXIT_FAILURE);
            break;
    }

    // there should be NO argument after the options
    if (argc != optind) {
        usage(exeName);
    }

    if (!qSubChannelFileName) {
        fprintf(stderr, "no Q sub-channel filename specified\n");
        usage(exeName);
    }

    if (!strcmp("-", qSubChannelFileName)) {
        qSubChannelFile = stdin;
    } else {
        qSubChannelFile = fopen2(qSubChannelFileName, O_RDONLY, 0666);
        if (!qSubChannelFile) {
            unixError("fopen2", qSubChannelFileName, 1);
        }
    }

    for (psc = notpsc = crc_fail = 0;  ;  ) {

        memset(&qsc.buf, 0x00, sizeof(qsc.buf));
        if (1 != fread(&qsc, sizeof(qsc), 1, qSubChannelFile)) {
            break;
        }

        if (qsc_check_crc(&qsc.buf)) {
#ifdef DEBUG_CRC
            fprintf(stderr, "skipping sector %d due to CRC mismatch\n", qsc.requested);
#endif
            ++crc_fail;
            continue;
        }

        // this splits psc count exactly in half on the microadvantage, which is a non-standard way of indicating psc is not supported?
        if (qsc_get_psc(&qsc.buf)) {
            ++psc;
        } else {
            ++notpsc;
        }

        switch (qsc_get_mode(&qsc.buf)) {

            case QSC_MODE_INDEX:
            {
                qsc_index_t index;
                char requestedMsf[ MSF_LEN + 1 ];
                char  absoluteMsf[ MSF_LEN + 1 ];
                char  relativeMsf[ MSF_LEN + 1 ];

                if (   qsc_get_index(&qsc.buf, &index)
                    || qsc_lsn_to_ascii(qsc.requested, requestedMsf)
                    || qsc_lsn_to_ascii(index.relativeLsn, relativeMsf)
                    || qsc_lsn_to_ascii(index.absoluteLsn, absoluteMsf)
                   )
                {
                    fprintf(stderr, "index at sector %d has out of range members\n", qsc.requested);
                    continue;
                }

#ifdef DEBUG_INDEX
                printf(
                    "track=%02d, index=%02d, rel=%s (%06d), abs=%s (%06d), rqst=%s (%06d)\n"
                    , index.track
                    , index.index
                    , relativeMsf
                    , index.relativeLsn
                    , absoluteMsf
                    , index.absoluteLsn
                    , requestedMsf
                    , qsc.requested
                    );
#endif
                break;
            }

            case QSC_MODE_MCN:
            {
                char mcn[ MCN_LEN + 1 ];

                if (qsc_get_mcn(&qsc.buf, mcn)) {
                    fprintf(stderr, "mcn at sector %d has out of range members\n", qsc.requested);
                    continue;
                }
#ifdef DEBUG_MCN
                printf("mcn at sector %d is %s\n", qsc.requested, mcn);
#endif
                break;
            }

            case QSC_MODE_ISRC:
            default:
            {
                char isrc[ ISRC_LEN + 1 ];

                if (qsc_get_isrc(&qsc.buf, isrc)) {
                    fprintf(stderr, "isrc at sector %d has out of range members\n", qsc.requested);
                    continue;
                }
#ifdef DEBUG_ISRC
                printf("isrc at sector %d is %s\n", qsc.requested, isrc);
#endif
                break;

            }

            // N.B. if only lead-in and unrecognized printed, they alternate 14 times, the # of tracks on the first test CD
            // ALSO, they fail CRC...  most likely a coincidence
            //

            case QSC_MODE_MULTI_SESSION:
                printf("multi-session record at sector %d\n", qsc.requested);
                break;

            case QSC_MODE_UNKNOWN:
                fprintf(stderr, "unrecognized ctl_adr at sector %d\n", qsc.requested);
                break;
        }
    }

    if (!feof(qSubChannelFile)) {
        unixError("fread", qSubChannelFileName, 1);
    }

    if (stdin != qSubChannelFile) {
        fclose(qSubChannelFile);
    }

    printf("psc is %d;  notpsc is %d\n", psc, notpsc);
    printf("skipped %d sectors due to crc failure\n", crc_fail);

    exit(EXIT_SUCCESS);
}
