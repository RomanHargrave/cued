//
// cdio2.c: libcdio "enhancements"
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

#ifdef HAVE_CONFIG_H
#include <cued/cued_config.h> // CUED_HAVE_CDTEXT_V2, CUED_HAVE_PARANOIA
#endif

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdio.h>
#include <cdio/util.h> // cdio_from_bcd8
#include <cdio/mmc.h>
#ifdef CUED_HAVE_CDIO_MMC_LL_CMDS_H
#include <cdio/mmc_ll_cmds.h>
#endif
#include <cued/cdio2.h>

#include <string.h> // strlen, strerror, strchr
#include <errno.h>
#include <stdlib.h> // EXIT_FAILURE
#include <unistd.h> // getopt


void cdio2_unix_error(const char *fn, const char *arg, int isFatal)
{
    int quoted = strlen(arg);

    cdio_log(isFatal ? CDIO_LOG_ASSERT : CDIO_LOG_ERROR,
        "%s(%s%s%s) returned error \"%s\" (errno = %d)",
        fn, quoted ? "\"" : "", arg, quoted ? "\"" : "", strerror(errno), errno);

    if (isFatal) {
        exit(EXIT_FAILURE);
    }
}


static cdio_log_handler_t oldLogFn;


static void cdio2_log_handler(cdio_log_level_t level, const char *message)
{
    if (level >= cdio_loglevel_default) {
        const char *levelStr;

        switch (level) {

            case CDIO_LOG_DEBUG:
                levelStr = "Debug";
                break;

            case CDIO_LOG_INFO:
                levelStr = "Information";
                break;

            case CDIO_LOG_WARN:
                levelStr = "Warning";
                break;

            case CDIO_LOG_ERROR:
                levelStr = "Error";
                break;

            case CDIO_LOG_ASSERT:
                levelStr = "Critical";
                fprintf(stderr, "\n%s (%d): %s\n\n", levelStr, level, message);
                return;

            default:
                levelStr = "UNKNOWN";
                break;
        }

        fprintf(stderr, "%s (%d): %s\n", levelStr, level, message);
    }

    // for comparison
    //oldLogFn(level, message);
}


void cdio2_set_log_handler()
{
    oldLogFn = cdio_log_set_handler(cdio2_log_handler);
}


void cdio2_logv(cdio_log_level_t level, const char *format, va_list args)
{
    char msgbuf[1024] = { 0, };
  
    // N.B. GNU libc documentation indicates it always NULL terminates
    vsnprintf(msgbuf, sizeof(msgbuf), format, args);

    cdio_log(level, "%s", msgbuf);
}


void cdio2_abort(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    cdio2_logv(CDIO_LOG_ASSERT, format, args);
    va_end(args);

    exit(EXIT_FAILURE);
}


void cdio2_fprint_cd_text(FILE *cueFile, CdIo_t *cdObj, track_t track, const char *prefix)
{
    int i, quoted;
    cdtext_t *cdtext;
    const char *field;

#ifdef CUED_HAVE_CDTEXT_V2
    cdtext = cdio_get_cdtext(cdObj);

    // set to the first cd-text block AND make sure we're using the new library (side effect)
    if (cdtext) {
        (void) cdtext_select_language(cdtext, CDTEXT_LANGUAGE_UNKNOWN);
    }
#else
    cdtext = cdio_get_cdtext(cdObj, track);
#endif

    if (cdtext) {
        for (i = MIN_CDTEXT_FIELD;  i < MAX_CDTEXT_FIELDS;  ++i) {
#ifdef CUED_HAVE_CDTEXT_V2
            field = cdtext_get_const(cdtext, (cdtext_field_t) i, track);
#else
            field = cdtext_get_const((cdtext_field_t) i, cdtext);
#endif
            // checking for field[0] is for Nero, which sometime has zero length
            if (field && field[0]) {
                quoted = strchr(field, ' ') ? 1 : 0;
                fprintf(cueFile, "%s%s %s%s%s\n", prefix, cdtext_field2str((cdtext_field_t) i),
                    quoted ? "\"" : "", field, quoted ? "\"" : "");
            }
        }

        //cdtext_destroy(cdtext);
    }
}


void cdio2_driver_error(driver_return_code_t ec, const char *when)
{
    if (DRIVER_OP_SUCCESS != ec) {
        cdio_error("received following error during %s: %s", when, cdio_driver_errmsg(ec));
    }
}


#ifdef CUED_HAVE_PARANOIA

void cdio2_paranoia_msg(cdrom_drive_t *paranoiaCtlObj, const char *when)
{
    char *msg;

    msg = cdio_cddap_messages(paranoiaCtlObj);
    if (msg) {
        cdio_info("paranoia returned the following messages during %s:\n%s", when, msg);
    }

    msg = cdio_cddap_errors(paranoiaCtlObj);
    if (msg) {
        cdio_error("paranoia returned the following errors during %s:\n%s", when, msg);
    }
}


void cdio2_paranoia_callback(long int frame, paranoia_cb_mode_t prc)
{
    cdio_log_level_t level;

    switch (prc) {

        case PARANOIA_CB_READ:
        case PARANOIA_CB_VERIFY:
        case PARANOIA_CB_OVERLAP:
            level = CDIO_LOG_DEBUG;
            break;

        case PARANOIA_CB_FIXUP_ATOM:
        case PARANOIA_CB_FIXUP_EDGE:
            level = CDIO_LOG_INFO;
            break;
    
        default:
            level = CDIO_LOG_ERROR;
            break;
    }

    cdio_log(level, "paranoia reports \"%s\" at sector \"%d\" (frame=\"%ld\")",  paranoia_cb_mode2str[prc], (int) (frame / CD_FRAMEWORDS), frame);
}

#endif // CUED_HAVE_PARANOIA


int cdio2_get_track_channels(CdIo_t *cdObj, track_t track)
{
    int channels = cdio_get_track_channels(cdObj, track);

    switch (channels) {

        case 2:
        case 4:
            break;

        case -1:
            cdio_warn("failed to get number of channels for track %02d; assuming 2 channels", track);
            channels = 2;
            break;

        default:
            cdio_log(channels ? CDIO_LOG_WARN : CDIO_LOG_INFO,
                "overriding number of channels for track %02d from %d to 2", track, channels);
            channels = 2;
            break;
    }

    return channels;
}


lsn_t cdio2_get_track_length(CdIo_t *cdObj, track_t track)
{
    lsn_t firstSector, lastSector;

    firstSector = cdio_get_track_lsn(cdObj, track);
    if (CDIO_INVALID_LSN == firstSector) {
        cdio2_abort("failed to get first sector number for track %02d", track);

        return CDIO_INVALID_LSN;
    }

    // TODO:  problem here
    lastSector = cdio_get_track_last_lsn(cdObj, track);
    if (CDIO_INVALID_LSN == lastSector) {
        cdio2_abort("failed to get last sector number for track %02d", track);

        return CDIO_INVALID_LSN;
    }

    return lastSector - firstSector + 1;
}


void cdio2_get_length(char *length, lsn_t sectors)
{
    int minutes, seconds;

    minutes = sectors / QSC_FPM;
    sectors %= QSC_FPM;
    seconds = sectors / QSC_FPS + (sectors % QSC_FPS > QSC_FPS / 2);
    if (60 == seconds) {
        seconds  = 0;
        minutes += 1;
    }

    if (minutes > 9) {
        cdio2_set_2_digits(&length[0], minutes);
        length[2] = ':';
        cdio2_set_2_digits_nt(&length[3], seconds);
    } else {
        length[0] = QSC_BCD_TO_ASCII(minutes);
        length[1] = ':';
        cdio2_set_2_digits_nt(&length[2], seconds);
    }
}


/* Maximum blocks to retrieve. Would be nice to customize this based on
   drive capabilities.
*/
#define MAX_CD_READ_BLOCKS 16
#define CD_READ_TIMEOUT_MS mmc_timeout_ms * (MAX_CD_READ_BLOCKS/2)


/*! issue an MMC READ CD MSF command.
*/
driver_return_code_t
mmc_read_cd_msf ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn,
                  int read_sector_type, bool b_digital_audio_play,
                  bool b_sync, uint8_t header_codes, bool b_user_data,
                  bool b_edc_ecc, uint8_t c2_error_information,
                  uint8_t subchannel_selection, uint16_t i_blocksize,
                  uint32_t i_blocks )
{
  mmc_cdb_t cdb = {{0, }};
  uint8_t cdb9 = 0;

  CDIO_MMC_SET_COMMAND  (cdb.field, CDIO_MMC_GPCMD_READ_MSF);
  CDIO_MMC_SET_READ_TYPE(cdb.field, read_sector_type);
  if (b_digital_audio_play) { cdb.field[1] |= 0x2; }
  
  if (b_sync)      { cdb9 |= 128; }
  if (b_user_data) { cdb9 |=  16; }
  if (b_edc_ecc)   { cdb9 |=   8; }
  cdb9 |= (header_codes & 3)         << 5;
  cdb9 |= (c2_error_information & 3) << 1;
  cdb.field[9]  = cdb9;

  cdb.field[10] = (subchannel_selection & 7);
  
  {
    unsigned int j = 0;
    driver_return_code_t i_ret = DRIVER_OP_SUCCESS;
    const uint8_t i_cdb = mmc_get_cmd_len(cdb.field[0]);
    msf_t start_msf, end_msf;

    cdio_lsn_to_msf(i_lsn, &end_msf);
        
    while (i_blocks > 0) {
      const unsigned i_blocks2 = (i_blocks > MAX_CD_READ_BLOCKS) 
        ? MAX_CD_READ_BLOCKS : i_blocks;
      void *p_buf2 = ((char *)p_buf ) + (j * i_blocksize);

      start_msf = end_msf;
      cdio_lsn_to_msf(i_lsn + j + i_blocks2, &end_msf);
      
      cdb.field[3] = cdio_from_bcd8(start_msf.m);
      cdb.field[4] = cdio_from_bcd8(start_msf.s);
      cdb.field[5] = cdio_from_bcd8(start_msf.f);
      cdb.field[6] = cdio_from_bcd8(  end_msf.m);
      cdb.field[7] = cdio_from_bcd8(  end_msf.s);
      cdb.field[8] = cdio_from_bcd8(  end_msf.f);

      i_ret = mmc_run_cmd_len (p_cdio, CD_READ_TIMEOUT_MS,
                               &cdb, i_cdb,
                               SCSI_MMC_DATA_READ, 
                               i_blocksize * i_blocks2,
                               p_buf2);

      if (i_ret) { return i_ret; }

      i_blocks -= i_blocks2;
      j += i_blocks2;
    }

    return i_ret;
  }
}
