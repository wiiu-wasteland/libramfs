/*
 * ramfs.c
 * ramfs library functions
 * Copyright (C) 2019 rw-r-r-0644
 */

#include "ramfs_internal.h"

/* ramfs binary symbols */
extern char _binary_ramfs_tar_start[];
extern char _binary_ramfs_tar_end[];

/* ramfs initialization flag */
static int ramfs_initialized = 0;

/* ramfs_init: intialize ramfs */
int ramfs_init(void)
{
	if (ramfs_initialized)
		return 0;

	/* create ramfs file entries */
	ramfs_tar_parse(_binary_ramfs_tar_start, _binary_ramfs_tar_end);

	/* mount ramfs */
	if (ramfs_mount() == -1)
	{
		ramfs_free_nodes(NULL, 0);
		return -1;
	}

	ramfs_initialized = 1;

	return 0;
}

/* ramfs_exit: exit ramfs */
int ramfs_exit(void)
{
	if (!ramfs_initialized)
		return -1;

	/* unmount ramfs */
	ramfs_unmount();

	/* deallocate the nodes tree */
	ramfs_free_nodes(NULL, 0);

	ramfs_initialized = 0;

	return 0;
}

/* ramfs_get_file: get direct access to a file in ramfs */
int ramfs_get_file(const char *path, const char **ptr, int *size);
{
	ramfs_node *n = ramfs_get_node();

	if (!n || n->type != RAMFS_FILE)
		return -1;

	if (ptr)
		*ptr = n->cont;
	if (size)
		*size = n->size;

	return 0;
}
