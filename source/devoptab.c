/*
 * devoptab.c
 * ramfs devoptab implementation
 * Copyright (C) 2019 rw-r-r-0644
 */

#include "ramfs_internal.h"

#if defined(__WIIU__) || defined(__SWITCH__)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <stdint.h>
#include <unistd.h>

/* file informations structure */
typedef struct
{
	ramfs_node *fsnode;
	off_t pos;
} ramfs_file_t;

/* directory informations structure */
typedef struct
{
	ramfs_node *fsdir;
	ramfs_node *ent;
	int32_t idx;
} ramfs_dir_t;

/* filesystem mode for all files or directories */
#define ROMFS_DIR_MODE	(S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH)
#define ROMFS_FILE_MODE	(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH)

static int ramfs_open (struct _reent *r, void *fileStruct, const char *ipath, int flags, int mode)
{
	ramfs_file_t *fobj = (ramfs_file_t *)fileStruct;

	/* attempt to find the correct node for the path */
	fobj->fsnode = ramfs_get_node(ipath);

	/* the path doesn't exist */
	if (!fobj->fsnode)
	{
		if(flags & O_CREAT)
			r->_errno = EROFS;
		else
			r->_errno = ENOENT;
		return -1;
	}
	/* the file already exists but creation was requested */
	else if ((flags & O_CREAT) && (flags & O_EXCL))
	{
		r->_errno = EEXIST;
		return -1;
	}

	/* the path is not a file */
	if (fobj->fsnode->type != RAMFS_FILE)
	{
		r->_errno = EINVAL;
		return -1;
	}

	/* initialize file position */
	fobj->pos = 0;

	return 0;
}


static int ramfs_close(struct _reent *r, void *fd)
{
	/* nothing to do */
	return 0;
}

static ssize_t ramfs_read(struct _reent *r, void *fd, char *ptr, size_t len)
{
	ramfs_file_t *fobj = (ramfs_file_t *)fd;	

	/* reached file's end */
	if(fobj->pos >= fobj->fsnode->size)
		return 0;

	/* attempting to read after file end, truncate len */
	if((fobj->pos + len) > fobj->fsnode->size)
		len = fobj->fsnode->size - fobj->pos;

	/* copy the requested content */
	memcpy(ptr, fobj->fsnode->cont + fobj->pos, len);

	/* advance position by bytes read */
	fobj->pos += len;

	/* Return read lenght */
	return len;
}

static off_t ramfs_seek(struct _reent *r, void *fd, off_t pos, int dir)
{
	ramfs_file_t *fobj = (ramfs_file_t *)fd;
	off_t start;

	/* set relative seek start */
	switch (dir)
	{
		case SEEK_SET:
			start = 0;
			break;
		case SEEK_CUR:
			start = fobj->pos;
			break;
		case SEEK_END:
			start = fobj->fsnode->size;
			break;
		default:
			r->_errno = EINVAL;
			return -1;
    }

	if(pos < 0)
	{
		/* attempting to seek before file start */
		if(start + pos < 0)
		{
			r->_errno = EINVAL;
			return -1;
		}
	}
	/* the position index overflows */
	else if(INT32_MAX - pos < start)
	{
		r->_errno = EOVERFLOW;
		return -1;
	}

	/* set file position */
    fobj->pos = start + pos;

	/* return set position */
    return fobj->pos;
}

static int ramfs_fstat(struct _reent *r, void *fd, struct stat *st)
{
	ramfs_file_t *fobj = (ramfs_file_t *)fd;

	/* clear stat structure */
	memset(st, 0, sizeof(struct stat));

	/* set known fields */
	st->st_size  = fobj->fsnode->size;
	st->st_atime = st->st_mtime = st->st_ctime = fobj->fsnode->mtime;
	st->st_ino = fobj->fsnode->inode;
	st->st_mode  = ROMFS_FILE_MODE;
	st->st_nlink = 1;
	st->st_blksize = 512;
	st->st_blocks  = (st->st_blksize + 511) / 512;

	return 0;
}

static int ramfs_stat(struct _reent *r, const char *ipath, struct stat *st) {
	/* attempt to find the correct node for the path */
	ramfs_node *fsnode = ramfs_get_node(ipath);

	/* the path doesn't exist */
	if (!fsnode)
	{
		r->_errno = ENOENT;
		return -1;
	}

	/* clear stat structure */
	memset(st, 0, sizeof(struct stat));

	/* set known fields */
	st->st_size  = fsnode->size;
	st->st_atime = st->st_mtime = st->st_ctime = fsnode->mtime;
	st->st_ino = fsnode->inode;
	st->st_mode  = (fsnode->type == RAMFS_FILE) ? ROMFS_FILE_MODE : ROMFS_DIR_MODE;
	st->st_nlink = 1;
	st->st_blksize = 512;
	st->st_blocks  = (st->st_blksize + 511) / 512;

	return 0;
}

static int ramfs_chdir(struct _reent *r, const char *ipath)
{
	/* attempt to find the correct node for the path */
	ramfs_node *fsdir = ramfs_get_node(ipath);

	/* the path doesn't exist */
	if (!fsdir)
	{
		r->_errno = ENOENT;
		return -1;
	}

	/* the path is not a directory */
	if (fsdir->type != RAMFS_DIR)
	{
		r->_errno = EINVAL;
		return -1;
	}

	/* set current directory */
	fs_cwd = fsdir;

	return 0;
}

static DIR_ITER* ramfs_diropen(struct _reent *r, DIR_ITER *dirState, const char *ipath)
{
	ramfs_dir_t* dirobj = (ramfs_dir_t*)(dirState->dirStruct);

	/* attempt to find the correct node for the path */
	dirobj->fsdir = ramfs_get_node(ipath);

	/* the path doesn't exist */
	if (!dirobj->fsdir)
	{
		r->_errno = ENOENT;
		return NULL;
	}

	/* the path is not a directory */
	if (dirobj->fsdir->type != RAMFS_DIR)
	{
		r->_errno = EINVAL;
		return NULL;
	}

	/* initialize directory iterator to the first entry */
	dirobj->ent = dirobj->fsdir->ent;
	dirobj->idx = 0;

	return dirState;
}

static int ramfs_dirreset(struct _reent *r, DIR_ITER *dirState)
{
	ramfs_dir_t* dirobj = (ramfs_dir_t*)(dirState->dirStruct);

	/* reset directory iterator to the first entry */
	dirobj->ent = dirobj->fsdir->ent;
	dirobj->idx = 0;

	return 0;
}

static int ramfs_dirnext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	ramfs_dir_t* dirobj = (ramfs_dir_t*)(dirState->dirStruct);

	/* the first entry is current directory '.' */
	if (dirobj->idx == 0)
	{
		memset(filestat, 0, sizeof(*filestat));
		filestat->st_ino  = dirobj->fsdir->inode;
		filestat->st_mode = ROMFS_DIR_MODE;
		strcpy(filename, ".");
		dirobj->idx = 1;
		return 0;
	}

	/* the second entry is upper directory '..' */
	if (dirobj->idx == 1)
	{
		/* check for an upper directory */
		ramfs_node* updir = dirobj->fsdir->up;

		/* we reached fs top, use fs_root */
		if (!updir)
			updir = &fs_root;

		memset(filestat, 0, sizeof(*filestat));
		filestat->st_ino = updir->inode;
		filestat->st_mode = ROMFS_DIR_MODE;
		strcpy(filename, "..");
		dirobj->idx = 2;
		return 0;
	}

	/* the directory is empty */	
	if (!dirobj->ent)
	{
		r->_errno = ENOENT;
		return -1;
	}

	/* write entry data */
	memset(filestat, 0, sizeof(*filestat));
	filestat->st_ino = dirobj->ent->inode;
	filestat->st_mode = (dirobj->ent->type == RAMFS_FILE) ? ROMFS_FILE_MODE : ROMFS_DIR_MODE;
	strcpy(filename, dirobj->ent->name);

	/* go to the next entry */
	dirobj->ent = dirobj->ent->next;

	return 0;
}

static int ramfs_dirclose(struct _reent *r, DIR_ITER *dirState)
{
	/* nothing to do */
	return 0;
}

static devoptab_t ramfs_devoptab =
{
	.name         = "ramfs",
	.structSize   = sizeof(ramfs_file_t),
	.open_r       = ramfs_open,
	.close_r      = ramfs_close,
	.read_r       = ramfs_read,
	.seek_r       = ramfs_seek,
	.fstat_r      = ramfs_fstat,
	.stat_r       = ramfs_stat,
	.chdir_r      = ramfs_chdir,
	.dirStateSize = sizeof(ramfs_dir_t),
	.diropen_r    = ramfs_diropen,
	.dirreset_r   = ramfs_dirreset,
	.dirnext_r    = ramfs_dirnext,
	.dirclose_r   = ramfs_dirclose,
	.deviceData   = NULL,
};

int ramfs_mount()
{
	/* add the ramfs devoptab to devices list */
	return AddDevice(&ramfs_devoptab);
}

void ramfs_unmount()
{
	/* remove the ramfs devoptab from devices list */
	RemoveDevice("ramfs:");
}

#else

int ramfs_mount() { return 0 };
void ramfs_unmount() {}

#endif
