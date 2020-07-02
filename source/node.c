/*
 * fstree.c
 * Tree of nodes containing filesystem entries
 * Copyright (C) 2019 rw-r-r-0644
 */

/* current filesystem directory */
static ramfs_node *fs_cwd = &fs_root;

/* the number of nodes */
static uint32_t n_nodes = 0;

/* ramfs_get_node: parse path string and return the corresponding node */
ramfs_node *ramfs_get_node (const char *cpath)
{
	/* remove the device label when present */
	char* col_pos = strchr(cpath, ':');
	if (col_pos)
		cpath = col_pos + 1;

	/* check if it's a relative path */
	ramfs_node *n = (cpath[0] == '/') ? &fs_root : fs_cwd;

	/* duplicate path to do the parsing */
	char *path = strdup(cpath), *path_s;

	/* split the path string */
	for (char *cur = strtok_r(path, "/", &path_s); cur; cur = strtok_r(NULL, "/", &path_s))
	{
		/* the current node doesn't exist or isn't a directory */
		if (!n || (n->type != NODE_DIR))
		{
			n = NULL;
			break;
		}

		/* current directory entry '.' */
		if (!strcmp(cur, "."))
		{
			n = n;
			continue;
		}

		/* upper directory entry '..' */
		if (!strcmp(cur, ".."))
		{
			if (n->up)
				n = n->up;
			continue;
		}

		/* attempt to find the requested entry */
		for (n = n->ent; n && strcmp(n->name, cur); n = n->next);
	}
	
	/* free up working memory */
	free(path);

	return n;
}

/* ramfs_create_node: create a node and all its parent directories */
void ramfs_create_node (const char *cpath, time_t mtime, int32_t isdir, uint32_t size, char *content)
{
	ramfs_node *n = &fs_root;
	
	/* duplicate path to do the parsing */
	char *path = strdup(cpath), *path_s;
	
	/* the currently parsed directory and the next */
	char *cur, *next = strtok_r(path, "/", &path_s);
	
	/* loop until the whole path and last file is created */
	while ((cur = next) != NULL)
	{
		/* get next path entry if existent */
		next = strtok_r(NULL, "/", &path_s);
		
		/* current directory entry '.' */
		if (!strcmp(cur, "."))
		{
			n = n;
			continue;
		}
		
		/* upper directory entry '..' */
		else if (!strcmp(cur, ".."))
		{
			if (n->up)
				n = n->up;
			continue;
		}
		
		/* attempt to find the current node */
		ramfs_node *c = NULL;
		for (c = n->ent; c && strcmp(c->name, cur); c = c->next);
		
		/* found it */
		if (c)
		{
			n = c;
			continue;
		}
		
		/* create the current path node */
		c = malloc(sizeof(ramfs_node));
		c->name = strdup(cur);
		c->inode = ++n_nodes;
		c->mtime = mtime;
		
		/* there's no other path next, this is the final node */
		if (!next)
		{
			c->size = size;
			if (isdir)
			{
				c->type = NODE_DIR;
				c->ent = NULL;	
			}
			else
			{
				c->type = NODE_FILE;
				c->cont = content;
			}
		}
		/* there's another path next, this is a dirextory */ 
		else
		{
			c->type = NODE_DIR;
			c->ent = NULL;
		}
		
		/* add the node to tree */
		c->up = n;
		c->next = n->ent;
		n->ent = c;
		
		/* child node is now parent node */
		n = c;
	}
	
	/* free up working memory */
	free(path);
}

/* ramfs_free_nodes: deallocate the tree of nodes */
void ramfs_free_nodes (ramfs_node *n, int32_t recursion)
{
	/* store a list of nodes to free as the tree is trasversed */
	static ramfs_node **l_free = NULL;
	static uint32_t l_freecnt = 0;
	if (!l_free)
		l_free = malloc(n_nodes * sizeof(ramfs_node *));
	
	/* start from tree's root */
	if (!n)
		n = &fs_root;
	
	/* limit to 50 recursions to prevent stack overflows */
	if (recursion > 50)
		return;
	
	/* trasverse the node tree */
	for (ramfs_node *c = n->ent; c; c = c->next)
	{
		if (c->type == NODE_DIR)
			node_destroytree(c, recursion + 1);
		l_free[l_freecnt++] = c;
	}
	
	/* if we're a recursive call, return now */
	if (recursion)
		return;
	
	/* free up all the nodes in the list */
	for (int32_t i = 0; i < l_freecnt; i++)
		free(l_free[i]);
	
	/* free up the list */
	free(l_free);
	l_freecnt = 0;
	l_free = NULL;
	fs_root.ent = NULL;
}
