/* ads129xr.c - ADS1294R ADS1296R ADS1298R ECG acqusition spi device */

/*
 * Copyright (c) 2021 scottwan@foxmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ads129xr

#include <drivers/sensor.h>
#include "ads129xr.h"

// Compatible with "ti,ads129xr"
#define DT_DRV_COMPAT ti_ads129xr
