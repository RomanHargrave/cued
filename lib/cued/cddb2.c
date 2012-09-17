//
// cddb2.c
//
// Copyright (C) 2008 Robert William Fuller <hydrologiccycle@gmail.com>
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

#include "config.h" // PACKAGE_VERSION
#include "opt.h"
#include "cued.h" // CUED_PRODUCT_NAME
#include "macros.h" // NELEMS
#include "dmalloc.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdio.h>
#include <cdio/logging.h>
#include "cddb2.h"

#include <stdlib.h> // malloc
#include <ctype.h> // toupper
#include <stdarg.h>


static cddb_log_handler_t oldLogFn;

static void cddb2_log_handler(cddb_log_level_t oldLevel, const char *message)
{
    cdio_log_level_t newLevel;

    switch (oldLevel) {

        case CDDB_LOG_DEBUG:
            newLevel = CDIO_LOG_DEBUG;
            break;

        case CDDB_LOG_INFO:
            newLevel = CDIO_LOG_INFO;
            break;

        case CDDB_LOG_WARN:
            newLevel = CDIO_LOG_WARN;
            break;

        case CDDB_LOG_ERROR:
            newLevel = CDIO_LOG_ERROR;
            break;

        case CDDB_LOG_CRITICAL:
            newLevel = CDIO_LOG_ASSERT;
            break;

        default:
            newLevel = CDIO_LOG_WARN;
            break;
    }

    cdio_log(newLevel, "%s", message);
}

void cddb2_set_log_handler()
{
    oldLogFn = cddb_log_set_handler(cddb2_log_handler);
}

static void cddb2_logv(cddb_log_level_t level, const char *format, va_list args)
{
    char msgbuf[1024] = { 0, };
  
    // N.B. GNU libc documentation indicates it always NULL terminates
    vsnprintf(msgbuf, sizeof(msgbuf), format, args);

    cddb2_log_handler(level, msgbuf);
}

static void cddb2_abort(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    cddb2_logv(CDDB_LOG_CRITICAL, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}

static void cddb2_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    cddb2_logv(CDDB_LOG_ERROR, format, args);
    va_end(args);
}


#define CDDB2_FLAG_DISABLE_CDDB     0x00000001
#define CDDB2_FLAG_DISABLE_CACHE    0x00000002
#define CDDB2_FLAG_USE_HTTP         0x00000004

static int flags;
static char *cacheDir;
static char *server;
static int port;
static int timeout;
static char *email;
static int matchIndex;

void cddb2_opt_register_params()
{
    opt_param_t opts[] = {

        { "no-cddb",       &flags, NULL, OPT_SET_FLAG, CDDB2_FLAG_DISABLE_CDDB },
        { "no-cddb-cache", &flags, NULL, OPT_SET_FLAG, CDDB2_FLAG_DISABLE_CACHE },
        { "cddb-http",     &flags, NULL, OPT_SET_FLAG, CDDB2_FLAG_USE_HTTP },

        { "cddb-match",     &matchIndex,    opt_set_nat_no,     OPT_REQUIRED },
        { "cddb-server",    &server,        opt_set_string,     OPT_REQUIRED },
        { "cddb-cache",     &cacheDir,      opt_set_string,     OPT_REQUIRED },
        { "cddb-port",      &port,          opt_set_port,       OPT_REQUIRED },
        { "cddb-email",     &email,         opt_set_string,     OPT_REQUIRED },
        { "cddb-timeout",   &timeout,       opt_set_whole_no,   OPT_REQUIRED }
    };

    opt_register_params(opts, NELEMS(opts), 0, 0);
}


#define cddb2_fatal(n) { cddb2_abort(n);  goto cleanup; }


static cddb_conn_t *cddb2_create_connection_object()
{
    cddb_conn_t *dbObj = cddb_new();
    if (!dbObj) {
        cddb2_fatal("out of memory creating CDDB connection object");
    }

    if (TSTF(CDDB2_FLAG_DISABLE_CACHE, flags)) {
        cddb_cache_disable(dbObj);
    }

    if (TSTF(CDDB2_FLAG_USE_HTTP, flags)) {
        cddb_http_enable(dbObj);
        if (!port) {
            cddb_set_server_port(dbObj, 80);
        }
    }

    if (server) {
        cddb_set_server_name(dbObj, server);
    }

    if (port) {
        cddb_set_server_port(dbObj, port);
    }

    if (timeout) {
        cddb_set_timeout(dbObj, timeout);
    }

    if (email) {
        cddb_set_email_address(dbObj, email);
    }

    if (cacheDir) {
        cddb_cache_set_dir(dbObj, cacheDir);
    }

    cddb_set_client(dbObj, CUED_PRODUCT_NAME, PACKAGE_VERSION);

    return dbObj;

cleanup:

    if (dbObj) {
        cddb_destroy(dbObj);
    }

    return NULL;
}


static cddb_disc_t *cddb2_create_query_object(CdIo_t *cdObj)
{
    cddb_disc_t *discObj;
    cddb_track_t *trackObj;
    track_t tracks, firstTrack, lastTrack;
    lba_t frameOffset;
    int track;

    discObj = cddb_disc_new();
    if (!discObj) {
        cddb2_fatal("out of memory creating CDDB disc object");
    }

    tracks = cdio_get_num_tracks(cdObj);
    if (CDIO_INVALID_TRACK == tracks) {
        cddb2_fatal("failed to get number of tracks");
    }

    firstTrack = cdio_get_first_track_num(cdObj);
    if (CDIO_INVALID_TRACK == firstTrack) {
        cddb2_fatal("failed to get first track number");
    }
    
    lastTrack = firstTrack + tracks - 1;

    for (track = firstTrack;  track <= lastTrack;  ++track) {

        trackObj = cddb_track_new();
        if (!trackObj) {
            cddb2_fatal("out of memory creating CDDB track object");
        }
        
        frameOffset = cdio_get_track_lba(cdObj, track);
        if (CDIO_INVALID_LBA == frameOffset) {
            cddb2_abort("failed to get offset of track %02d", track);
            goto cleanup;
        }

        cddb_track_set_frame_offset(trackObj, frameOffset);
        cddb_disc_add_track(discObj, trackObj);
    }

    frameOffset = cdio_get_track_lba(cdObj, CDIO_CDROM_LEADOUT_TRACK);
    if (CDIO_INVALID_LBA == frameOffset) {
        cddb2_fatal("failed to get length of disc");
    }

    cddb_disc_set_length(discObj, frameOffset / CDIO_CD_FRAMES_PER_SEC);

    if (!cddb_disc_calc_discid(discObj)) {
        cddb2_fatal("failed to calculate CDDB disc ID");
    }

    return discObj;

cleanup:

    if (discObj) {
        cddb_disc_destroy(discObj);
    }

    return NULL;
}


cddb_disc_t *cddb2_get_match(CdIo_t *cdObj, cddb_conn_t **dbObj, int *matches, int matchIndex)
{
    cddb_disc_t *discObj = NULL;
    int i;

    *dbObj = cddb2_create_connection_object();
    if (!*dbObj) {
        cddb2_fatal("failed to create CDDB connection object");
    }

    discObj = cddb2_create_query_object(cdObj);
    if (!discObj) {
        cddb2_fatal("failed to create CDDB query object");
    }

    *matches = cddb_query(*dbObj, discObj);
    if (-1 == *matches) {
        cddb2_fatal("failed to query CDDB");
    }

    if (matchIndex) {
        if (!*matches) {
            cddb2_fatal("failed to find a single match for disc in CDDB, despite option --cddb-match=");
        } else if (matchIndex > *matches) {
            cddb2_abort("value specified to --cddb-match= is too large; %d matches found", *matches);
            goto cleanup;
        }

        for (i = 1;  i < matchIndex;  ++i) {
            if (1 != cddb_query_next(*dbObj, discObj)) {
                cddb2_fatal("getting next CDDB object failed in unexpected way (internal error)");
            }
        }
    } else if (!*matches) {
        goto cleanup;
    }

    if (1 != cddb_read(*dbObj, discObj)) {
        cddb2_fatal("failed to read CDDB disc object");
    }

    return discObj;

cleanup:

    if (discObj) {
        cddb_disc_destroy(discObj);
    }
    if (*dbObj) {
        cddb_destroy(*dbObj);
    }

    return NULL;
}


cddb_disc_t *cddb2_get_first_match(CdIo_t *cdObj, cddb_conn_t **dbObj, int *matches)
{
    return cddb2_get_match(cdObj, dbObj, matches, 0);
}


cddb_disc_t *cddb2_get_next_match(cddb_conn_t *dbObj)
{
    cddb_disc_t *discObj = cddb_disc_new();
    if (!discObj) {
        cddb2_fatal("out of memory creating CDDB disc object");
    }

    if (1 != cddb_query_next(dbObj, discObj)) {
        goto cleanup;
    }

    if (1 != cddb_read(dbObj, discObj)) {
        cddb2_fatal("failed to read next CDDB disc object");
    }

    return discObj;

cleanup:

    if (discObj) {
        cddb_disc_destroy(discObj);
    }

    return NULL;
}


cddb_disc_t *cddb2_get_disc(CdIo_t *cdObj, int verbose)
{
    cddb_conn_t *dbObj = NULL;
    cddb_disc_t *discObj = NULL;
    FILE *save;
    int matches = 0;

    if (TSTF(CDDB2_FLAG_DISABLE_CDDB, flags)) {
        return NULL;
    } else if (verbose) {
        printf("progress: querying cddb\n");
    }

    if (!matchIndex) {
        discObj = cddb2_get_first_match(cdObj, &dbObj, &matches);
        switch (matches) {

            case -1:
                cddb2_fatal("failed to query CDDB");
                break;

            case 0:
                cddb2_fatal("failed to find match for disc in CDDB; if this is okay, re-run with --no-cddb option");
                break;

            case 1:
                cddb2_log_handler(CDDB_LOG_WARN, "found one match in CDDB; using it");
                break;

            default:
                cddb2_error("found %d matches in CDDB", matches);

                matches = 1;
                save = stdout;
                do {

                    stdout = stderr;
                        printf("\n\nSpecify option --cddb-match=%d to use the following CDDB match:\n\n", matches);
                        cddb_disc_print(discObj);
                    stdout = save;

                    ++matches;

                    cddb_disc_destroy(discObj);

                } while ((discObj = cddb2_get_next_match(dbObj)));

                cddb2_fatal("choose a CDDB match, then re-run with --cddb-match= option");
                break;
        }
    } else {
        discObj = cddb2_get_match(cdObj, &dbObj, &matches, matchIndex);
    }

    if (dbObj) {
        cddb_destroy(dbObj);
    }

    return discObj;

cleanup:

    if (discObj) {
        cddb_disc_destroy(discObj);
    }
    if (dbObj) {
        cddb_destroy(dbObj);
    }

    return NULL;
}


static char **cddbCategories;

char *cddb2_get_category(cddb_disc_t *cddbObj)
{
    // upper case the first character of each cddb category
    if (!cddbCategories) {
        int i, n;

        for (n = 0;  strcmp(CDDB_CATEGORY[n++], "invalid");  )
            ;
        cddbCategories = (char **) calloc(n, sizeof(char *));
        if (!cddbCategories) {
            cddb2_abort("out of memory allocating cddb categories");
            return NULL;
        }

        for (i = 0;  i < n;  ++i) {
            cddbCategories[i] = strdup(CDDB_CATEGORY[i]);
            if (cddbCategories[i]) {
                cddbCategories[i][0] = toupper(cddbCategories[i][0]);
            } else {
                cddb2_abort("out of memory allocating cddb category string");
                return NULL;
            }
            //printf("category is %s\n", cddbCategories[i]);
        }
    }

    return cddbCategories[ cddb_disc_get_category(cddbObj) ];
}


void cddb2_init()
{
    cddb2_set_log_handler();
    cddb2_opt_register_params();

    // don't return album artist as track artist;  this would complicate
    // handling compilations of various artists
    //
    libcddb_set_flags(CDDB_F_NO_TRACK_ARTIST);
}


void cddb2_cleanup()
{
    int i;

    if (cddbCategories) {
        i = 0;
        do {
            libc_free(cddbCategories[i]);
        } while (strcmp(CDDB_CATEGORY[i++], "invalid"));

        free(cddbCategories);
        cddbCategories = NULL;
    }
}


cddb_track_t *cddb2_get_track(cddb_disc_t *cddbObj, track_t track)
{
    cddb_track_t *trackObj;

    if (cddbObj) {
        trackObj = cddb_disc_get_track(cddbObj, track ? (track - 1) : track);
        if (!trackObj) {
            cddb2_abort("failed to get cddb track object");
        }
    } else {
        trackObj = NULL;
    }

    return trackObj;
}
