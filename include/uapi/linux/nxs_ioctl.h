#ifndef _UAPI__NXS_IOCTL_H_
#define _UAPI__NXS_IOCTL_H_

#include <linux/types.h>

#define NXS_MAX_FUNCTION_COUNT	16
#define NXS_MAX_REQ_NAME_SIZE	32
struct nxs_request_function {
	char name[NXS_MAX_REQ_NAME_SIZE];
	int count;
	uint32_t array[NXS_MAX_FUNCTION_COUNT];
	/* for multitap */
	int sibling_handle;
	int handle;
};

enum {
	NXS_FUNCTION_QUERY_INVALID = 0,
	NXS_FUNCTION_QUERY_DEVINFO = 1,
	NXS_FUNCTION_QUERY_MAX
};

#define NXS_DEV_NAME_MAX	64
struct nxs_function_devinfo {
	char dev_name[NXS_DEV_NAME_MAX];
};

struct nxs_query_function {
	int handle;
	int query;
	union {
		struct nxs_function_devinfo devinfo;
	};
};

struct nxs_remove_function {
	int handle;
};

#define NXS_IOCTL_BASE		'U'
#define NXS_REQUEST_FUNCTION	_IOWR(NXS_IOCTL_BASE, 1, \
				      struct nxs_request_function)
#define NXS_REMOVE_FUNCTION	_IOW(NXS_IOCTL_BASE, 2, \
				     struct nxs_remove_function)
#define NXS_QUERY_FUNCTION	_IOWR(NXS_IOCTL_BASE, 3, \
				      struct nxs_query_function)

#endif
