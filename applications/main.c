/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-05     RT-Thread    first version
 */
#include <board.h>
#include <rtthread.h>
#include "led.h"
#include "key.h"
#include "radio_app.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define MCU_VER     "1.0.2"

int main(void)
{
    LOG_I("System Version is %s\r\n",MCU_VER);
    radio_init();
    storage_init();
    pvd_init();
    rtc_init();
    warning_init();
    led_init();
    button_init();
    power_init();
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
