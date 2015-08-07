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
#include "tsb_scm.h"

#define UART_RBR_THR_DLL    (UART_BASE + 0x0)
#define UART_IER_DLH        (UART_BASE + 0x4)
#define UART_FCR_IIR        (UART_BASE + 0x8)
#define UART_LCR            (UART_BASE + 0xc)
#define UART_LSR            (UART_BASE + 0x14)

#define UART_DLL     ((UART_CLOCK_DIVIDER >> 0) & 0xff)
#define UART_DLH     ((UART_CLOCK_DIVIDER >> 8) & 0xff)
#define UART_LCR_DLAB  (0x1 << 7) /* Divisor latch */
#define UART_LCR_DLS_8 (0x3 << 0) /* 8 bit */

#define UART_FCR_IIR_IID0_FIFOE  (1 << 0) /* FIFO Enable */
#define UART_FCR_IIR_IID1_RFIFOR (1 << 1) /* FIFO RX Reset */
#define UART_FCR_IIR_IID1_XFIFOR (1 << 2) /* FIFO RX Reset */

#define UART_LSR_THRE (0x1 << 5)
#define UART_LSR_TX_EMPTY (0x1 << 6)

/**
 * @brief Initialize the debug serial port
 *
 * @param None
 *
 * @returns Nothing
 */
void chip_dbginit(void) {
    int i;

    /* TA-18 Operate UART RX and TX at 115200Hz */
    /* enable UART RX/TX pins */
    tsb_set_pinshare(TSB_PIN_UART_RXTX);

    /* enable UART clocks */
    tsb_clk_enable(TSB_CLK_UARTP);
    tsb_clk_enable(TSB_CLK_UARTS);

    /* reset UART module */
    tsb_reset(TSB_RST_UARTP);
    tsb_reset(TSB_RST_UARTS);

    /*
     * The controller requires "several cycles" after reset to stabilize before
     * register writes will work. Try this a few times.
     *
     * And the LORD spake, saying, "First shalt thou take out the Holy Pin,
     * then shalt thou count to three, no more, no less. Three shall be the
     * number thou shalt count, and the number of the counting shall be three.
     * Four shalt thou not count, neither count thou two, excepting that thou
     * then proceed to three. Five is right out. Once the number three, being
     * the third number, be reached, then lobbest thou thy Holy Hand Grenade
     * of Antioch towards thy foe, who being naughty in My sight, shall snuff
     * it.
     *      -The Book of Armaments
     */
    for (i = 0; i < 3; i++) {
        putreg32(UART_LCR_DLAB | UART_LCR_DLS_8, UART_LCR);
        putreg32(UART_DLL, UART_RBR_THR_DLL);
        putreg32(UART_DLH, UART_IER_DLH);
        putreg32(UART_LCR_DLS_8, UART_LCR);
        putreg32(0x0, UART_IER_DLH);
        putreg32(UART_FCR_IIR_IID0_FIFOE | UART_FCR_IIR_IID1_RFIFOR |
                 UART_FCR_IIR_IID1_XFIFOR, UART_FCR_IIR);
    }
}

/**
 * @brief Print a character out the debug serial port
 *
 * @param c The character to display
 *
 * @returns Nothing
 */
void chip_dbgputc(int c) {
    while ((getreg32(UART_LSR) & UART_LSR_THRE) != UART_LSR_THRE)
        ;

    putreg32(c, UART_RBR_THR_DLL);
}

/**
 * @brief Flush the debug serial port
 *
 * @param Nothing
 *
 * @returns Nothing
 */
void chip_dbgflush(void) {
    while ((getreg32(UART_LSR) & UART_LSR_TX_EMPTY) != UART_LSR_TX_EMPTY)
        ;
}
