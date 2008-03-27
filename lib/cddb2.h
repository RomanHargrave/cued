//
// cddb2.h
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

#ifndef CDDB2_H_INCLUDED
#define CDDB2_H_INCLUDED

#include <getopt.h>
#include <cddb/cddb.h>

#include <stdlib.h> // exit

typedef struct _CdIo CdIo_t;
typedef uint8_t track_t;


#define CDDB2_OPTIONS \
    "\t--no-cddb              disable cddb access\n" \
    "\t--no-cddb-cache        disable cddb cache\n" \
    "\t--cddb-http            alternate cddb access method\n" \
    "\t--cddb-match=number    choose cddb entry to use for disc\n" \
    "\t--cddb-server=hostname alternate cddb server\n" \
    "\t--cddb-port=number     alternate cddb port\n" \
    "\t--cddb-email=user@host alternate cddb email address\n" \
    "\t--cddb-timeout=seconds alternate cddb timeout\n" \
    "\t--cddb-cache=dir       alternate cddb cache directory\n"

extern void cddb2_opt_register_params();

extern void cddb2_set_log_handler();

extern cddb_disc_t *cddb2_get_disc(CdIo_t *cdObj);
extern cddb_disc_t *cddb2_get_match      (CdIo_t *cdObj, cddb_conn_t **dbObj, int *matches, int matchIndex);
extern cddb_disc_t *cddb2_get_first_match(CdIo_t *cdObj, cddb_conn_t **dbObj, int *matches);
extern cddb_disc_t *cddb2_get_next_match(cddb_conn_t *dbObj);

extern cddb_track_t *cddb2_get_track(cddb_disc_t *cddbObj, track_t track);
extern int cddb2_apply_pattern(
    CdIo_t *cdObj, cddb_disc_t *cddbObj, cddb_track_t *trackObj,
    const char *pattern, char *extension,
    track_t track,
    char *resultBuffer, int bufferSize,
    int terminator
    );
extern int cddb2_get_file_path(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, char *fileNameExt,
    track_t track,
    char *fileNameBuffer, int bufferSize
    );

extern void cddb2_set_tag(void *context, char *optarg, char *optionName);
extern void cddb2_make_tag_files(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, char *fileNameExt,
    track_t firstTrack, track_t lastTrack,
    char *resultBuffer, int bufferSize
    );
extern int cddb2_has_tags();

extern void cddb2_cleanup();

extern int cddb2_rip_year;


#endif // CDDB2_H_INCLUDED
