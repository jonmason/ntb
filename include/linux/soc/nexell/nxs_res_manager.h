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

#define NXS_FUNCTION_ANY	255
enum {
	NXS_FUNCTION_USER_NONE	 = 0,
	NXS_FUNCTION_USER_KERNEL = 1,
	NXS_FUNCTION_USER_APP,
};

struct nxs_function {
	struct list_head list;
	u32 function;
	u32 index;
	u32 user;
	bool multitap_follow;
};

struct nxs_function_request {
	int nums_of_function;
	struct list_head head;
};

struct nxs_function_instance {
	struct list_head dev_list;
	/* for multitap */
	struct list_head dev_sibling_list;
	struct list_head sibling_list;
	struct nxs_function_request *req;
	int type;
	void *priv;
	u32 id;
};

struct nxs_query_function;
struct nxs_function_builder {
	void *priv;
	struct nxs_function_instance *
		(*build)(struct nxs_function_builder *pthis,
			 const char *name,
			 struct nxs_function_request *);
	int (*free)(struct nxs_function_builder *pthis,
		    struct nxs_function_instance *);
	struct nxs_function_instance *
		(*get)(struct nxs_function_builder *pthis,
		       int handle);
	int (*query)(struct nxs_function_builder *pthis,
		     struct nxs_query_function *query);
};

int  register_nxs_dev(struct nxs_dev *);
void unregister_nxs_dev(struct nxs_dev *);
struct nxs_function_instance *
request_nxs_function(const char *, struct nxs_function_request *);
void remove_nxs_function(struct nxs_function_instance *);
struct nxs_dev *get_nxs_dev(u32 function, u32 index, u32 user,
			    bool multitap_follow);
void put_nxs_dev(struct nxs_dev *);
int register_nxs_function_builder(struct nxs_function_builder *);
void unregister_nxs_function_builder(struct nxs_function_builder *);
void mark_multitap_follow(struct list_head *);
void free_function_request(struct nxs_function_request *req);

#endif
