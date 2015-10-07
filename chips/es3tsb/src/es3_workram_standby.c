/**
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This file contains code to run in workram before calling code in ROM to enter
 * standby, and after code in ROM resumed from standby.
 * This file is only needed by the third stage FW. It is here in the boot ROM
 * source tree as code to test the standby/resume function of the boot ROM
 */

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "bootrom.h"
#include "chipapi.h"
#include "unipro.h"
#include "tsb_unipro.h"
#include "debug.h"
#include "utils.h"
#include "tsb_scm.h"

int chip_enter_hibern8_client(void) {
    uint32_t tx_reset_offset, rx_reset_offset;
    uint32_t cportid;
    int rc;
    uint32_t tempval;

    dbgprint("Wait for hibernate\n");
    do {
        rc = chip_unipro_attr_read(TSB_HIBERNATE_ENTER_IND, &tempval, 0,
                                   ATTR_LOCAL);
    } while (!rc && tempval == 0);
    if (rc) {
        return rc;
    }

    for (cportid = 0; cportid < CPORT_MAX; cportid++) {
        tx_reset_offset = TX_SW_RESET_00 + (cportid << 2);
        rx_reset_offset = RX_SW_RESET_00 + (cportid << 2);

        putreg32(CPORT_SW_RESET_BITS,
                 (volatile unsigned int*)(AIO_UNIPRO_BASE + tx_reset_offset));
        putreg32(CPORT_SW_RESET_BITS,
                 (volatile unsigned int*)(AIO_UNIPRO_BASE + rx_reset_offset));
    }
    dbgprint("hibernate entered\n");

    return 0;
}

int chip_exit_hibern8_client(void) {
    int rc;
    uint32_t tempval;

    tsb_clk_enable(TSB_CLK_UNIPROSYS);
    dbgprint("Try to exit hibernate\n");
    tempval = 1;
    rc = chip_unipro_attr_write(TSB_HIBERNATE_EXIT_REQ, tempval, 0,
                                ATTR_LOCAL);
    if (rc) {
        return rc;
    }

    do {
        rc = chip_unipro_attr_read(TSB_HIBERNATE_EXIT_IND, &tempval, 0,
                                   ATTR_LOCAL);
    } while (!rc && tempval == 0);
    if (rc) {
        return rc;
    }
    dbgprint("hibernate exit\n");
    return 0;
}


int chip_enter_hibern8_server(void) {
    uint32_t tx_reset_offset, rx_reset_offset;
    uint32_t cportid;
    int rc;
    uint32_t tempval;

    dbgprint("entering hibernate\n");
    for (cportid = 0; cportid < CPORT_MAX; cportid++) {
        tx_reset_offset = TX_SW_RESET_00 + (cportid << 2);
        rx_reset_offset = RX_SW_RESET_00 + (cportid << 2);

        putreg32(CPORT_SW_RESET_BITS,
                 (volatile unsigned int*)(AIO_UNIPRO_BASE + tx_reset_offset));
        putreg32(CPORT_SW_RESET_BITS,
                 (volatile unsigned int*)(AIO_UNIPRO_BASE + rx_reset_offset));
    }

    tempval = 1;
    rc = chip_unipro_attr_write(TSB_HIBERNATE_ENTER_REQ, tempval, 0,
                                ATTR_LOCAL);
    if (rc) {
        return rc;
    }

    dbgprint("wait for hibernate\n");
    do {
        rc = chip_unipro_attr_read(TSB_HIBERNATE_ENTER_IND, &tempval, 0,
                                   ATTR_LOCAL);
    } while (!rc && tempval == 0);
    if (rc) {
        return rc;
    }

    dbgprint("hibernate entered\n");

    dbgprint("wait for hibernate exit\n");
    do {
        rc = chip_unipro_attr_read(TSB_HIBERNATE_EXIT_IND, &tempval, 0,
                                   ATTR_LOCAL);
    } while (!rc && tempval == 0);
    if (rc) {
        return rc;
    }
    dbgprint("hibernate exit\n");
    return 0;
}

/* use all 4 GPIOs 3, 4, 5, 23 as wakeup source */
#define TEST_WAKEUPSRC 0x1111
int standby_sequence(void) {
    int (*enter_standby_func)(void);
    int status;

    putreg32(TEST_WAKEUPSRC, WAKEUPSRC);
#ifdef _STANDBY_WAIT_FOR_SERVER
    chip_enter_hibern8_client();
#endif
    while (0 != getreg32((volatile unsigned int*)UNIPRO_CLK_EN));
    tsb_clk_disable(TSB_CLK_UNIPROSYS);

    putreg32(0, HB8CLK_EN);
    delay_ns(1500);
    putreg32(1, RETFFSAVE);
    delay_ns(100);
    putreg32(0, RETFFSAVE);

    /**
     * Following code cannot run in workram
     * let's use the code in ROM, as part of the boot ROM shared function
     */
    enter_standby_func = get_shared_function(SHARED_FUNCTION_ENTER_STANDBY);

    status = enter_standby_func();
    return status;
}

void resume_sequence_in_workram(void) {
    putreg32(SRSTRELEASE_UNIPRO_SYSRESET_N, SOFTRESETRELEASE1);

    /* delay 0.1us or more */
    delay_ns(100);

    putreg32(1, RETFFSTR);

    /* delay 5us or more */
    delay_ns(5000);

    putreg32(0, RETFFSTR);

    /* delay 5us or more */
    delay_ns(5000);

    putreg32(1, HB8CLK_EN);

    /* delay 1.5us or more */
    delay_ns(1500);

    putreg32(0, ISO_FOR_IO_EN);

    putreg32(0, BOOTRET_O);

    chip_init();

    dbginit();

    dbgprint("Resumed from standby\n");
#ifdef _STANDBY_WAIT_FOR_SERVER
    chip_exit_hibern8_client();
#endif
}
