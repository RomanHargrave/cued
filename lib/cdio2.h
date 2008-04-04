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

#include "qsc.h"

#include <cdio/logging.h>
#include <cdio/paranoia.h>

#include <stdio.h>
#include <stdarg.h>

#define CDIO2_LENGTH_LEN 5


extern void cdio2_set_log_handler();
extern void cdio2_unix_error(const char *fn, const char *arg, int isFatal);
extern void cdio2_logv(cdio_log_level_t level, const char *format, va_list args);
extern void cdio2_abort(const char *format, ...) GNUC_PRINTF(1, 2);

extern int cdio2_get_track_channels(CdIo_t *cdObj, track_t track);

extern void cdio2_fprint_cd_text(FILE *cueFile, CdIo_t *cdObj, track_t track, const char *prefix);

extern void cdio2_paranoia_msg(cdrom_drive_t *paranoiaCtlObj, char *when);
extern void cdio2_driver_error(driver_return_code_t ec, char *when);
extern void cdio2_paranoia_callback(long int frame, paranoia_cb_mode_t prc);

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


#endif // CDIO2_H_INCLUDED
