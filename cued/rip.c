//
// rip.c
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

#include "unix.h"
#include "cued.h"
#include "util.h"

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdio.h>
#include <cdio/mmc.h> // CDIO_MMC_READ_TYPE_ANY
#include "cdio2.h"
#include "rip.h"
#include "format.h"

#include <stdlib.h> // free
#include <unistd.h> // unlink
#include <sndfile.h>




#include <cdio/util.h> // cdio_from_bcd8

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
    lba_t i_lba = QSC_LSN_TO_LBA(i_lsn);
    const uint8_t i_cdb = mmc_get_cmd_len(cdb.field[0]);
    msf_t start_msf, end_msf;

    if (qsc_lba_to_msf(i_lba, &end_msf)) {
      return DRIVER_OP_BAD_PARAMETER;
    }
        
    while (i_blocks > 0) {
      const unsigned i_blocks2 = (i_blocks > MAX_CD_READ_BLOCKS) 
        ? MAX_CD_READ_BLOCKS : i_blocks;
      void *p_buf2 = ((char *)p_buf ) + (j * i_blocksize);

      start_msf = end_msf;
      if (qsc_lba_to_msf(i_lba + j + i_blocks2, &end_msf)) {
        return DRIVER_OP_BAD_PARAMETER;
      }
      
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


void cued_init_rip_data(rip_context_t *rip)
{
    memset(rip->ripData, 0x00, sizeof(rip->ripData));
    memset(rip->mcn,     0x00, sizeof(rip->mcn));
    rip->year = 0;

    rip->trackHint  = 0;
    rip->crcFailure = 0;
    rip->crcSuccess = 0;
}


static void cued_parse_qsc(qsc_buffer_t *qsc, rip_context_t *rip)
{
    int flags;
    qsc_index_t index;
    lba_t *currLba;
    char *isrc;

    if (qsc_check_crc(qsc)) {
        ++rip->crcFailure;
        return;
    }

    ++rip->crcSuccess;

    switch (qsc_get_mode(qsc)) {

        case QSC_MODE_INDEX:
            if (!qsc_get_index(qsc, &index)) {

                // set this for ISRC case
                rip->trackHint = index.track;

                currLba = &rip->ripData[index.track].indices[index.index];
                if (!*currLba || index.absoluteLba < *currLba) {
                    *currLba = index.absoluteLba;

                    // do not do this for every record;  hence, inside the if statement
                    flags = 0;
                    SETF(RIP_F_DATA_VALID, flags);
                    if (qsc_has_pre_emphasis(qsc)) {
                        SETF(RIP_F_DATA_PRE_EMPHASIS, flags);
                    }
                    if (qsc_has_copy_permitted(qsc)) {
                        SETF(RIP_F_DATA_COPY_PERMITTED, flags);
                    }
                    if (qsc_has_four_channels(qsc)) {
                        SETF(RIP_F_DATA_FOUR_CHANNELS, flags);
                    }
                    rip->ripData[index.track].flags = flags;
                }
            } else {
                cdio_warn("invalid index found in q sub-channel");
            }
            break;

        case QSC_MODE_MCN:
            if (!rip->mcn[0]) {
                if (qsc_get_mcn(qsc, rip->mcn)) {
                    cdio_warn("invalid mcn found in q sub-channel");
                    rip->mcn[0] = 0;
                }
            }
            break;

        case QSC_MODE_ISRC:
            isrc = rip->ripData[rip->trackHint].isrc;
            if (!isrc[0]) {
                if (qsc_get_isrc(qsc, isrc)) {
                    cdio_warn("invalid isrc found in q sub-channel");
                    isrc[0] = 0;
                } else if (!rip->year) {
                    rip->year = qsc_get_isrc_year(isrc);
                    cdio_info("set rip year to %d\n", rip->year);
                }
            }
            break;

        default:
            break;
    }
}


typedef struct _audio_buffer_t {

    int16_t buf[CD_FRAMEWORDS];

} audio_buffer_t;


typedef driver_return_code_t
(*mmc_read_cd_fn)(const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn,
                  int read_sector_type, bool b_digital_audio_play,
                  bool b_sync, uint8_t header_codes, bool b_user_data,
                  bool b_edc_ecc, uint8_t c2_error_information,
                  uint8_t subchannel_selection, uint16_t i_blocksize,
                  uint32_t i_blocks);


driver_return_code_t
mmc_read_cd_leadout ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn,
                  int read_sector_type, bool b_digital_audio_play,
                  bool b_sync, uint8_t header_codes, bool b_user_data,
                  bool b_edc_ecc, uint8_t c2_error_information,
                  uint8_t subchannel_selection, uint16_t i_blocksize,
                  uint32_t i_blocks )
{
    rip_context_t *rip;
    uint8_t *buf;
    driver_return_code_t drc;
    int sectors;

    cdio_warn("reading lead-out");

    rip = (rip_context_t *) util_get_context(p_cdio);
    if (!rip) {
        cdio2_abort("failed to get rip context for reading lead-out");
    }

    // (ab)use the drive's firmware by requesting a read that starts inside
    // the program area, but extends into the lead out (some drives do not
    // check to see if a read terminates inside the program area)
    // 

    sectors = i_lsn - rip->endOfDiscSector + i_blocks + 1;
    buf = (uint8_t *) malloc(sectors * i_blocksize);
    if (!buf) {
        cdio2_abort("out of memory reading %d sectors", sectors);
    }

    drc = mmc_read_cd(
        p_cdio,
        buf, rip->endOfDiscSector - 1,
        read_sector_type, b_digital_audio_play, b_sync, header_codes, b_user_data,
        b_edc_ecc, c2_error_information, subchannel_selection, i_blocksize,
        sectors);
    if (DRIVER_OP_SUCCESS == drc) {
        memcpy(p_buf, buf + i_blocksize * (sectors - i_blocks), i_blocksize * i_blocks);
    }

    free(buf);

    return drc;
}


static driver_return_code_t cued_read_audio(rip_context_t *rip, lsn_t firstSector, long sectors, audio_buffer_t *pbuf, int retry)
{
    mmc_read_cd_fn readfn;
    uint8_t *mbuf, *dbuf;
    int blockSize, subchannel, i;
    driver_return_code_t drc;
    qsc_file_buffer_t qsc;

    blockSize = sizeof(audio_buffer_t);

    if (ripGetIndices || rip->qSubChannelFileName) {
        if (ripUseFormattedQsc) {
            subchannel = 2;
            blockSize += sizeof(qsc_buffer_t);
        } else {
            blockSize += sizeof(mmc_raw_pwsc_t);
            if (ripUseEccQsc) {
                subchannel = 4;
            } else {
                subchannel = 1;
            }
        }
    } else {
        subchannel = 0;
    }

    // TODO:  should check for ripRead* be both here and in cued_rip_to_file?
    // does paranoia present a problem?
    //

    if (ripReadPregap && firstSector < 0) {
        readfn = mmc_read_cd_msf;
    } else if (ripReadLeadout && firstSector + sectors > rip->endOfDiscSector) {
        if (firstSector >= rip->endOfDiscSector) {
            readfn = mmc_read_cd_leadout;
        } else {
            readfn = mmc_read_cd;
            cdio_warn("reading lead-out");
        }
    } else if (subchannel || ripDapFixup) {
        readfn = mmc_read_cd;
    } else {
        readfn = NULL;
    }

    if (sectors > rip->allocatedSectors) {
        free(rip->mmcBuf);
        rip->mmcBuf = (uint8_t *) malloc(sectors * blockSize);
        if (rip->mmcBuf) {
            rip->allocatedSectors = sectors;
        } else {
            rip->allocatedSectors = 0;
            cdio2_abort("out of memory reading %ld sectors", sectors);
        }
    }

    do {

        if (readfn) {

            drc = readfn(
                rip->cdObj,
                (pbuf && !subchannel) ? (void *) pbuf : rip->mmcBuf,
                firstSector,

                // expected read_sector_type;  could be CDIO_MMC_READ_TYPE_CDDA
                CDIO_MMC_READ_TYPE_ANY,

                // DAP (Digital Audio Play)
                ripDapFixup ? true : false,

                // return SYNC header
                false,

                // header codes
                0,

                // must return user data according to mmc5 spec
                true,

                // no EDC or ECC is included with audio data
                false,

                // C2 error information is synthetic;  it is not needed to get Q sub-channel,
                // even though it is an adjacent field according to the standard
                //
                false,

                subchannel,
                blockSize,
                sectors);

            if (DRIVER_OP_SUCCESS == drc) {

                if (subchannel) {

                    mbuf = dbuf = rip->mmcBuf;
                    for (i = 0;  i < sectors;  ++i) {

                        if (pbuf) {
                            memcpy(pbuf[i].buf, mbuf, sizeof(audio_buffer_t));
                        } else {
                            //dbuf = rip->mmcBuf + i * sizeof(audio_buffer_t);
                            if (dbuf != mbuf) {
                                memmove(dbuf, mbuf, sizeof(audio_buffer_t));
                            }
                            dbuf += sizeof(audio_buffer_t);
                        }
                        mbuf += sizeof(audio_buffer_t);

                        // if (subchannel) {

                        if (ripUseFormattedQsc) {
                            memcpy(&qsc.buf, mbuf, sizeof(qsc_buffer_t));
                            mbuf += sizeof(qsc_buffer_t);
                        } else {
                            pwsc_get_qsc((mmc_raw_pwsc_t *) mbuf, &qsc.buf);
                            mbuf += sizeof(mmc_raw_pwsc_t);
                        }

                        if (ripGetIndices) {
                            cued_parse_qsc(&qsc.buf, rip);
                        }

                        if (!ripUseParanoia && rip->qSubChannelFileName) {
                            qsc.requested = firstSector + i;
                            if (1 != fwrite(&qsc, sizeof(qsc), 1, rip->qSubChannelFile)) {
                                // probably out of disk space, which is bad, because most things rely on it
                                cdio2_unix_error("fwrite", rip->qSubChannelFileName, 0);
                                cdio2_abort("failed to write to file \"%s\"", rip->qSubChannelFileName);
                            }
                        }
                    }
                }

                return drc;
            }

        } else {

            drc = cdio_read_audio_sectors(rip->cdObj, pbuf ? (void *) pbuf : rip->mmcBuf, firstSector, sectors);
            if (DRIVER_OP_SUCCESS == drc) {
                return drc;
            }
        }

        cdio2_driver_error(drc, "read of audio sector");

    } while (retry--);

    cdio_error("skipping extraction of audio sectors %d through %ld in track %02d", firstSector, firstSector + sectors - 1, rip->currentTrack);

    return drc;
}


static long cued_read_paranoid(cdrom_drive_t *paranoiaCtlObj, void *pb, lsn_t firstSector, long sectors)
{
    rip_context_t *rip;
    long rc;
    driver_return_code_t drc;

    rip = (rip_context_t *) util_get_context(paranoiaCtlObj->p_cdio);
    if (!rip) {
        cdio2_abort("failed to get rip context in paranoid read");
    }

    drc = cued_read_audio(rip, firstSector, sectors, (audio_buffer_t *) pb, 0);
    if (DRIVER_OP_SUCCESS == drc) {
        rc = sectors;
    } else {
        rc = drc;
    }

    return rc;
}


static const char *cued_fmt_to_ext(int soundFileFormat)
{
    switch (soundFileFormat) {

        case SF_FORMAT_WAV:
            return ".wav";
            break;

        case SF_FORMAT_FLAC:
            return ".flac";
            break;

        default:
            cdio2_abort("internal error of unknown sound file format 0x%X specified", soundFileFormat);
            break;
    }

    return ".unknown";
}


void cued_rip_to_file(rip_context_t *rip)
{
    SF_INFO sfinfo;
    PIT(SNDFILE, sfObj);
    PIT(int16_t, pbuf);
    lsn_t currSector, prc, offsetSectors;
    int wordsToWrite, wordsWritten, i;

    int offsetWords = rip->offsetWords;
    track_t track = rip->currentTrack;

    memset(&sfinfo, 0x00, sizeof(sfinfo));
    sfinfo.samplerate = 44100;
    sfinfo.channels = rip->channels;
    sfinfo.format = SF_FORMAT_PCM_16 | rip->soundFileFormat;

    offsetSectors = offsetWords / CD_FRAMEWORDS;
    rip->firstSector += offsetSectors;
    rip->lastSector  += offsetSectors;
    offsetWords %= CD_FRAMEWORDS;

    if (offsetWords < 0) {
        rip->firstSector -= 1;
    } else if (offsetWords > 0) {
        rip->lastSector  += 1;
    }
    // else offsetWords is zero b/c the offset fell on a sector boundary

    currSector = rip->firstSector;

    if (ripUseParanoia) {
        lsn_t seekSector;

        if (currSector < 0 && !ripReadPregap) {
            seekSector = 0;
        } else {
            seekSector = currSector;
        }

        // TODO:  paranoia has a problem with reading leadout
        if (seekSector < rip->endOfDiscSector) {
            prc = cdio_paranoia_seek(rip->paranoiaRipObj, seekSector, SEEK_SET);
            cdio2_paranoia_msg(rip->paranoiaCtlObj, "paranoia seek");
            if (-1 == prc) {
                cdio_error("paranoia returned \"%d\" during seek to \"%d\"; skipping track %02d", prc, seekSector, track);

                return;
            }
        }
    }

    if (ripExtract) {

        // does not return on error
        (void) format_get_file_path(rip->cdObj, rip->cddbObj,
            rip->fileNamePattern, cued_fmt_to_ext(rip->soundFileFormat), track,
            rip->fileNameBuffer, rip->bufferSize
            );

        sfObj = sf_open(rip->fileNameBuffer, SFM_WRITE, &sfinfo);
        if (!sfObj) {
            cdio_error("sf_open(\"%s\") returned \"%s\"; skipping extraction of track %02d", rip->fileNameBuffer, sf_strerror(sfObj), track);

            return;
        }
    }

    for (;  currSector <= rip->lastSector;  ++currSector) {

        if ((currSector < 0 && !ripReadPregap) || (currSector >= rip->endOfDiscSector && !ripReadLeadout)) {

            // N.B.  assume that if mmcBuf is not NULL, it is >= sizeof(audio_buffer_t)
            if (!rip->mmcBuf) {

                // use twice the audio_buffer_t size for reading 1 sector;
                // this should accomodate any extra headers/sub-channel data
                // requested later in the normal read path
                //
                rip->mmcBuf = (uint8_t *) malloc(2 * sizeof(audio_buffer_t));
                if (rip->mmcBuf) {
                    rip->allocatedSectors = 1;
                } else {
                    cdio2_abort("out of memory allocating overread sector");
                }
            }

            memset(rip->mmcBuf, 0x00, sizeof(audio_buffer_t));
            pbuf = rip->mmcBuf16;

        } else {

            // TODO:  need to update track indices on skip of sector (continue)

            if (ripUseParanoia) {
                pbuf = cdio_paranoia_read_limited(rip->paranoiaRipObj, cdio2_paranoia_callback, rip->retries);
                cdio2_paranoia_msg(rip->paranoiaCtlObj, "read of audio sector");
                if (!pbuf) {
                    cdio_error("paranoia did not return data; skipping extraction of audio sector %d in track %02d", currSector, track);
                    continue;
                }
            } else {
                if (DRIVER_OP_SUCCESS == cued_read_audio(rip, currSector, 1, NULL, rip->retries)) {
                    pbuf = rip->mmcBuf16;
                } else {
                    continue;
                }
            }
        }

        wordsToWrite = CD_FRAMEWORDS;

        // N.B. firstSector == lastSector is not possible if offsetWords is non-zero
        //
        if (rip->firstSector == currSector) {
            if (offsetWords < 0) {
                pbuf += CD_FRAMEWORDS + offsetWords;
                wordsToWrite  = -offsetWords;
            } else if (offsetWords > 0) {
                pbuf += offsetWords;
                wordsToWrite -= offsetWords;
            }
        } else if (rip->lastSector == currSector) {
            if (offsetWords < 0) {
                wordsToWrite += offsetWords;
            } else if (offsetWords > 0) {
                wordsToWrite  = offsetWords;    
            }
        }

        if (ripExtract) {
            wordsWritten = sf_write_short(sfObj, pbuf, wordsToWrite);
            if (wordsWritten != wordsToWrite) {

                // probably out of disk space, which is bad, because most things rely on it
                cdio2_abort("failed to write to file \"%s\" due to \"%s\"", rip->fileNameBuffer, sf_strerror(sfObj));
            }
        }

        if (!track && !ripNoisyPregap) {
            for (i = 0;  i < wordsToWrite;  ++i) {
                if (pbuf[i]) {
                    SETF(RIP_F_NOISY_PREGAP, rip->flags);
                    break;
                }
            }
        }
    }

    if (!track && !ripNoisyPregap) {
        SETF(RIP_F_SILENT_PREGAP, rip->flags);
    }

    if (ripExtract) {
        sf_close(sfObj);
    }
}


static void cued_rip_prologue(rip_context_t *rip)
{
    rip->mmcBuf = NULL;
    rip->allocatedSectors = 0;

    if (ripUseParanoia) {
        char *msg = 0;
        int rc;

        // N.B. this behavior does not match documentation:
        // the 0 here appears to prevent the message "Checking <filename> for cdrom..."
        rip->paranoiaCtlObj = cdio_cddap_identify_cdio(rip->cdObj, 0, &msg);
        if (rip->paranoiaCtlObj) {

            if (msg) {
                cdio_warn("identify returned paranoia message(s) \"%s\"", msg);
            }
            cdio_cddap_verbose_set(rip->paranoiaCtlObj, CDDA_MESSAGE_LOGIT, CDDA_MESSAGE_LOGIT);

            rc = cdio_cddap_open(rip->paranoiaCtlObj);
            cdio2_paranoia_msg(rip->paranoiaCtlObj, "open of device");
            if (!rc) {
                rip->paranoiaRipObj = cdio_paranoia_init(rip->paranoiaCtlObj);
                cdio2_paranoia_msg(rip->paranoiaCtlObj, "initialization of paranoia");
                if (!rip->paranoiaRipObj) {
                    cdio2_abort("out of memory initializing paranoia");
                }

                cdio_paranoia_modeset(rip->paranoiaRipObj, PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP);
                // N.B. not needed at the moment
                cdio2_paranoia_msg(rip->paranoiaCtlObj, "setting of paranoia mode");

                rip->save_read_paranoid = rip->paranoiaCtlObj->read_audio;
                rip->paranoiaCtlObj->read_audio = cued_read_paranoid;
            } else {
                cdio_cddap_close_no_free_cdio(rip->paranoiaCtlObj);

                cdio_error("disabling paranoia");
                CLRF(RIP_F_USE_PARANOIA, rip->flags);
            }
        } else {
            cdio_error("disabling paranoia due to the following message(s):\n%s", msg);
            CLRF(RIP_F_USE_PARANOIA, rip->flags);
        }
    }

    if (rip->qSubChannelFileName) {
        if (!strcmp("-", rip->qSubChannelFileName)) {
            rip->qSubChannelFile = stdout;
        } else {
            (void) format_get_file_path(rip->cdObj, rip->cddbObj, rip->qSubChannelFileName, "", 0, rip->fileNameBuffer, rip->bufferSize);

            // replaced O_EXCL with O_TRUNC to allow using /dev/null for testing
            rip->qSubChannelFile = fopen2(rip->fileNameBuffer, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
            if (!rip->qSubChannelFile) {
                cdio2_unix_error("fopen2", rip->fileNameBuffer, 0);
                cdio_error("not creating sub-channel file \"%s\"", rip->fileNameBuffer);

                rip->qSubChannelFileName = 0;
            }
        }
    }

    rip->endOfDiscSector = cdio_get_disc_last_lsn(rip->cdObj);
    if (CDIO_INVALID_LSN == rip->endOfDiscSector) {
        cdio2_abort("failed to get last sector number");
    } else {
        //cdio_debug("end of disc sector is %d", rip->endOfDiscSector);
    }
}


static void cued_rip_epilogue(rip_context_t *rip)
{
    free(rip->mmcBuf);

    if (ripUseParanoia) {
        rip->paranoiaCtlObj->read_audio = rip->save_read_paranoid;
        cdio_paranoia_free(rip->paranoiaRipObj);
        cdio_cddap_close_no_free_cdio(rip->paranoiaCtlObj);
    }

    if (rip->qSubChannelFileName && rip->qSubChannelFile != stdout) {
        fclose(rip->qSubChannelFile);
    }

    if (rip->crcFailure || rip->crcSuccess) {
        int totalCrcs = rip->crcSuccess + rip->crcFailure;

        if (rip->crcFailure * 100 / totalCrcs > 5) {
            cdio_warn("greater than 5 percent of Q sub-channel records failed CRC check (try --qsc-fq?)");
        }
        if (ripVerbose) {
            printf("progress: correctly read %d of %d Q sub-channel records\n", rip->crcSuccess, totalCrcs);
        }
    }
}


void cued_rip_data_track(rip_context_t *rip)
{
    uint8_t buf[CDIO_CD_FRAMESIZE];
    FILE *dataFile;
    lsn_t currSector;
    driver_return_code_t drc;

    if (ripExtract) {

        // does not return on error
        (void) format_get_file_path(rip->cdObj, rip->cddbObj,
            rip->fileNamePattern, ".data", rip->currentTrack,
            rip->fileNameBuffer, rip->bufferSize
            );

        dataFile = fopen2(rip->fileNameBuffer, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0666);
        if (!dataFile) {
            cdio2_unix_error("fopen2", rip->fileNameBuffer, 0);
            cdio_error("skipping extraction of data track %02d", rip->currentTrack);
            return;
        }

        for (currSector = rip->firstSector;  currSector <= rip->lastSector;  ++currSector) {
            drc = cdio_read_data_sectors(rip->cdObj, buf, currSector, CDIO_CD_FRAMESIZE, 1);
            if (DRIVER_OP_SUCCESS == drc) {
                if (1 != fwrite(buf, sizeof(buf), 1, dataFile)) {
                    // probably out of disk space, which is bad, because most things rely on it
                    cdio2_unix_error("fwrite", rip->fileNameBuffer, 0);
                    cdio2_abort("failed to write to file \"%s\"", rip->fileNameBuffer);
                }
            } else {
                cdio2_driver_error(drc, "read of data sector");
                cdio_error("error reading sector %d; skipping extraction of data track %02d", currSector, rip->currentTrack);
                //if (unlink(rip->fileNameBuffer)) {
                //    cdio2_unix_error("unlink", rip->fileNameBuffer, 1);
                //}
                break;
            }
        }

        fclose(dataFile);
    }
}


typedef void (*rip_fn_t)(rip_context_t *rip);


void cued_rip_disc(rip_context_t *rip)
{
    cued_rip_prologue(rip);

    if (ripToOneFile) {

        // TODO:  broken for cd-extra;  stop at first data track?  in cued.c?

        if (TRACK_FORMAT_AUDIO != cdio_get_track_format(rip->cdObj, rip->firstTrack)) {
            cdio2_abort("track %02d is not an audio track", rip->firstTrack);
        }

        if (rip->firstTrack > 1) {
            rip->firstSector = cdio_get_track_lsn(rip->cdObj, rip->firstTrack);
            if (CDIO_INVALID_LSN == rip->firstSector) {
                cdio2_abort("failed to get first sector number for track %02d", rip->firstTrack);
            }
        } else {
            rip->firstSector = 0;
        }

        rip->lastSector = cdio_get_track_last_lsn(rip->cdObj, rip->lastTrack); // , rip->discLastTrack);
        if (CDIO_INVALID_LSN == rip->lastSector) {
            cdio2_abort("failed to get last sector number for track %02d", rip->lastTrack);
        }

        rip->channels = cdio2_get_track_channels(rip->cdObj, rip->firstTrack);

        if (ripVerbose) {
            printf("progress: reading sectors from %d to %d\n", rip->firstSector, rip->lastSector);
        }

        rip->currentTrack = 0;
        cued_rip_to_file(rip);

    } else {

        rip_fn_t ripfn;
        track_format_t format;
        track_t track;

        for (track = rip->firstTrack;  track <= rip->lastTrack;  ++track) {

            rip->firstSector = cdio_get_track_lsn(rip->cdObj, track);
            if (CDIO_INVALID_LSN == rip->firstSector) {
                cdio2_abort("failed to get first sector number for track %02d", track);
            }

            format = cdio_get_track_format(rip->cdObj, track);
            switch (format) {

                case TRACK_FORMAT_AUDIO:
                    rip->channels = cdio2_get_track_channels(rip->cdObj, track);

                    // rip first track pregap to track 00 file
                    if (1 == track && rip->firstSector > 0) {

                        lsn_t saveFirstSector = rip->firstSector;

                        if (ripVerbose) {
                            printf("progress: reading track %02d\n", 0);
                        }

                        rip->lastSector = rip->firstSector - 1;
                        rip->firstSector = 0;
                        rip->currentTrack = 0;
                        cued_rip_to_file(rip);

                        if (ripSilentPregap && ripExtract) {
                            if (unlink(rip->fileNameBuffer)) {
                                cdio2_unix_error("unlink", rip->fileNameBuffer, 1);
                            }
                        }

                        rip->firstSector = saveFirstSector;
                    }
                    ripfn = cued_rip_to_file;
                    break;

                case TRACK_FORMAT_DATA:
                    //ripfn = cued_rip_data_track;
                    //break;

                default:
                    cdio_warn("track %02d is not an audio track; skipping track", track);
                    continue;
            }

            rip->lastSector = cdio_get_track_last_lsn(rip->cdObj, track); // , rip->discLastTrack);
            if (CDIO_INVALID_LSN == rip->lastSector) {
                cdio2_abort("failed to get last sector number for track %02d", track);
            } else {
                //cdio_debug("track %02d last sector is %d", track, rip->lastSector);
            }

            if (ripVerbose) {
                printf("progress: reading track %02d\n", track);
            }

            rip->currentTrack = track;
            ripfn(rip);
        }
    }

    cued_rip_epilogue(rip);
}
