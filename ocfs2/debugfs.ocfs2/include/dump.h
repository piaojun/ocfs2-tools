/*
 * dump.h
 *
 * Function prototypes, macros, etc. for related 'C' files
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 * Authors: Sunil Mushran
 */

#ifndef __DUMP_H__
#define __DUMP_H__

void dump_super_block (ocfs2_super_block *sb);
void dump_local_alloc (ocfs2_local_alloc *loc);
void dump_inode (ocfs2_dinode *in);
void dump_disk_lock (ocfs2_disk_lock *dl);
void dump_extent_list (ocfs2_extent_list *ext);
void dump_extent_block (ocfs2_extent_block *blk);
void dump_dir_entry (GArray *arr);
void dump_config (char *buf);
void dump_publish (char *buf);
void dump_vote (char *buf);

#endif		/* __DUMP_H__ */