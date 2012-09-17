//
// cued.c
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
//      cued diskImage.nrg
//      cued diskImage.bin
//      cued /dev/cdrom
//      cued diskImage.toc
//      cued diskImage.cue
//      etc.
//

#include "unix.h"
#include "macros.h"
#include "cued.h" // CUED_PRODUCT_NAME
#include "opt.h"
#include "util.h"
#include "dmalloc.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdio.h>
#include "cdio2.h"
#include "cddb2.h"
#include "format.h"
#include "rip.h"
#include "sheet.h"
#include "qsc.h" // MSF_LEN, msf_to_ascii

#include <sndfile.h>

#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // unlink
#include <limits.h> // PATH_MAX


#define CUED_DEFAULT_RETRIES 20
#define TAG_FILE_EXT ".tag"


static void usage(const char *exeName)
{
    fprintf(stderr,
        "%s [options] <device1> [device2] [device3...]\n"
        "    where [options] are:\n"
                CDDB2_OPTIONS
                "\t-b track        begin ripping at track (default is first)\n"
                "\t-c format       generate cue sheet (see --format-help)\n"
                "\t-d debug level  defaults to \"%s\"\n"
                "\t-e track        end ripping at track (default is last)\n"
                "\t-f              extract to flac (requires -x and -n)\n"
                "\t-i              get ALL indices\n"
                "\t-n format       name tag and wave files (see --format-help)\n"
                "\t-o samples      offset correction\n"
                "\t-p              enable paranoia\n"
                "\t-q format       output Q sub-channel (mutually exclusive with -p)\n"
                "\t-r retries      defaults to %d\n"
                "\t-s speed        set CD-ROM drive speed\n"
                "\t-t format       tag; use repeatedly (requires -n; see --format-help)\n"
                "\t-v              report progress\n"
                "\t-w              rip to one file (requires -x and -n, or -q, or -i)\n"
                "\t-x              extract tracks (requires -n)\n"
                "\t--dap-fixup     drive fixes audio errors using interpolation (>= MMC-4)\n"
                "\t--qsc-ecc       read Q sub-channel using error correction (requires -i)\n"
                "\t--qsc-format    read Q sub-channel using formatted method (requires -i)\n"
                "\t--read-lead-out read disc lead-out for missing samples, if necessary\n"
                "\t--read-pre-gap  read disc pre-gap for missing samples, if necessary\n"
                "\t--format-help   display format help\n"
                "\n"
                , exeName
                , "warn"
                , CUED_DEFAULT_RETRIES
                );

    exit(EXIT_FAILURE);
}


static void cued_set_loglevel(void *context, char *optarg, const char *optionName)
{
    ssize_t sst;

    if (SUBSTREQ("de", optarg) || SUBSTREQ("db", optarg) || SUBSTREQ("max", optarg)) {
        cdio_loglevel_default = CDIO_LOG_DEBUG;
    } else if (SUBSTREQ("info", optarg) || SUBSTREQ("nfo", optarg)) {
        cdio_loglevel_default = CDIO_LOG_INFO;
    } else if (SUBSTREQ("war", optarg)) {
        cdio_loglevel_default = CDIO_LOG_WARN;
    } else if (SUBSTREQ("err", optarg)) {
        cdio_loglevel_default = CDIO_LOG_ERROR;
    } else if (SUBSTREQ("ass", optarg) || SUBSTREQ("cr", optarg) || SUBSTREQ("min", optarg)) {
        cdio_loglevel_default = CDIO_LOG_ASSERT;
    } else if (strtol2(optarg, NULL, 10, &sst)) {
        cdio2_abort("invalid debug level of \"%s\" specified", optarg);
    } else {
        cdio_loglevel_default = (cdio_log_level_t) sst;
    }
}


static void cued_set_sf(void *context, char *optarg, const char *optionName)
{
    *(int *) context = SF_FORMAT_FLAC;
}


static void cued_format_help(void *context, char *optarg, const char *optionName)
{
    fprintf(stderr,
    "\nSome options, such as -x and -c, take a format parameter, which is used\n"
    "to generate filenames & tags.  The syntax of a format parameter is as follows:\n\n"
    "1. Normal characters, including spaces, are used in the filename or tag.\n"
    "2. A / character indicates a directory, which will be created if necessary.\n"
    "   (Does not apply to tags.)\n"
    "3. A percent character, followed by another character, indicates substitution.\n"
    "4. Valid substitutions are as follows:\n"
    "   %%L=track length  %%M=disc length  %%N=track no.  %%O=no. tracks\n"
    "   %%V=version of %s               %%S is the date/time (Internet RFC 3339)\n"
    "   CD-TEXT substitutions:\n"
    "   track: %%t=title %%a=artist %%i=genre %%m=composer %%r=arranger %%w=songwriter\n"
    "   disc:  %%c=title %%d=artist %%j=genre %%n=composer %%s=arranger %%x=songwriter\n"
    "   CDDB substitutions:\n"
    "   track: %%T=title %%A=artist\n"
    "   disc:  %%C=title %%D=artist %%I=genre %%B=category %%K=cat no. %%Y=year %%F=ID\n"
    "   %%%% is a %% character      %%< is a < character      %%> is a > character\n"
    "5. The < character indicates conditional behavior when following a substition.\n"
    "   If the preceding substitution, such as %%A, is not empty, then the\n"
    "   characters up to a > character are considered, otherwise they are ignored.\n"
    "6. A conditional indicated with <<, negates the logic.\n\n"
    , CUED_PRODUCT_NAME
    );

    exit(EXIT_FAILURE);
}


// procedural model fits too nicely;  no object disorientation needed
//

int main(int argc, char *const argv[])
{
    // things that need default values
    //
    const char *optCueFileNamePattern, *optFileNamePattern, *optQSubChannelFileName;
    int optFirstRipTrack, optLastRipTrack;
    int optFlags, optSpeed, optOffsetWords;
    int optRetries, optSoundFileFormat;

    opt_param_t opts[] = {
        { "b", &optFirstRipTrack,        opt_set_nat_no,     OPT_REQUIRED },
        { "c", &optCueFileNamePattern,   opt_set_string,     OPT_REQUIRED },
        { "d", NULL,                     cued_set_loglevel,  OPT_REQUIRED },
        { "e", &optLastRipTrack,         opt_set_nat_no,     OPT_REQUIRED },
        { "f", &optSoundFileFormat,      cued_set_sf,        OPT_NONE },
        { "n", &optFileNamePattern,      opt_set_string,     OPT_REQUIRED },
        { "o", &optOffsetWords,          opt_set_int,        OPT_REQUIRED },
        { "q", &optQSubChannelFileName,  opt_set_string,     OPT_REQUIRED },
        { "r", &optRetries,              opt_set_whole_no,   OPT_REQUIRED },
        { "s", &optSpeed,                opt_set_nat_no,     OPT_REQUIRED },
        { "t", NULL,                     format_set_tag,     OPT_REQUIRED },
        { "format-help",  NULL,          cued_format_help,   OPT_NONE },

        { "i", &optFlags, NULL,          OPT_SET_FLAG, RIP_F_GET_INDICES },
        { "p", &optFlags, NULL,          OPT_SET_FLAG, RIP_F_USE_PARANOIA },
        { "v", &optFlags, NULL,          OPT_SET_FLAG, RIP_F_VERBOSE },
        { "w", &optFlags, NULL,          OPT_SET_FLAG, RIP_F_RIP_TO_ONE_FILE },
        { "x", &optFlags, NULL,          OPT_SET_FLAG, RIP_F_EXTRACT },
        { "dap-fixup",     &optFlags, NULL, OPT_SET_FLAG, RIP_F_DAP_FIXUP },
        { "qsc-ecc",       &optFlags, NULL, OPT_SET_FLAG, RIP_F_USE_ECC_QSC },
        { "qsc-format",    &optFlags, NULL, OPT_SET_FLAG, RIP_F_USE_FORMATTED_QSC },
        { "read-lead-out", &optFlags, NULL, OPT_SET_FLAG, RIP_F_READ_LEADOUT },
        { "read-pre-gap",  &optFlags, NULL, OPT_SET_FLAG, RIP_F_READ_PREGAP },
    };

    // things that do not need to be freed and will be initialized on first use
    //
    rip_context_t rip;
    PIT(const char, devName);
    PIT(const char, exeName);
    int i;
    track_t tracks, discFirstTrack, discLastTrack;
    char fileNameBuffer[PATH_MAX];

    // this should be first
    cdio2_set_log_handler();
    opt_set_error_handler(cdio2_abort);
    cddb2_init();

    // set defaults before option parsing
    //
    cdio_loglevel_default = CDIO_LOG_WARN;
    optCueFileNamePattern = optFileNamePattern = optQSubChannelFileName = NULL;
    optFirstRipTrack = optLastRipTrack = 0;
    optFlags = optSpeed = optOffsetWords = 0;
    optRetries = CUED_DEFAULT_RETRIES;
    optSoundFileFormat = SF_FORMAT_WAV;

    exeName = basename2(argv[0]);
    opt_register_params(opts, NELEMS(opts), 15, 15);
    switch (opt_parse_args(argc, argv)) {

        case OPT_SUCCESS:
            break;

        case OPT_INVALID:
            cdio_log(CDIO_LOG_ASSERT, "unknown option \"%c\" specified", optopt);
            usage(exeName);
            break;

        case OPT_NOT_FOUND:
            cdio2_unix_error("getopt_long", "", 1);
            break;

        case OPT_FAILURE:
        default:
            cdio2_abort("unexpected result from opt_parse_args (internal error)");
            break;
    }

    // convert no. samples to no. words
    if (optOffsetWords) {
        optOffsetWords *= 2;
    }

    // there should be at least one argument after the options
    if (argc - 1 < optind) {
        cdio_log(CDIO_LOG_ASSERT, "no device specified");
        usage(exeName);
    }

    for (i = optind;  i < argc;  ++i) {

        devName = argv[i];

        rip.fileNamePattern     = optFileNamePattern;
        rip.soundFileFormat     = optSoundFileFormat;

        rip.flags               = optFlags;
        rip.offsetWords         = optOffsetWords;
        rip.qSubChannelFileName = optQSubChannelFileName;

        rip.retries             = optRetries;

        rip.fileNameBuffer      = fileNameBuffer;
        rip.bufferSize          = sizeof(fileNameBuffer);

        rip.cueFileNamePattern  = optCueFileNamePattern;

        rip.cdObj = cdio_open(devName, DRIVER_UNKNOWN);
        if (!rip.cdObj) {
            cdio2_abort("unrecognized device \"%s\"", devName);
        }

        if (TSTF(RIP_F_VERBOSE, rip.flags)) {
            printf("progress: opened device \"%s\"\n", devName);
        }

        if (util_add_context(rip.cdObj, &rip)) {
            cdio2_abort("out of memory allocating rip context");
        }

        if (optSpeed) {
            driver_return_code_t rc;
            rc = cdio_set_speed(rip.cdObj, optSpeed);
            cdio2_driver_error(rc, "set CD-ROM speed");
        }

        if (CDIO_DISC_MODE_CD_DA != cdio_get_discmode(rip.cdObj)) {
            cdio_warn("not an audio disc; ignoring non-audio tracks");
        }

        tracks = cdio_get_num_tracks(rip.cdObj);
        if (CDIO_INVALID_TRACK == tracks) {
            cdio2_abort("failed to get number of tracks");
        }

        discFirstTrack = cdio_get_first_track_num(rip.cdObj);
        if (CDIO_INVALID_TRACK == discFirstTrack) {
            cdio2_abort("failed to get first track number");
        }

        discLastTrack = discFirstTrack + tracks - 1;

        // firstRipTrack and lastRipTrack cannot be negative because opt_set_nat_no is used
        //

        if (!optFirstRipTrack) {
            rip.firstTrack = discFirstTrack;
        } else if (optFirstRipTrack > discLastTrack) {
            cdio2_abort("cannot rip track %d; last track is %02d; check -b option", optFirstRipTrack, discLastTrack);
            rip.firstTrack = discLastTrack;
        } else {
            rip.firstTrack = optFirstRipTrack;
        }

        if (!optLastRipTrack) {
            rip.lastTrack = discLastTrack;
        } else if (optLastRipTrack > discLastTrack) {
            cdio2_abort("cannot rip track %d; last track is %02d; check -e option", optLastRipTrack, discLastTrack);
            rip.lastTrack = discLastTrack;
        } else if (optLastRipTrack < rip.firstTrack) {
            cdio2_abort("end track is less than begin track (%02d < %02d); check -b and -e options", optLastRipTrack, rip.firstTrack);
            rip.lastTrack = rip.firstTrack;
        } else {
            rip.lastTrack = optLastRipTrack;
        }

        rip.cddbObj = cddb2_get_disc(rip.cdObj, TSTF(RIP_F_VERBOSE, rip.flags));

        // user may want to know cue file will not be created before ripping all tracks
        // b/c they may have specified -i
        //
        if (rip.cueFileNamePattern) {

            // set output file from options
            //
            if (!strcmp("-", rip.cueFileNamePattern)) {
                rip.cueFile = stdout;
            } else {

                // removed ".cue" extension to allow using /dev/null for testing
                (void) format_get_file_path(rip.cdObj, rip.cddbObj, rip.cueFileNamePattern, "", 0, fileNameBuffer, sizeof(fileNameBuffer));

                // replaced O_EXCL with O_TRUNC to allow using /dev/null for testing
                rip.cueFile = fopen2(fileNameBuffer, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
                if (!rip.cueFile) {
                    cdio2_unix_error("fopen2", fileNameBuffer, 0);
                    cdio_error("not creating cue file \"%s\"", fileNameBuffer);

                    rip.cueFileNamePattern = 0;
                }
            }
        }

        // must be called whether ripping tracks or merely creating cuesheet
        // because cuesheet relies on rip data being initialized
        //
        cued_init_rip_data(&rip);

        if (rip.fileNamePattern) {

            format_make_tag_files(
                rip.cdObj, rip.cddbObj,
                rip.fileNamePattern, TAG_FILE_EXT,
                (1 == rip.firstTrack) ? 0 : rip.firstTrack,
                rip.lastTrack,
                fileNameBuffer, sizeof(fileNameBuffer)
                );

            if (TSTF(RIP_F_EXTRACT, rip.flags)) {

                cued_rip_disc(&rip);

                // remove track 0 tag file if track 0 pre-gap file was either removed or never generated
                if (format_has_tags() && 1 == rip.firstTrack && !TSTF(RIP_F_NOISY_PREGAP, rip.flags)) {
                    cddb_track_t *trackObj = cddb2_get_track(rip.cddbObj, 0);
                    if (!format_apply_pattern(rip.cdObj, rip.cddbObj, trackObj,
                            rip.fileNamePattern, TAG_FILE_EXT, 0, fileNameBuffer, sizeof(fileNameBuffer), 0))
                    {
                        if (unlink(fileNameBuffer)) {
                            cdio2_unix_error("unlink", fileNameBuffer, 0);
                        }
                    }
                    else
                    {
                        cdio_error("could not make filename to unlink track 0 tag file");
                    }
                }
            }
        } else if (TSTF(RIP_F_GET_INDICES, rip.flags) || rip.qSubChannelFileName) {
            cued_rip_disc(&rip);
        }

        if (rip.cueFileNamePattern) {

            lba_t *ripLba;
            char *isrc;
            lba_t pregap;
            track_t track;

            for (track = discFirstTrack;  track <= discLastTrack;  ++track) {

                // for image files, libcdio may have the pregap;  if so, use it
                pregap = cdio_get_track_pregap_lba(rip.cdObj, track);
                if (CDIO_INVALID_LBA != pregap) {
                    ripLba = rip.ripData[track].indices;
                    if (!ripLba[0]) {
                        ripLba[0] = pregap;
                        cdio_info("using pregap from cdio");
                    } else {
                        cdio_warn("ignoring cdio pregap for track %02d because Q sub-channel had pregap", track);
                    }
                    if (!ripLba[1]) {
                        ripLba[1] = cdio_get_track_lba(rip.cdObj, track);
                        if (CDIO_INVALID_LBA == ripLba[1]) {
                            cdio2_abort("failed to get first sector number for track %02d", track);
                        }
                    }
                }

                // for image files, libcdio may have the isrc;  if so, use it
                isrc = cdio_get_track_isrc(rip.cdObj, track);
                if (isrc) {
                    // rip.ripData[track].isrc is already null terminated
                    strncpy(rip.ripData[track].isrc, isrc, ISRC_LEN);
                    libc_free(isrc);
                }
            }

            // Nero does not properly handle the pregap for the first track
            if (DRIVER_NRG == cdio_get_driver_id(rip.cdObj) && 1 == discFirstTrack) {
                lba_t lba = cdio_get_track_lba(rip.cdObj, discFirstTrack);
                if (CDIO_INVALID_LBA == lba) {
                    cdio2_abort("failed to get first sector number for track %02d", discFirstTrack);
                } else if (CDIO_PREGAP_SECTORS != lba) {
                    char *mcn = cdio_get_mcn(rip.cdObj);

                    // (heuristic) check for DAO
                    if (mcn) {
                        libc_free(mcn);
                        rip.ripData[discFirstTrack].indices[0] = CDIO_PREGAP_SECTORS;
                        rip.ripData[discFirstTrack].indices[1] = lba;
                    }
                }
            }

            cued_write_cuefile(&rip, devName, discFirstTrack, discLastTrack);
        }

        if (rip.cddbObj) {
            cddb_disc_destroy(rip.cddbObj);
        }
        if (util_remove_context(rip.cdObj)) {
            cdio2_abort("failed to remove rip context");
        }
        cdio_destroy(rip.cdObj);
    }

    cddb2_cleanup();
    format_cleanup();

    exit(EXIT_SUCCESS);
}
