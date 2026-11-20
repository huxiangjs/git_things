/*
 * MIT License
 *
 * Copyright (c) 2023 Hoozz <huxiangjs@foxmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gitt_log.h>
#include <gitt.h>
#include <gitt_type.h>
#include <gitt_commit.h>
#include <gitt_tree.h>
#include <gitt_blob.h>
#include <gitt_errno.h>

#define GITT_DEFAULT_DATE		"0"
#define GITT_DEFAULT_ZONE		"+0000"

/*
 * The author email, built from the device id (GITT_EMAIL_FORMAT from
 * gitt_config.h): the id plus the fixed "@" + version + ".GITT"
 * suffix, bytes including the NUL
 */
#define GITT_EMAIL_SIZE		(GITT_DEVICE_ID_SIZE + 10)

/* The author date / zone buffers, bytes including the NUL */
#define GITT_COMMIT_DATE_SIZE	16
#define GITT_COMMIT_ZONE_SIZE	8

/* SHA-1 hex string length, chars (no NUL) */
#define GITT_SHA1_HEX_SIZE	40

static void gitt_repository_blob_dump(struct gitt_repository *repository,
				      char *path, uint8_t *data, uint32_t size)
{
	struct gitt *g = gitt_containerof(repository, struct gitt, repository);

	if (g->file_dump)
		g->file_dump(g, path, data, size);
}

/**
 * @brief Initialize GITT
 *
 * @param g struct gitt
 * @return int     0: no error
 * @return int other: error
 */
int gitt_init(struct gitt *g)
{
	int ret;

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	g->repository.privkey = g->privkey;
	g->repository.url = g->url;
	g->repository.buf = g->buf;
	g->repository.buf_len = g->buf_len;
	g->repository.blob_dump = gitt_repository_blob_dump;

	ret = gitt_repository_init(&g->repository);
	if (ret) {
		gitt_log_error("Initializing the repository fail\n");
		goto err0;
	}

	ret = gitt_repository_update_head(&g->repository);
	if (ret) {
		gitt_log_error("Update head fail\n");
		goto err1;
	}

	return 0;

err1:
	gitt_repository_end(&g->repository);
err0:
	return ret;
}

/**
 * @brief Pull the repository (fetch the latest file snapshot from remote)
 *
 * Always a fresh shallow snapshot of the tip tree (depth 1); the
 * restored files are passed to the file_dump callback.
 *
 * @param g struct gitt
 * @return int     0: no error
 * @return int other: error
 */
int gitt_pull(struct gitt *g)
{
	int ret;

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	ret = gitt_repository_pull(&g->repository);
	if (ret) {
		gitt_log_error("Pull the repository fail\n");
		return ret;
	}

	return 0;
}

/*
 * File tree builders
 */

static int gitt_file_dir(const char *path, char *dir, uint16_t dir_size)
{
	const char *slash = strrchr(path, '/');
	size_t len;

	if (!slash) {
		dir[0] = '\0';
		return 0;
	}
	len = (size_t)(slash - path);
	if (len + 1 > dir_size)
		return -GITT_ERRNO_INVAL;
	memcpy(dir, path, len);
	dir[len] = '\0';
	return 0;
}

static const char *gitt_file_base(const char *path)
{
	const char *slash = strrchr(path, '/');

	return slash ? slash + 1 : path;
}

/* Collect the direct child directories of <dir> implied by the files */
static uint8_t gitt_collect_children(struct gitt_file *files, uint8_t file_num,
				     const char *dir, char names[][GITT_FILE_PATH_SIZE])
{
	uint8_t count = 0;
	uint8_t i;
	uint8_t j;
	char d[GITT_FILE_PATH_SIZE];
	const char *child;
	const char *slash;
	size_t child_len;

	for (i = 0; i < file_num; i++) {
		if (gitt_file_dir(files[i].path, d, sizeof(d)))
			continue;

		/*
		 * d must be a descendant of dir. The child name is the
		 * first path component of d below dir.
		 */
		if (dir[0]) {
			if (strncmp(d, dir, strlen(dir)) || d[strlen(dir)] != '/')
				continue;
			child = d + strlen(dir) + 1;
		} else {
			if (!strlen(d))
				continue;
			child = d;
		}

		slash = strchr(child, '/');
		child_len = slash ? (size_t)(slash - child) : strlen(child);
		if (!child_len)
			continue;

		for (j = 0; j < count; j++) {
			if (!strncmp(names[j], child, child_len) &&
			    names[j][child_len] == '\0')
				break;
		}
		if (j < count)
			continue;

		if (count >= GITT_MAX_DIR_CHILDREN)
			break;
		memcpy(names[count], child, child_len);
		names[count][child_len] = '\0';
		count++;
	}

	return count;
}

static int gitt_build_tree(struct gitt_file *files, uint8_t file_num,
			   char (*const blob_sha1)[GITT_SHA1_HEX_SIZE + 1],
			   const char *dir,
			   struct gitt_tree *out,
			   struct gitt_tree *pool, uint8_t *pool_idx)
{
	uint8_t i;
	uint8_t entry = 0;
	uint8_t child_num;
	char d[GITT_FILE_PATH_SIZE];
	char children[GITT_MAX_DIR_CHILDREN][GITT_FILE_PATH_SIZE];
	int ret;

	/* Files directly under <dir> */
	for (i = 0; i < file_num; i++) {
		if (gitt_file_dir(files[i].path, d, sizeof(d)))
			return -GITT_ERRNO_INVAL;
		if (strcmp(d, dir))
			continue;

		if (entry >= GITT_TREE_MAX_ENTRIES) {
			gitt_log_error("Too many entries in %s\n", dir);
			return -GITT_ERRNO_INVAL;
		}
		strcpy(out->entries[entry].mode, GITT_TREE_FILE_MODE);
		strncpy(out->entries[entry].name, gitt_file_base(files[i].path),
			GITT_TREE_NAME_SIZE - 1);
		out->entries[entry].name[GITT_TREE_NAME_SIZE - 1] = '\0';
		strcpy(out->entries[entry].sha1, blob_sha1[i]);
		entry++;
	}

	/* Child directories */
	child_num = gitt_collect_children(files, file_num, dir, children);
	for (i = 0; i < child_num; i++) {
		char child_dir[GITT_FILE_PATH_SIZE];
		struct gitt_tree *sub;

		if (entry >= GITT_TREE_MAX_ENTRIES) {
			gitt_log_error("Too many entries in %s\n", dir);
			return -GITT_ERRNO_INVAL;
		}
		if (*pool_idx >= GITT_MAX_TREES) {
			gitt_log_error("Too many trees\n");
			return -GITT_ERRNO_INVAL;
		}

		snprintf(child_dir, sizeof(child_dir), "%s%s%s", dir,
			 dir[0] ? "/" : "", children[i]);

		sub = &pool[(*pool_idx)++];
		memset(sub, 0, sizeof(*sub));
		ret = gitt_build_tree(files, file_num, blob_sha1, child_dir,
				      sub, pool, pool_idx);
		if (ret)
			return ret;

		strcpy(out->entries[entry].mode, GITT_TREE_DIR_MODE);
		strncpy(out->entries[entry].name, children[i],
			GITT_TREE_NAME_SIZE - 1);
		out->entries[entry].name[GITT_TREE_NAME_SIZE - 1] = '\0';
		strcpy(out->entries[entry].sha1, sub->sha1);
		entry++;
	}

	out->entry_num = entry;
	return gitt_tree_sha1_update(out);
}

/**
 * @brief Push files (a commit with a real tree of blob objects)
 *
 * @param g struct gitt
 * @param files the files to back up
 * @param file_num number of files
 * @param message the commit message (can be NULL)
 * @return int     0: no error
 * @return int other: error
 */
int gitt_commit_files(struct gitt *g, struct gitt_file *files,
		      uint8_t file_num, char *message)
{
	int retval;
	int ret;
	int count;
	struct gitt_commit commit = {0};
	struct gitt_obj objs[GITT_MAX_FILES + GITT_MAX_TREES + 1];
	struct gitt_tree *pool;
	struct gitt_tree root;
	uint8_t pool_idx = 0;
	uint8_t i;
	uint8_t obj_num = 0;
	char date[GITT_COMMIT_DATE_SIZE];
	char zone[GITT_COMMIT_ZONE_SIZE];
	char id[GITT_EMAIL_SIZE];
	char blob_sha1[GITT_MAX_FILES][GITT_SHA1_HEX_SIZE + 1];

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	if (files == NULL || !file_num) {
		gitt_log_error("Files cannot be null or empty\n");
		return -GITT_ERRNO_INVAL;
	}

	if (file_num > GITT_MAX_FILES) {
		gitt_log_error("Too many files\n");
		return -GITT_ERRNO_INVAL;
	}

	/*
	 * The author email is built from the device id; check that it
	 * fits before anything is allocated.
	 */
	if (strlen(g->device.id) &&
	    snprintf(NULL, 0, GITT_EMAIL_FORMAT, g->device.id,
		     GITT_VERSION) >= sizeof(id)) {
		gitt_log_error("Device id is too long\n");
		return -GITT_ERRNO_INVAL;
	}

	/* Compute the SHA-1 of every blob */
	for (i = 0; i < file_num; i++) {
		ret = gitt_blob_sha1_update(files[i].data, files[i].size,
					    blob_sha1[i]);
		if (ret)
			return ret;
	}

	/* Build the tree hierarchy (the root tree first, then the subtrees) */
	pool = (struct gitt_tree *)malloc(sizeof(struct gitt_tree) * GITT_MAX_TREES);
	if (!pool)
		return -GITT_ERRNO_NOMEM;
	memset(pool, 0, sizeof(struct gitt_tree) * GITT_MAX_TREES);

	memset(&root, 0, sizeof(root));
	ret = gitt_build_tree(files, file_num, blob_sha1, "", &root, pool,
			      &pool_idx);
	if (ret) {
		free(pool);
		return ret;
	}

	/* Fill the commit */
	commit.tree.sha1     = root.sha1;
	commit.parent.sha1   = GITT_COMMIT_AUTO_BASE;

	/* Fill date */
	if (g->get_date && !g->get_date(date, sizeof(date))) {
		commit.author.date     = date;
		commit.committer.date  = date;
	} else {
		commit.author.date     = GITT_DEFAULT_DATE;
		commit.committer.date  = GITT_DEFAULT_DATE;
	}

	/* Fill zone */
	if (g->get_zone && !g->get_zone(zone, sizeof(zone))) {
		commit.author.zone     = zone;
		commit.committer.zone  = zone;
	} else {
		commit.author.zone     = GITT_DEFAULT_ZONE;
		commit.committer.zone  = GITT_DEFAULT_ZONE;
	}

	/*
	 * Fill name and email (the id length was checked above). The
	 * committer carries the same identity as the author: the device
	 * both creates and commits the change (like 'git commit'), the
	 * GITT library is only the code that runs on it.
	 */
	if (strlen(g->device.id)) {
		snprintf(id, sizeof(id), GITT_EMAIL_FORMAT, g->device.id,
			 GITT_VERSION);
		commit.author.email    = id;
	} else {
		commit.author.email    = GITT_UNKNOWN_EMAIL;
	}
	commit.author.name     = strlen(g->device.name) ? g->device.name : GITT_UNKNOWN_NAME;
	commit.committer.email = commit.author.email;
	commit.committer.name  = commit.author.name;

	commit.message         = message ? message : "";

	/* Build the object list: blobs, trees, commit (last) */
	for (i = 0; i < file_num; i++) {
		objs[obj_num].type = GITT_OBJ_TYPE_BLOB;
		objs[obj_num].data = files[i].data;
		objs[obj_num].size = files[i].size;
		obj_num++;
	}

	objs[obj_num].type = GITT_OBJ_TYPE_TREE;
	objs[obj_num].data = &root;
	objs[obj_num].size = gitt_tree_length(&root);
	obj_num++;

	for (i = 0; i < pool_idx; i++) {
		objs[obj_num].type = GITT_OBJ_TYPE_TREE;
		objs[obj_num].data = &pool[i];
		objs[obj_num].size = gitt_tree_length(&pool[i]);
		obj_num++;
	}

	objs[obj_num].type = GITT_OBJ_TYPE_COMMIT;
	objs[obj_num].data = &commit;
	objs[obj_num].size = gitt_commit_length(&commit);
	obj_num++;

	/* Try to push */
	count = 0;
	do {
		retval = gitt_repository_push_commit(&g->repository, &commit,
						      objs, obj_num);
		count++;

		if (retval == -GITT_ERRNO_RETRY) {
			gitt_log_info("Retry count: %d\n", count);

			/* Update */
			ret = gitt_pull(g);
			if (ret) {
				free(pool);
				return ret;
			}
		} else {
			gitt_log_debug("End code: %d\n", retval);
			break;
		}
	} while (count < GITT_TRY_NUMBER);

	free(pool);
	return retval;
}

/**
 * @brief Free GITT
 *
 * @param g struct gitt
 */
void gitt_end(struct gitt *g)
{
	gitt_repository_end(&g->repository);

	g->get_date = NULL;
	g->get_zone = NULL;
	g->file_dump = NULL;
	g->url = NULL;
	g->privkey = NULL;
	g->buf = NULL;
	g->buf_len = 0;
}

/**
 * @brief Get the version of gitt
 *
 * @return char* version
 */
char *gitt_version(void)
{
	return GITT_VERSION;
}
