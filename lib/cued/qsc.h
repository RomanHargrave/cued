//
// qsc.h
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

#ifndef QSC_H_INCLUDED
#define QSC_H_INCLUDED

#include "qsc_types.h"


#define MCN_LEN 13
#define ISRC_LEN 12
#define MSF_LEN 8


typedef enum _qsc_mode_t {

    QSC_MODE_MIN = 1,

    QSC_MODE_INDEX = 1,
    QSC_MODE_TRACK_INFO = 1,
    QSC_MODE_MCN = 2,
    QSC_MODE_ISRC = 3,

    QSC_MODE_MULTI_SESSION = 5,

    QSC_MODE_PMA_UNSKIP_TRACK = 4,
    QSC_MODE_PMA_UNSKIP_INTERVAL = 6,

    QSC_MODE_MAX = 6,

    QSC_MODE_UNKNOWN = -1

} qsc_mode_t;

typedef struct _qsc_index_t {

    int track;
    int index;

    lba_t relativeLba;
    lba_t absoluteLba;

} qsc_index_t;

typedef struct _qsc_buffer_t {

    uint8_t subcodeData[16];

} qsc_buffer_t;

typedef struct _qsc_file_buffer_t {

    qsc_buffer_t buf;
    lsn_t requested;

} qsc_file_buffer_t;


extern int qsc_check_crc(qsc_buffer_t *qsc);

extern qsc_mode_t qsc_get_mode(qsc_buffer_t *qsc);

extern int qsc_get_mcn  (qsc_buffer_t *qsc, char *mcn);
extern int qsc_get_isrc (qsc_buffer_t *qsc, char *isrc);
extern int qsc_get_index(qsc_buffer_t *qsc, qsc_index_t *index);

extern int qsc_has_pre_emphasis  (qsc_buffer_t *qsc);
extern int qsc_has_copy_permitted(qsc_buffer_t *qsc);
extern int qsc_has_four_channels (qsc_buffer_t *qsc);

extern int qsc_get_psc  (qsc_buffer_t *qsc);

extern int qsc_get_isrc_year(char *isrc);

extern int qsc_msf_to_ascii(msf_t *msf_in, char *ascii);

extern int qsc_lba_to_ascii(lba_t lba, char *ascii);
extern int qsc_lsn_to_ascii(lsn_t lsn, char *ascii);

extern crc16_t qsc_crc_data(uint8_t *data, ssize_t len);


#define QSC_BCD_TO_ASCII(n) ((n) + '0')
#define QSC_ASCII_TO_BCD(n) ((n) - '0')


// frames per second
#define QSC_FPS 75

// frames per minute
#define QSC_FPM (QSC_FPS * 60)

// frames in pre-gap
#define QSC_FPG (QSC_FPS * 2)

#define QSC_LSN_TO_LBA(n) ((n) + QSC_FPG)
#define QSC_LBA_TO_LSN(n) ((n) - QSC_FPG)


#endif // QSC_H_INCLUDED
