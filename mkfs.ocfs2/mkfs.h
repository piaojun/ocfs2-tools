/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * mkfs2.h
 *
 * Definitions, function prototypes, etc.
 *
 * Copyright (C) 2004, 2005 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2,  as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 */

#define _LARGEFILE64_SOURCE

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <libgen.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <ctype.h>

#include "ocfs2.h"
#include "bitops.h"

/* jfs_compat.h defines these */
#undef cpu_to_be32
#undef be32_to_cpu

#include "ocfs1_fs_compat.h"

typedef unsigned short kdev_t;

#include <signal.h>
#include <libgen.h>

#include "kernel-jbd.h"


#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define MIN_RESERVED_TAIL_BLOCKS    8

#define LEADING_SPACE_BLOCKS    2
#define SLOP_BLOCKS             0
#define FILE_ENTRY_BLOCKS       8
#define SUPERBLOCK_BLOCKS       1
#define PUBLISH_BLOCKS(i,min)   (i<min ? min : i)
#define VOTE_BLOCKS(i,min)      (i<min ? min : i)
#define AUTOCONF_BLOCKS(i,min)  ((2+4) + (i<min ? min : i))
#define NUM_LOCAL_SYSTEM_FILES  6

#define OCFS2_OS_LINUX           0
#define OCFS2_OS_HURD            1
#define OCFS2_OS_MASIX           2
#define OCFS2_OS_FREEBSD         3
#define OCFS2_OS_LITES           4

#define OCFS2_DFL_MAX_MNT_COUNT          20
#define OCFS2_DFL_CHECKINTERVAL          0

#define SYSTEM_FILE_NAME_MAX   40

#define ONE_GB_SHIFT           30

#define BITMAP_WARNING_LEN     1572864
#define BITMAP_AUTO_MAX        786432

#define MAX_CLUSTER_SIZE       1048576
#define MIN_CLUSTER_SIZE       4096
#define AUTO_CLUSTER_SIZE      65536


enum {
	SFI_JOURNAL,
	SFI_CLUSTER,
	SFI_LOCAL_ALLOC,
	SFI_HEARTBEAT,
	SFI_CHAIN,
	SFI_OTHER
};


typedef struct _SystemFileInfo SystemFileInfo;

struct _SystemFileInfo {
	char *name;
	int type;
	int global;
	int mode;
};

struct BitInfo {
	uint32_t used_bits;
	uint32_t total_bits;
};

typedef struct _AllocGroup AllocGroup;
typedef struct _SystemFileDiskRecord SystemFileDiskRecord;

struct _AllocGroup {
	char *name;
	ocfs2_group_desc *gd;
	SystemFileDiskRecord *alloc_inode;
	uint32_t chain_free;
	uint32_t chain_total;
	struct _AllocGroup *next;
};


struct _SystemFileDiskRecord {
	uint64_t fe_off;
        uint16_t suballoc_bit;
	uint64_t extent_off;
	uint64_t extent_len;
	uint64_t file_size;

	uint64_t chain_off;
	AllocGroup *group;

	struct BitInfo bi;
	struct _AllocBitmap *bitmap;

	int flags;
	int links;
	int mode;
	int cluster_bitmap;
};

typedef struct _AllocBitmap AllocBitmap;

struct _AllocBitmap {
	AllocGroup **groups;

	uint32_t valid_bits;
	uint32_t unit;
	uint32_t unit_bits;

	char *name;

	uint64_t fe_disk_off;

	SystemFileDiskRecord *bm_record;
	SystemFileDiskRecord *alloc_record;
	int num_chains;
};

typedef struct _DirData DirData;

struct _DirData {
	uint64_t disk_off;
	uint64_t disk_len;

	void *buf;
	int buf_len;

	int last_off;
	uint64_t fe_disk_off;

	int link_count;

	SystemFileDiskRecord *record;
};

typedef struct _State State;

struct _State {
  	char *progname;

	int verbose;
	int quiet;
	int force;
	int prompt;

	uint32_t blocksize;
	uint32_t blocksize_bits;

	uint32_t cluster_size;
	uint32_t cluster_size_bits;

	uint64_t specified_size_in_blocks;
	uint64_t volume_size_in_bytes;
	uint32_t volume_size_in_clusters;
	uint64_t volume_size_in_blocks;

	uint32_t pagesize_bits;

	uint64_t reserved_tail_size;

	unsigned int initial_nodes;

	uint64_t journal_size_in_bytes;

	char *vol_label;
	char *device_name;
	char *uuid;
	uint32_t vol_generation;

	int fd;

	time_t format_time;

	AllocBitmap *global_bm;
	AllocGroup *system_group;
	uint32_t nr_cluster_groups;
	uint16_t global_cpg;
	uint16_t tail_group_bits;
	uint32_t first_cluster_group;
	uint64_t first_cluster_group_blkno;
};

int ocfs2_check_volume(State *s);