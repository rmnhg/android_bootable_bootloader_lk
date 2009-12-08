/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <dev/keys.h>
#include <dev/gpio_keypad.h>
#include <lib/ptable.h>
#include <dev/flash.h>
#include <smem.h>

#define LINUX_MACHTYPE_SURF  1007016
#define LINUX_MACHTYPE_FFA   1007017
#define LINUX_MACHTYPE_FLUID 1007018

//Enum values for 7x30 target platforms.
enum platform
{
    HW_PLATFORM_UNKNOWN = 0,
    HW_PLATFORM_SURF    = 1,
    HW_PLATFORM_FFA     = 2,
    HW_PLATFORM_FLUID   = 3,
    HW_PLATFORM_32BITS  = 0x7FFFFFFF
};

static struct ptable flash_ptable;

/* for these partitions, start will be offset by either what we get from
 * smem, or from the above offset if smem is not useful. Also, we should
 * probably have smem_ptable code populate our flash_ptable.
 *
 * When smem provides us with a full partition table, we can get rid of
 * this altogether.
 *
 */
static struct ptentry board_part_list[] = {
	{
		.start = 0,
		.length = 40,
		.name = "boot",
	},
	{
		.start = 56,
		.length = 304 /* 76MB */,
		.name = "system",
	},
	{
		.start = 364,
		.length = 304 /* 76MB */,
		.name = "cache",
	},
	{
		.start = 672,
		.length = 304 /* 76MB */,
		.name = "userdata",
	},
};
static int num_parts = sizeof(board_part_list)/sizeof(struct ptentry);

void smem_ptable_init(void);
unsigned smem_get_apps_flash_start(void);

void keypad_init(void);

static int emmc_boot = -1;  /* set to uninitialized */
int target_is_emmc_boot(void);

void target_init(void)
{
	unsigned offset;
	struct flash_info *flash_info;
	int i;

	dprintf(INFO, "target_init()\n");

	keys_init();
	keypad_init();

	if (target_is_emmc_boot())
		return;

	ptable_init(&flash_ptable);
	smem_ptable_init();

	flash_init();
	flash_info = flash_get_info();
	ASSERT(flash_info);

	offset = smem_get_apps_flash_start();
	if (offset == 0xffffffff)
	        while(1);

	for (i = 0; i < num_parts; i++) {
		struct ptentry *ptn = &board_part_list[i];
		unsigned len = ptn->length;

		if ((len == 0) && (i == num_parts - 1))
			len = flash_info->num_blocks - offset - ptn->start;
		ptable_add(&flash_ptable, ptn->name, offset + ptn->start,
			   len, ptn->flags);
	}

	ptable_dump(&flash_ptable);
	flash_set_ptable(&flash_ptable);
}

unsigned board_machtype(void)
{
    struct smem_board_info board_info;
    unsigned int board_info_struct_len = sizeof(board_info);
    enum platform platform_type = 0;
    unsigned smem_status;

    smem_status = smem_read_alloc_entry(SMEM_BOARD_INFO_LOCATION,
					&board_info, board_info_struct_len );
    if(smem_status)
    {
      dprintf(CRITICAL, "ERROR: unable to read shared memory for Hardware Platform\n");
    }

    if (board_info.format == 3)
    {
        platform_type = board_info.hw_platform;
        switch (platform_type)
	{
	case HW_PLATFORM_SURF:
	    return LINUX_MACHTYPE_SURF;
	case HW_PLATFORM_FFA:
	    return LINUX_MACHTYPE_FFA;
	case HW_PLATFORM_FLUID:
	    return LINUX_MACHTYPE_FLUID;
	default:
            return LINUX_MACHTYPE_SURF;
	}
    }
    return LINUX_MACHTYPE_SURF;
}

int target_is_emmc_boot(void)
{
    struct smem_board_info board_info;
    unsigned int board_info_struct_len = sizeof(board_info);
    unsigned smem_status;

    if (emmc_boot != -1) return emmc_boot;

    emmc_boot = 0;

    smem_status = smem_read_alloc_entry(SMEM_BOARD_INFO_LOCATION,
					&board_info, board_info_struct_len );
    if(!smem_status)
    {
        /* Success retrieving build info */
        if ('S' == *((char *)board_info.build_id + 13))
            emmc_boot = 1;
    }

    return emmc_boot;
}