//
// format.h
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

#ifndef FORMAT_H_INCLUDED
#define FORMAT_H_INCLUDED

#include <cddb/cddb.h>


extern int format_apply_pattern(
    CdIo_t *cdObj, cddb_disc_t *cddbObj, cddb_track_t *trackObj,
    const char *pattern, char *extension,
    track_t track,
    char *resultBuffer, int bufferSize,
    int terminator
    );
extern int format_get_file_path(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, char *fileNameExt,
    track_t track,
    char *fileNameBuffer, int bufferSize
    );

extern void format_set_tag(void *context, char *optarg, char *optionName);
extern void format_make_tag_files(
    CdIo_t *cdObj, cddb_disc_t *cddbObj,
    const char *fileNamePattern, char *fileNameExt,
    track_t firstTrack, track_t lastTrack,
    char *resultBuffer, int bufferSize
    );
extern int format_has_tags();
extern void format_cleanup();


#endif // FORMAT_H_INCLUDED
