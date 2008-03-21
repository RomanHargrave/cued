//
// cdio2.c: libcdio "enhancements"
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

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include "cdio2.h"
#include "qsc.h"

#include <string.h> // strlen, strerror, strchr
#include <errno.h>
#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // getopt


void cdio2_unix_error(const char *fn, const char *arg, int isFatal)
{
    int quoted = strlen(arg);

    cdio_log(isFatal ? CDIO_LOG_ASSERT : CDIO_LOG_ERROR,
        "%s(%s%s%s) returned error \"%s\" (errno = %d)",
        fn, quoted ? "\"" : "", arg, quoted ? "\"" : "", strerror(errno), errno);

    if (isFatal) {
        exit(EXIT_FAILURE);
    }
}


static cdio_log_handler_t oldLogFn;


static void cdio2_log_handler(cdio_log_level_t level, const char *message)
{
    if (level >= cdio_loglevel_default) {
        const char *levelStr;

        switch (level) {

            case CDIO_LOG_DEBUG:
                levelStr = "Debug";
                break;

            case CDIO_LOG_INFO:
                levelStr = "Information";
                break;

            case CDIO_LOG_WARN:
                levelStr = "Warning";
                break;

            case CDIO_LOG_ERROR:
                levelStr = "Error";
                break;

            case CDIO_LOG_ASSERT:
                levelStr = "Critical";
                fprintf(stderr, "\n%s (%d): %s\n\n", levelStr, level, message);
                return;

            default:
                levelStr = "UNKNOWN";
                break;
        }

        fprintf(stderr, "%s (%d): %s\n", levelStr, level, message);
    }

    // for comparison
    //oldLogFn(level, message);
}


void cdio2_set_log_handler()
{
    oldLogFn = cdio_log_set_handler(cdio2_log_handler);
}


void cdio2_logv(cdio_log_level_t level, const char *format, va_list args)
{
    char msgbuf[1024] = { 0, };
  
    // N.B. GNU libc documentation indicates it always NULL terminates
    vsnprintf(msgbuf, sizeof(msgbuf), format, args);

    cdio_log(level, msgbuf);
}


void cdio2_abort(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    cdio2_logv(CDIO_LOG_ASSERT, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}


void cdio2_fprint_cd_text(FILE *cueFile, CdIo_t *cdObj, track_t track, const char *prefix)
{
    int i, quoted;
    cdtext_t *cdtext;

    cdtext = cdio_get_cdtext(cdObj, track);
    if (cdtext) {
        for (i = MIN_CDTEXT_FIELD;  i < MAX_CDTEXT_FIELDS;  ++i) {

            if (cdtext->field[i] && cdtext->field[i][0]) {
                quoted = strchr(cdtext->field[i], ' ') ? 1 : 0;
                fprintf(cueFile, "%s%s %s%s%s\n", prefix, cdtext_field2str((cdtext_field_t) i),
                    quoted ? "\"" : "", cdtext->field[i], quoted ? "\"" : "");
            }
        }

        cdtext_destroy(cdtext);
    }
}


void cdio2_paranoia_msg(cdrom_drive_t *paranoiaCtlObj, char *when)
{
    char *msg;

    msg = cdio_cddap_messages(paranoiaCtlObj);
    if (msg) {
        cdio_info("paranoia returned the following messages during %s:\n%s", when, msg);
    }

    msg = cdio_cddap_errors(paranoiaCtlObj);
    if (msg) {
        cdio_error("paranoia returned the following errors during %s:\n%s", when, msg);
    }
}


void cdio2_driver_error(driver_return_code_t ec, char *when)
{
    if (DRIVER_OP_SUCCESS != ec) {
        cdio_error("received following error during %s: %s", when, cdio_driver_errmsg(ec));
    }
}


void cdio2_paranoia_callback(long int frame, paranoia_cb_mode_t prc)
{
    cdio_log_level_t level;

    switch (prc) {

        case PARANOIA_CB_READ:
        case PARANOIA_CB_VERIFY:
        case PARANOIA_CB_OVERLAP:
            level = CDIO_LOG_DEBUG;
            break;

        case PARANOIA_CB_FIXUP_ATOM:
        case PARANOIA_CB_FIXUP_EDGE:
            level = CDIO_LOG_INFO;
            break;
    
        default:
            level = CDIO_LOG_ERROR;
            break;
    }

    cdio_log(level, "paranoia reports \"%s\" at sector \"%d\" (frame=\"%ld\")",  paranoia_cb_mode2str[prc], (int) (frame / CD_FRAMEWORDS), frame);
}


int cdio2_get_track_channels(CdIo_t *cdObj, track_t track)
{
    int channels = cdio_get_track_channels(cdObj, track);

    switch (channels) {

        case 2:
        case 4:
            break;

        case -1:
            cdio_warn("failed to get number of channels for track %02d; assuming 2 channels", track);
            channels = 2;
            break;

        default:
            cdio_log(channels ? CDIO_LOG_WARN : CDIO_LOG_INFO,
                "overriding number of channels for track %02d from %d to 2", track, channels);
            channels = 2;
            break;
    }

    return channels;
}
