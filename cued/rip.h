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

#include "cddb2.h" // cddb_disc_t
#include "qsc.h" // MCN_LEN, ISRC_LEN
#include "pwsc.h"

#include <cdio/paranoia.h>
#include <stdio.h>


#define CUED_MAX_INDICES 100


typedef struct _mmc_audio_buffer_t {

    int16_t buf[CD_FRAMEWORDS];
    union {
        mmc_raw_pwsc_t rawPWsc;
        qsc_buffer_t fmtQsc;
    };

} mmc_audio_buffer_t;

typedef struct _rip_context_t {

    //
    // cued_rip_disc parameters
    //

    CdIo_t *cdObj;

    // sound file naming information
    //
    const char *fileNamePattern;
    int soundFileFormat;
    cddb_disc_t *cddbObj;

    // rip parameters
    //
    lsn_t firstTrack;
    lsn_t lastTrack;
    int ripToOneFile;
    int offsetWords;
    int getIndices;
    int useFormattedQsc;
    const char *qSubChannelFileName;

    // paranoia parameters
    //
    int useParanoia;
    cdrom_drive_t *paranoiaCtlObj;
    cdrom_paranoia_t *paranoiaRipObj;
    int retries;

    // buffer for naming files
    //
    char *fileNameBuffer;
    int bufferSize;

    //
    // cued_rip_to_file parameters
    //

    lsn_t firstSector, lastSector;
    track_t currentTrack;

    int channels;

    lsn_t endOfDiscSector;
    FILE *qSubChannelFile;

    //
    // cued_write_cuefile parameters
    //

    const char *cueFileNamePattern;
    FILE *cueFile;

    //
    // cued_read_audio variables
    //
    mmc_audio_buffer_t audioBuf;

    //
    // cued_read_paranoid parameters
    //
    uint8_t *mmcBuf;
    long allocatedSectors;
    long (*save_read_paranoid)(cdrom_drive_t *, void *, lsn_t, long);

    //
    // rip data
    //
    char  mcn[ MCN_LEN + 1 ];
    char  isrc   [ CDIO_CD_MAX_TRACKS + 1 ][ ISRC_LEN + 1 ];
    lsn_t indices[ CDIO_CD_MAX_TRACKS + 1 ][ CUED_MAX_INDICES ];
    int   silent_pregap;
    int   noisy_pregap;
    int   year;

    //
    // cued_parse_qsc variables
    //
    int trackHint;
    int crcFailure;
    int crcSuccess;

} rip_context_t;


extern void cued_init_rip_data(rip_context_t *rip);
extern void cued_rip_disc     (rip_context_t *rip);
extern void cued_rip_to_file  (rip_context_t *rip);


#endif // RIP_H_INCLUDED
