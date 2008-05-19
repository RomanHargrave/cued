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
#include "macros.h"

#include <cdio/paranoia.h>
#include <stdio.h>


#define CUED_MAX_INDICES 100


typedef struct _rip_data_t {

    lsn_t indices[ CUED_MAX_INDICES ];
    char  isrc   [ ISRC_LEN + 1 ];
    int flags;

} rip_data_t;


#define RIP_F_VERBOSE             0x00000001
#define RIP_F_RIP_TO_ONE_FILE     0x00000002
#define RIP_F_GET_INDICES         0x00000004
#define RIP_F_USE_FORMATTED_QSC   0x00000008
#define RIP_F_USE_PARANOIA        0x00000010
#define RIP_F_EXTRACT             0x00000020
#define RIP_F_SILENT_PREGAP       0x00000040
#define RIP_F_NOISY_PREGAP        0x00000080
#define RIP_F_USE_ECC_QSC         0x00000100
#define RIP_F_READ_PREGAP         0x00000200
#define RIP_F_DAP_FIXUP           0x00000400

#define RIP_F_DATA_VALID          0x00000001
#define RIP_F_DATA_PRE_EMPHASIS   0x00000002
#define RIP_F_DATA_COPY_PERMITTED 0x00000004
#define RIP_F_DATA_FOUR_CHANNELS  0x00000008


#define ripVerbose          TSTF(RIP_F_VERBOSE,              rip->flags)
#define ripToOneFile        TSTF(RIP_F_RIP_TO_ONE_FILE,      rip->flags)
#define ripGetIndices       TSTF(RIP_F_GET_INDICES,          rip->flags)
#define ripUseFormattedQsc  TSTF(RIP_F_USE_FORMATTED_QSC,    rip->flags)
#define ripUseParanoia      TSTF(RIP_F_USE_PARANOIA,         rip->flags)
#define ripExtract          TSTF(RIP_F_EXTRACT,              rip->flags)
#define ripSilentPregap     TSTF(RIP_F_SILENT_PREGAP,        rip->flags)
#define ripNoisyPregap      TSTF(RIP_F_NOISY_PREGAP,         rip->flags)
#define ripUseEccQsc        TSTF(RIP_F_USE_ECC_QSC,          rip->flags)
#define ripReadPregap       TSTF(RIP_F_READ_PREGAP,          rip->flags)
#define ripDapFixup         TSTF(RIP_F_DAP_FIXUP,            rip->flags)


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
    int flags;
    int offsetWords;
    const char *qSubChannelFileName;

    // paranoia parameters
    //
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

    union {
        uint8_t *mmcBuf;
        int16_t *mmcBuf16;
    };
    long allocatedSectors;

    //
    // cued_read_paranoid parameters
    //

    long (*save_read_paranoid)(cdrom_drive_t *, void *, lsn_t, long);

    //
    // rip data
    //

    rip_data_t ripData[ CDIO_CD_MAX_TRACKS + 1 ];
    char mcn[ MCN_LEN + 1 ];
    int year;

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
