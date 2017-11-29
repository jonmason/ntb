/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongkeun, Choi <jkchoi@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NX_REARCAM_VENDOR_H__
#define __NX_REARCAM_VENDOR_H__

void nx_rearcam_set_enable(void *, bool);
void nx_rearcam_sensor_init_func(struct i2c_client *client);
void *nx_rearcam_alloc_vendor_context(void *priv, struct device *dev);
bool nx_rearcam_pre_turn_on(void *);
void nx_rearcam_post_turn_off(void *);
void nx_rearcam_free_vendor_context(void *);
bool nx_rearcam_decide(void *);
void nx_rearcam_draw_rgb_overlay(int, int, int, int, void *, void *);
void nx_rearcam_draw_parking_guide_line(void *, void *, int, int , int, int);
#endif
