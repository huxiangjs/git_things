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
#include <gitt_type.h>
#include <gitt_log.h>
#include <gitt_tree.h>
#include <gitt_command.h>
#include <gitt_repository.h>
#include <gitt_errno.h>

/* SHA-1 digest size, bytes; hex string length, chars (no NUL) */
#define GITT_SHA1_SIZE		20
#define GITT_SHA1_HEX_SIZE	40

/* The 40 zero chars: the "no id yet" placeholder of a new ref */
#define GITT_SHA1_ZERO		"0000000000000000000000000000000000000000"

/* Minimum user buffer: the pack ends with a 20 byte SHA-1 checksum */
#define GITT_MIN_BUF_SIZE	20

/*
 * The file registry (path <-> SHA-1) helpers
 */

static int gitt_repository_file_add(struct gitt_repository *repository,
				    const char *path, const char *sha1)
{
	struct gitt_repository_file *file;

	if (repository->file_num >= GITT_REPOSITORY_MAX_FILES) {
		gitt_log_error("File registry is full\n");
		return -GITT_ERRNO_INVAL;
	}

	file = &repository->files[repository->file_num++];
	if (strlen(path) + 1 > sizeof(file->path)) {
		gitt_log_error("File path is too long: %s\n", path);
		return -GITT_ERRNO_INVAL;
	}
	strcpy(file->path, path);
	strcpy(file->sha1, sha1);
	gitt_log_debug("File registered: %s\n", path);
	return 0;
}

static int gitt_repository_tree_add(struct gitt_repository *repository,
				    const char *sha1, const char *path)
{
	uint8_t i;

	if (repository->tree_num >= GITT_REPOSITORY_MAX_TREES) {
		gitt_log_error("Tree registry is full\n");
		return -GITT_ERRNO_INVAL;
	}
	if (strlen(path) + 1 > sizeof(repository->tree_path[0])) {
		gitt_log_error("Tree path is too long: %s\n", path);
		return -GITT_ERRNO_INVAL;
	}
	i = repository->tree_num++;
	strcpy(repository->tree_sha1[i], sha1);
	strcpy(repository->tree_path[i], path);
	gitt_log_debug("Tree registered: %s (%s)\n", path, sha1);
	return 0;
}

static void gitt_obj_dump_callback(struct gitt_obj *obj)
{
	struct gitt_commit commit = {0};
	struct gitt_unpack *unpack = gitt_containerof(obj, struct gitt_unpack, obj);
	struct gitt_repository *repository = gitt_containerof(unpack, struct gitt_repository, unpack);
	int ret;

	if (obj->type != GITT_OBJ_TYPE_COMMIT)
		/* Trees and blobs are handled in gitt_repository_checkout() */
		return;

	ret = gitt_commit_parse(obj->data, (uint16_t)obj->size, &commit);
	if (ret)
		return;

	repository->commit_dump(repository, &commit);

	/*
	 * Remember the root tree of the tip commit only. The pack can
	 * contain other commits (the parent in a shallow fetch, older
	 * history in a full fetch) and none of them must overwrite the
	 * tree of the tip.
	 */
	if (!strcmp(obj->sha1, repository->head)) {
		if (strlen(commit.tree.sha1))
			strcpy(repository->root_tree_sha1, commit.tree.sha1);
	}
}

/*
 * Restore the file tree from the object store
 *
 * Runs after the whole pack has been received: the pack order does not
 * guarantee that a tree comes before the blobs it references, so the
 * tree is walked from the object store (which holds every received
 * object), not while streaming.
 */

static void gitt_repository_sha_to_hex(const uint8_t sha1[GITT_SHA1_SIZE],
				       char hex[GITT_SHA1_HEX_SIZE + 1])
{
	static const char *tables = "0123456789abcdef";
	uint8_t i;

	for (i = 0; i < GITT_SHA1_SIZE; i++) {
		hex[i * 2] = tables[sha1[i] >> 4];
		hex[i * 2 + 1] = tables[sha1[i] & 0xf];
	}
	hex[GITT_SHA1_HEX_SIZE] = '\0';
}

static struct gitt_unpack_obj_entry *gitt_repository_store_find(struct gitt_repository *repository,
								uint8_t type, const char *sha1_hex)
{
	char hex[GITT_SHA1_HEX_SIZE + 1];
	uint8_t i;

	for (i = 0; i < repository->unpack.store_num; i++) {
		if (repository->unpack.store[i].type != type)
			continue;
		gitt_repository_sha_to_hex(repository->unpack.store[i].sha1, hex);
		if (!strcmp(hex, sha1_hex))
			return &repository->unpack.store[i];
	}

	return NULL;
}

static int gitt_repository_tree_walk(struct gitt_repository *repository,
				     const char *path, const char *tree_sha1)
{
	struct gitt_unpack_obj_entry *entry;
	struct gitt_tree tree;
	char fullpath[GITT_REPOSITORY_MAX_PATH];
	uint8_t i;
	int ret;

	entry = gitt_repository_store_find(repository, GITT_OBJ_TYPE_TREE, tree_sha1);
	if (!entry) {
		gitt_log_error("Missing tree object: %s\n", tree_sha1);
		return -GITT_ERRNO_INVAL;
	}

	ret = gitt_tree_parse((char *)entry->data, (uint16_t)entry->size, &tree);
	if (ret) {
		gitt_log_error("Parse tree fail\n");
		return ret;
	}

	for (i = 0; i < tree.entry_num; i++) {
		ret = snprintf(fullpath, sizeof(fullpath),
			       "%s%s%s", path,
			       path[0] ? "/" : "", tree.entries[i].name);

		if (ret <= 0 || ret >= sizeof(fullpath)) {
			gitt_log_error("File path is too long\n");
			return -GITT_ERRNO_INVAL;
		}

		if (!strcmp(tree.entries[i].mode, GITT_TREE_DIR_MODE)) {
			ret = gitt_repository_tree_add(repository,
					tree.entries[i].sha1, fullpath);
			if (ret)
				return ret;
			ret = gitt_repository_tree_walk(repository, fullpath,
							tree.entries[i].sha1);
			if (ret)
				return ret;
		} else {
			ret = gitt_repository_file_add(repository,
					fullpath, tree.entries[i].sha1);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int gitt_repository_checkout(struct gitt_repository *repository)
{
	struct gitt_commit commit = {0};
	char hex[GITT_SHA1_HEX_SIZE + 1];
	uint8_t i;
	int ret;

	/* The registry describes the tree of the current head: rebuild it */
	repository->file_num = 0;
	repository->tree_num = 0;

	/* Find the tip commit in the store */
	for (i = 0; i < repository->unpack.store_num; i++) {
		if (repository->unpack.store[i].type != GITT_OBJ_TYPE_COMMIT)
			continue;
		gitt_repository_sha_to_hex(repository->unpack.store[i].sha1, hex);
		if (strcmp(hex, repository->head))
			continue;

		ret = gitt_commit_parse((char *)repository->unpack.store[i].data,
					(uint16_t)repository->unpack.store[i].size,
					&commit);
		if (ret) {
			gitt_log_error("Parse commit fail\n");
			return ret;
		}

		if (!strlen(commit.tree.sha1)) {
			gitt_log_error("Commit has no tree\n");
			return -GITT_ERRNO_INVAL;
		}

		/* Walk the root tree and collect every file */
		ret = gitt_repository_tree_walk(repository, "", commit.tree.sha1);
		if (ret)
			return ret;

		break;
	}
	if (i == repository->unpack.store_num) {
		gitt_log_error("Tip commit not found in the pack\n");
		return -GITT_ERRNO_INVAL;
	}

	/* Write out every blob */
	for (i = 0; i < repository->file_num; i++) {
		struct gitt_unpack_obj_entry *entry;

		entry = gitt_repository_store_find(repository, GITT_OBJ_TYPE_BLOB,
						   repository->files[i].sha1);
		if (!entry) {
			gitt_log_error("Missing blob: %s\n",
				       repository->files[i].sha1);
			return -GITT_ERRNO_INVAL;
		}
		if (repository->blob_dump)
			repository->blob_dump(repository, repository->files[i].path,
					      (uint8_t *)entry->data, entry->size);
	}

	return 0;
}

static int gitt_command_pack_dump_callback(void *param, char *data, int size)
{
	struct gitt_unpack *unpack = (struct gitt_unpack *)param;

	return gitt_unpack_update(unpack, (uint8_t *)data, (uint16_t)size);
}

static void gitt_unpack_header_dump_callback(uint32_t *version, uint32_t *number)
{
	gitt_log_debug("version:%u, number of objects:%u; ", *version, *number);
}

static void gitt_unpack_verify_dump_callback(bool pass, struct gitt_sha1 *sha1)
{
	char sha1_hex[GITT_SHA1_HEX_SIZE + 1];
	int ret;

	gitt_log_debug("verify %s, ", pass ? "pass" : "not pass");

	ret = gitt_sha1_hexdigest(sha1, sha1_hex);
	if (!ret)
		gitt_log_debug("SHA-1: %s\n", sha1_hex);
}

/**
 * @brief Initialization repository
 *
 * @param repository
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repository_init(struct gitt_repository *repository)
{
	if (!repository->privkey || !repository->url) {
		gitt_log_error("Privkey and repository cannot be empty\n");
		return -GITT_ERRNO_INVAL;
	}

	if (!repository->buf || repository->buf_len < GITT_MIN_BUF_SIZE) {
		gitt_log_error("Buffer cannot be empty and the length cannot be less than 20\n");
		return -GITT_ERRNO_INVAL;
	}

	repository->head[0] = '\0';
	repository->ssh = NULL;
	repository->shallow = 0;
	repository->files_loaded = 0;
	repository->root_tree_sha1[0] = '\0';
	repository->file_num = 0;
	repository->tree_num = 0;

	return 0;
}

/**
 * @brief Clone repository
 *
 * @param repository
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repository_clone(struct gitt_repository *repository)
{
	repository->head[0] = '\0';
	return gitt_repository_pull(repository);
}

static int gitt_pack_data_dump_callback(void *p, uint8_t *buf, uint16_t size)
{
	struct gitt_repository *repository = gitt_containerof(p, struct gitt_repository, pack);

	gitt_log_debug("write pack: %ubyte\n", size);
	return gitt_command_write_pack(repository->ssh, buf, size);
}

/**
 * @brief Push commit (with the tree and blob objects)
 *
 * @param repository
 * @param commit the new commit (its id must be up to date)
 * @param objs the objects to push: all trees and blobs, the commit object
 *        must be the last one
 * @param obj_num number of objects
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repository_push_commit(struct gitt_repository *repository,
				struct gitt_commit *commit,
				struct gitt_obj *objs, uint8_t obj_num)
{
	int ret;
	uint8_t i;
	char remote_head[GITT_SHA1_HEX_SIZE + 1];
	char refs[GITT_REPOSITORY_REF_SIZE];
	char parent[GITT_SHA1_HEX_SIZE + 1];
	int auto_base = 0;

	gitt_log_debug("Start connecting\n");
	repository->ssh = gitt_command_start_receive(repository->url, repository->privkey);
	if (!repository->ssh)
		return -GITT_ERRNO_INVAL;

	gitt_log_debug("Get remote head\n");
	ret = gitt_command_get_head(repository->ssh, remote_head, refs);
	if (ret)
		goto err0;

	/*
	 * A 40-zero head (git-receive-pack's way of advertising an empty
	 * repository) means there is no head yet.
	 */
	{
		int zero = 1;
		for (i = 0; i < GITT_SHA1_HEX_SIZE; i++)
			if (remote_head[i] != '0') {
				zero = 0;
				break;
			}
		if (zero && strlen(remote_head) == GITT_SHA1_HEX_SIZE)
			remote_head[0] = '\0';
	}

	/*
	 * Auto base: resolve to the current remote head. The resolved
	 * value is kept in this function's buffer and the AUTO_BASE
	 * marker is restored before returning, so a retry (after the
	 * remote head has moved) is resolved against the new head.
	 */
	if (commit->parent.sha1 && !strcmp(commit->parent.sha1, GITT_COMMIT_AUTO_BASE)) {
		auto_base = 1;
		strcpy(parent, remote_head);
		commit->parent.sha1 = parent;
	}

	/* Check parent */
	if (commit->parent.sha1 && strlen(commit->parent.sha1) &&
	    strcmp(commit->parent.sha1, remote_head)) {
		ret = -GITT_ERRNO_RETRY;
		gitt_log_debug("The current local record is not up to date\n");
		goto err0;
	}

	/* Update commit id */
	ret = gitt_commit_sha1_update(commit);
	if (ret) {
		gitt_log_error("Update commit id fail\n");
		goto err0;
	}

	/*
	 * Resolving the auto base may have changed the parent line,
	 * which changes the length of the commit object.
	 */
	objs[obj_num - 1].size = gitt_commit_length(commit);

	/*
	 * Choose the ref to update. When the remote has no refs yet,
	 * this creates a new branch (the old id is 40 zeros).
	 */
	{
		static const char zero_id[GITT_SHA1_HEX_SIZE + 1] = GITT_SHA1_ZERO;
		const char *old_id = strlen(remote_head) ? remote_head : zero_id;
		const char *ref = strlen(refs) ? refs :
			(strlen(repository->refs) ? repository->refs :
				GITT_DEFAULT_REF);

		gitt_log_debug("Set pack\n");
		ret = gitt_command_set_pack(repository->ssh, old_id, commit->id.sha1,
					    ref);
		if (ret)
			goto err0;
	}

	/* Initialize header */
	repository->pack.buf = repository->buf;
	repository->pack.buf_len = repository->buf_len;
	repository->pack.obj_num = obj_num;
	repository->pack.data_dump = gitt_pack_data_dump_callback;
	ret = gitt_pack_init(&repository->pack);
	if (ret)
		goto err0;

	/* Update to pack */
	for (i = 0; i < obj_num; i++) {
		gitt_log_debug("Pack object %u: %s\n", i,
			       GITT_OBJ_STR(objs[i].type));
		ret = gitt_pack_update(&repository->pack, &objs[i]);
		if (ret)
			goto err1;
	}

	gitt_pack_end(&repository->pack);

	ret = gitt_command_get_push_status(repository->ssh);
	if (ret)
		goto err0;

	gitt_command_end(repository->ssh);

	/* Update head */
	memcpy(repository->head, commit->id.sha1, sizeof(commit->id.sha1));
	if (strlen(refs))
		strcpy(repository->refs, refs);
	else if (!strlen(repository->refs))
		/* A new ref was created */
		strcpy(repository->refs, GITT_DEFAULT_REF);
	gitt_log_debug("Head updated: %s\n", repository->head);

	if (auto_base)
		commit->parent.sha1 = GITT_COMMIT_AUTO_BASE;

	return 0;

err1:
	gitt_pack_end(&repository->pack);
err0:
	if (auto_base)
		commit->parent.sha1 = GITT_COMMIT_AUTO_BASE;
	gitt_command_end(repository->ssh);
	return ret;
}

/**
 * @brief Pull repository (Get new commits)
 *
 * @param repository
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repository_pull(struct gitt_repository *repository)
{
	/*
	 * Default error for the paths below that only know that
	 * "something went wrong"; every checked call overwrites it.
	 */
	int ret = -GITT_ERRNO_INVAL;
	char remote_head[GITT_SHA1_HEX_SIZE + 1];
	char refs[GITT_REPOSITORY_REF_SIZE];

	gitt_log_debug("Start connecting\n");
	repository->ssh = gitt_command_start_upload(repository->url, repository->privkey);
	if (!repository->ssh)
		return -GITT_ERRNO_INVAL;

	gitt_log_debug("Get remote head\n");
	ret = gitt_command_get_head(repository->ssh, remote_head, refs);
	if (ret)
		goto err0;

	/* The remote repository is empty: nothing to fetch */
	if (!strlen(remote_head)) {
		gitt_log_debug("Remote repository is empty\n");

		ret = gitt_command_say_byebye(repository->ssh);
		if (ret)
			goto err0;

		gitt_command_end(repository->ssh);
		return 0;
	}

	/*
	 * Clone || Pull. A fetch is needed when:
	 * - there is no local head yet (clone)
	 * - the pull is shallow (a fresh snapshot is always wanted)
	 * - the head is known but its files were never fetched in this
	 *   session (e.g. only 'update_head' ran, as during init)
	 * - the remote head has advanced
	 */
	if (!strlen(repository->head) ||
	    repository->shallow ||
	    !repository->files_loaded ||
	    memcmp(repository->head, remote_head, sizeof(remote_head))) {
		gitt_log_debug("Start %s\n",
			       !strlen(repository->head) ? "clone" : "pull");

		/*
		 * The MCU is stateless: it keeps no objects between pulls,
		 * so 'have' is never sent. Every fetch is a fresh snapshot
		 * of the tip tree (an incremental pack would be missing the
		 * blobs of the unchanged files). Shallow fetches are
		 * repeated even when the local head already matches,
		 * because the files themselves are not kept.
		 */
		ret = gitt_command_want(repository->ssh, remote_head, NULL,
					repository->shallow ? 1 : 0);
		if (ret)
			goto err0;

		/* Update head */
		memcpy(repository->head, remote_head, sizeof(remote_head));
		if (strlen(refs))
			strcpy(repository->refs, refs);
		gitt_log_debug("Head updated: %s\n", repository->head);
	} else {
		gitt_log_debug("Already up to date\n");

		ret = gitt_command_say_byebye(repository->ssh);
		if (ret)
			goto err0;

		gitt_command_end(repository->ssh);
		return 0;
	}

	/* Initialize Unpack and prepare to unpack */
	repository->unpack.buf = repository->buf;
	repository->unpack.buf_len = repository->buf_len;
	repository->unpack.header_dump = gitt_unpack_header_dump_callback;
	repository->unpack.obj_dump = gitt_obj_dump_callback;
	repository->unpack.verify_dump = gitt_unpack_verify_dump_callback;
	ret = gitt_unpack_init(&repository->unpack);
	if (ret)
		goto err0;

	gitt_log_debug("Get pack\n");
	ret = gitt_command_get_pack(repository->ssh, gitt_command_pack_dump_callback,
				    &repository->unpack);
	if (ret)
		goto err1;

	/* Restore the file tree from the received objects */
	gitt_log_debug("Checkout\n");
	ret = gitt_repository_checkout(repository);
	if (ret)
		goto err1;

	repository->files_loaded = 1;
	gitt_unpack_store_clear(&repository->unpack);
	gitt_unpack_end(&repository->unpack);
	gitt_command_end(repository->ssh);

	return 0;

err1:
	gitt_unpack_store_clear(&repository->unpack);
	gitt_unpack_end(&repository->unpack);
err0:
	gitt_command_end(repository->ssh);
	return ret;
}

int gitt_repository_update_head(struct gitt_repository *repository)
{
	struct gitt_ssh* ssh;
	int ret = -GITT_ERRNO_INVAL;

	ssh = gitt_command_start_upload(repository->url, repository->privkey);
	if (!ssh)
		return ret;

	ret = gitt_command_get_head(ssh, repository->head, repository->refs);
	if (ret)
		goto err;

	ret = gitt_command_say_byebye(ssh);
	if (ret)
		goto err;

	gitt_command_end(ssh);
	gitt_log_debug("Head updated: %s\n", repository->head);

	/*
	 * An empty head and refs mean the remote repository has no
	 * commits yet, which is a valid state (first push).
	 */
	return 0;

err:
	gitt_command_end(ssh);
	return ret;
}

int gitt_repository_end(struct gitt_repository *repository)
{
	repository->privkey = NULL;
	repository->url = NULL;
	repository->buf = NULL;
	repository->buf_len = 0;

	return 0;
}
