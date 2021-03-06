/*
 * drv_adc.c
 *
 *  Created on: 21 ???. 2020 ?.
 *      Author: vladyslav.daliavskyi
 */

#include "drv_adc.h"

static const drvADC_def_t drvADC_def[drvADC_Count] = {
        {
                .adc = halADC1,
                .channel = halADC_Channel4,
                .dataSize = halADC_dataSize12b,
                .timer = halADC_timer2
        }
};

void drvADCInit(drvADC_struct* adcStruct, drvADC adc, uint32_t frequency, QueueHandle_t queue) {

    if (adcStruct == NULL) return;

    halADC_initStruct initStruct;

    initStruct.adc                              = drvADC_def[adc].adc;
    initStruct.channel                          = drvADC_def[adc].channel;
    initStruct.dataSize                         = drvADC_def[adc].dataSize;
    initStruct.triggerTimerInitStruct.timer     = drvADC_def[adc].timer;
    initStruct.triggerTimerInitStruct.frequency = frequency;
    initStruct.queue                            = queue;

    initStruct.clockPrescaler                   = halADC_clockDiv2;
    initStruct.convMode                         = halADC_triggerConvMode;
    initStruct.dataAlign                        = halADC_dataAlignRight;

    halADCInit((halADC_struct*)adcStruct, (const halADC_initStruct*) &initStruct);
}

void drvADCStart(drvADC_struct* adcStruct) {
    if (adcStruct == NULL) return;
    halADCStart((halADC_struct*)adcStruct);
}

void drvADCStop(drvADC_struct* adcStruct) {
    if (adcStruct == NULL) return;
    halADCStop((halADC_struct*)adcStruct);
}

void drvADCGetValue(drvADC_struct* adcStruct) {
    if (adcStruct == NULL) return;
    halADCGetValue((halADC_struct*)adcStruct);
}
