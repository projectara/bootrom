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

#include "chip.h"
#include "chipapi.h"
#include "debug.h"
#include "tsb_scm.h"

#ifdef _HANDSHAKE
/* See: T6WU0XBG-0001 APBridge Functional Specification for pinshare bits */
#define GPIO_REQ 16
#define GPIO_RESP 17
#define GPIO_TEST_STATUS 18
#define GPIO_TEST_STATUS_OK 0
#define GPIO_TEST_STATUS_FAIL 1
#define HANDSHAKE_GPIO_CLR_BITS (1 << 4)
#define HANDSHAKE_GPIO_SET_BITS (1 << 10) | (1 << 11)
#endif

void chip_init(void) {
#ifdef BOOT_FROM_SLOW_ROM

    #define ARMV7M_NVIC_BASE    0xe000e000
    #define NVIC_VECTAB_OFFSET  0x0d08 /* Vector table offset register */
    #define NVIC_VECTAB         (ARMV7M_NVIC_BASE + NVIC_VECTAB_OFFSET)

    extern uint32_t _vectors[32];

    putreg32((uint32_t)&_vectors[0], NVIC_VECTAB);
#endif
    /* Configure clocks */
    tsb_clk_init();
#ifdef CONFIG_GPIO
    chip_gpio_init();
#endif
#ifdef _HANDSHAKE
    tsb_clr_pinshare(HANDSHAKE_GPIO_CLR_BITS);
    tsb_set_pinshare(HANDSHAKE_GPIO_SET_BITS);
    chip_gpio_direction_in(GPIO_RESP);
    chip_gpio_direction_out(GPIO_REQ, 0);
    chip_gpio_direction_out(GPIO_TEST_STATUS, GPIO_TEST_STATUS_OK);
#endif

#ifdef CONFIG_BRIDGED_SPI
    chip_spi_master_init();
#endif
}

extern char _workram_start;
extern char _bootrom_data_area, _bootrom_text_area;
int chip_validate_data_load_location(void *base, uint32_t length) {
    if ((uint32_t)base < (uint32_t)&_workram_start) {
        return -1;
    }
#if (CONFIG_CHIP_REVISION >= CHIP_REVISION_ES3) && (BOOT_STAGE == 1)
    if ((uint32_t)base + length >= (uint32_t)&_bootrom_data_area) {
#else
    if ((uint32_t)base + length >= (uint32_t)&_bootrom_text_area) {
#endif
        return -1;
    }
    return 0;
}

#ifdef _HANDSHAKE
 /**
  * @brief Perform a handshake with the external simulation controller
  *
  * @param none
  *
  * @returns Nothing.
  */
void chip_handshake_with_test_controller(void) {
    dbgflush();
    while (chip_gpio_get_value(GPIO_RESP) != 0);
    chip_gpio_set_value(GPIO_REQ, 1);
    while (chip_gpio_get_value(GPIO_RESP) == 0);
    chip_gpio_set_value(GPIO_REQ, 0);
    while (chip_gpio_get_value(GPIO_RESP) != 0);
}

/**
 * @brief Indicate success/failure to external simulation controller
 *
 * Indicate test pass/continue or fail/halt by setting GPIO18 to
 * 0 or 1 (respectively) and execute the handshake on GPIO16,17.
 *
 * @param status Indicate if the test was successful and/or should continue
 *               if the status is zero. Non-zero status indicates a failure
 *               and the test should stop.
 *
 * @returns Nothing.
 */
void chip_handshake_boot_status(uint32_t status) {
    chip_gpio_set_value(GPIO_TEST_STATUS, status?
            GPIO_TEST_STATUS_FAIL : GPIO_TEST_STATUS_OK);
    chip_handshake_with_test_controller();
}
#endif
