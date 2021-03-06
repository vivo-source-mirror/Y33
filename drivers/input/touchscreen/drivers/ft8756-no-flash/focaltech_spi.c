/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2012-2019, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/************************************************************************
*
* File Name: focaltech_spi.c
*
*    Author: FocalTech Driver Team
*
*   Created: 2019-03-21
*
*  Abstract: new spi protocol communication with TP
*
*   Version: v1.0
*
* Revision History:
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "focaltech_core.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define SPI_RETRY_NUMBER            3
#define CS_HIGH_DELAY               150 /* unit: us */
#define SPI_BUF_LENGTH              256

#define DATA_CRC_EN                 0x20
#define WRITE_CMD                   0x00
#define READ_CMD                    (0x80 | DATA_CRC_EN)

#define SPI_DUMMY_BYTE              3
#define SPI_HEADER_LENGTH           (SPI_DUMMY_BYTE + 6)

#define MTK_SPI_PACKET_SIZE         1024
/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/

/*****************************************************************************
* Static variables
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

/*****************************************************************************
* functions body
*****************************************************************************/
/* spi interface */
static int fts_spi_transfer(u8 *tx_buf, u8 *rx_buf, u32 len)
{
    int ret = 0;
    struct spi_device *spi = fts8756_fts_data->spi;
    struct spi_message msg;
    struct spi_transfer xfer[2];
    u32 packet_length = 0;
    u32 packet_remainder = 0;

    if (!spi || !tx_buf || !rx_buf || !len) {
        FTS_ERROR("spi_device/tx_buf/rx_buf/len(%d) is invalid", len);
        return -EINVAL;
    }

    memset(&xfer[0], 0, sizeof(struct spi_transfer));
    memset(&xfer[1], 0, sizeof(struct spi_transfer));

    spi_message_init(&msg);
    packet_length = len - packet_remainder;
    xfer[0].tx_buf = &tx_buf[0];
    xfer[0].rx_buf = &rx_buf[0];
    xfer[0].len = packet_length;
    spi_message_add_tail(&xfer[0], &msg);

    ret = spi_sync(spi, &msg);
    if (ret) {
        FTS_ERROR("spi_sync fail,ret:%d", ret);
        return ret;
    }

    return ret;
}

static void crckermit(u8 *data, u16 len, u16 *crc_out)
{
    u16 i = 0;
    u16 j = 0;
    u16 crc = 0xFFFF;

    for ( i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x01)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc = (crc >> 1);
        }
    }

    *crc_out = crc;
}

static int rdata_check(u8 *rdata, u32 rlen)
{
    u16 crc_calc = 0;
    u16 crc_read = 0;

    crckermit(rdata, rlen - 2, &crc_calc);
    crc_read = (u16)(rdata[rlen - 1] << 8) + rdata[rlen - 2];
    if (crc_calc != crc_read) {
        return -EIO;
    }

    return 0;
}

int fts8756_fts_write(u8 *writebuf, u32 writelen)
{
    int ret = 0;
    int i = 0;
    struct fts_ts_data *ts_data = fts8756_fts_data;
    u8 *txbuf = NULL;
    u8 *rxbuf = NULL;
    u32 txlen = 0;
    u32 txlen_need = writelen + SPI_HEADER_LENGTH;
    u32 datalen = writelen - 1;

    if (!writebuf || !writelen) {
        FTS_ERROR("writebuf/len is invalid");
        return -EINVAL;
    }

    mutex_lock(&ts_data->spilock);
    if (txlen_need > SPI_BUF_LENGTH) {
        txbuf = kzalloc(txlen_need, GFP_KERNEL|GFP_DMA);
        if (NULL == txbuf) {
            FTS_ERROR("txbuf malloc fail");
            ret = -ENOMEM;
            goto err_write;
        }

        rxbuf = kzalloc(txlen_need, GFP_KERNEL|GFP_DMA);
        if (NULL == rxbuf) {
            FTS_ERROR("rxbuf malloc fail");
            ret = -ENOMEM;
            goto err_write;
        }
    } else {
        txbuf = ts_data->bus_tx_buf;
        rxbuf = ts_data->bus_rx_buf;
        memset(txbuf, 0x0, SPI_BUF_LENGTH);
        memset(rxbuf, 0x0, SPI_BUF_LENGTH);
    }

    txbuf[txlen++] = writebuf[0];
    txbuf[txlen++] = WRITE_CMD;
    txbuf[txlen++] = (datalen >> 8) & 0xFF;
    txbuf[txlen++] = datalen & 0xFF;
    if (datalen > 0) {
        txlen = txlen + SPI_DUMMY_BYTE;
        memcpy(&txbuf[txlen], &writebuf[1], datalen);
        txlen = txlen + datalen;
    }

    for (i = 0; i < SPI_RETRY_NUMBER; i++) {
        ret = fts_spi_transfer(txbuf, rxbuf, txlen);
        if ((0 == ret) && ((rxbuf[3] & 0xA0) == 0)) {
            break;
        } else {
            FTS_DEBUG("data write(status=%x),retry=%d,ret=%d",
                      rxbuf[3], i, ret);
            ret = -EIO;
            udelay(CS_HIGH_DELAY);
        }
    }

err_write:
    if (txlen_need > SPI_BUF_LENGTH) {
        if (txbuf) {
            kfree(txbuf);
            txbuf = NULL;
        }

        if (rxbuf) {
            kfree(rxbuf);
            rxbuf = NULL;
        }
    }
    udelay(CS_HIGH_DELAY);
    mutex_unlock(&ts_data->spilock);
	
    return ret;
}

int fts8756_fts8756_fts_write_reg_byte(u8 addr, u8 value)
{
    u8 writebuf[2] = { 0 };

    writebuf[0] = addr;
    writebuf[1] = value;
    return fts8756_fts_write(writebuf, 2);
}

int fts8756_fts_read(u8 *cmd, u32 cmdlen, u8 *data, u32 datalen)
{
    int ret = 0;
    int i = 0;
    struct fts_ts_data *ts_data = fts8756_fts_data;
    u8 *txbuf = NULL;
    u8 *rxbuf = NULL;
    u32 txlen = 0;
    u32 txlen_need = datalen + SPI_HEADER_LENGTH;
    u8 ctrl = READ_CMD;
    u32 dp = 0;

    if (!cmd || !cmdlen || !data || !datalen) {
        FTS_ERROR("cmd/cmdlen/data/datalen is invalid");
        return -EINVAL;
    }

    mutex_lock(&ts_data->spilock);
    if (txlen_need > SPI_BUF_LENGTH) {
        txbuf = kzalloc(txlen_need, GFP_KERNEL|GFP_DMA);
        if (NULL == txbuf) {
            FTS_ERROR("txbuf malloc fail");
            ret = -ENOMEM;
            goto err_read;
        }

        rxbuf = kzalloc(txlen_need, GFP_KERNEL|GFP_DMA);
        if (NULL == rxbuf) {
            FTS_ERROR("rxbuf malloc fail");
            ret = -ENOMEM;
            goto err_read;
        }
    } else {
        txbuf = ts_data->bus_tx_buf;
        rxbuf = ts_data->bus_rx_buf;
        memset(txbuf, 0x0, SPI_BUF_LENGTH);
        memset(rxbuf, 0x0, SPI_BUF_LENGTH);
    }

    txbuf[txlen++] = cmd[0];
    txbuf[txlen++] = ctrl;
    txbuf[txlen++] = (datalen >> 8) & 0xFF;
    txbuf[txlen++] = datalen & 0xFF;
    dp = txlen + SPI_DUMMY_BYTE;
    txlen = dp + datalen;
    if (ctrl & DATA_CRC_EN) {
        txlen = txlen + 2;
    }

    for (i = 0; i < SPI_RETRY_NUMBER; i++) {
        ret = fts_spi_transfer(txbuf, rxbuf, txlen);
        if ((0 == ret) && ((rxbuf[3] & 0xA0) == 0)) {
            memcpy(data, &rxbuf[dp], datalen);
            /* crc check */
            if (ctrl & DATA_CRC_EN) {
                ret = rdata_check(&rxbuf[dp], txlen - dp);
                if (ret < 0) {
                    FTS_INFO("read data(addr:%x) crc check incorrect", cmd[0]);
                    vts_communication_abnormal_collect(TOUCH_VCODE_I2C_EVENT);
                    goto err_read;
                }
            }
            break;
        } else {
            FTS_DEBUG("data read(status=%x),retry=%d,ret=%d",
                      rxbuf[3], i, ret);
            ret = -EIO;
            udelay(CS_HIGH_DELAY);
        }
    }

err_read:
    if (txlen_need > SPI_BUF_LENGTH) {
        if (txbuf) {
            kfree(txbuf);
            txbuf = NULL;
        }

        if (rxbuf) {
            kfree(rxbuf);
            rxbuf = NULL;
        }
    }
	udelay(CS_HIGH_DELAY);
    mutex_unlock(&ts_data->spilock);
	
    return ret;
}

int fts8756_fts8756_fts_read_reg_byte(u8 addr, u8 *value)
{
    return fts8756_fts_read(&addr, 1, value, 1);
}

int fts8756_write_then_read(u8 addr, u8 value)
{
	int ret = 0;
	u8 temp_data = 0;

	ret = fts8756_fts8756_fts_write_reg_byte(addr, value);
	if (ret < 0)
		VTI("fail to wirte 0x%x register value: %d", addr, value);

	mdelay(5);
	ret = fts8756_fts8756_fts_read_reg_byte(addr, &temp_data);
	if (ret < 0)
		VTI("fail to read 0x%x register value: %d", addr, temp_data);

	if (temp_data != value) {
		VTI("Fail to write 0x%x value: %d", addr, value);
		ret = -1;
	} else {
		VTI("Success to write 0x%x value: %d", addr, value);
		ret = 0;
	}
	return ret;
}
