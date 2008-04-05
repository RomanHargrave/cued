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

#define DO_NOT_WANT_PARANOIA_COMPATIBILITY
#include <cdio/cdio.h>
#include <cdio/mmc.h> // CDIO_MMC_READ_TYPE_ANY
#include "cdio2.h"
#include "rip.h"
#include "format.h"

#include <stdlib.h> // free
#include <unistd.h> // unlink
#include <sndfile.h>


char  rip_mcn[ MCN_LEN + 1 ];
char  rip_isrc   [ CDIO_CD_MAX_TRACKS + 1 ][ ISRC_LEN + 1 ];
lsn_t rip_indices[ CDIO_CD_MAX_TRACKS + 1 ][ CUED_MAX_INDICES ];
int   rip_silent_pregap;
int   rip_noisy_pregap;
int   rip_year;

static int trackHint;


void cued_cleanup_rip_data()
{
    memset(rip_indices, 0x00, sizeof(rip_indices));
    memset(rip_mcn,     0x00, sizeof(rip_mcn));
    memset(rip_isrc,    0x00, sizeof(rip_isrc));
    trackHint = 0;
    rip_noisy_pregap = 0;
    rip_silent_pregap = 0;
    rip_year = 0;
}


static void cued_parse_qsc(void *qsc)
{
    qsc_index_t index;
    lba_t *currLba;
    char *isrc;

    if (qsc_check_crc(qsc)) {
        return;
    }

    switch (qsc_get_mode(qsc)) {

        case QSC_MODE_INDEX:
            if (!qsc_get_index(qsc, &index)) {

                // set this for ISRC case
                trackHint = index.track;

                currLba = &rip_indices[ index.track ][ index.index ];
                if (!*currLba || index.absoluteLba < *currLba) {
                    *currLba = index.absoluteLba;
                }

            } else {
                cdio_warn("invalid index found in q sub-channel");
            }
            break;

        case QSC_MODE_MCN:
            if (!rip_mcn[0]) {
                if (qsc_get_mcn(qsc, rip_mcn)) {
                    cdio_warn("invalid mcn found in q sub-channel");
                    rip_mcn[0] = 0;
                }
            }
            break;

        case QSC_MODE_ISRC:
            isrc = rip_isrc[trackHint];
            if (!isrc[0]) {
                if (qsc_get_isrc(qsc, isrc)) {
                    cdio_warn("invalid isrc found in q sub-channel");
                    isrc[0] = 0;
                } else if (!rip_year) {
                    rip_year = qsc_get_isrc_year(isrc);
                    cdio_info("set rip year to %d\n", rip_year);
                }
            }
            break;

        default:
            break;
    }
}


typedef struct _mmc_audio_buffer_t {

    int16_t buf[CD_FRAMEWORDS];
    DECLARE_QSC(qsc);

} mmc_audio_buffer_t;


void cued_rip_to_file(

    // sound file information
    //
    char *fileName,
    int channels, int soundFileFormat,

    CdIo_t *cdObj,
    lsn_t firstSector, lsn_t lastSector,
    track_t track,

    int offsetWords,
    lsn_t endOfDiscSector,
    int getIndices,

    // paranoia
    //
    int useParanoia,
    cdrom_drive_t *paranoiaCtlObj, cdrom_paranoia_t *paranoiaRipObj,
    int retries,

    // for error reporting
    const char *qSubChannelFileName, FILE *qSubChannelFile)
{
    typedef struct _audio_buffer_t {

        mmc_audio_buffer_t mmc;
        lsn_t requested;

    } audio_buffer_t;

    audio_buffer_t audioBuf;

    SF_INFO sfinfo;
    PIT(SNDFILE, sfObj);
    PIT(int16_t, pbuf);
    lsn_t currSector, prc, offsetSectors;
    driver_return_code_t rc;
    int wordsToWrite, wordsWritten, i;

    memset(&sfinfo, 0x00, sizeof(sfinfo));
    sfinfo.samplerate = 44100;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_PCM_16 | soundFileFormat;

    offsetSectors = offsetWords / CD_FRAMEWORDS;
    firstSector += offsetSectors;
    lastSector  += offsetSectors;
    offsetWords %= CD_FRAMEWORDS;

    if (offsetWords < 0) {
        firstSector -= 1;
    } else if (offsetWords > 0) {
        lastSector  += 1;
    }
    // else offsetWords is zero b/c the offset fell on a sector boundary

    currSector = firstSector;

    if (useParanoia) {
        lsn_t seekSector;

        if (currSector < 0) {
            seekSector = 0;
        } else {
            seekSector = currSector;
        }

        if (seekSector < endOfDiscSector) {
            prc = cdio_paranoia_seek(paranoiaRipObj, seekSector, SEEK_SET);
            cdio2_paranoia_msg(paranoiaCtlObj, "paranoia seek");
            if (-1 == prc) {
                cdio_error("paranoia returned \"%d\" during seek to \"%d\"; skipping track %02d", prc, seekSector, track);

                return;
            }
        }
    }

    sfObj = sf_open(fileName, SFM_WRITE, &sfinfo);
    if (!sfObj) {
        cdio_error("sf_open(\"%s\") returned \"%s\"; skipping extraction of track %02d", fileName, sf_strerror(sfObj), track);

        return;
    }

    for (;  currSector <= lastSector;  ++currSector) {

        if (currSector < 0 || currSector >= endOfDiscSector) {

            memset(audioBuf.mmc.buf, 0x00, sizeof(audioBuf.mmc.buf));
            pbuf = audioBuf.mmc.buf;

        } else {

            // TODO:  need to update track indices on skip of sector

            if (useParanoia) {
                pbuf = cdio_paranoia_read_limited(paranoiaRipObj, cdio2_paranoia_callback, retries);
                cdio2_paranoia_msg(paranoiaCtlObj, "read of audio sector");
                if (!pbuf) {
                    cdio_error("paranoia did not return data; skipping extraction of audio sector %d in track %02d", currSector, track);
                    continue;
                }
            } else {

                if (qSubChannelFileName || getIndices) {

                    rc = mmc_read_cd(
                        cdObj,
                        &audioBuf.mmc,
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

                        // C2 error information is synthetic;  it is not needed to get Q subchannel,
                        // even though it is an adjacent field according to the standard
                        //
                        false,

                        // select Q subchannel
                        2,

                        sizeof(audioBuf.mmc),

                        // number of sectors
                        1);

                    if (DRIVER_OP_SUCCESS == rc) {

                        if (getIndices) {
                            cued_parse_qsc(audioBuf.mmc.qsc);
                        }

                        if (qSubChannelFileName) {
                            audioBuf.requested = currSector;
                            if (1 != fwrite(audioBuf.mmc.qsc, sizeof(audioBuf.mmc.qsc) + sizeof(audioBuf.requested), 1, qSubChannelFile)) {
                                // probably out of disk space, which is bad, because most things rely on it
                                cdio2_unix_error("fwrite", qSubChannelFileName, 0);
                                cdio2_abort("failed to write to file \"%s\"", qSubChannelFileName);
                            }
                        }
                    }

                } else {
                    rc = cdio_read_audio_sector(cdObj, audioBuf.mmc.buf, currSector);
                }

                if (DRIVER_OP_SUCCESS != rc) {
                    cdio2_driver_error(rc, "read of audio sector");
                    cdio_error("skipping extraction of audio sector %d in track %02d", currSector, track);
                    continue;
                }

                pbuf = audioBuf.mmc.buf;
            }
        }

        wordsToWrite = CD_FRAMEWORDS;

        // N.B. firstSector == lastSector is not possible if offsetWords is non-zero
        //
        if (firstSector == currSector) {
            if (offsetWords < 0) {
                pbuf += CD_FRAMEWORDS + offsetWords;
                wordsToWrite  = -offsetWords;
            } else if (offsetWords > 0) {
                pbuf += offsetWords;
                wordsToWrite -= offsetWords;
            }
        } else if (lastSector == currSector) {
            if (offsetWords < 0) {
                wordsToWrite += offsetWords;
            } else if (offsetWords > 0) {
                wordsToWrite  = offsetWords;    
            }
        }

        wordsWritten = sf_write_short(sfObj, pbuf, wordsToWrite);
        if (wordsWritten != wordsToWrite) {

            // probably out of disk space, which is bad, because most things rely on it
            cdio2_abort("failed to write to file \"%s\" due to \"%s\"", fileName, sf_strerror(sfObj));
        }

        if (!track && !rip_noisy_pregap) {
            for (i = 0;  i < wordsToWrite;  ++i) {
                if (pbuf[i]) {
                    rip_noisy_pregap = 1;
                    break;
                }
            }
        }
    }

    if (!track && !rip_noisy_pregap) {
        rip_silent_pregap = 1;
    }

    sf_close(sfObj);
}


typedef struct _paranoia_audio_buffer_t {

    int16_t buf[CD_FRAMEWORDS];

} paranoia_audio_buffer_t;


static mmc_audio_buffer_t *mmcBuf;
static long allocatedSectors;


void cued_free_paranoia_buf()
{
    free(mmcBuf);
    mmcBuf = NULL;
    allocatedSectors = 0;
}


long cued_read_audio(cdrom_drive_t *paranoiaCtlObj, void *pb, lsn_t firstSector, long sectors)
{
    paranoia_audio_buffer_t *pbuf = (paranoia_audio_buffer_t *) pb;

    long rc;
    driver_return_code_t drc;
    int i;

    if (sectors > allocatedSectors) {
        free(mmcBuf);
        mmcBuf = (mmc_audio_buffer_t *) malloc(sectors * sizeof(mmc_audio_buffer_t));
        if (mmcBuf) {
            allocatedSectors = sectors;
        } else {
            allocatedSectors = 0;
            cdio2_abort("out of memory reading %ld sectors", sectors);
        }
    }

    drc = mmc_read_cd(
        paranoiaCtlObj->p_cdio,
        mmcBuf,
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

        // C2 error information is synthetic;  it is not needed to get Q subchannel,
        // even though it is an adjacent field according to the standard
        //
        false,

        // select Q subchannel
        2,

        sizeof(mmc_audio_buffer_t),
        sectors);

    if (DRIVER_OP_SUCCESS == drc) {
        for (i = 0;  i < sectors;  ++i) {
            memcpy(pbuf[i].buf, mmcBuf[i].buf, sizeof(paranoia_audio_buffer_t));
            cued_parse_qsc(mmcBuf[i].qsc);
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


void cued_rip_disc(

    // sound file information
    //
    const char *fileNamePattern,
    cddb_disc_t *cddbObj,
    int soundFileFormat,

    CdIo_t *cdObj,
    lsn_t firstTrack, lsn_t lastTrack,

    int ripToOneFile,
    int offsetWords,
    int getIndices,

    // paranoia
    //
    int useParanoia,
    cdrom_drive_t *paranoiaCtlObj, cdrom_paranoia_t *paranoiaRipObj,
    int retries,

    const char *qSubChannelFileName,

    char *fileNameBuffer, int bufferSize)
{
    PIT(FILE, qSubChannelFile);
    lsn_t firstSector, lastSector, endOfDiscSector;
    int channels;
    track_t track;

    if (qSubChannelFileName) {
        if (!strcmp("-", qSubChannelFileName)) {
            qSubChannelFile = stdout;
        } else {
            qSubChannelFile = fopen2(qSubChannelFileName, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0666);
            if (!qSubChannelFile) {
                cdio2_unix_error("fopen2", qSubChannelFileName, 0);
                cdio_error("not creating sub-channel file \"%s\"", qSubChannelFileName);

                qSubChannelFileName = 0;
            }
        }
    }

    endOfDiscSector = cdio_get_disc_last_lsn(cdObj);
    if (CDIO_INVALID_LSN == endOfDiscSector) {
        cdio2_abort("failed to get last sector number");
    } else {
        //cdio_debug("end of disc sector is %d", endOfDiscSector);
    }

    if (ripToOneFile) {

        if (TRACK_FORMAT_AUDIO != cdio_get_track_format(cdObj, firstTrack)) {
            cdio2_abort("track %02d is not an audio track", firstTrack);
        }

        if (firstTrack > 1) {
            firstSector = cdio_get_track_lsn(cdObj, firstTrack);
            if (CDIO_INVALID_LSN == firstSector) {
                cdio2_abort("failed to get first sector number for track %02d", firstTrack);
            }
        } else {
            firstSector = 0;
        }

        lastSector = cdio_get_track_last_lsn(cdObj, lastTrack);
        if (CDIO_INVALID_LSN == lastSector) {
            cdio2_abort("failed to get last sector number for track %02d", lastTrack);
        }

        channels = cdio2_get_track_channels(cdObj, firstTrack);

        // does not return on error
        (void) format_get_file_path(cdObj, cddbObj, fileNamePattern, cued_fmt_to_ext(soundFileFormat), 0, fileNameBuffer, bufferSize);

        if (verbose) {
            printf("progress: reading sectors from %d to %d\n", firstSector, lastSector);
        }

        cued_rip_to_file(
            fileNameBuffer,
            channels,
            soundFileFormat,
            cdObj,
            firstSector,
            lastSector,
            0,
            offsetWords,
            endOfDiscSector,
            getIndices,
            useParanoia,
            paranoiaCtlObj,
            paranoiaRipObj,
            retries,
            qSubChannelFileName,
            qSubChannelFile
            );

    } else {

        for (track = firstTrack;  track <= lastTrack;  ++track) {

            if (TRACK_FORMAT_AUDIO != cdio_get_track_format(cdObj, track)) {
                cdio_warn("track %02d is not an audio track; skipping track", track);
                continue;
            }

            firstSector = cdio_get_track_lsn(cdObj, track);
            if (CDIO_INVALID_LSN == firstSector) {
                cdio2_abort("failed to get first sector number for track %02d", track);
            }

            channels = cdio2_get_track_channels(cdObj, track);

            // rip first track pregap to track 00 file
            if (1 == track && firstSector > 0) {

                // does not return on error
                (void) format_get_file_path(cdObj, cddbObj, fileNamePattern, cued_fmt_to_ext(soundFileFormat), 0, fileNameBuffer, bufferSize);

                if (verbose) {
                    printf("progress: reading track %02d\n", 0);
                }

                cued_rip_to_file(
                    fileNameBuffer,
                    channels,
                    soundFileFormat,
                    cdObj,
                    0,
                    firstSector - 1,
                    0,
                    offsetWords,
                    endOfDiscSector,
                    getIndices,
                    useParanoia,
                    paranoiaCtlObj,
                    paranoiaRipObj,
                    retries,
                    qSubChannelFileName,
                    qSubChannelFile
                    );

                if (rip_silent_pregap) {
                    if (unlink(fileNameBuffer)) {
                        cdio2_unix_error("unlink", fileNameBuffer, 1);
                    }
                }
            }

            lastSector = cdio_get_track_last_lsn(cdObj, track);
            if (CDIO_INVALID_LSN == lastSector) {
                cdio2_abort("failed to get last sector number for track %02d", track);
            } else {
                //cdio_debug("track %02d last sector is %d", track, lastSector);
            }

            // does not return on error
            (void) format_get_file_path(cdObj, cddbObj, fileNamePattern, cued_fmt_to_ext(soundFileFormat), track, fileNameBuffer, bufferSize);

            if (verbose) {
                printf("progress: reading track %02d\n", track);
            }

            cued_rip_to_file(
                fileNameBuffer,
                channels,
                soundFileFormat,
                cdObj,
                firstSector,
                lastSector,
                track,
                offsetWords,
                endOfDiscSector,
                getIndices,
                useParanoia,
                paranoiaCtlObj,
                paranoiaRipObj,
                retries,
                qSubChannelFileName,
                qSubChannelFile
                );
        }
    }

    cued_free_paranoia_buf();

    if (qSubChannelFileName && qSubChannelFile != stdout) {
        fclose(qSubChannelFile);
    }
}
