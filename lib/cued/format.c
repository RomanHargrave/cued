//
// format.c
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
#include "unix.h"
#include "cued.h" // CUED_PRODUCT_NAME
#include "dlist.h"
#include "macros.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdio.h>
#include "format.h"
#include "cdio2.h"
#include "cddb2.h"

#include <stdlib.h> // malloc
#include <string.h>


#define ALTERNATION_CHAR '<'
#define TERMINATION_CHAR '>'

#define ASSIGN(c) \
    { \
        if (resultIndex == bufferSize) { \
            cdio2_abort("format would produce result exceeding maximum length"); \
            goto error; \
        } else { \
            resultBuffer[ resultIndex++ ] = (c); \
        } \
    }


int format_apply_pattern(
    CdIo_t *cdObj, cddb_disc_t *cddbObj, cddb_track_t *trackObj,
    const char *pattern, const char *extension,
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

            // format_apply_pattern is always called with TERMINATION_CHAR except
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
                cdio2_abort("format ends improperly without a %c", TERMINATION_CHAR);
                goto error;

            case '%':
                patternChar = pattern[ patternIndex++ ];
                if (terminator == patternChar) {
                    // %% was not working in glibc at the time this was written
                    cdio2_abort("format contains percent without substitution code");
                    goto error;
                }

                switch (patternChar) {
                    //
                    // N.B.  if a case is added to this switch, it must also be added to the switch
                    // or the strchr in the for loop in the alternation code
                    //
                    case 0:
                        cdio_log(CDIO_LOG_ASSERT, "format contains percent without substitution code");
                        cdio2_abort("format ends improperly without a %c", TERMINATION_CHAR);
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
                                cdio2_abort("failed to get first sector number for track %02d", 1);
                                goto error;
                            }
                        }
                        cdio2_get_length(nstr, lsn);
                        field = nstr;
                        break;

                    case 'M':
                        lsn = cdio_get_disc_last_lsn(cdObj);
                        if (CDIO_INVALID_LSN == lsn) {
                            cdio2_abort("failed to get last sector number");
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
                            cdio2_abort("failed to get number of tracks");
                            goto error;
                        }
                        cdio2_set_2_digits_nt(nstr, n);
                        field = nstr;
                        break;

#if 0
                    // doesn't work too well if you don't rip before creating tags
                    case 'y':
                        if (format_rip_year) {
                            if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%d", format_rip_year)) {
                                cdio2_abort("year %d exceeds %ld characters (internal error)", format_rip_year, sizeof(nstr) - 1);
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
                                    cdio2_abort("internal error getting cdtext for format");
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
                            cdio2_abort("failed to get date/time for format");
                            goto error;
                        }
                        field = nstr;
                        break;

                    case 'V':
                        if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "v" PACKAGE_VERSION)) {
                            cdio2_abort("%s version (v%s) exceeds %zu characters (internal error)",
                                        CUED_PRODUCT_NAME, PACKAGE_VERSION, sizeof(nstr) - 1);
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
                                        cdio2_abort("cddb year exceeds %zu digits (year=%d)", sizeof(nstr) - 1, n);
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
                                field = cddb2_get_category(cddbObj);
                                break;

                            case 'I':
                                field = cddb_disc_get_genre(cddbObj);
                                break;

                            case 'K':
                                n = cddb_disc_get_category(cddbObj);
                                if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%d", n)) {
                                    cdio2_abort("cddb category exceeds %zu digits (category=%d)", sizeof(nstr) - 1, n);
                                    goto error;
                                }
                                field = nstr;
                                break;

                            case 'F':
                                n = cddb_disc_get_discid(cddbObj);
                                if (ssizeof(nstr) <= snprintf(nstr, sizeof(nstr), "%x", n)) {
                                    cdio2_abort("cddb disc id exceeds %zu characters (id=%x)", sizeof(nstr) - 1, n);
                                    goto error;
                                }
                                field = nstr;
                                break;

                            default:
                                cdio2_abort("format contains unrecognized substitution code \"%c\"", patternChar);
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
                        n = format_apply_pattern(cdObj, cddbObj, trackObj,
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
                                    cdio2_abort("format ends improperly without a %c", TERMINATION_CHAR);
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
                                                cdio2_abort("format contains unrecognized substitution code \"%c\"", patternChar);
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


int format_get_file_path(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, const char *fileNameExt,
    track_t track,
    char *fileNameBuffer, int bufferSize
    )
{
    cddb_track_t *trackObj;
    char *lastSlash;
    int rc;

    trackObj = cddb2_get_track(cddbObj, track);
    rc = format_apply_pattern(cdObj, cddbObj, trackObj, fileNamePattern, fileNameExt, track, fileNameBuffer, bufferSize, 0);
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
        cdio2_abort("failed to make filename for track %02d from pattern \"%s\"", track, fileNamePattern);
    }

    return rc;
}


typedef struct _tag_t
{
    char *pattern;
    d_list_node_t listNode;

} tag_t;

static DLIST_DECLARE(tagList)


void format_set_tag(void *context, char *optarg, const char *optionName)
{
    tag_t *tag = (tag_t *) malloc(sizeof(tag_t));
    if (!tag) {
        cdio2_abort("out of memory allocating tag");
    }

    tag->pattern = optarg;
    dListInsertTail(&tagList, &tag->listNode);
}


int format_has_tags()
{
    return dListIsEmpty(&tagList) ? 0 : 1;
}


void format_cleanup()
{
    d_list_node_t *node;
    tag_t *tag;

    for (node = tagList.next;  node != &tagList;  ) {
        tag = FIELD_TO_STRUCT(node, listNode, tag_t);
        node = node->next;
        dListRemoveNode(&tag->listNode);
        free(tag);
    }
}


void format_make_tag_files(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, const char *fileNameExt,
    track_t firstTrack, track_t lastTrack,
    char *resultBuffer, int bufferSize)
{
    FILE *tagFile;
    cddb_track_t *trackObj;
    d_list_node_t *node;
    tag_t *tag;
    int rc;
    track_t track;

    if (!format_has_tags()) {
        return;
    }

    for (track = firstTrack;  track <= lastTrack;  ++track) {

        trackObj = cddb2_get_track(cddbObj, track);

        rc = format_get_file_path(cdObj, cddbObj, fileNamePattern, fileNameExt, track, resultBuffer, bufferSize);
        if (rc) {
            cdio_error("failed to make filename for tags for track %02d", track);

            continue;
        }

        tagFile = fopen2(resultBuffer, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0666);
        if (!tagFile) {
            cdio2_unix_error("fopen2", resultBuffer, 0);
            cdio_error("not creating tag file \"%s\"", resultBuffer);

            continue;
        }
        // begin life of tag file

            for (node = tagList.next;  node != &tagList;  node = node->next) {
                tag = FIELD_TO_STRUCT(node, listNode, tag_t);

                rc = format_apply_pattern(cdObj, cddbObj, trackObj, tag->pattern, "\n", track, resultBuffer, bufferSize, 0);
                if (!rc) {
                    if ('\n' != resultBuffer[0]) {
                        fprintf(tagFile, "%s", resultBuffer);
                    }
                } else {
                    cdio_error("could not create tag for track %02d from pattern \"%s\"", track, tag->pattern);

                    continue;
                }
            }

        // end life of tag file
        fclose(tagFile);
    }
}
