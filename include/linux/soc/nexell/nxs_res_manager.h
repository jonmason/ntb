/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#ifndef _NXS_RES_MANAGER_H
#define _NXS_RES_MANAGER_H

struct nxs_dev;
struct list_head;

int  register_nxs_dev(struct nxs_dev *);
void unregister_nxs_dev(struct nxs_dev *);
struct nxs_function *request_nxs_function(const char *name,
					  struct nxs_function_request *f,
					  bool use_builder);
void remove_nxs_function(struct nxs_function *f, bool use_builder);
struct nxs_dev *get_nxs_dev(u32 function, u32 index, u32 user,
			    bool multitap_follow);
void put_nxs_dev(struct nxs_dev *);
int register_nxs_function_builder(struct nxs_function_builder *);
void unregister_nxs_function_builder(struct nxs_function_builder *);
void mark_multitap_follow(struct list_head *);

#endif
