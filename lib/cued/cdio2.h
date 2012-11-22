//
// cdio2.h
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

#ifndef CDIO2_H_INCLUDED
#define CDIO2_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "cued_config.h" // CUED_HAVE_PARANOIA
#endif

#include "qsc.h"

#include <cdio/logging.h>

#ifdef CUED_HAVE_PARANOIA
#ifdef CUED_HAVE_CDIO_PARANOIA_PARANOIA_H
#include <cdio/paranoia/paranoia.h>
#else
#include <cdio/paranoia.h>
#endif // CUED_HAVE_CDIO_PARANOIA_PARANOIA_H
#endif // CUED_HAVE_PARANOIA

#include <stdio.h>
#include <stdarg.h>

#define CDIO2_LENGTH_LEN 5


extern void cdio2_set_log_handler();
extern void cdio2_unix_error(const char *fn, const char *arg, int isFatal);
extern void cdio2_logv(cdio_log_level_t level, const char *format, va_list args);
extern void cdio2_abort(const char *format, ...) GNUC_PRINTF(1, 2);

extern int cdio2_get_track_channels(CdIo_t *cdObj, track_t track);

extern void cdio2_fprint_cd_text(FILE *cueFile, CdIo_t *cdObj, track_t track, const char *prefix);
extern void cdio2_driver_error(driver_return_code_t ec, const char *when);

#ifdef CUED_HAVE_PARANOIA
extern void cdio2_paranoia_msg(cdrom_drive_t *paranoiaCtlObj, const char *when);
extern void cdio2_paranoia_callback(long int frame, paranoia_cb_mode_t prc);
#endif

extern lsn_t cdio2_get_track_length(CdIo_t *cdObj, track_t track);
extern void cdio2_get_length(char *length, lsn_t sectors);

static inline
void cdio2_set_2_digits(char *digits, int n)
{
    digits[0] = QSC_BCD_TO_ASCII(n / 10);
    digits[1] = QSC_BCD_TO_ASCII(n % 10);
}

static inline
void cdio2_set_2_digits_nt(char *digits, int n)
{
    cdio2_set_2_digits(digits, n);
    digits[2] = 0;
}

extern driver_return_code_t
mmc_read_cd_msf ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn,
                  int read_sector_type, bool b_digital_audio_play,
                  bool b_sync, uint8_t header_codes, bool b_user_data,
                  bool b_edc_ecc, uint8_t c2_error_information,
                  uint8_t subchannel_selection, uint16_t i_blocksize,
                  uint32_t i_blocks );


#endif // CDIO2_H_INCLUDED
