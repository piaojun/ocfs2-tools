/*
 * commands.c
 *
 * handles debugfs commands
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
 * Authors: Sunil Mushran, Manish Singh
 */

#include <main.h>
#include <commands.h>
#include <dump.h>
#include <readfs.h>
#include <utils.h>
#include <journal.h>

typedef void (*PrintFunc) (void *buf);
typedef gboolean (*WriteFunc) (char **data, void *buf);

typedef void (*CommandFunc) (char **args);

typedef struct _Command Command;

struct _Command
{
	char        *cmd;
	CommandFunc  func;
};

static Command  *find_command (char  *cmd);
static char    **get_data     (void);

static void do_open (char **args);
static void do_close (char **args);
static void do_cd (char **args);
static void do_ls (char **args);
static void do_pwd (char **args);
static void do_mkdir (char **args);
static void do_rmdir (char **args);
static void do_rm (char **args);
static void do_read (char **args);
static void do_write (char **args);
static void do_quit (char **args);
static void do_help (char **args);
static void do_dump (char **args);
static void do_lcd (char **args);
static void do_curdev (char **args);
static void do_super (char **args);
static void do_inode (char **args);
static void do_config (char **args);
static void do_publish (char **args);
static void do_vote (char **args);
static void do_journal (char **args);

extern gboolean allow_write;

char *device = NULL;
int   dev_fd = -1;
__u32 blksz_bits = 0;
__u32 clstrsz_bits = 0;
__u64 root_blkno = 0;
__u64 sysdir_blkno = 0;
__u64 dlm_blkno = 0;
char *curdir = NULL;
char *superblk = NULL;
char *rootin = NULL;
char *sysdirin = NULL;


static Command commands[] =
{
  { "open",   do_open   },
  { "close",  do_close  },
  { "cd",     do_cd     },
  { "ls",     do_ls     },
  { "pwd",    do_pwd    },

  { "mkdir",  do_mkdir  },
  { "rmdir",  do_rmdir  },
  { "rm",     do_rm     },

  { "lcd",    do_lcd    },

  { "read",   do_read   },
  { "write",  do_write  },

  { "help",   do_help   },
  { "?",      do_help   },

  { "quit",   do_quit   },
  { "q",      do_quit   },

  { "dump",   do_dump   },
  { "cat",    do_dump   },

  { "curdev", do_curdev },

  { "show_super_stats", do_super },
  { "stats", do_super },

  { "show_inode_info", do_inode },
  { "stat", do_inode },

  { "nodes", do_config },
  { "publish", do_publish },
  { "vote", do_vote },

  { "logdump", do_journal }

};


/*
 * find_command()
 *
 */
static Command * find_command (char *cmd)
{
	int i;

	for (i = 0; i < sizeof (commands) / sizeof (commands[0]); i++)
		if (strcmp (cmd, commands[i].cmd) == 0)
			return &commands[i];

	return NULL;
}					/* find_command */

/*
 * do_command()
 *
 */
void do_command (char *cmd)
{
	char    **args;
	Command  *command;

	if (*cmd == '\0')
		return;

	args = g_strsplit (cmd, " ", -1);

	command = find_command (args[0]);

	if (command)
		command->func (args);
	else
		printf ("Unrecognized command: %s\n", args[0]);

	g_strfreev (args);
}					/* do_command */

/*
 * do_open()
 *
 */
static void do_open (char **args)
{
	char *dev = args[1];
	ocfs2_dinode *inode;
	ocfs2_super_block *sb;
	__u32 len;

	if (device)
		do_close (NULL);

	if (dev == NULL) {
		printf ("open requires a device argument\n");
		goto bail;
	}

	dev_fd = open (dev, allow_write ? O_RDONLY : O_RDWR);
	if (dev_fd == -1) {
		printf ("could not open device %s\n", dev);
		goto bail;
	}

	device = g_strdup (dev);

	if (read_super_block (dev_fd, &superblk) != -1)
		curdir = g_strdup ("/");
	else {
		close (dev_fd);
		dev_fd = -1;
		goto bail;
	}

	inode = (ocfs2_dinode *)superblk;
	sb = &(inode->id2.i_super);
	/* set globals */
	clstrsz_bits = sb->s_clustersize_bits;
	blksz_bits = sb->s_blocksize_bits;
	root_blkno = sb->s_root_blkno;
	sysdir_blkno = sb->s_system_dir_blkno;

	/* read root inode */
	len = 1 << blksz_bits;
	if (!(rootin = malloc(len)))
		DBGFS_FATAL("%s", strerror(errno));
	if ((pread64(dev_fd, rootin, len, (root_blkno << blksz_bits))) == -1)
		DBGFS_FATAL("%s", strerror(errno));

	/* read sysdir inode */
	len = 1 << blksz_bits;
	if (!(sysdirin = malloc(len)))
		DBGFS_FATAL("%s", strerror(errno));
	if ((pread64(dev_fd, sysdirin, len, (sysdir_blkno << blksz_bits))) == -1)
		DBGFS_FATAL("%s", strerror(errno));

	/* load sysfiles blknums */
	read_sysdir (dev_fd, sysdirin);

bail:
	return ;
}					/* do_open */

/*
 * do_close()
 *
 */
static void do_close (char **args)
{
	if (device) {
		g_free (device);
		device = NULL;
		close (dev_fd);
		dev_fd = -1;

		g_free (curdir);
		curdir = NULL;

		safefree (superblk);
		safefree (rootin);
		safefree (sysdirin);
	} else
		printf ("device not open\n");

	return ;
}					/* do_close */

/*
 * do_cd()
 *
 */
static void do_cd (char **args)
{

}					/* do_cd */

/*
 * do_ls()
 *
 */
static void do_ls (char **args)
{
	char *opts = args[1];
	ocfs2_dinode *inode;
	__u32 blknum;
	char *buf = NULL;
	GArray *dirarr = NULL;
	__u32 len;

	if (dev_fd == -1) {
		printf ("device not open\n");
		goto bail;
	}

	len = 1 << blksz_bits;
	if (!(buf = malloc(len)))
		DBGFS_FATAL("%s", strerror(errno));

	if (opts) {
		blknum = atoi(opts);
		if ((read_inode (dev_fd, blknum, buf, len)) == -1) {
			printf("Not an inode\n");
			goto bail;
		}
		inode = (ocfs2_dinode *)buf;
	} else {
		inode = (ocfs2_dinode *)rootin;
	}

	if (!S_ISDIR(inode->i_mode)) {
		printf("Not a dir\n");
		goto bail;
	}

	dirarr = g_array_new(0, 1, sizeof(struct ocfs2_dir_entry));

	read_dir (dev_fd, &(inode->id2.i_list), inode->i_size, dirarr);

	dump_dir_entry (dirarr);

bail:
	safefree (buf);

	if (dirarr)
		g_array_free (dirarr, 1);
	return ;

}					/* do_ls */

/*
 * do_pwd()
 *
 */
static void do_pwd (char **args)
{
	printf ("%s\n", curdir ? curdir : "No dir");
}					/* do_pwd */

/*
 * do_mkdir()
 *
 */
static void do_mkdir (char **args)
{
	printf ("%s\n", __FUNCTION__);
}					/* do_mkdir */

/*
 * do_rmdir()
 *
 */
static void do_rmdir (char **args)
{
	printf ("%s\n", __FUNCTION__);
}					/* do_rmdir */

/*
 * do_rm()
 *
 */
static void do_rm (char **args)
{
  printf ("%s\n", __FUNCTION__);
}					/* do_rm */

/*
 * do_read()
 *
 */
static void do_read (char **args)
{

}					/* do_read */

/*
 * do_write()
 *
 */
static void do_write (char **args)
{

}					/* do_write */

/*
 * do_help()
 *
 */
static void do_help (char **args)
{
	printf ("curdev\t\t\t\tShow current device\n");
	printf ("open <device>\t\t\tOpen a device\n");
	printf ("close\t\t\t\tClose a device\n");
	printf ("show_super_stats, stats [-h]\tShow superblock\n");
	printf ("show_inode_info, stat <blknum>\tShow inode\n");
	printf ("pwd\t\t\t\tPrint working directory\n");
	printf ("ls <blknum>\t\t\tList directory\n");
	printf ("cat <blknum> [outfile]\t\tPrints or concatenates file to stdout/outfile\n");
	printf ("dump <blknum> <outfile>\t\tDumps file to outfile\n");
	printf ("nodes\t\t\t\tList of nodes\n");
	printf ("publish\t\t\t\tPublish blocks\n");
	printf ("vote\t\t\t\tVote blocks\n");
	printf ("logdump <blknum>\t\tPrints journal file\n");
	printf ("help, ?\t\t\t\tThis information\n");
	printf ("quit, q\t\t\t\tExit the program\n");
}					/* do_help */

/*
 * do_quit()
 *
 */
static void do_quit (char **args)
{
	exit (0);
}					/* do_quit */

/*
 * do_lcd()
 *
 */
static void do_lcd (char **args)
{

}					/* do_lcd */

/*
 * do_curdev()
 *
 */
static void do_curdev (char **args)
{
	printf ("%s\n", device ? device : "No device");
}					/* do_curdev */

/*
 * get_data()
 *
 */
static char ** get_data (void)
{
	return NULL;
}					/* get_data */

/*
 * do_super()
 *
 */
static void do_super (char **args)
{
	char *opts = args[1];
	ocfs2_dinode *in;
	ocfs2_super_block *sb;

	if (dev_fd == -1) {
		printf ("device not open\n");
		goto bail;
	}

	in = (ocfs2_dinode *)superblk;
	sb = &(in->id2.i_super);
	dump_super_block(sb);

	if (!opts || strncmp(opts, "-h", 2))
		dump_inode(in);

bail:
	return ;
}					/* do_super */

/*
 * do_inode()
 *
 */
static void do_inode (char **args)
{
	char *opts = args[1];
	ocfs2_dinode *inode;
	__u32 blknum;
	char *buf = NULL;
	__u32 buflen;

	if (dev_fd == -1) {
		printf ("device not open\n");
		goto bail;
	}

	buflen = 1 << blksz_bits;
	if (!(buf = malloc(buflen)))
		DBGFS_FATAL("%s", strerror(errno));

	if (opts) {
		blknum = atoi(opts);
		if ((read_inode (dev_fd, blknum, buf, buflen)) == -1) {
			printf("Not an inode\n");
			goto bail;
		}
		inode = (ocfs2_dinode *)buf;
	} else {
		inode = (ocfs2_dinode *)rootin;
	}

	dump_inode(inode);

	if ((inode->i_flags & OCFS2_LOCAL_ALLOC_FL))
		dump_local_alloc(&(inode->id2.i_lab));
	else
		traverse_extents(dev_fd, &(inode->id2.i_list), NULL, 1);

bail:
	safefree (buf);
	return ;
}					/* do_inode */

/*
 * do_config()
 *
 */
static void do_config (char **args)
{
	char *dlmbuf = NULL;

	if (dev_fd == -1)
		printf ("device not open\n");
	else {
		if (read_file (dev_fd, dlm_blkno, -1, &dlmbuf) == -1)
			goto bail;
		dump_config (dlmbuf);
	}

bail:
	safefree (dlmbuf);
	return ;
}					/* do_config */

/*
 * do_publish()
 *
 */
static void do_publish (char **args)
{
	char *dlmbuf = NULL;

	if (dev_fd == -1)
		printf ("device not open\n");
	else {
		if (read_file (dev_fd, dlm_blkno, -1, &dlmbuf) == -1)
			goto bail;
		dump_publish (dlmbuf);
	}

bail:
	safefree (dlmbuf);
	return ;
}					/* do_publish */

/*
 * do_vote()
 *
 */
static void do_vote (char **args)
{
	char *dlmbuf = NULL;
	
	if (dev_fd == -1)
		printf ("device not open\n");
	else {
		if (read_file (dev_fd, dlm_blkno, -1, &dlmbuf) == -1)
			goto bail;
		dump_vote (dlmbuf);
	}

bail:
	safefree (dlmbuf);
	return ;
}					/* do_vote */

/*
 * do_dump()
 *
 */
static void do_dump (char **args)
{
	__u64 blknum = 0;
	__s32 outfd = -1;
	int flags;
	int op = 0;  /* 0 = dump, 1 = cat */
	char *outfile = NULL;

	if (dev_fd == -1) {
		printf ("device not open\n");
		goto bail;
	}

	if (!strncasecmp (args[0], "cat", 3)) {
		outfd = 1;
		op = 1;
	}

	if (args[1])
		blknum = strtoull (args[1], NULL, 0);

	if (!blknum)
		goto bail;

	if (args[2]) {
		outfile = args[2];
		flags = O_WRONLY| O_CREAT | O_LARGEFILE;
		flags |= ((op) ? O_APPEND : O_TRUNC);
		if ((outfd = open (outfile, flags)) == -1) {
			printf ("unable to open file %s\n", outfile);
			goto bail;
		}
	}

	read_file (dev_fd, blknum, outfd, NULL);

bail:
	if (outfd > 2)
		close (outfd);

	return ;
}					/* do_dump */

/*
 * do_journal()
 *
 */
static void do_journal (char **args)
{
	char *logbuf = NULL;
	__u64 blknum = 0;
	__s32 len = 0;

	if (args[1])
		blknum = strtoull (args[1], NULL, 0);
	if (!blknum)
		goto bail;

	if (dev_fd == -1)
		printf ("device not open\n");
	else {
		if ((len = read_file (dev_fd, blknum, -1, &logbuf)) == -1)
			goto bail;
		read_journal (logbuf, (__u64)len);
	}

bail:
	safefree (logbuf);
	return ;
}					/* do_journal */
