#ifndef _RAMFS_INTERNAL_H
#define _RAMFS_INTERNAL_H

#include <stdint.H>

typedef enum
{
	RAMFS_DIR,
	RAMFS_FILE,
} ramfs_node_type;

typedef struct ramfs_node ramfs_node;
struct ramfs_node
{
	char *name;
	uint32_t inode;
	uint32_t size;
	ramfs_node_type type;
	time_t mtime;
	union
	{
		char *cont;			/* cont: file content for files */
		ramfs_node *ent;	/* ent: subdirectory entry for directories */
	};
	ramfs_node *next;
	ramfs_node *up;
};

/* ramfs devoptab */
int ramfs_mount();
void ramfs_unmount();

/* node tree functions */
ramfs_node *ramfs_get_node (const char *cpath);
void ramfs_create_node (const char *cpath, time_t mtime, int32_t isdir, uint32_t size, char *content);
void ramfs_free_nodes (ramfs_node *n, int32_t recursion);

/* tar functions */
void ramfs_tar_parse(char *ptr, char *end);

#endif
