//
// rip.h
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

#ifndef RIP_H_INCLUDED
#define RIP_H_INCLUDED

#include <stdio.h>

#include <cdio/cdio.h>
#include <cdio/paranoia.h>

#include "cddb2.h" // cddb_disc_t
#include "qsc.h" // MCN_LEN, ISRC_LEN


#define CUED_MAX_INDICES 100

extern void cued_rip_disc(

    // sound file information
    //
    const char *fileNamePattern,
    cddb_disc_t *cddbObj,
    int soundFileFormat,

    CdIo_t *cdObj,
    lsn_t firstTrack, lsn_t lastTrack,

    int ripToOneFile,
    int offsetWords,
    int getIndices,

    // paranoia
    //
    int useParanoia,
    cdrom_drive_t *paranoiaCtlObj, cdrom_paranoia_t *paranoiaRipObj,
    int retries,

    const char *qSubChannelFileName,

    char *fileNameBuffer, int bufferSize
    );

extern void cued_rip_to_file(

    // sound file information
    //
    char *filename,
    int channels, int soundFileFormat,

    CdIo_t *cdObj,
    lsn_t firstSector, lsn_t lastSector,

    track_t track,
    int offsetWords,
    lsn_t endOfDiscSector,
    int getIndices,

    // paranoia
    //
    int useParanoia,
    cdrom_drive_t *paranoiaCtlObj, cdrom_paranoia_t *paranoiaRipObj,
    int retries,

    // for error reporting
    const char *qSubChannelFileName, FILE *qSubChannelFile
    );

extern long cued_read_audio(cdrom_drive_t *paranoiaCtlObj, void *pb, lsn_t firstSector, long sectors);

extern void cued_init_rip_data();
extern void cued_free_paranoia_buf();

extern char  rip_mcn[ MCN_LEN + 1 ];
extern char  rip_isrc   [ CDIO_CD_MAX_TRACKS + 1 ][ ISRC_LEN + 1 ];
extern lsn_t rip_indices[ CDIO_CD_MAX_TRACKS + 1 ][ CUED_MAX_INDICES ];
extern int   rip_silent_pregap;
extern int   rip_noisy_pregap;
extern int   rip_year;


#endif // RIP_H_INCLUDED
