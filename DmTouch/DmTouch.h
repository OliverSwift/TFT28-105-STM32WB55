/**********************************************************************************************
 Copyright (c) 2014 DisplayModule. All rights reserved.

 Redistribution and use of this source code, part of this source code or any compiled binary
 based on this source code is permitted as long as the above copyright notice and following
 disclaimer is retained.

 DISCLAIMER:
 THIS SOFTWARE IS SUPPLIED "AS IS" WITHOUT ANY WARRANTIES AND SUPPORT. DISPLAYMODULE ASSUMES
 NO RESPONSIBILITY OR LIABILITY FOR THE USE OF THE SOFTWARE.
 ********************************************************************************************/
/*
 * Copyright 2021 - Olivier DEBON
 */

#ifndef DM_TOUCH_h
#define DM_TOUCH_h

#include "dm_platform.h"

#define RESCALE_FACTOR (4096)

typedef struct calibrationMatrix {
  int32_t  a, b, c,	d, e, f;
} CalibrationMatrix;

typedef struct {
  void (*init)();
  void (*setPenIRQ)();
  bool (*readTouchData)(uint16_t *posX, uint16_t *posY);
  bool (*isTouched)();
  void (*setCalibrationMatrix)(CalibrationMatrix calibrationMatrix);
  void (*setPrecison)(uint8_t samplesPerMeasurement);
} DmTouch;

void setupDmTouch(DmTouch *handle, GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *irq_port, uint16_t irq_pin);

#endif



