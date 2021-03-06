/*
 * ul_fat_fs.c
 *
 *  Created on: Jul 29, 2020
 *      Author: Sigma
 */

#include "ul_fat_fs.h"

static ulFatFS_struct* current_fatfs;

/*
 * Adapter block
 */

static volatile DSTATUS Stat = STA_NOINIT;

static DSTATUS SD_CheckStatus(BYTE lun) {
    Stat = STA_NOINIT;

    if (drvSDGetStatus(&current_fatfs->sd) == drvSD_OK) {
        Stat &= ~STA_NOINIT;
    }

    return Stat;
}

static DSTATUS SD_initialize(BYTE lun) {
    Stat = STA_NOINIT;

#if !defined(DISABLE_SD_INIT)

    if (drvSDInit(&current_fatfs->sd) == drvSD_OK) {
        Stat = SD_CheckStatus(lun);
    }

#else
    Stat = SD_CheckStatus(lun);
#endif

    return Stat;
}

static DSTATUS SD_status(BYTE lun)
{
  return SD_CheckStatus(lun);
}

static DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;

    if (drvSDReadBlocks(&current_fatfs->sd, buff, sector, count) == drvSD_OK) {
        /* wait until the read operation is finished */
        while (drvSDGetCardState(&current_fatfs->sd) != drvSD_TRANSFER_OK);
        res = RES_OK;
    }

    return res;
}

#if _USE_WRITE == 1

static DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;

    if (drvSDWriteBlocks(&current_fatfs->sd, (uint8_t*)buff, sector, count) == drvSD_OK) {
        /* wait until the read operation is finished */
        while (drvSDGetCardState(&current_fatfs->sd) != drvSD_TRANSFER_OK) {
        }
        res = RES_OK;
    }

    return res;
}
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1
static DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff) {
    DRESULT res = RES_ERROR;

    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            res = RES_OK;
            break;

        case GET_SECTOR_COUNT:
            drvSDGetSDInfo(&current_fatfs->sd);
            *(DWORD*) buff = current_fatfs->sd.SDIO.sdInfo.LogBlockNbr;
            res = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            drvSDGetSDInfo(&current_fatfs->sd);
            *(WORD*) buff = current_fatfs->sd.SDIO.sdInfo.LogBlockSize;
            res = RES_OK;
            break;

        case GET_BLOCK_SIZE:
            drvSDGetSDInfo(&current_fatfs->sd);
            *(DWORD*) buff = current_fatfs->sd.SDIO.sdInfo.LogBlockSize / SD_BLOCK_SIZE;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}
#endif /* _USE_IOCTL == 1 */

static const Diskio_drvTypeDef  SD_Driver =
{
  SD_initialize, SD_status, SD_read,
#if  _USE_WRITE == 1
  SD_write,
#endif /* _USE_WRITE == 1 */

#if  _USE_IOCTL == 1
  SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};


/*
 * End adapter block
 */

ulFatFS_error ulFatFSInit(ulFatFS_struct* fatfs) {
    current_fatfs = fatfs;
    fatfs->sd.sddriver = drvSDDriver1;

    fatfs->sdMutex = xSemaphoreCreateMutex();

    return FATFS_LinkDriver(&SD_Driver) == 0 ? ulFatFS_OK : ulFatFS_DISK_ERR;
}

ulFatFS_error ulFatFSMountSD(ulFatFS_struct* fatfs) {
    if (fatfs == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    FRESULT functionRes = f_mount(&current_fatfs->FatFs, "", 0);
    xSemaphoreGive(fatfs->sdMutex);
    return functionRes;
}

ulFatFS_error ulFatFSUnmountSD(ulFatFS_struct* fatfs) {
    if (fatfs == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    FRESULT functionRes = f_mount(NULL, "", 0);
    xSemaphoreGive(fatfs->sdMutex);
    return functionRes;
}

ulFatFS_error ulFatFSCreateFolder(ulFatFS_struct* fatfs, char* folderName) {
    if (fatfs == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    FRESULT functionRes = f_mkdir(folderName);
    xSemaphoreGive(fatfs->sdMutex);
    return functionRes;
}

ulFatFS_error ulFatFSFindCount(ulFatFS_struct* fatfs, char* folderPath, char* fileMask,
        ulFatFS_elementType elementType, uint8_t* count) {
    if (fatfs == NULL || folderPath == NULL || fileMask == NULL || count == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    DIR dir;
    FILINFO fno;
    uint8_t tmp_count = 0;
    FRESULT fr = f_findfirst(&dir, &fno, folderPath, fileMask);
    switch (fr) {
        case FR_NO_PATH: {
            *count = 0;
            fr = FR_OK;
            break;
        }
        case FR_OK: {
            while (*(fno.fname)) {
                if (fr != (FRESULT) ulFatFS_OK) {
                    xSemaphoreGive(fatfs->sdMutex);
                    return fr;
                }
                switch (elementType) {
                    case ulFatFS_Folder:
                        if (fno.fattrib & AM_DIR) {
                            tmp_count++;
                        }
                        break;
                    case ulFatFS_File:
                        if (!(fno.fattrib & AM_DIR)) {
                            tmp_count++;
                        }
                        break;
                }
                fr = f_findnext(&dir, &fno);
            }
            fr = f_closedir(&dir);
            *count = (fr == (FRESULT) ulFatFS_OK) ? tmp_count : SD_ERROR_VALUE;
            break;
        }
        default: {
            break;
        }
    }
    xSemaphoreGive(fatfs->sdMutex);
    return fr;

}

ulFatFS_error ulFatFSOpenFile(ulFatFS_struct* fatfs, ulFatFS_File_t* file, const char* fileName,
        ulFatFS_FileOpenAttribute_t posix) {
    if (fatfs == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    FRESULT functionRes = f_open(file, fileName, posix);
    xSemaphoreGive(fatfs->sdMutex);
    return functionRes;
}

ulFatFS_error ulFatFSWrite(ulFatFS_struct* fatfs, ulFatFS_File_t* file, const uint8_t* data,
        uint16_t dataLen) {
    if (fatfs == NULL || data == NULL || file == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    uint16_t writtenBytes = 0;
    FRESULT fr = f_write(file, data, dataLen, (unsigned int*) &writtenBytes);
    if (fr != (FRESULT) ulFatFS_OK) {
        xSemaphoreGive(fatfs->sdMutex);
        return fr;
    }
    xSemaphoreGive(fatfs->sdMutex);
    return writtenBytes == dataLen ? ulFatFS_OK : ulFatFS_COMPLETE_OPERATION_ERROR;
}

ulFatFS_error ulFatFSWriteString(ulFatFS_struct* fatfs, ulFatFS_File_t* file, const char* str) {
    if (fatfs == NULL || str == NULL || file == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    uint16_t writtenBytes = 0;
    uint16_t dataLen = strlen(str);
    FRESULT fr = f_write(file, str, dataLen, (unsigned int*) &writtenBytes);
    if (fr != (FRESULT) ulFatFS_OK) {
        xSemaphoreGive(fatfs->sdMutex);
        return fr;
    }
    xSemaphoreGive(fatfs->sdMutex);
    return writtenBytes == dataLen ? ulFatFS_OK : ulFatFS_COMPLETE_OPERATION_ERROR;
}

ulFatFS_error ulFatFSRead(ulFatFS_struct* fatfs, ulFatFS_File_t* file, uint8_t* data,
        uint16_t dataLen) {
    if (fatfs == NULL || data == NULL || file == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    uint16_t readBytes = 0;
    FRESULT fr = f_read(file, data, dataLen, (unsigned int*) &readBytes);
    if (fr != (FRESULT) ulFatFS_OK) {
        xSemaphoreGive(fatfs->sdMutex);
        return fr;
    }
    xSemaphoreGive(fatfs->sdMutex);
    return readBytes == dataLen ? ulFatFS_OK : ulFatFS_COMPLETE_OPERATION_ERROR;

}

ulFatFS_error ulFatFSCloseFile(ulFatFS_struct* fatfs, ulFatFS_File_t* file) {
    if (fatfs == NULL || file == NULL) {
        return ulFatFS_DATA_NULL_POINT;
    }
    xSemaphoreTake(fatfs->sdMutex, portMAX_DELAY);
    FRESULT fr = f_close(file);
    xSemaphoreGive(fatfs->sdMutex);
    return fr;
}
