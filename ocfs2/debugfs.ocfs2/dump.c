/*
 * dump.c
 *
 * dumps ocfs2 structures
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

#include <main.h>
#include <commands.h>
#include <dump.h>
#include <readfs.h>
#include <utils.h>
#include <journal.h>

extern char *superblk;
extern __u32 blksz_bits;

/*
 * dump_super_block()
 *
 */
void dump_super_block(ocfs2_super_block *sb)
{
	int i;
	char *str;

	printf("Revision: %u.%u\n", sb->s_major_rev_level, sb->s_minor_rev_level);
	printf("Mount Count: %u   Max Mount Count: %u\n", sb->s_mnt_count,
	       sb->s_max_mnt_count);

	printf("State: %u   Errors: %u\n", sb->s_state, sb->s_errors);

	str = ctime((time_t*)&sb->s_lastcheck);
	printf("Check Interval: %u   Last Check: %s", sb->s_checkinterval, str);

	printf("Creator OS: %u\n", sb->s_creator_os);
	printf("Feature Compat: %u   Incompat: %u   RO Compat: %u\n",
	       sb->s_feature_compat, sb->s_feature_incompat,
	       sb->s_feature_ro_compat);

	printf("Root Blknum: %llu   System Dir Blknum: %llu\n",
	       sb->s_root_blkno, sb->s_system_dir_blkno);

	printf("Block Size Bits: %u   Cluster Size Bits: %u\n",
	       sb->s_blocksize_bits, sb->s_clustersize_bits);

	printf("Max Nodes: %u\n", sb->s_max_nodes);
	printf("Label: %s\n", sb->s_label);
	printf("UUID: ");
	for (i = 0; i < 16; i++)
		printf("%02X", sb->s_uuid[i]);
	printf("\n");

	return ;
}				/* dump_super_block */

/*
 * dump_local_alloc()
 *
 */
void dump_local_alloc (ocfs2_local_alloc *loc)
{
	printf("Local Bitmap Offset: %u   Size: %u\n",
	       loc->la_bm_off, loc->la_size);

	printf("\tTotal: %u   Used: %u   Clear: %u\n",
	       loc->la_bm_bits, loc->la_bits_set,
	       (loc->la_bm_bits - loc->la_bits_set));

	return ;
}				/* dump_local_alloc */

/*
 * dump_inode()
 *
 */
void dump_inode(ocfs2_dinode *in)
{
	struct passwd *pw;
	struct group *gr;
	char *str;
	__u16 mode;
	GString *flags = NULL;

	if (S_ISREG(in->i_mode))
		str = "regular";
	else if (S_ISDIR(in->i_mode))
		str = "directory";
	else if (S_ISCHR(in->i_mode))
		str = "char device";
	else if (S_ISBLK(in->i_mode))
		str = "block device";
	else if (S_ISFIFO(in->i_mode))
		str = "fifo";
	else if (S_ISLNK(in->i_mode))
		str = "symbolic link";
	else if (S_ISSOCK(in->i_mode))
		str = "socket";
	else
		str = "unknown";

	mode = in->i_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

	flags = g_string_new(NULL);
	if (in->i_flags & OCFS2_VALID_FL)
		g_string_append (flags, "valid ");
	if (in->i_flags & OCFS2_UNUSED2_FL)
		g_string_append (flags, "unused2 ");
	if (in->i_flags & OCFS2_ORPHANED_FL)
		g_string_append (flags, "orphan ");
	if (in->i_flags & OCFS2_UNUSED3_FL)
		g_string_append (flags, "unused3 ");
	if (in->i_flags & OCFS2_SYSTEM_FL)
		g_string_append (flags, "system ");
	if (in->i_flags & OCFS2_SUPER_BLOCK_FL)
		g_string_append (flags, "superblock ");
	if (in->i_flags & OCFS2_LOCAL_ALLOC_FL)
		g_string_append (flags, "localalloc ");
	if (in->i_flags & OCFS2_BITMAP_FL)
		g_string_append (flags, "allocbitmap ");
	if (in->i_flags & OCFS2_JOURNAL_FL)
		g_string_append (flags, "journal ");
	if (in->i_flags & OCFS2_DLM_FL)
		g_string_append (flags, "dlm ");

	printf("Inode: %llu   Mode: 0%0o   Generation: %u\n",
	       in->i_blkno, mode, in->i_generation);

	printf("Type: %s   Flags: %s\n", str, flags->str);

	pw = getpwuid(in->i_uid);
	gr = getgrgid(in->i_gid);
	printf("User: %d (%s)   Group: %d (%s)   Size: %llu\n",
	       in->i_uid, (pw ? pw->pw_name : "unknown"),
	       in->i_gid, (gr ? gr->gr_name : "unknown"),
	       in->i_size);

	printf("Links: %u   Clusters: %u\n", in->i_links_count, in->i_clusters);

	dump_disk_lock (&(in->i_disk_lock));

	str = ctime((time_t*)&in->i_ctime);
	printf("ctime: 0x%llx -- %s", in->i_ctime, str);
	str = ctime((time_t*)&in->i_atime);
	printf("atime: 0x%llx -- %s", in->i_atime, str);
	str = ctime((time_t*)&in->i_mtime);
	printf("mtime: 0x%llx -- %s", in->i_mtime, str);
	str = ctime((time_t*)&in->i_dtime);
	printf("dtime: 0x%llx -- %s", in->i_dtime, str);

	printf("Last Extblk: %llu\n", in->i_last_eb_blk);
	printf("Sub Alloc Node: %u   Sub Alloc Blknum: %llu\n",
	       in->i_suballoc_node, in->i_suballoc_blkno);

	if (in->i_flags & OCFS2_BITMAP_FL)
		printf("Bitmap Total: %u   Used: %u   Clear: %u\n",
		       in->id1.bitmap1.i_total, in->id1.bitmap1.i_used,
		       (in->id1.bitmap1.i_total - in->id1.bitmap1.i_used));

	if (flags)
		g_string_free (flags, 1);
	return ;
}				/* dump_inode */

/*
 * dump_disk_lock()
 *
 */
void dump_disk_lock (ocfs2_disk_lock *dl)
{
	ocfs2_super_block *sb = &(((ocfs2_dinode *)superblk)->id2.i_super);
	int i, j, k;
	__u32 node_map;

	printf("Lock Master: %u   Level: 0x%0x   Seqnum: %llu\n",
	       dl->dl_master, dl->dl_level, dl->dl_seq_num);

	printf("Lock Node Map: ");
	for (i = 0, j = 0; i < 8 && j < sb->s_max_nodes; ++i) {
		if (i)
			printf("               ");
		node_map = dl->dl_node_map[i];
		for (k = 0; k < 32 && j < sb->s_max_nodes; k++, j++)
			printf ("%d%c", ((node_map & (1 << k)) ? 1 : 0),
				(((k + 1) % 8) ? '\0' : ' '));
		printf ("\n");
	}

	return ;
}				/* dump_disk_lock */

/*
 * dump_extent_list()
 *
 */
void dump_extent_list (ocfs2_extent_list *ext)
{
	ocfs2_extent_rec *rec;
	int i;

	printf("Tree Depth: %u   Count: %u   Next Free Rec: %u\n",
	       ext->l_tree_depth, ext->l_count, ext->l_next_free_rec);

	if (!ext->l_next_free_rec)
		goto bail;

	printf("## File Offset   Num Clusters   Disk Offset\n");

	for (i = 0; i < ext->l_next_free_rec; ++i) {
		rec = &(ext->l_recs[i]);
		printf("%-2d %-11u   %-12u   %llu\n", i, rec->e_cpos,
		       rec->e_clusters, rec->e_blkno);
	}

bail:
	return ;
}				/* dump_extent_list */

/*
 * dump_extent_block()
 *
 */
void dump_extent_block (ocfs2_extent_block *blk)
{
	printf ("SubAlloc Blknum: %llu   SubAlloc Node: %u\n",
	       	blk->h_suballoc_blkno, blk->h_suballoc_node);

	printf ("Blknum: %llu   Parent: %llu   Next Leaf: %llu\n",
		blk->h_blkno, blk->h_parent_blk, blk->h_next_leaf_blk);

	return ;
}				/* dump_extent_block */

/*
 * dump_dir_entry()
 *
 */
void dump_dir_entry (GArray *arr)
{
	struct ocfs2_dir_entry *rec;
	int i;

	printf("%-20s %-4s %-4s %-2s %-4s\n",
	       "Inode", "Rlen", "Nlen", "Ty", "Name");

	for (i = 0; i < arr->len; ++i) {
		rec = &(g_array_index(arr, struct ocfs2_dir_entry, i));
		printf("%-20llu %-4u %-4u %-2u %s\n", rec->inode,
		       rec->rec_len, rec->name_len, rec->file_type, rec->name);
	}

	return ;
}				/* dump_dir_entry */

/*
 * dump_config()
 *
 */
void dump_config (char *buf)
{
	char *p;
	ocfs_node_config_hdr *hdr;
	ocfs_node_config_info *node;
	ocfs2_super_block *sb = &(((ocfs2_dinode *)superblk)->id2.i_super);
	__u16 port;
	char addr[32];
	struct in_addr ina;
	int i, j;

	hdr = (ocfs_node_config_hdr *)buf;

	printf("Version: %u   Num Nodes: %u   Last Node: %u   SeqNum: %llu\n",
	       hdr->version, hdr->num_nodes, hdr->last_node, hdr->cfg_seq_num);

	dump_disk_lock (&(hdr->disk_lock));

	printf("%-4s %-32s %-15s %-6s %s\n",
	       "Node", "Name", "IP Addr", "Port", "UUID");

	p = buf + (2 << blksz_bits);
	for (i = 0; i < sb->s_max_nodes; ++i) {
		node = (ocfs_node_config_info *)p;
		if (!*node->node_name)
			continue;

		port  = htonl(node->ipc_config.ip_port);

		ina.s_addr = node->ipc_config.addr_u.ip_addr4;
		strcpy (addr, inet_ntoa(ina));

		printf("%-4u %-32s %-15s %-6u ", i, node->node_name, addr, port);
		for (j = 0; j < OCFS2_GUID_LEN; j++)
			printf("%c", node->guid.guid[j]);
		printf("\n");
		p += (1 << blksz_bits);
	}

	return ;
}				/* dump_config */

/*
 * dump_publish()
 *
 */
void dump_publish (char *buf)
{
	ocfs_publish *pub;
	char *p;
	GString *pub_flag;
	ocfs2_super_block *sb = &(((ocfs2_dinode *)superblk)->id2.i_super);
	__u32 i, j;

	printf("%-2s %-3s %-3s %-3s %-15s %-15s %-15s %-*s %-s\n",
	       "No", "Mnt", "Vot", "Dty", "LockId", "Seq", "Time", sb->s_max_nodes,
	       "Map", "Type");

	p = buf + ((2 + 4 + sb->s_max_nodes) << blksz_bits);
	for (i = 0; i < sb->s_max_nodes; ++i) {
		pub = (ocfs_publish *)p;

		pub_flag = g_string_new (NULL);
		get_publish_flag (pub->vote_type, pub_flag);

		printf("%-2d  %1u   %1u   %1u  %-15llu %-15llu %-15llu ",
		       i, pub->mounted, pub->vote, pub->dirty, pub->lock_id,
		       pub->publ_seq_num, pub->time);

		for (j = 0; j < sb->s_max_nodes; j++)
			printf ("%d", ((pub->vote_map & (1 << j)) ? 1 : 0));

		printf(" %-s\n", pub_flag->str);

		g_string_free (pub_flag, 1);

		p += (1 << blksz_bits);
	}

	return ;	
}				/* dump_publish */

/*
 * dump_vote()
 *
 */
void dump_vote (char *buf)
{
	ocfs_vote *vote;
	char *p;
	GString *vote_flag;
	ocfs2_super_block *sb = &(((ocfs2_dinode *)superblk)->id2.i_super);
	__u32 i;

	printf("%-2s %-2s %-1s %-15s %-15s %-s\n",
	       "No", "NV", "O", "LockId", "Seq", "Type");

	p = buf + ((2 + 4 + sb->s_max_nodes + sb->s_max_nodes) << blksz_bits);
	for (i = 0; i < sb->s_max_nodes; ++i) {
		vote = (ocfs_vote *)p;

		vote_flag = g_string_new (NULL);
		get_vote_flag (vote->type, vote_flag);

		printf("%-2u %-2u %-1u %-15llu %-15llu %-s\n", i,
		       vote->node, vote->open_handle, vote->lock_id,
		       vote->vote_seq_num, vote_flag->str);

		g_string_free (vote_flag, 1);
		p += (1 << blksz_bits);
	}

	return ;
}				/* dump_vote */