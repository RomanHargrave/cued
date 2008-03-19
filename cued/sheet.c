//
// sheet.c
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

#include "macros.h"
#include "unix.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include "sheet.h"
#include "qsc.h"
#include "cdio2.h"
#include "rip.h"
#include "cued.h" // CUED_VERSION

#include <stdlib.h> // atoll


void cued_write_cuefile(
    FILE *cueFile,
    CdIo_t *cdObj,
    const char *devName,
    track_t firstTrack, track_t lastTrack)
{
    PIT(char, mcn);
    PIT(char, devFileName);
    long long nmcn;
    int quoted, channels;
    track_t track;
    lsn_t firstTrackSector;

    devFileName = noextname(devName);
    if (!devFileName) {
        cdio2_abort("out of memory creating FILE command for \"%s\"", devName);
    }

    // output disc level cue info
    //

    fprintf(cueFile, "REM COMMENT Generated by " CUED_PRODUCT_NAME " " CUED_VERSION "\n");

    mcn = cdio_get_mcn(cdObj);
    if (mcn && (nmcn = atoll(mcn))) {
        if (strchr(mcn, ' ')) {

            // Nero is buggy, to say the least
            fprintf(cueFile, "CATALOG %013lld\n", nmcn);
        } else {
            fprintf(cueFile, "CATALOG %s\n", mcn);
        }
    } else if (rip_mcn[0]) {
        fprintf(cueFile, "CATALOG %s\n", rip_mcn);
    }
    free(mcn);

    cdio2_fprint_cd_text(cueFile, cdObj, 0, "");

    quoted = strchr(basename2(devFileName), ' ') ? 1 : 0;
    fprintf(cueFile
        , "FILE %s%s.bin%s BINARY\n"
        , quoted ? "\"" : ""
        , basename2(devFileName)
        , quoted ? "\"" : ""
        );
    free(devFileName);

    firstTrackSector = cdio_get_track_lsn(cdObj, 1);
    if (CDIO_INVALID_LSN == firstTrackSector) {
        cdio2_abort("failed to get first sector number for first track");
    }

    for (track = firstTrack;  track <= lastTrack;  ++track) {

        lsn_t *ripLsn;
        lsn_t lsn, useLsn;
        track_flag_t flagrc;
        int preemp, copy, index;
        char msfStr[ MSF_LEN + 1 ];

        if (TRACK_FORMAT_AUDIO != cdio_get_track_format(cdObj, track)) {
            cdio_warn("track %02d is not an audio track; skipping track", track);
            continue;
        }

        channels = cdio2_get_track_channels(cdObj, track);

        flagrc = cdio_get_track_preemphasis(cdObj, track);
        if (CDIO_TRACK_FLAG_TRUE != flagrc && CDIO_TRACK_FLAG_FALSE != flagrc) {
            cdio_warn("failed to get pre-emphasis flag for track %02d; assuming no pre-emphasis", track);
            preemp = 0;
        } else {
            preemp = (CDIO_TRACK_FLAG_TRUE == flagrc) ? 1 : 0;
        }

        flagrc = cdio_get_track_copy_permit(cdObj, track);
        if (CDIO_TRACK_FLAG_TRUE != flagrc && CDIO_TRACK_FLAG_FALSE != flagrc) {
            cdio_warn("failed to get copy permitted flag for track %02d; assuming no copy permitted", track);
            copy = 0;
        } else {
            copy = (CDIO_TRACK_FLAG_TRUE == flagrc) ? 1 : 0;
        }

        // output track level cue info
        //

        fprintf(cueFile, "  TRACK %02d AUDIO\n", track);
        cdio2_fprint_cd_text(cueFile, cdObj, track, "    ");
        if (preemp || copy || (4 == channels)) {
            fprintf(cueFile, "    FLAGS%s%s%s\n", copy ? " DCP" : "", (4 == channels) ? " 4CH" : "", preemp ? " PRE" : "");
        }

        if (rip_isrc[track][0]) {
            fprintf(cueFile, "    ISRC %s\n", rip_isrc[track]);
        }

        lsn = cdio_get_track_lsn(cdObj, track);
        if (CDIO_INVALID_LSN == lsn) {
            cdio2_abort("failed to get first sector number for track %02d", track);
        }

        ripLsn = rip_indices[track];
        if (CDIO_INVALID_LSN != ripLsn[1]) {
            for (index = 0;  index < CUED_MAX_INDICES;  ++index) {

                useLsn = ripLsn[index];
                if (CDIO_INVALID_LSN == useLsn) {
                    if (!index) {
                        continue;
                    } else {
                        break;
                    }
                }

                if (index <= 1 && useLsn > lsn) {
                    cdio_warn("index %02d from Q sub-channel for track %02d is greater than index from TOC (lsn=%d > lsn=%d); using TOC",
                        index, track, useLsn, lsn);
                    useLsn = lsn;
                }

                if (1 == track && !index && rip_silent_pregap) {

                    // pregap for first track was silent; track 0 was deleted; use PREGAP directive in cuesheet
                    if (!qsc_lsn_to_ascii_for_cue(firstTrackSector, msfStr)) {
                        fprintf(cueFile, "    PREGAP %s\n", msfStr);
                    } else {
                        cdio2_abort("failed to convert pre-gap for track %02d", track);
                    }
                } else {

                    // if pregap is silent, adjust index for the fact that pregap was not saved
                    if (rip_silent_pregap) {
                        useLsn -= firstTrackSector;
                    }

                    if (!qsc_lsn_to_ascii_for_cue(useLsn, msfStr)) {
                        fprintf(cueFile, "    INDEX %02d %s\n", index, msfStr);
                    } else {
                        cdio2_abort("failed to convert index %02d for track %02d", index, track);
                    }
                }
            }
        } else {
            useLsn = lsn;

            if (rip_silent_pregap) {
                useLsn -= firstTrackSector;
                if (1 == track) {
                    if (!qsc_lsn_to_ascii_for_cue(firstTrackSector, msfStr)) {
                        fprintf(cueFile, "    PREGAP %s\n", msfStr);
                    } else {
                        cdio2_abort("failed to convert pre-gap for track %02d", track);
                    }
                }
            }

            if (!qsc_lsn_to_ascii_for_cue(useLsn, msfStr)) {
                fprintf(cueFile, "    INDEX %02d %s\n", 1, msfStr);
            } else {
                cdio2_abort("failed to convert index for track %02d", track);
            }
        }
    }

    if (cueFile != stdout) {
        fclose(cueFile);
    }
}
