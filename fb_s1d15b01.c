/*
 * FB driver for the s1d15b01 LCD Controller
 *
 * The display is monochrome and the video memory is RGB565.
 * Any pixel value except 0 turns the pixel on.
 *
 * Copyright (C) 2013 Noralf Tronnes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include "fbtft.h"

#define DRVNAME        "fb_s1d15b01"
#define WIDTH          132
#define HEIGHT         65
#define PAGES     (HEIGHT/8)

static int init_display(struct fbtft_par *par)
{
        fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

        par->fbtftops.reset(par);

        gpio_set_value(par->gpio.reset, 0);     // HW reset
        gpio_set_value(par->gpio.reset, 1);
        mdelay(10);
        write_reg(par, 0xE2);                   // Soft reset
        mdelay(10);
        write_reg(par, 0xA6);                   // lcd in mod normal (0xA7 pt. negativ)
        write_reg(par, 0xA1);                   // A1 A0 column norm/reverse
        write_reg(par, 0xC0);                   // C0 C8 line norm/reverse
        write_reg(par, 0xA3);                   // bias a2 a3 1/9bias, 1/7bias
        write_reg(par, 0x24);                   // setare tensiune V5
        write_reg(par, 0x80);                   // mod setare volum electric
        write_reg(par, 0x2F);                   // mod booster+stabilizator tensiune on
        write_reg(par, 0xAF);                   // lcd on (0xAE pt. lcd off)
        write_reg(par, 0xA5);                   // lcd all on
        write_reg(par, 0xA4);                   // lcd normal

        return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
        fbtft_par_dbg(DEBUG_SET_ADDR_WIN, par, "%s(xs=%d, ys=%d, xe=%d, ye=%d)\n", __func__, xs, ys, xe, ye);

        write_reg(par, 0x12 & 0x0f);
        write_reg(par, ((0x12 >> 4) & 0x0f) | 0x10);
        write_reg(par, 0xb0 | (0 & 0x07));
}

static int write_vmem(struct fbtft_par *par, size_t offset, size_t len)
{
        u16 *vmem16 = (u16 *)par->info->screen_base;
        u8 *buf = par->txbuf.buf;
        int x, y, i;
        int ret = 0;

        fbtft_par_dbg(DEBUG_WRITE_VMEM, par, "%s()\n", __func__);

        for (y = 0; y < PAGES; y++) {
                buf = par->txbuf.buf;
                for (x = 0; x < WIDTH; x++) {
                        *buf = 0x00;
                        for (i = 0; i < 8; i++)
                                *buf |= (vmem16[((y*8*WIDTH)+(i*WIDTH))+x] ? 1 : 0) << i;
                        buf++;
                }
                write_reg(par, 0x12 & 0x0f);
                write_reg(par, ((0x12 >> 4) & 0x0f) | 0x10);
                write_reg(par, 0xb0 | (y & 0x07));

                gpio_set_value(par->gpio.dc, 1);
                ret = par->fbtftops.write(par, par->txbuf.buf, WIDTH);
                gpio_set_value(par->gpio.dc, 0);
        }

        if (ret < 0)
                dev_err(par->info->device, "%s: write failed and returned: %d\n", __func__, ret);

        return ret;
}

static struct fbtft_display display = {
        .regwidth = 8,
        .width = WIDTH,
        .height = HEIGHT,
        .fbtftops = {
                .init_display = init_display,
                .set_addr_win = set_addr_win,
                .write_vmem = write_vmem,
        },
        .backlight = 1,
};
FBTFT_REGISTER_DRIVER(DRVNAME, &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("spi:s1d15b01");

MODULE_DESCRIPTION("FB driver for the s1d15b01 LCD Controller");
MODULE_AUTHOR("Manchuk Oleksii");
MODULE_LICENSE("GPL");
