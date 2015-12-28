/*
 * Copyright (c) 2015 Google, Inc.
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
 * This file implements SPI device driver interface, to be used by Bridged-PHY
 * SPI over Greybus.
 * It is different from the es3_spi.c, which implements the interface defined
 * in data_loading.h
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "appcfg.h"
#include "chip.h"
#include "debug.h"
#include "nuttx_dev_if.h"
#include "device_spi.h"
#include "spi-gb.h"
#include "utils.h"
#include "tsb_scm.h"

/* only support 1 device on CS0 */
#define CONFIG_SPI_MAX_CHIPS 1
#define CS0_GPIO_PINS_BIT BIT(7)
#define CS0_GPIO 15

/* only support 8 bits-per-word mode */
#define CONFIG_SPI_BPW_MASK BIT(7)

#define CONFIG_SPI_MIN_FREQ 15000
#define CONFIG_SPI_MAX_FREQ 24000000
#define CONFIG_SPI_MAX_DIV 0xFFFF

#define DW_SPI_CTRLR0   0x00
#define DW_SPI_CTRLR1   0x04
#define DW_SPI_SSIENR   0x08
#define DW_SPI_SER      0x10
#define DW_SPI_BAUDR    0x14
#define DW_SPI_TXFTLR   0x18
#define DW_SPI_RXFTLR   0x1C
#define DW_SPI_TXFLR    0x20
#define DW_SPI_RXFLR    0x24
#define DW_SPI_SR       0x28
#define DW_SPI_IMR      0x2C
#define DW_SPI_ISR      0x30
#define DW_SPI_DMACR    0x4C
#define DW_SPI_DMATDLR  0x50
#define DW_SPI_DMARDLR  0x54
#define DW_SPI_DR       0x60

/** bit for DW_SPI_CTRLR0 */
#define SPI_CTRLR0_SCPH         BIT(6)
#define SPI_CTRLR0_SCPOL        BIT(7)
#define SPI_CTRLR0_TMOD_TX      BIT(8)
#define SPI_CTRLR0_TMOD_RX      BIT(9)
#define SPI_CTRL0_TMOD_MASK     (0x03 << 8)
#define SPI_CTRLR0_DFS32_OFFSET 16
#define SPI_CTRLR0_DFS32_MASK   (0x1f << SPI_CTRLR0_DFS32_OFFSET)

/** bit for DW_SPI_SR */
#define SPI_SR_TFNF_MASK    BIT(1)
#define SPI_SR_TFE_MASK     BIT(2)
#define SPI_SR_RFNE_MASK    BIT(3)
#define SPI_SR_RFF_MASK     BIT(4)

/** bit for DW_SPI_IMR */
#define SPI_IMR_TXOIM_MASK  BIT(1)
#define SPI_IMR_RXFIM_MASK  BIT(4)

/** bit for DW_SPIISR */
#define SPI_ISR_RXFIS_MASK  BIT(4)

#define FSSI_CLK            48000000

/**
 * NOTE: The name of the device matters.
 * If the name of the device is set to "spidev", Linux kernel will treat
 * it as plain SPI device. In this case "flashrom" on AP can be used to
 * program the SPI ROM here.
 * When the name is set to "w25q16dw" (or some other flash part name that
 * kernel recognizes), Linux kernel will treat it as MTD device. In this
 * case "flashcp" on AP can be used to program the SPI ROM here
 */
static struct device_spi_cfg chips_info[CONFIG_SPI_MAX_CHIPS] = {
    {0, 8, CONFIG_SPI_MAX_FREQ, "w25q16dw"},
};

extern int spi_clk_usage_count;
/**
 * @brief private SPI device information
 */
static struct tsb_spi_info {
    /** SPI device base address */
    uint32_t reg_base;

    /** struct for SPI controller capability store */
    struct master_spi_caps caps;

    /** struct for chips configuration store */
    struct device_spi_cfg *dev_cfg;
} spi_info;

/**
 * @brief Read value from register.
 *
 * This function returns register content by basic register read function.
 *
 * @param base Base address of this Controller.
 * @param addr Specific register of offset.
 *
 * @return Returns content for a specific register.
 */
static uint32_t tsb_spi_read(uint32_t base, uint32_t addr)
{
    return getreg32(base + addr);
}

/**
 * @brief Write value to register.
 *
 * This function write value to register by basic register write function.
 *
 * @param base Base address of this Controller.
 * @param addr Specific register of offset.
 * @param val The content will be write for Specific register.
 */
static void tsb_spi_write(uint32_t base, uint32_t addr, uint32_t val)
{
    putreg32(val, base + addr);
}

static int es3_spi_select(struct device *dev, uint8_t devid) {
    struct tsb_spi_info *info = &spi_info;

    if (info->caps.csnum <= devid) {
        return -EINVAL;
    }

    /**
     * The SPI master controls the CS line differently than expected.
     * It releases CS whenever there is no data in the TX DR.
     * However we do expect the CS to remain active between different
     * decriptors in a single transfer here.
     * We can try to figure out how to configure the SPI master to do so,
     * but in the meantime, we can also control the pin as a GPIO directly.
     */
    tsb_set_pinshare(CS0_GPIO_PINS_BIT);
    chip_gpio_direction_out(CS0_GPIO, 0);
    return 0;
}

static int es3_spi_deselect(struct device *dev, uint8_t devid) {
    struct tsb_spi_info *info = &spi_info;

    if (info->caps.csnum <= devid) {
        return -EINVAL;
    }

    /**
     * The SPI master controls the CS line differently than expected.
     * It releases CS whenever there is no data in the TX DR.
     * However we do expect the CS to remain active between different
     * decriptors in a single transfer here.
     * We can try to figure out how to configure the SPI master to do so,
     * but in the meantime, we can also control the pin as a GPIO directly.
     */
    chip_gpio_direction_out(CS0_GPIO, 1);
    tsb_clr_pinshare(CS0_GPIO_PINS_BIT);
    return 0;
}

static int es3_spi_setfrequency(struct device *dev,
                                uint8_t cs,
                                uint32_t *frequency) {
    struct tsb_spi_info *info = &spi_info;
    uint32_t freq;
    uint32_t div;

    if (info->caps.csnum <= cs) {
        return -EINVAL;
    }

    freq = *frequency;

    if (freq < CONFIG_SPI_MIN_FREQ || freq > info->dev_cfg[cs].max_speed_hz ||
        freq == 0) {
        return -EINVAL;
    }

    div = FSSI_CLK / freq;

    if (div > CONFIG_SPI_MAX_DIV) {
        return -EINVAL;
    }
    tsb_spi_write(info->reg_base, DW_SPI_BAUDR, div);
    return 0;
}

static int es3_spi_setmode(struct device *dev, uint8_t cs, uint8_t mode) {
    struct tsb_spi_info *info = &spi_info;
    uint32_t ctrl0 = 0;

    if (info->caps.csnum <= cs) {
        return -EINVAL;
    }
    /* check mode whether supported */
    if (info->dev_cfg[cs].mode & ~mode) {
        return -EINVAL;
    }

    ctrl0 = tsb_spi_read(info->reg_base, DW_SPI_CTRLR0);

    /* run in TMOD=00 */
    ctrl0 &= ~SPI_CTRL0_TMOD_MASK;

    if (mode & SPI_MODE_CPHA)
        ctrl0 |= SPI_CTRLR0_SCPH;
    else
        ctrl0 &= ~SPI_CTRLR0_SCPH;

    if (mode & SPI_MODE_CPOL)
        ctrl0 |= SPI_CTRLR0_SCPOL;
    else
        ctrl0 &= ~SPI_CTRLR0_SCPOL;

    tsb_spi_write(info->reg_base, DW_SPI_CTRLR0, ctrl0);
    return 0;
}

static int es3_spi_setbits(struct device *dev, uint8_t cs, uint8_t nbits) {
    struct tsb_spi_info *info = &spi_info;
    uint32_t ctrl0 = 0;

    if (info->caps.csnum <= cs) {
        return -EINVAL;
    }
    if (info->dev_cfg[cs].bpw != nbits) {
        return -EINVAL;
    }

    ctrl0 = tsb_spi_read(info->reg_base, DW_SPI_CTRLR0);
    ctrl0 &= ~SPI_CTRLR0_DFS32_MASK;
    ctrl0 |= ((nbits - 1) << SPI_CTRLR0_DFS32_OFFSET);

    tsb_spi_write(info->reg_base, DW_SPI_CTRLR0, ctrl0);

    return 0;
}

static int es3_spi_exchange(struct device *dev,
                     struct device_spi_transfer *transfer) {
    struct tsb_spi_info *info = &spi_info;
    uint32_t u32_dr, sr_reg;
    uint8_t *txbuf = NULL;
    uint8_t *rxbuf = NULL;
    uint32_t txcnt = 0, rxcnt = 0;
    uint8_t u8_dr;

    /* check transfer buffer */
    if (!transfer->txbuffer && !transfer->rxbuffer) {
        return -EINVAL;
    }

    tsb_spi_write(info->reg_base, DW_SPI_SSIENR, 1);
    txbuf = transfer->txbuffer;
    rxbuf = transfer->rxbuffer;

    /**
     * When TMOD is set to 0, as here the code is doing,
     * TX and RX are always working, and the CLK is only active when there
     * is data to be shifted out.
     * So we need to write and read the same amount of data, whether the
     * txbuf or rxbuf is NULL or not. The only difference is if the data read
     * from RX is stored in rxbuf or discarded
     */
    while((txcnt < transfer->nwords) || (rxcnt < transfer->nwords)) {
        sr_reg = tsb_spi_read(info->reg_base, DW_SPI_SR);

        if (txcnt < transfer->nwords &&
            (sr_reg & SPI_SR_TFNF_MASK) &&
            !(sr_reg & SPI_SR_RFF_MASK)) {
            u8_dr = (txbuf) ? *(txbuf++) : 0;
            tsb_spi_write(info->reg_base, DW_SPI_DR, u8_dr);
            txcnt++;
        }

        if (rxcnt < transfer->nwords && (sr_reg & SPI_SR_RFNE_MASK)) {
            u32_dr = tsb_spi_read(info->reg_base, DW_SPI_DR);
            if (rxbuf) {
                *(rxbuf++) = (uint8_t)(u32_dr & 0xFF);
            }
            rxcnt++;
        }
    }
    tsb_spi_write(info->reg_base, DW_SPI_SSIENR, 0);
    return 0;
}

static int es3_get_master_caps(struct device *dev,
                               struct master_spi_caps *caps) {
    struct tsb_spi_info *info = &spi_info;

    caps->modes = info->caps.modes;
    caps->flags = info->caps.flags;
    caps->bpw = info->caps.bpw;
    caps->csnum = info->caps.csnum;
    caps->min_speed_hz = info->caps.min_speed_hz;
    caps->max_speed_hz = info->caps.max_speed_hz;
    return 0;
}

static int es3_get_device_cfg(struct device *dev,
                              uint8_t cs,
                              struct device_spi_cfg *dev_cfg) {
    struct tsb_spi_info *info = &spi_info;

    dev_cfg->mode = info->dev_cfg[cs].mode;
    dev_cfg->bpw = info->dev_cfg[cs].bpw;
    dev_cfg->max_speed_hz = info->dev_cfg[cs].max_speed_hz;
    memcpy(dev_cfg->name, &info->dev_cfg[cs].name,
           sizeof(info->dev_cfg[cs].name));
    return 0;
}

static struct device_spi_type_ops es3_spi_ops = {
    .lock = NULL,      /* not used in this project */
    .unlock = NULL,    /* not used in this project */
    .select = es3_spi_select,
    .deselect = es3_spi_deselect,
    .setfrequency = es3_spi_setfrequency,
    .setmode = es3_spi_setmode,
    .setbits = es3_spi_setbits,
    .exchange = es3_spi_exchange,
    .get_master_caps = es3_get_master_caps,
    .get_device_cfg = es3_get_device_cfg
};

void chip_spi_master_init(void) {
    struct tsb_spi_info *info = &spi_info;

    info->reg_base = SPI_BASE;

    /* Set Capability */
    info->caps.csnum = CONFIG_SPI_MAX_CHIPS;
    info->caps.modes = (SPI_MODE_CPHA | SPI_MODE_CPOL);

    info->caps.bpw = CONFIG_SPI_BPW_MASK;

    info->caps.min_speed_hz = CONFIG_SPI_MIN_FREQ;
    info->caps.max_speed_hz = CONFIG_SPI_MAX_FREQ;

    info->dev_cfg = chips_info;

    /* initialize the HW */
    tsb_clk_enable(TSB_CLK_SPIP);
    tsb_clk_enable(TSB_CLK_SPIS);
    spi_clk_usage_count++;

    /* register device to greybus */
    retister_spi_device((void *)&es3_spi_ops);
}
