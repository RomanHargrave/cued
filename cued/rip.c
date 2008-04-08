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

#include "macros.h"
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


void cued_init_rip_data(rip_context_t *rip)
{
    memset(rip->indices, 0x00, sizeof(rip->indices));
    memset(rip->mcn,     0x00, sizeof(rip->mcn));
    memset(rip->isrc,    0x00, sizeof(rip->isrc));
    rip->noisy_pregap = 0;
    rip->silent_pregap = 0;
    rip->year = 0;

    rip->trackHint = 0;
    rip->crcFailure = 0;
    rip->crcSuccess = 0;
}


static void cued_parse_qsc(qsc_buffer_t *qsc, rip_context_t *rip)
{
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

                currLba = &rip->indices[ index.track ][ index.index ];
                if (!*currLba || index.absoluteLba < *currLba) {
                    *currLba = index.absoluteLba;
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
            isrc = rip->isrc[rip->trackHint];
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


static int16_t *cued_read_audio(rip_context_t *rip, lsn_t currSector)
{
    driver_return_code_t rc;
    qsc_file_buffer_t qsc;

    if (rip->qSubChannelFileName || rip->getIndices) {

        rc = mmc_read_cd(
            rip->cdObj,
            &rip->audioBuf,
            currSector,

            // expected read_sector_type;  could be CDIO_MMC_READ_TYPE_CDDA
            CDIO_MMC_READ_TYPE_ANY,

            // DAP (Digital Audio Play);  if this is true, immediate error with msi, hp, microadvantage
            false,

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

            // select sub-channel
            (rip->useFormattedQsc ? 2 : 1),
            (rip->useFormattedQsc ? sizeof(rip->audioBuf.fmtQsc) : sizeof(rip->audioBuf.rawPWsc)) + sizeof(rip->audioBuf.buf),

            // number of sectors
            1);

        if (DRIVER_OP_SUCCESS == rc) {

            if (rip->useFormattedQsc) {
                qsc.buf = rip->audioBuf.fmtQsc;
            } else {
                pwsc_get_qsc(&rip->audioBuf.rawPWsc, &qsc.buf);
            }

            cued_parse_qsc(&qsc.buf, rip);

            if (rip->qSubChannelFileName) {
                qsc.requested = currSector;
                if (1 != fwrite(&qsc, sizeof(qsc), 1, rip->qSubChannelFile)) {
                    // probably out of disk space, which is bad, because most things rely on it
                    cdio2_unix_error("fwrite", rip->qSubChannelFileName, 0);
                    cdio2_abort("failed to write to file \"%s\"", rip->qSubChannelFileName);
                }
            }
        }

    } else {
        rc = cdio_read_audio_sector(rip->cdObj, rip->audioBuf.buf, currSector);
    }

    if (DRIVER_OP_SUCCESS != rc) {
        cdio2_driver_error(rc, "read of audio sector");
        cdio_error("skipping extraction of audio sector %d in track %02d", currSector, rip->currentTrack);

        return NULL;
    } else {

        return rip->audioBuf.buf;
    }
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

    if (rip->useParanoia) {
        lsn_t seekSector;

        if (currSector < 0) {
            seekSector = 0;
        } else {
            seekSector = currSector;
        }

        if (seekSector < rip->endOfDiscSector) {
            prc = cdio_paranoia_seek(rip->paranoiaRipObj, seekSector, SEEK_SET);
            cdio2_paranoia_msg(rip->paranoiaCtlObj, "paranoia seek");
            if (-1 == prc) {
                cdio_error("paranoia returned \"%d\" during seek to \"%d\"; skipping track %02d", prc, seekSector, track);

                return;
            }
        }
    }

    sfObj = sf_open(rip->fileNameBuffer, SFM_WRITE, &sfinfo);
    if (!sfObj) {
        cdio_error("sf_open(\"%s\") returned \"%s\"; skipping extraction of track %02d", rip->fileNameBuffer, sf_strerror(sfObj), track);

        return;
    }

    for (;  currSector <= rip->lastSector;  ++currSector) {

        if (currSector < 0 || currSector >= rip->endOfDiscSector) {

            memset(rip->audioBuf.buf, 0x00, sizeof(rip->audioBuf.buf));
            pbuf = rip->audioBuf.buf;

        } else {

            // TODO:  need to update track indices on skip of sector (continue)

            if (rip->useParanoia) {
                pbuf = cdio_paranoia_read_limited(rip->paranoiaRipObj, cdio2_paranoia_callback, rip->retries);
                cdio2_paranoia_msg(rip->paranoiaCtlObj, "read of audio sector");
                if (!pbuf) {
                    cdio_error("paranoia did not return data; skipping extraction of audio sector %d in track %02d", currSector, track);
                    continue;
                }
            } else {
                pbuf = cued_read_audio(rip, currSector);
                if (!pbuf) {
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

        wordsWritten = sf_write_short(sfObj, pbuf, wordsToWrite);
        if (wordsWritten != wordsToWrite) {

            // probably out of disk space, which is bad, because most things rely on it
            cdio2_abort("failed to write to file \"%s\" due to \"%s\"", rip->fileNameBuffer, sf_strerror(sfObj));
        }

        if (!track && !rip->noisy_pregap) {
            for (i = 0;  i < wordsToWrite;  ++i) {
                if (pbuf[i]) {
                    rip->noisy_pregap = 1;
                    break;
                }
            }
        }
    }

    if (!track && !rip->noisy_pregap) {
        rip->silent_pregap = 1;
    }

    sf_close(sfObj);
}


typedef struct _paranoia_audio_buffer_t {

    int16_t buf[CD_FRAMEWORDS];

} paranoia_audio_buffer_t;


long cued_read_paranoid(cdrom_drive_t *paranoiaCtlObj, void *pb, lsn_t firstSector, long sectors)
{
    paranoia_audio_buffer_t *pbuf = (paranoia_audio_buffer_t *) pb;
    rip_context_t *rip;
    uint8_t *mbuf;
    long rc;
    driver_return_code_t drc;
    int i;
    qsc_buffer_t qsc;

    rip = (rip_context_t *) util_get_context(paranoiaCtlObj->p_cdio);
    if (!rip) {
        cdio2_abort("failed to get rip context in paranoid read");
    }

    if (sectors > rip->allocatedSectors) {
        free(rip->mmcBuf);
        rip->mmcBuf = (uint8_t *) malloc(sectors * (sizeof(paranoia_audio_buffer_t)
                    + (rip->useFormattedQsc ? sizeof(qsc_buffer_t) : sizeof(mmc_raw_pwsc_t))));
        if (rip->mmcBuf) {
            rip->allocatedSectors = sectors;
        } else {
            rip->allocatedSectors = 0;
            cdio2_abort("out of memory reading %ld sectors", sectors);
        }
    }

    drc = mmc_read_cd(
        paranoiaCtlObj->p_cdio,
        rip->mmcBuf,
        firstSector,

        // expected read_sector_type;  could be CDIO_MMC_READ_TYPE_CDDA
        CDIO_MMC_READ_TYPE_ANY,

        // DAP (Digital Audio Play);  if this is true, immediate error with msi, hp, microadvantage
        false,

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

        // select sub-channel
        (rip->useFormattedQsc ? 2 : 1),
        (rip->useFormattedQsc ? sizeof(qsc_buffer_t) : sizeof(mmc_raw_pwsc_t)) + sizeof(paranoia_audio_buffer_t),
        sectors);

    if (DRIVER_OP_SUCCESS == drc) {
        mbuf = rip->mmcBuf;
        for (i = 0;  i < sectors;  ++i) {

            memcpy(pbuf[i].buf, mbuf, sizeof(paranoia_audio_buffer_t));
            mbuf += sizeof(paranoia_audio_buffer_t);

            if (rip->useFormattedQsc) {
                cued_parse_qsc((qsc_buffer_t *) mbuf, rip);
                mbuf += sizeof(qsc_buffer_t);
            } else {
                pwsc_get_qsc((mmc_raw_pwsc_t *) mbuf, &qsc);
                cued_parse_qsc(&qsc, rip);
                mbuf += sizeof(mmc_raw_pwsc_t);
            }

        }
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


void cued_rip_disc(rip_context_t *rip)
{
    if (rip->useParanoia) {
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

                if (rip->getIndices) {
                    rip->save_read_paranoid = rip->paranoiaCtlObj->read_audio;
                    rip->paranoiaCtlObj->read_audio = cued_read_paranoid;
                    rip->mmcBuf = NULL;
                    rip->allocatedSectors = 0;
                }
            } else {
                cdio_cddap_close_no_free_cdio(rip->paranoiaCtlObj);

                cdio_error("disabling paranoia");
                rip->useParanoia = 0;
            }
        } else {
            cdio_error("disabling paranoia due to the following message(s):\n%s", msg);
            rip->useParanoia = 0;
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

    if (rip->ripToOneFile) {

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

        rip->lastSector = cdio_get_track_last_lsn(rip->cdObj, rip->lastTrack);
        if (CDIO_INVALID_LSN == rip->lastSector) {
            cdio2_abort("failed to get last sector number for track %02d", rip->lastTrack);
        }

        rip->channels = cdio2_get_track_channels(rip->cdObj, rip->firstTrack);

        // does not return on error
        (void) format_get_file_path(rip->cdObj, rip->cddbObj,
            rip->fileNamePattern, cued_fmt_to_ext(rip->soundFileFormat), 0,
            rip->fileNameBuffer, rip->bufferSize
            );

        if (verbose) {
            printf("progress: reading sectors from %d to %d\n", rip->firstSector, rip->lastSector);
        }

        rip->currentTrack = 0;
        cued_rip_to_file(rip);

    } else {

        track_t track;

        for (track = rip->firstTrack;  track <= rip->lastTrack;  ++track) {

            if (TRACK_FORMAT_AUDIO != cdio_get_track_format(rip->cdObj, track)) {
                cdio_warn("track %02d is not an audio track; skipping track", track);
                continue;
            }

            rip->firstSector = cdio_get_track_lsn(rip->cdObj, track);
            if (CDIO_INVALID_LSN == rip->firstSector) {
                cdio2_abort("failed to get first sector number for track %02d", track);
            }

            rip->channels = cdio2_get_track_channels(rip->cdObj, track);

            // rip first track pregap to track 00 file
            if (1 == track && rip->firstSector > 0) {

                lsn_t saveFirstSector = rip->firstSector;

                // does not return on error
                (void) format_get_file_path(rip->cdObj, rip->cddbObj,
                    rip->fileNamePattern, cued_fmt_to_ext(rip->soundFileFormat), 0,
                    rip->fileNameBuffer, rip->bufferSize);

                if (verbose) {
                    printf("progress: reading track %02d\n", 0);
                }

                rip->lastSector = rip->firstSector - 1;
                rip->firstSector = 0;
                rip->currentTrack = 0;
                cued_rip_to_file(rip);

                if (rip->silent_pregap) {
                    if (unlink(rip->fileNameBuffer)) {
                        cdio2_unix_error("unlink", rip->fileNameBuffer, 1);
                    }
                }

                rip->firstSector = saveFirstSector;
            }

            rip->lastSector = cdio_get_track_last_lsn(rip->cdObj, track);
            if (CDIO_INVALID_LSN == rip->lastSector) {
                cdio2_abort("failed to get last sector number for track %02d", track);
            } else {
                //cdio_debug("track %02d last sector is %d", track, rip->lastSector);
            }

            // does not return on error
            (void) format_get_file_path(rip->cdObj, rip->cddbObj,
                rip->fileNamePattern, cued_fmt_to_ext(rip->soundFileFormat), track,
                rip->fileNameBuffer, rip->bufferSize);

            if (verbose) {
                printf("progress: reading track %02d\n", track);
            }

            rip->currentTrack = track;
            cued_rip_to_file(rip);
        }
    }

    if (rip->useParanoia) {
        if (rip->getIndices) {
            free(rip->mmcBuf);
            rip->paranoiaCtlObj->read_audio = rip->save_read_paranoid;
        }
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
        if (verbose) {
            printf("progress: correctly read %d of %d Q sub-channel records\n", rip->crcSuccess, totalCrcs);
        }
    }
}
