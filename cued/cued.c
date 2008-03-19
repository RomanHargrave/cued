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
//  TODO:
//
//      HIGH PRIORITY
//
//          add credit for self
//              By the way, there should be a note in the libcdio doc that you wrote
//              the section on pregaps. If you want to find a place to add that,
//              that'd be cool as well.
//
//          fix 272 byte offset in bincue and cdrdao
//              fix up regression tests
//
//          NRG enhancements
//
//              isrc's are in the nrg created from the bin/cue, although mcn is not
//                  12 bytes...  they go in the zero field + 2 more bytes of the so-called "Sector size"
//
//              look at cd text in nrg
//
//              create nrg image from bincue
//                  look at mcn and check for pregap
//                      no mcn created, but pregap is
//
//              look at mcn in nrg
//                  looks like a space!
//                      is this correct?
//
//      MEDIUM PRIORITY
//
//          overread support
//              need to handle error of read inside/outside audio track during offset correction?
//
//              can read 2 second audio lead-in/lead-out with read-msf?  mmc-read?
//
//              stuff to read cd text/qsc in toc/lead-in?
//
//          handle other types of tracks (such as data)
//              some cd's in my collection may be mixed mode
//                  cddb talks about cd-extra
//
//      LOW PRIORITY
//
//          opt functions should return int?
//
//          always check return of cddb2_get_file_path?
//

#include "unix.h"
#include "macros.h"
#include "cddb2.h"
#include "qsc.h" // MSF_LEN, msf_to_ascii
#include "cued.h" // CUED_VERSION
#include "opt.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include "rip.h"
#include "cdio2.h"
#include "sheet.h"

#include <sndfile.h>

#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // unlink


#define CUED_DEFAULT_RETRIES 20
#define TAG_FILE_EXT ".tag"


static void usage(const char *exeName)
{
    fprintf(stderr,
        "%s [options] device\n"
        "    where options are:\n"
                CDDB2_OPTIONS
                "\t-b track       begin ripping at track (default is first)\n"
                "\t-c format      generate cue sheet (see --format-help)\n"
                "\t-d debug level defaults to \"%s\"\n"
                "\t-e track       end ripping at track (default is last)\n"
                "\t-f             extract to flac (requires -x)\n"
                "\t-i             get ALL indices (requires -x and -c)\n"
                "\t-n format      name tag and wave files (see --format-help)\n"
                "\t-o samples     offset correction (EAC-30) (requires -x)\n"
                "\t-p             enable paranoia (requires -x)\n"
                "\t-q filename    output Q sub-channel (requires -x without -p)\n"
                "\t-r retries     defaults to %d (requires -p)\n"
                "\t-s speed       set CD-ROM drive speed\n"
                "\t-t format      tag; use repeatedly (requires -n; see --format-help)\n"
                "\t-v             report progress\n"
                "\t-w             rip to one file (requires -x and -n)\n"
                "\t-x             extract tracks (requires -n)\n"
                "\t--format-help  display format help\n"
                "\n"
                , exeName
                , "warn"
                , CUED_DEFAULT_RETRIES
                );

    exit(EXIT_FAILURE);
}


static void cued_set_loglevel(void *context, char *optarg, char *optionName)
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


static void cued_set_sf(void *context, char *optarg, char *optionName)
{
    *(int *) context = SF_FORMAT_FLAC;
}

int verbose;


static void cued_format_help(void *context, char *optarg, char *optionName)
{
    fprintf(stderr,
    "\nSome options, such as -x and -c, take a format parameter, which is used\n"
    "to generate filenames & tags.  The syntax of a format parameter is as follows:\n\n"
    "1. Normal characters, including spaces, are used in the filename or tag.\n"
    "2. A / character indicates a directory, which will be created if necessary.\n"
    "   (Does not apply to tags.)\n"
    "3. A percent character, followed by another character, indicates substitution.\n"
    "4. Valid substitutions are as follows:\n"
    "   %%N is the track number             %%O is the total number of tracks\n"
    "   %%T is the track title              %%C is the disc title\n"
    "   %%L is the track length             %%M is the disc length\n"
    "   %%Y is the disc release year        %%I is the disc genre\n"
    "   %%B is the cddb category            %%K is the cddb category as a number\n"
    "   %%F is the cddb disc ID             %%D is the disc artist\n"
    "   %%V is the version of %s          %%S is the date/time (Internet RFC 3339)\n"
    "   %%A is the track artist (if and only if the album has various artists)\n"
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
    // things that will need to be freed
    //
    PIT(CdIo_t, cdObj);
    PIT(cdrom_drive_t, paranoiaCtlObj);
    PIT(cdrom_paranoia_t, paranoiaRipObj);
    PIT(FILE, cueFile);
    cddb_disc_t *cddbObj = NULL;

    // things that need default values
    //
    const char *cueFileNamePattern, *fileNamePattern, *qSubChannelFileName;
    int firstRipTrack, lastRipTrack;
    int ripToOneFile, extract, speed, useParanoia, offsetWords, getIndices;
    int retries, soundFileFormat;

    // things that do not need to be freed and will be initialized on first use
    //
    PIT(const char, devName);
    PIT(const char, exeName);
    int rc;
    track_t firstTrack, lastTrack, tracks;
    char fileNameBuffer[PATH_MAX];

    // this should be first
    cdio2_set_log_handler();
    cddb2_set_log_handler();
    opt_set_error_handler(cdio2_abort);

    // don't return album artist as track artist;  this would complicate
    // handling compilations of various artists
    //
    libcddb_set_flags(CDDB_F_NO_TRACK_ARTIST);

    // set defaults before option parsing
    //
    cdio_loglevel_default = CDIO_LOG_WARN;
    cueFileNamePattern = fileNamePattern = qSubChannelFileName = NULL;
    verbose = 0;
    firstRipTrack = lastRipTrack = 0;
    ripToOneFile = extract = speed = useParanoia = offsetWords = getIndices = 0;
    retries = CUED_DEFAULT_RETRIES;
    soundFileFormat = SF_FORMAT_WAV;

    opt_param_t opts[] = {
        { "b", &firstRipTrack,          opt_set_nat_no,     OPT_REQUIRED },
        { "c", &cueFileNamePattern,     opt_set_string,     OPT_REQUIRED },
        { "d", NULL,                    cued_set_loglevel,  OPT_REQUIRED },
        { "e", &lastRipTrack,           opt_set_nat_no,     OPT_REQUIRED },
        { "f", &soundFileFormat,        cued_set_sf,        OPT_NONE },
        { "i", &getIndices,             opt_set_flag,       OPT_NONE },
        { "n", &fileNamePattern,        opt_set_string,     OPT_REQUIRED },
        { "o", &offsetWords,            opt_set_int,        OPT_REQUIRED },
        { "p", &useParanoia,            opt_set_flag,       OPT_NONE },
        { "q", &qSubChannelFileName,    opt_set_string,     OPT_REQUIRED },
        { "r", &retries,                opt_set_whole_no,   OPT_REQUIRED },
        { "s", &speed,                  opt_set_nat_no,     OPT_REQUIRED },
        { "t", NULL,                    cddb2_set_tag,      OPT_REQUIRED },
        { "v", &verbose,                opt_set_flag,       OPT_NONE },
        { "w", &ripToOneFile,           opt_set_flag,       OPT_NONE },
        { "x", &extract,                opt_set_flag,       OPT_NONE },
        { "format-help",   NULL,        cued_format_help,   OPT_NONE }
    };

    opt_register_params(opts, NELEMS(opts), 15, 15);
    cddb2_opt_register_params();

    exeName = basename2(argv[0]);

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

    if (offsetWords) {
        offsetWords = (offsetWords - 30) * 2;
    }

    // there should be only one argument after the options
    if (argc - 1 != optind) {
        if (argc - 1 > optind) {
            cdio_log(CDIO_LOG_ASSERT, "more than one device specified");
        } else {
            cdio_log(CDIO_LOG_ASSERT, "no device specified");
        }
        usage(exeName);
    }

    devName = argv[optind];

    cdObj = cdio_open(devName, DRIVER_UNKNOWN);
    if (!cdObj) {
        cdio2_abort("unrecognized device \"%s\"", devName);
    }

    if (speed) {
        driver_return_code_t rc;
        rc = cdio_set_speed(cdObj, speed);
        cdio2_driver_error(rc, "set CD-ROM speed");
    }

    if (CDIO_DISC_MODE_CD_DA != cdio_get_discmode(cdObj)) {
        cdio_warn("not an audio disc; ignoring non-audio tracks");
    }

    if (useParanoia) {
        char *msg = 0;

        // N.B. this behavior does not match documentation:
        // the 0 here appears to prevent the message "Checking <filename> for cdrom..."
        paranoiaCtlObj = cdio_cddap_identify_cdio(cdObj, 0, &msg);
        if (paranoiaCtlObj) {

            if (msg) {
                cdio_warn("identify returned paranoia message(s) \"%s\"", msg);
            }
            cdio_cddap_verbose_set(paranoiaCtlObj, CDDA_MESSAGE_LOGIT, CDDA_MESSAGE_LOGIT);

            rc = cdio_cddap_open(paranoiaCtlObj);
            cdio2_paranoia_msg(paranoiaCtlObj, "open of device");
            if (!rc) {
                paranoiaRipObj = cdio_paranoia_init(paranoiaCtlObj);
                cdio2_paranoia_msg(paranoiaCtlObj, "initialization of paranoia");
                if (!paranoiaRipObj) {
                    cdio2_abort("out of memory initializing paranoia");
                }

                cdio_paranoia_modeset(paranoiaRipObj, PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP);
                // N.B. not needed at the moment
                cdio2_paranoia_msg(paranoiaCtlObj, "setting of paranoia mode");

                if (getIndices) {
                    paranoiaCtlObj->read_audio = cued_read_audio;
                }
            } else {
                cdio_cddap_close_no_free_cdio(paranoiaCtlObj);

                cdio_error("disabling paranoia");
                useParanoia = 0;
            }
        } else {
            cdio_error("disabling paranoia due to the following message(s):\n%s", msg);
            useParanoia = 0;
        }
    }

    // these could (should?) use paranoia in the future
    //

    tracks = cdio_get_num_tracks(cdObj);
    if (CDIO_INVALID_TRACK == tracks) {
        cdio2_abort("failed to get number of tracks");
    }

    firstTrack = cdio_get_first_track_num(cdObj);
    if (CDIO_INVALID_TRACK == firstTrack) {
        cdio2_abort("failed to get first track number");
    }

    lastTrack = firstTrack + tracks - 1;

    // firstRipTrack and lastRipTrack cannot be negative because opt_set_nat_no is used
    //

    if (!firstRipTrack) {
        firstRipTrack = firstTrack;
    } else if (firstRipTrack > lastTrack) {
        cdio2_abort("cannot rip track %d; last track is %02d; check -b option", firstRipTrack, lastTrack);
        firstRipTrack = lastTrack;
    }

    if (!lastRipTrack) {
        lastRipTrack = lastTrack;
    } else if (lastRipTrack > lastTrack) {
        cdio2_abort("cannot rip track %d; last track is %02d; check -e option", lastRipTrack, lastTrack);
        lastRipTrack = lastTrack;
    } else if (lastRipTrack < firstRipTrack) {
        cdio2_abort("end track is less than begin track (%02d < %02d); check -b and -e options", lastRipTrack, firstRipTrack);
        lastRipTrack = firstRipTrack;
    }

    // this could (should?) use paranoia in the future
    cddbObj = cddb2_get_disc(cdObj);

    // must be called whether ripping tracks or merely creating cuesheet
    // because cuesheet relies on rip data being initialized
    //
    cued_init_rip_data();

    // user may want to know cue file will not be created before ripping all tracks
    // b/c they may have specified -i
    //
    if (cueFileNamePattern) {

        // set output file from options
        //
        if (!strcmp("-", cueFileNamePattern)) {
            cueFile = stdout;
        } else {

            // removed ".cue" extension to allow using /dev/null for testing
            (void) cddb2_get_file_path(cddbObj, cueFileNamePattern, "", 0, fileNameBuffer, sizeof(fileNameBuffer));

            // replaced O_EXCL with O_TRUNC to allow using /dev/null for testing
            cueFile = fopen2(fileNameBuffer, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
            if (!cueFile) {
                cdio2_unix_error("fopen2", fileNameBuffer, 0);
                cdio_error("not creating cue file \"%s\"", fileNameBuffer);

                cueFileNamePattern = 0;
            }
        }
    }

    if (fileNamePattern) {

        cddb2_make_tag_files(
            cddbObj,
            fileNamePattern, TAG_FILE_EXT,
            (1 == firstRipTrack) ? 0 : firstRipTrack,
            lastRipTrack,
            fileNameBuffer, sizeof(fileNameBuffer)
            );

        if (extract) {
            cued_rip_disc(
                fileNamePattern,
                cddbObj,
                soundFileFormat,
                cdObj,
                firstRipTrack, lastRipTrack,
                ripToOneFile,
                offsetWords,
                getIndices,
                useParanoia,
                paranoiaCtlObj, paranoiaRipObj,
                retries,
                qSubChannelFileName,
                fileNameBuffer, sizeof(fileNameBuffer)
                );

            // remove track 0 tag file if track 0 pre-gap file was either removed or never generated
            if (cddb2_has_tags() && 1 == firstRipTrack && !rip_noisy_pregap) {
                cddb_track_t *trackObj = cddb2_get_track(cddbObj, 0);
                if (!cddb2_apply_pattern(cddbObj, trackObj, fileNamePattern, TAG_FILE_EXT, 0, fileNameBuffer, sizeof(fileNameBuffer), 0)) {
                    if (unlink(fileNameBuffer)) {
                        cdio2_unix_error("unlink", fileNameBuffer, 0);
                    }
                } else {
                    cdio_error("could not make filename to unlink track 0 tag file");
                }
            }
        }
    }

    if (cueFileNamePattern) {

        lsn_t *ripLsn;
        lsn_t pregap;
        track_t track;

        // for image files, libcdio may have the pregap;  if so, use it
        for (track = firstTrack;  track <= lastTrack;  ++track) {
            pregap = cdio_get_track_pregap_lsn(cdObj, track);
            if (CDIO_INVALID_LSN != pregap) {
                ripLsn = rip_indices[track];
                if (CDIO_INVALID_LSN == ripLsn[0]) {
                    ripLsn[0] = pregap;
                    cdio_info("using pregap from cdio");
                } else {
                    cdio_warn("ignoring cdio pregap for track %02d because Q sub-channel had pregap", track);
                }
                if (CDIO_INVALID_LSN == ripLsn[1]) {
                    ripLsn[1] = cdio_get_track_lsn(cdObj, track);
                }
            }
        }

        // this could (should?) use paranoia in the future
        cued_write_cuefile(cueFile, cdObj, devName, firstTrack, lastTrack);
    }

    if (cddbObj) {
        cddb_disc_destroy(cddbObj);
    }
    cddb2_cleanup();
    if (useParanoia) {
        cdio_paranoia_free(paranoiaRipObj);
        cdio_cddap_close_no_free_cdio(paranoiaCtlObj);
    }
    cdio_destroy(cdObj);

    exit(EXIT_SUCCESS);
}
