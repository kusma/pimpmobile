/*

gbfs_stdio.c -- devkitARM stdio integration code for GBFS

Copyright (c) 2007 Erik Faye-Lund

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use
of this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software in
       a product, an acknowledgment in the product documentation would be
       appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.

*/

#include "gbfs_stdio.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "gbfs.h"

#include <assert.h>
#define ASSERT(expr) assert(expr)

#include <sys/iosupport.h>
#include <stdio.h>

struct file_state
{
	const u8   *data;
	u32         size;
	int         pos;
};

static const GBFS_FILE *file_system = NULL;

static const char *make_absolute_path(const char *path)
{
	/* skip file system identifier, we only support one anyway */
	if (strchr (path, ':') != NULL)
	{
		path = strchr (path, ':') + 1;
	}

	/* no subdirectory-support. everything is in the root. */
	if (path[0] == '/') path++;

	return path;
}

static int gbfs_open_r(struct _reent *r, void *fileStruct, const char *path, int flags,int mode)
{
	struct file_state *file = (struct file_state *)fileStruct;

	/* we're a read-only file system */
	if ((flags & 3) != O_RDONLY)
	{
		r->_errno = EROFS;
		return -1;
	}

	path = make_absolute_path(path);

	/* init file-data */
	file->size = 0;
	file->pos  = 0;
	file->data = gbfs_get_obj(file_system, path, &file->size);

	/* check for failure */
	if (NULL == file->data) return -1;

	return (int)file;
}

static int gbfs_close_r(struct _reent *r, int fd)
{
	struct file_state *file = (struct file_state *)fd;

	/* nothing to be done */

	return 0;
}

static off_t gbfs_seek_r(struct _reent *r, int fd, off_t pos, int dir)
{
	int position;
	struct file_state *file = (struct file_state *)fd;

	/* select origo */
	switch (dir)
	{
	case SEEK_SET:
		position = pos;
	break;

	case SEEK_CUR:
		position = file->pos + pos;
	break;

	case SEEK_END:
		position = file->size + pos;
	break;

	default:
		r->_errno = EINVAL;
		return -1;
	}

	/* check for seeks outside the file */
	if (position < 0 || position > file->size)
	{
		r->_errno = EINVAL;
		return -1;
	}

	/* update state */
	file->pos = position;
	return file->pos;
}

static ssize_t gbfs_read_r(struct _reent *r, int fd, char *ptr, size_t len)
{
	int i;
	struct file_state *file = (struct file_state *)fd;

	/* clamp len to file-length */
	if (len + file->pos > file->size)
	{
		r->_errno = EOVERFLOW;
		len = file->size - file->pos;
	}

	/* if we're already at the end of the file, there's nothing to copy */
	if (file->pos == file->size) return 0;

	/* copy data */
	for (i = 0; i < len; ++i)
	{
		ASSERT(file->pos < file->size);
		ASSERT(file->pos >= 0);
		*ptr++ = file->data[file->pos++];
	}

	return i;
}

static void gbfs_stat_file(struct file_state *file, struct stat *st)
{
	st->st_dev = 0; /* device id */
	st->st_ino = 0; /* TODO: find a way to get the file-index instead here */
	st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IFREG;
	st->st_nlink = 1;
	st->st_uid = 0; /* root user */
	st->st_gid = 0; /* wheel group */
	st->st_rdev = 0;
	st->st_size = file->size;
	st->st_blksize = 1; /* we're not working on blocks, so one byte per block is all fine */
	st->st_blocks  = (st->st_size + 511) / 512;

	st->st_atime = 0; /* 1970-01-01 00:00:00 FTW */
	st->st_mtime = 0;
	st->st_ctime = 0;
}


static int gbfs_fstat_r(struct _reent *r, int fd, struct stat *st)
{
	struct file_state *file = (struct file_state *)fd;
	gbfs_stat_file(file, st);
}

static int gbfs_stat_r(struct _reent *r, const char *path, struct stat *st)
{
	struct file_state file;

	path = make_absolute_path(path);

	/* init file-data */
	file.size = 0;
	file.pos  = 0;
	file.data = gbfs_get_obj(file_system, path, &file.size);

	if (NULL == file.data)
	{
		r->_errno = ENOENT;
		return -1;
	}

	gbfs_stat_file(&file, st);
	return 0;
}

static int gbfs_chdir_r(struct _reent *r, const char *path)
{
	/* skip file system identifier, we only support one anyway */
	path = make_absolute_path(path);

	/* no subdirs, so we'll only support the top level - ie empty string after stripping */
	if (path[0] != '\0')
	{
		r->_errno = ENOTDIR;
		return -1;
	}

	return 0;
}

struct dir_state
{
	int current_file;
};

static DIR_ITER* gbfs_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	struct dir_state* dir = (struct dir_state*)dirState->dirStruct;

	/* skip file system identifier, we only support one anyway */
	path = make_absolute_path(path);

	/* the only supported path is the root of the file system. */
	if (path[0] != '\0')
	{
		if (NULL == gbfs_get_obj(file_system, path, NULL)) r->_errno = ENOENT;
		else  r->_errno = ENOTDIR;
		return NULL;
	}

	/* reset file index */
	dir->current_file = 0;

	return (DIR_ITER*)dir;
}

static int gbfs_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
	struct dir_state* dir = (struct dir_state*)dirState->dirStruct;

	/* reset file index */
	dir->current_file = 0;

	return 0;
}

static int gbfs_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	struct file_state file;
	struct dir_state* dir = (struct dir_state*)dirState->dirStruct;

	/* init file-data */
	file.size = 0;
	file.pos  = 0;
	file.data = gbfs_get_nth_obj(file_system, dir->current_file, filename, &file.size);

	/* check for failure (no more files) */
	if (file.data == NULL) return -1;

	/* stat */
	gbfs_stat_file(&file, filestat);

	/* skip to next file */
	dir->current_file++;

	return 0;
}

static int gbfs_dirclose_r (struct _reent *r, DIR_ITER *dirState)
{
	return 0;
}

static devoptab_t d;
int gbfs_init(int set_default)
{
	int dev_id;

	file_system = find_first_gbfs_file((void*)0x08000000);
	if (NULL == file_system) return 0;

	memset(&d, 0, sizeof(devoptab_t));
	d.name = "gbfs";
	d.structSize   = sizeof(struct file_state);
	d.dirStateSize = sizeof(struct dir_state);
	d.open_r     = gbfs_open_r;
	d.close_r    = gbfs_close_r;
	d.seek_r     = gbfs_seek_r;
	d.read_r     = gbfs_read_r;
	d.stat_r     = gbfs_stat_r;
	d.fstat_r    = gbfs_fstat_r;
	d.chdir_r    = gbfs_chdir_r;
	d.diropen_r  = gbfs_diropen_r;
	d.dirreset_r = gbfs_dirreset_r;
	d.dirnext_r  = gbfs_dirnext_r;
	d.dirclose_r = gbfs_dirclose_r;

	dev_id = AddDevice(&d);
	if (0 != set_default) setDefaultDevice(dev_id);

	return 1;
}
