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
    QSC_MODE_MCN = 2,
    QSC_MODE_ISRC = 3,

    QSC_MODE_LEAD_IN = 5,

    QSC_MODE_PMA_UNSKIP_TRACK = 4,
    QSC_MODE_PMA_UNSKIP_INTERVAL = 6,

    QSC_MODE_MAX = 6,

    QSC_MODE_UNKNOWN = -1

} qsc_mode_t;

typedef struct _qsc_index_t {

    int track;
    int index;

    lsn_t relativeLsn;
    lsn_t absoluteLsn;

} qsc_index_t;


#define DECLARE_QSC(n) uint8_t n[16]

extern int qsc_check_crc(void *qsc);

extern qsc_mode_t qsc_get_mode(void *qsc);

extern int qsc_get_mcn  (void *qsc, char *mcn);
extern int qsc_get_isrc (void *qsc, char *isrc);
extern int qsc_get_index(void *qsc, qsc_index_t *index);

extern int qsc_get_psc  (void *qsc);

extern int qsc_msf_to_ascii(msf_t *msf_in, char *ascii);
extern int qsc_lsn_to_ascii(lsn_t lsn, char *ascii);
extern int qsc_lsn_to_ascii_for_cue(lsn_t lsn, char *ascii);
extern int qsc_msf_to_lsn(msf_t *msf, int *lsn);
extern int qsc_lsn_to_msf(lsn_t lsn, msf_t *msf);

extern crc16_t qsc_crc_data(uint8_t *data, ssize_t len);


#endif // QSC_H_INCLUDED
