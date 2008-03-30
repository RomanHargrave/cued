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

#include "cddb2.h"
#include "unix.h"
#include "cued.h"
#include "opt.h"
#include "macros.h"
#include "util.h"
#include "dlist.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include "cdio2.h"

#include <stdlib.h> // malloc


#define ALTERNATION_CHAR '<'
#define TERMINATION_CHAR '>'


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

    cdio_log(newLevel, message);
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


static int disableCddb;
static int disableCache;
static int useHttp;

static char *cacheDir;
static char *server;
static int port;
static int timeout;
static char *email;
static int matchIndex;

void cddb2_opt_register_params()
{
    opt_param_t opts[] = {

        { "no-cddb",        &disableCddb,   opt_set_flag,       OPT_NONE },
        { "no-cddb-cache",  &disableCache,  opt_set_flag,       OPT_NONE },
        { "cddb-http",      &useHttp,       opt_set_flag,       OPT_NONE },

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

    if (disableCache) {
        cddb_cache_disable(dbObj);
    }

    if (useHttp) {
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

    cddb_set_client(dbObj, CUED_PRODUCT_NAME, CUED_VERSION);

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


cddb_disc_t *cddb2_get_disc(CdIo_t *cdObj)
{
    cddb_conn_t *dbObj = NULL;
    cddb_disc_t *discObj = NULL;
    FILE *save;
    int matches = 0;

    if (disableCddb) {
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

#define ASSIGN(c) \
    { \
        if (resultIndex == bufferSize) { \
            cddb2_abort("format would produce result exceeding maximum length"); \
            goto error; \
        } else { \
            resultBuffer[ resultIndex++ ] = (c); \
        } \
    }


int cddb2_apply_pattern(
    CdIo_t *cdObj, cddb_disc_t *cddbObj, cddb_track_t *trackObj,
    const char *pattern, char *extension,
    track_t track,
    char *resultBuffer, int bufferSize,
    int terminator)
{
    const char *field;
    cdtext_t *cdtext;
    lsn_t lsn;
    int resultIndex = 0;
    int patternIndex = 0;
    int patternChar;
    int n;

    // expanded from 9 for %S
    char nstr[ TIME_RFC_3339_LEN + 1 ];

    for (;;) {

        patternChar = pattern[ patternIndex++ ];
        if (terminator == patternChar) {

            // cddb2_apply_pattern is always called with TERMINATION_CHAR except
            // at the top level;  hence, if terminator is 0, this is NOT
            // the recursive case
            //
            if (!terminator) {

                // append extension
                do {
                    ASSIGN(*extension)
                } while (*extension++);

                // indicate success
                return 0;
            }

            // this IS the recursive case
            if (resultIndex != bufferSize) {

                // need to return two items after recursion;  do not want to recurse
                // with pointers to those items in consideration of readability!
                // instead, null terminate resultBuffer so strlen can be used
                // by the caller to determine how much was added to resultBuffer
                // and update resultIndex accordingly;  patternIndex is simply
                // returned
                //
                resultBuffer[resultIndex] = 0;

                return patternIndex;
            } else {

                // this will error;  no need to duplicate the error message in the macro;
                // duplication reduces maintainability
                //
                ASSIGN(0)
            }
        }

        switch (patternChar) {

            case 0:
                // already checked for terminator, so this is a recursive case
                cddb2_abort("format ends improperly without a %c", TERMINATION_CHAR);
                goto error;

            case '%':
                patternChar = pattern[ patternIndex++ ];
                if (terminator == patternChar) {
                    // %% was not working in glibc at the time this was written
                    cddb2_abort("format contains percent without substitution code");
                    goto error;
                }

                switch (patternChar) {
                    //
                    // N.B.  if a case is added to this switch, it must also be added to the switch
                    // or the strchr in the for loop in the alternation code
                    //
                    case 0:
                        cddb2_log_handler(CDDB_LOG_CRITICAL, "format contains percent without substitution code");
                        cddb2_abort("format ends improperly without a %c", TERMINATION_CHAR);
                        // avoid warning about field being used uninitialized
                        field = "";
                        goto error;

                    case '%':
                    case '<':
                    case '>':
                        // standard printf-style %% case, which wasn't working in glibc at the time
                        ASSIGN(patternChar)
                        // skip alternation code
                        goto next;

                    case 'L':
                        if (track) {
                            lsn = cdio2_get_track_length(cdObj, track);
                        } else {
                            lsn = cdio_get_track_lsn(cdObj, 1);
                            if (CDIO_INVALID_LSN == lsn) {
                                cddb2_abort("failed to get first sector number for track %02d", 1);
                                goto error;
                            }
                        }
                        cdio2_get_length(nstr, lsn);
                        field = nstr;
                        break;

                    case 'M':
                        lsn = cdio_get_disc_last_lsn(cdObj);
                        if (CDIO_INVALID_LSN == lsn) {
                            cddb2_abort("failed to get last sector number");
                            goto error;
                        }
                        cdio2_get_length(nstr, lsn);
                        field = nstr;
                        break;

                    case 'N':
                        // allow conditional handling of pre-gap track
                        if (track) {
                            cdio2_set_2_digits_nt(nstr, track);
                            field = nstr;
                        } else {
                            field = NULL;
                        }
                        break;

                    case 'O':
                        n = cdio_get_num_tracks(cdObj);
                        if (CDIO_INVALID_TRACK == n) {
                            cddb2_abort("failed to get number of tracks");
                            goto error;
                        }
                        cdio2_set_2_digits_nt(nstr, n);
                        field = nstr;
                        break;

#if 0
                    // doesn't work too well if you don't rip before creating tags
                    case 'y':
                        if (cddb2_rip_year) {
                            if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%d", cddb2_rip_year)) {
                                cddb2_abort("year %d exceeds %ld characters (internal error)", cddb2_rip_year, sizeof(nstr) - 1);
                                goto error;
                            }
                            field = nstr;                          
                        } else {
                            field = NULL;
                        }
                        break;
#endif

                    case 'a':
                    case 'd':
                    case 'i':
                    case 'j':
                    case 'm':
                    case 'n':
                    case 'r':
                    case 's':
                    case 'w':
                    case 'x':
                    case 't':
                    case 'c':
                        switch (patternChar) {
                            case 'c':
                            case 'd':
                            case 'j':
                            case 'n':
                            case 's':
                            case 'x':
                                cdtext = cdio_get_cdtext(cdObj, 0);
                                break;

                            default:
                                cdtext = cdio_get_cdtext(cdObj, track);
                                break;
                        }
                        if (cdtext) {
                            cdtext_field_t key;
                            switch (patternChar) {
                                case 'a':
                                case 'd':
                                    key = CDTEXT_PERFORMER;
                                    break;
                                case 'i':
                                case 'j':
                                    key = CDTEXT_GENRE;
                                    break;
                                case 'm':
                                case 'n':
                                    key = CDTEXT_COMPOSER;
                                    break;
                                case 'r':
                                case 's':
                                    key = CDTEXT_ARRANGER;
                                    break;
                                case 'w':
                                case 'x':
                                    key = CDTEXT_SONGWRITER;
                                    break;
                                case 't':
                                case 'c':
                                    key = CDTEXT_TITLE;
                                    break;
                                default:
                                    key = CDTEXT_INVALID;
                                    cddb2_abort("internal error getting cdtext for format");
                                    goto error;
                                    break;
                            }
                            field = cdtext_get_const(key, cdtext);
                        } else {
                            field = NULL;
                        }
                        break;

                    case 'S':
                        if (rfc3339time(nstr, sizeof(nstr))) {
                            cddb2_abort("failed to get date/time for format");
                            goto error;
                        }
                        field = nstr;
                        break;

                    case 'V':
                        if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), CUED_VERSION)) {
                            cddb2_abort("%s version exceeds %ld characters (internal error)", CUED_PRODUCT_NAME, sizeof(nstr) - 1);
                            goto error;
                        }
                        field = nstr;
                        break;

                    default:
                        //
                        // everything from this point on requires cddb objects
                        //
                        if (!trackObj) {
                            field = NULL;
                            break;
                        }

                        switch (patternChar) {
                            case 'T':
                                if (track) {
                                    field = cddb_track_get_title(trackObj);
                                } else {
                                    field = "Hidden Audio Pregap";
                                }
                                break;

                            case 'A':
                                field = cddb_track_get_artist(trackObj);
                                if (field && !track) {
                                    field = "Unknown Artist";
                                }
                                break;

                            case 'Y':
                                n = cddb_disc_get_year(cddbObj);
                                if (n) {
                                    if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%d", n)) {
                                        cddb2_abort("cddb year exceeds %ld digits (year=%d)", sizeof(nstr) - 1, n);
                                        goto error;
                                    }
                                    field = nstr;
                                } else {
                                    field = NULL;
                                }
                                break;

                            case 'C':
                                field = cddb_disc_get_title(cddbObj);
                                break;

                            case 'D':
                                field = cddb_disc_get_artist(cddbObj);
                                break;

                            case 'B':
                                field = cddbCategories[ cddb_disc_get_category(cddbObj) ];
                                break;

                            case 'I':
                                field = cddb_disc_get_genre(cddbObj);
                                break;

                            case 'K':
                                n = cddb_disc_get_category(cddbObj);
                                if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%d", n)) {
                                    cddb2_abort("cddb category exceeds %ld digits (category=%d)", sizeof(nstr) - 1, n);
                                    goto error;
                                }
                                field = nstr;
                                break;

                            case 'F':
                                n = cddb_disc_get_discid(cddbObj);
                                if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%x", n)) {
                                    cddb2_abort("cddb disc id exceeds %ld characters (id=%x)", sizeof(nstr) - 1, n);
                                    goto error;
                                }
                                field = nstr;
                                break;

                            default:
                                cddb2_abort("format contains unrecognized substitution code \"%c\"", patternChar);
                                goto error;
                        }
                        break;
                }

                // check for conditional after substitution code
                if (ALTERNATION_CHAR == pattern[patternIndex]) {

                    // skip the alternation character and peek at the next character
                    patternChar = pattern[++patternIndex];

                    // if the conditional logic evaluates to true, then recurse
                    if (   (ALTERNATION_CHAR == patternChar && !(field && field[0]))
                        || (ALTERNATION_CHAR != patternChar &&  (field && field[0])))
                    {
                        // if the next character is also an alternation character, skip it
                        if (ALTERNATION_CHAR == patternChar) {
                            ++patternIndex;
                        }

                        // recursion allows nested conditionals and substitution codes within conditionals
                        n = cddb2_apply_pattern(cdObj, cddbObj, trackObj,
                                &pattern[patternIndex], NULL, track,
                                &resultBuffer[resultIndex],
                                bufferSize - resultIndex,
                                TERMINATION_CHAR);
                        if (-1 == n) {
                            return -1;
                        }

                        // resultBuffer was NULL terminated so that resultIndex
                        // could be updated using strlen
                        //
                        resultIndex += strlen(&resultBuffer[resultIndex]);
                        patternIndex += n;

                    } else {

                        // conditional logic did NOT evaluate to true, so skip conditional;
                        // n handles nesting by counting the number of expected TERMINATION_CHAR's
                        //
                        for (n = 1;  ;  ) {
                            patternChar = pattern[ patternIndex++ ];
                            switch (patternChar) {

                                case 0:
                                    cddb2_abort("format ends improperly without a %c", TERMINATION_CHAR);
                                    goto error;

                                case TERMINATION_CHAR:
                                    if (!--n) {

                                        // finally out of conditional
                                        goto next;
                                    }
                                    break;

                                case '%':

                                    patternChar = pattern[patternIndex];
                                    switch (patternChar) {

                                        case '%':
                                        case '<':
                                        case '>':
                                            // skip escaped characters so they are not misinterpreted as conditionals
                                            ++patternIndex;
                                            break;

                                        case 0:
                                            // handle this error at the top of the for loop
                                            break;

                                        default:
                                            //
                                            // all other possibilities eliminated;  must be a substitution code
                                            //
                                            if (!strchr("LMNOVStaimrwcdjnsxTACDIBKYF", patternChar)) {
                                                cddb2_abort("format contains unrecognized substitution code \"%c\"", patternChar);
                                                goto error;
                                            }

                                            // don't bother to check if trackObj is valid because conditional behavior may exploit
                                            // it being invalid to skip these substitution codes
                                            //

                                            // does the substitution code have a nested conditional?
                                            if (ALTERNATION_CHAR == pattern[ patternIndex + 1 ]) {

                                                // expect to see another TERMINATION_CHAR for the nested conditional
                                                ++n;

                                                // skip the substitution and alternation characters
                                                patternIndex += 2;

                                                // could check for and skip another ALTERNATION_CHAR for the negation case,
                                                // but the default case should skip it in the outer switch
                                                //
                                            }
                                            break;
                                    }
                                    break;

                                default:
                                    // skip the non-special character in the conditional
                                    break;
                            }
                        }
                    }

                } else if (field) {
                    //
                    // this is the substitution case without alternation;  simply output the substitution
                    //
                    while (*field) {
                        ASSIGN(*field++)
                    }
                }
                break;

            // at last, the simplest case;  copy a character from the pattern to the result buffer
            default:
                ASSIGN(patternChar)
                break;
        }
    next:
        ;
    }

error:

    // safety measure;  unterminated strings can be nasty
    resultBuffer[resultIndex] = 0;

    return -1;
}


int cddb2_get_file_path(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, char *fileNameExt,
    track_t track,
    char *fileNameBuffer, int bufferSize
    )
{
    cddb_track_t *trackObj;
    char *lastSlash;
    int rc;

    // upper case the first character of each cddb category
    if (!cddbCategories) {
        int i, n;

        for (n = 0;  strcmp(CDDB_CATEGORY[n++], "invalid");  )
            ;
        cddbCategories = (char **) malloc(n * sizeof(char *));
        if (!cddbCategories) {
            cddb2_abort("out of memory allocating cddb categories");
        }

        for (i = 0;  i < n;  ++i) {
            cddbCategories[i] = strdup(CDDB_CATEGORY[i]);
            if (cddbCategories[i]) {
                cddbCategories[i][0] = toupper(cddbCategories[i][0]);
            } else {
                cddb2_abort("out of memory allocating cddb category string");
            }
            //printf("category is %s\n", cddbCategories[i]);
        }
    }

    trackObj = cddb2_get_track(cddbObj, track);
    rc = cddb2_apply_pattern(cdObj, cddbObj, trackObj, fileNamePattern, fileNameExt, track, fileNameBuffer, bufferSize, 0);
    if (!rc) {

        // make directories
        //
        lastSlash = strrchr(fileNameBuffer, '/');
        if (lastSlash) {
            *lastSlash = 0;
            if (mkdirp(fileNameBuffer)) {
                cdio2_unix_error("mkdir", fileNameBuffer, 1);
            }
            *lastSlash = '/';
        }
    } else {
        cddb2_abort("failed to make filename for track %02d from pattern \"%s\"", track, fileNamePattern);
    }

    return rc;
}


typedef struct _tag_t
{
    char *pattern;
    d_list_node_t listNode;

} tag_t;

static DLIST_DECLARE(tagList)


void cddb2_cleanup()
{
    d_list_node_t *node;
    tag_t *tag;

    free(cddbCategories);
    cddbCategories = NULL;

    for (node = tagList.next;  node != &tagList;  ) {
        tag = FIELD_TO_STRUCT(node, listNode, tag_t);
        node = node->next;
        dListRemoveNode(&tag->listNode);
        free(tag);
    }
}


void cddb2_set_tag(void *context, char *optarg, char *optionName)
{
    tag_t *tag = (tag_t *) malloc(sizeof(tag_t));
    if (!tag) {
        cddb2_abort("out of memory allocating tag");
    }

    tag->pattern = optarg;
    dListInsertTail(&tagList, &tag->listNode);
}


int cddb2_has_tags()
{
    return dListIsEmpty(&tagList) ? 0 : 1;
}


void cddb2_make_tag_files(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, char *fileNameExt,
    track_t firstTrack, track_t lastTrack,
    char *resultBuffer, int bufferSize)
{
    FILE *tagFile;
    cddb_track_t *trackObj;
    d_list_node_t *node;
    tag_t *tag;
    int rc;
    track_t track;

    if (!cddb2_has_tags()) {
        return;
    }

    for (track = firstTrack;  track <= lastTrack;  ++track) {

        trackObj = cddb2_get_track(cddbObj, track);

        rc = cddb2_get_file_path(cdObj, cddbObj, fileNamePattern, fileNameExt, track, resultBuffer, bufferSize);
        if (rc) {
            cddb2_error("failed to make filename for tags for track %02d", track);

            continue;
        }

        tagFile = fopen2(resultBuffer, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0666);
        if (!tagFile) {
            cdio2_unix_error("fopen2", resultBuffer, 0);
            cddb2_error("not creating tag file \"%s\"", resultBuffer);

            continue;
        }
        // begin life of tag file

            for (node = tagList.next;  node != &tagList;  node = node->next) {
                tag = FIELD_TO_STRUCT(node, listNode, tag_t);

                rc = cddb2_apply_pattern(cdObj, cddbObj, trackObj, tag->pattern, "\n", track, resultBuffer, bufferSize, 0);
                if (!rc) {
                    if ('\n' != resultBuffer[0]) {
                        fprintf(tagFile, resultBuffer);
                    }
                } else {
                    cddb2_error("could not create tag for track %02d from pattern \"%s\"", track, tag->pattern);

                    continue;
                }
            }

        // end life of tag file
        fclose(tagFile);
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
