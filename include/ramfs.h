#ifndef _RAMFS_H
#define _RAMFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ramfs_init: mount ramfs
 * returns 0 when successful or a negative value for errors */
int ramfs_init(void);

/* ramfs_exit: unmount ramfs
 * returns 0 when successful or a negative value for errors */
int ramfs_exit(void);

/* ramfs_get_file: get direct access to a file in ramfs 
 * returns 0 when successful or a negative value for errors  */
void ramfs_get_file(const char *path, const char **ptr, int *size);

#ifdef __cplusplus
}
#endif

#endif
