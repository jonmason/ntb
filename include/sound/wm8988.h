/*
 * wm8988.h  --  WM8988 Soc Audio driver platform data
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _WM8988_PDATA_H
#define _WM8988_PDATA_H

struct wm8988_data {
	bool capless;  /* Headphone outputs configured in capless mode */
	bool shared_lrclk;  /* DAC and ADC LRCLKs are wired together */
};

#endif
