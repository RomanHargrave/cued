//
// qsc.c
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

#include "qsc.h"


#undef DEBUG_CRC

#define CRC_MASK(n) (0xFF & (~(n)))
#define PSC_MASK(n) (0x01 & (n))
#define ADR_MASK(n) (0x0F & (n))
#define CTL_MASK(n) (0x0F & (n >> 4))

#define NIBBLE(v, n) ( \
    (((v)[ (n) / 2 ]) >> ((n) % 2 ? 0 : 4)) & 0x0F \
    )

#define ISRC_COUNTRY_OWNER_LEN 5
#define ISRC_YEAR_SERIAL_NO_LEN 7


#ifndef __CDIO_H__

struct msf_s {

    uint8_t m;
    uint8_t s;
    uint8_t f;
};

#endif // __CDIO_H__ 


// from mmc5r00.pdf: 4.3.4.5.2 and 6.25.3.2.3
// for ADR=0x01 in program area:
// (in lead-out, track=0xAA, index=01 bcd)
//
typedef struct _qsc_mode_1 {

    uint8_t track;
    uint8_t index;
    msf_t relative;
    uint8_t zero;
    msf_t absolute;

} qsc_mode_1;


typedef struct _qsc_mode_2 {

    uint8_t mcn[7];
    uint8_t zero;
    uint8_t absolute_frame;

} qsc_mode_2;


typedef struct _qsc_mode_3 {

    uint8_t country_owner[4];
    uint8_t year_serial_no[4];
    uint8_t absolute_frame;

} qsc_mode_3;


typedef struct _qsc_read_cd {

    union {
        struct {
            union {
                struct {

                    uint8_t ctl_adr;
                    union {

                        qsc_mode_1 mode_1;
                        qsc_mode_2 mode_2;
                        qsc_mode_3 mode_3;
                    };
                };

                uint8_t crc_data[10];

                struct {

                    uint8_t unused[8];
                    uint8_t absolute_frame;
                };
            };

            uint8_t crc_msb;
            uint8_t crc_lsb;
            uint8_t pad[3];
            uint8_t zero_psc;
        };

        // for compatibility between qsc_buffer_t and qsc_read_cd_t pointers
        // (ISO C aliasing rules)
        //
        qsc_buffer_t buf;
    };

} qsc_read_cd_t;


// from mmc5r00.pdf: 4.3.4.4
typedef enum _qsc_ctl_flag_t {

    qsc_ctl_flag_audio_pre_emphasis = 0x1,
    qsc_ctl_flag_data_track_incr    = 0x1,
    qsc_ctl_flag_copy_permitted     = 0x2,
    qsc_ctl_flag_data_track         = 0x4,
    qsc_ctl_flag_audio_four_channel = 0x8,
    qsc_ctl_flag_data_reserved      = 0x8

} qsc_ctl_flag_t;


// if a nibble is 4 bits, then a nabble must be 6 bits
// nabble is to nibble as weeble is to wobble
//
static inline
uint8_t NABBLE(uint8_t *v, int n)
{
    int byte = n * 3 / 4;
    uint8_t nabble;

    // 6-bit quantities have 4 forms that cycle every 3 bytes;  (6*4=24/8=3);
    // (assume the forms start on a byte boundary)
    //
    switch (n % 4)
    {
        case 0:
            // take the first 6 bits of the first byte
            nabble = (v[byte] >> 2) & 0x3F;
            break;

        case 1:
            // take the last 2 bits of the first byte and the first 4 bits of the second byte
            //nabble = ((v[byte] & 0x03) << 4) | ((v[ byte + 1 ] >> 4) & 0x0F);
            nabble =   ((v[byte] << 4) & 0x30) | ((v[ byte + 1 ] >> 4) & 0x0F);
            break;

        case 2:
            // take the last 4 bits of the first byte and the first 2 bits of the second byte
            //nabble = ((v[byte] & 0x0F) << 2) | ((v[ byte + 1 ] >> 6) & 0x03);
            nabble =   ((v[byte] << 2) & 0x3C) | ((v[ byte + 1 ] >> 6) & 0x03);
            break;

        case 3:
            // take the last 6 bits of the first byte
            nabble = v[byte] & 0x3F;
            break;

        default:
            // this cannot occur b/c n%4 cannot exceed 3!  BUT, this shuts up a gcc warning
            // about using nabble uninitialized;  in the future, expect this to be replaced
            // by a warning about unreachable code?
            //
            nabble = 0;
            break;
    }

    return nabble;
}


int qsc_has_pre_emphasis(qsc_buffer_t *p)
{
    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;
    int ctl = CTL_MASK(qsc->ctl_adr);

    return (qsc_ctl_flag_audio_pre_emphasis & ctl);
}


int qsc_has_copy_permitted(qsc_buffer_t *p)
{
    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;
    int ctl = CTL_MASK(qsc->ctl_adr);

    return (qsc_ctl_flag_copy_permitted & ctl);
}


int qsc_has_four_channels(qsc_buffer_t *p)
{
    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;
    int ctl = CTL_MASK(qsc->ctl_adr);

    return (qsc_ctl_flag_audio_four_channel & ctl);
}


int qsc_get_mcn(qsc_buffer_t *p, char *mcn)
{
    int i;
    uint8_t nibble;

    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;

    for (i = 0;  i < MCN_LEN;  ++i) {
        nibble = NIBBLE(qsc->mode_2.mcn, i);
        if (nibble <= 9) {
            mcn[i] = QSC_BCD_TO_ASCII(nibble);
        } else {
            return -1;
        }
    }

    mcn[MCN_LEN] = 0;

    return 0;
}


int qsc_get_isrc_year(char *isrc)
{
    int year;

    year  = 10 * QSC_ASCII_TO_BCD(isrc[ISRC_COUNTRY_OWNER_LEN]);
    year +=      QSC_ASCII_TO_BCD(isrc[ISRC_COUNTRY_OWNER_LEN + 1]);
    if (year >= 50) {
        year += 1900;
    } else {
        year += 2000;
    }

    return year;
}


int qsc_get_isrc(qsc_buffer_t *p, char *isrc)
{
    int i;
    uint8_t nibble, nabble;

    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;

    for (i = 0;  i < ISRC_COUNTRY_OWNER_LEN;  ++i) {
        nabble = NABBLE(qsc->mode_3.country_owner, i);
        if (nabble <= 9) {
            isrc[i] = QSC_BCD_TO_ASCII(nabble);
        } else if (nabble >= 0x11 && nabble <= 0x2A) {
            isrc[i] = 'A' - 0x11 + nabble;
        } else {
            return -1;
        }
    }

    // year (2 digit) and serial number (5 digit)
    for (i = 0;  i < ISRC_YEAR_SERIAL_NO_LEN;  ++i) {
        nibble = NIBBLE(qsc->mode_3.year_serial_no, i);
        if (nibble <= 9) {
            isrc[ i + ISRC_COUNTRY_OWNER_LEN ] = QSC_BCD_TO_ASCII(nibble);
        } else {
            return -1;
        }
    }

    // NULL terminate
    isrc[ISRC_LEN] = 0;

    return 0;
}


static inline
int bcd_to_int(uint8_t bcd, int *native)
{
    uint8_t nibble;

    nibble = NIBBLE(&bcd, 0);
    if (nibble <= 9) {
        *native = nibble * 10;
    } else {
        return -1;
    }

    nibble = NIBBLE(&bcd, 1);
    if (nibble <= 9) {
        *native += nibble;
    } else {
        return -1;
    }

    return 0;
}


static inline
int int_to_bcd(int native, uint8_t *bcd)
{
    if (native > 99) {
        return -1;
    }

    *bcd = ((native / 10) << 4) | (native % 10);

    return 0;
}


int qsc_msf_to_lsn(msf_t *msf, lsn_t *lsn)
{
    int min, sec, frm;

    if (   bcd_to_int(msf->m, &min)
        || bcd_to_int(msf->s, &sec)
        || bcd_to_int(msf->f, &frm)
       )
    {
        return -1;
    }

    *lsn = min * QSC_FPM + sec * QSC_FPS + frm - QSC_FPG;

    // from mmc mmc3r10g page 282
    if (min >= 90) {
        *lsn -= 450000;
    }

    return 0;
}


int qsc_lsn_to_msf(lsn_t lsn, msf_t *msf)
{
    int min, sec, frm;

    // from mmc mmc3r10g page 282
    lsn += QSC_FPG;
    if (lsn < 0) {
        lsn += 450000;
    }

    min  = lsn / QSC_FPM;
    lsn %= QSC_FPM;
    sec  = lsn / QSC_FPS;
    frm  = lsn % QSC_FPS;
  
    if (int_to_bcd(min, &msf->m)) {
        return -1;
    }

    (void) int_to_bcd(sec, &msf->s);
    (void) int_to_bcd(frm, &msf->f);

    return 0;
}


int qsc_msf_to_ascii(msf_t *msf_in, char *ascii)
{
    int nibble, i, j;
    uint8_t *msf = &msf_in->m;

    for (i = 0;  i < 3;  ++i) {
        for (j = 0;  j < 2;  ++j) {
            nibble = NIBBLE(msf, 2 * i + j);
            if (nibble <= 9) {
                ascii[ 3 * i + j ] = QSC_BCD_TO_ASCII(nibble);
            } else {
                return -1;
            }
        }
    }

    ascii[2] = ':';
    ascii[5] = ':';
    ascii[MSF_LEN] = 0;

    return 0;
}


int qsc_lsn_to_ascii(lsn_t lsn, char *ascii)
{
    msf_t msf;

    return (qsc_lsn_to_msf(lsn, &msf) || qsc_msf_to_ascii(&msf, ascii)) ? -1 : 0;
}


int qsc_get_index(qsc_buffer_t *p, qsc_index_t *index)
{
    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;

    if (   bcd_to_int(     qsc->mode_1.track,    &index->track)
        || bcd_to_int(     qsc->mode_1.index,    &index->index)
        || qsc_msf_to_lsn(&qsc->mode_1.relative, &index->relativeLsn)
        || qsc_msf_to_lsn(&qsc->mode_1.absolute, &index->absoluteLsn))
    {
        return -1;
    }

    return 0;
}


static crc16_t crc_table[]= {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


static inline
crc16_t crc_ccitt(crc16_t old_crc, uint8_t new_byte)
{
    // modified for CCITT from Robert Solovay's public domain version of crc16
    return (old_crc << 8) ^ crc_table[ (((old_crc >> 8) & 0xFF) ^ new_byte) ];
}


crc16_t qsc_crc_data(uint8_t *data, ssize_t len)
{
    int i;
    crc16_t crc = 0;

    for (i = 0;  i < len;  ++i) {
        crc = crc_ccitt(crc, data[i]);
    }

    return crc;
}


int qsc_check_crc(qsc_buffer_t *p)
{
    crc16_t crc;

    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;

    crc = qsc_crc_data(qsc->crc_data, sizeof(qsc->crc_data));

#ifdef DEBUG_CRC
    printf("crc_msb is %d;  crc_lsb is %d;  crc[0] is %d;  crc[1] is %d\n"
        , ~qscBuf.qsc.crc_msb & 0xFF
        , ~qscBuf.qsc.crc_lsb & 0xFF
        , (crc >> 8) & 0xFF
        , crc & 0xFF);
#endif

    if (   CRC_MASK(qsc->crc_msb) == ((crc >> 8) & 0xFF)
        && CRC_MASK(qsc->crc_lsb) == ((crc & 0xFF)))
    {
        return 0;
    } else {
        return -1;
    }
}


int qsc_get_psc(qsc_buffer_t *p)
{
    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;

    return PSC_MASK(qsc->zero_psc) ? 1 : 0;
}


qsc_mode_t qsc_get_mode(qsc_buffer_t *p)
{
    qsc_read_cd_t *qsc = (qsc_read_cd_t *) p;
    int adr = ADR_MASK(qsc->ctl_adr);

    if (adr >= QSC_MODE_MIN && adr <= QSC_MODE_MAX) {
        return (qsc_mode_t) adr;
    } else {
        return QSC_MODE_UNKNOWN;
    }
}
