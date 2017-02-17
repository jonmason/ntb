/*
 * Header file for the Nexell tieoff control
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SOC_NEXELL_TIEOFF_H__
#define __SOC_NEXELL_TIEOFF_H__

void nx_tieoff_set(u32 tieoff_index, u32 tieoff_value);
u32 nx_tieoff_get(u32 tieoff_index);

#endif /* __SOC_NEXELL_TIEOFF_H__ */
