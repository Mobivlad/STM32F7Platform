/*
 * bl_adc_file_write.h
 *
 *  Created on: Aug 5, 2020
 *      Author: Sigma
 */

#ifndef BL_BL_ADC_FILE_WRITER_H_
#define BL_BL_ADC_FILE_WRITER_H_

#include "ul_fat_fs.h"

#include "stdio.h"

#define BUFFER_SIZE             50
#define WRITE_BUFFER_SIZE       251

#define FOLDER_PREFIX           "adcData"
#define FILE_PREFIX             "file"
#define FILE_FORMAT             ".csv"

#define FILE_NAME_MASE          "%s/%s%d"FILE_FORMAT

#define FIND_FOLDER_PREFIX      FOLDER_PREFIX"*"
#define FOLDER_NAME_MASK        "%s%d"

#define ARRAY_SIZE(X)           (sizeof(x) / sizeof(x[0]))

#define MAX_VOLTAGE             3.3f
#define MAX_ADC_VAL             0x0FFF

#define WRITER_QUEUE_SIZE       200

typedef enum {
    blADCFileWriter_StartCommand = 0,
    blADCFileWriter_StopCommand
} blADCFileWriter_command;

typedef enum {
    blADCFileWriter_DATA = 0,
    blADCFileWriter_COMMAND
} blADCFileWriter_recordType;

typedef struct {
    blADCFileWriter_recordType  recordType;
    uint16_t                    data;
} blADCFileWriter_record;

typedef struct {
    ulFatFS_struct      fatFS;

    QueueHandle_t       fileWriterDataQueue;

    uint8_t             fileNumber;

    ulFatFS_File_t      currentFile;

    char fileNameBuffer[BUFFER_SIZE];
    char folderNameBuffer[BUFFER_SIZE];
    char dataStrBuffer[BUFFER_SIZE];

    char fileWriteData[WRITE_BUFFER_SIZE];
    uint16_t index;
} blADCFileWriter_struct;

typedef enum {
    blADCFW_OK = 0,
    blADCFW_EROOR,
    blADCFW_NULL_POINT
} blADCFW_error;

typedef enum {
    blADCFW_COPY_OK = 0,
    blADCFW_COPY_OVERLOAD,
} blADCFW_bufferCopyError;

blADCFW_error blADCFileWriterInit(blADCFileWriter_struct* fileWriterStruct);

void blADCFileWriterTask(void* parametr);


#endif /* BL_BL_ADC_FILE_WRITER_H_ */
