/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) Bao Project and Contributors. All rights reserved
 */

#ifndef __PLAT_PLATFORM_H__
#define __PLAT_PLATFORM_H__

#define DATA_MEMORY_ACCESS          0x13
#define L2D_CACHE_ACCESS            0x16
#define L2D_CACHE_REFILL            0x17
#define BUS_ACCESS                  0x19
#define EXTERNAL_MEMORY_REQUEST     0xC0

#define UART8250_REG_WIDTH   (4)
#define UART8250_PAGE_OFFSET (0x40)

#include <drivers/8250_uart.h>

#endif
