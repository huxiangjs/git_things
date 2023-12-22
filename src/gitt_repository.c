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

#include <string.h>
#include <gitt_type.h>
#include <gitt_log.h>
#include <gitt_command.h>
#include <gitt_repository.h>
#include <gitt_errno.h>

static void gitt_obj_dump_callback(struct gitt_obj *obj)
{
	int ret;
	struct gitt_commit commit;
	struct gitt_unpack *unpack = gitt_containerof(obj, struct gitt_unpack, obj);
	struct gitt_repository *repository = gitt_containerof(unpack, struct gitt_repository, unpack);

	if (obj->type == GITT_OBJ_TYPE_COMMIT && repository->commit_dump) {
		ret = gitt_commit_parse(obj->data, obj->size, &commit);
		if (!ret)
			repository->commit_dump(repository, &commit);
	} else {
		gitt_log_info("Skip type:%s, size:%d\n", GITT_OBJ_STR(obj->type), obj->size);
	}
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
	char sha1_hex[41];
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

	if (!repository->buf || repository->buf_len < 20) {
		gitt_log_error("Buffe cannot be empty and the length cannot be less than 20\n");
		return -GITT_ERRNO_INVAL;
	}

	repository->head[0] = '\0';
	repository->ssh = NULL;

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

int gitt_repository_push_commit(struct gitt_repository *repository, struct gitt_commit *commit)
{
	int ret;
	struct gitt_obj obj;
	char remote_head[41];
	char refs[32];

	gitt_log_debug("Start connecting\n");
	repository->ssh = gitt_command_start_receive(repository->url, repository->privkey);
	if (!repository->ssh)
		return -GITT_ERRNO_INVAL;

	gitt_log_debug("Get remote head\n");
	ret = gitt_command_get_head(repository->ssh, remote_head, refs);
	if (ret)
		goto err0;

	/* Auto base */
	if (commit->parent.sha1 && !strcmp(commit->parent.sha1, GITT_COMMIT_AUTO_BASE))
		commit->parent.sha1 = remote_head;

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

	gitt_log_debug("Set pack\n");
	ret = gitt_command_set_pack(repository->ssh, remote_head, commit->id.sha1,
				    strlen(refs) ? refs : repository->refs);
	if (ret)
		goto err0;

	/* Initialize header */
	repository->pack.buf = repository->buf;
	repository->pack.buf_len = repository->buf_len;
	repository->pack.obj_num = 1;
	repository->pack.data_dump = gitt_pack_data_dump_callback;
	ret = gitt_pack_init(&repository->pack);
	if (ret)
		goto err0;

	/* Update to pack */
	obj.type = GITT_OBJ_TYPE_COMMIT;
	obj.data = commit;
	obj.size = gitt_commit_length(commit);
	ret = gitt_pack_update(&repository->pack, &obj);
	if (ret)
		goto err1;

	gitt_pack_end(&repository->pack);

	ret = gitt_command_get_state(repository->ssh);
	if (ret)
		goto err0;

	gitt_command_end(repository->ssh);

	/* Update head */
	memcpy(repository->head, commit->id.sha1, sizeof(commit->id.sha1));
	if (strlen(refs))
		strcpy(repository->refs, refs);
	gitt_log_debug("Head updated: %s\n", repository->head);

	return 0;

err1:
	gitt_pack_end(&repository->pack);
err0:
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
	int ret;
	char remote_head[41];
	char refs[32];

	gitt_log_debug("Start connecting\n");
	repository->ssh = gitt_command_start_upload(repository->url, repository->privkey);
	if (!repository->ssh)
		return -GITT_ERRNO_INVAL;

	gitt_log_debug("Get remote head\n");
	ret = gitt_command_get_head(repository->ssh, remote_head, refs);
	if (ret)
		goto err0;

	/* Clone || Pull */
	if (!strlen(repository->head)) {
		gitt_log_debug("Start clone\n");

		ret = gitt_command_want(repository->ssh, remote_head, NULL);
		if (ret)
			goto err0;

		/* Update head */
		memcpy(repository->head, remote_head, sizeof(remote_head));
		if (strlen(refs))
			strcpy(repository->refs, refs);
		gitt_log_debug("Head updated: %s\n", repository->head);
	} else if (memcmp(repository->head, remote_head, sizeof(remote_head))) {
		gitt_log_debug("Start pull\n");

		ret = gitt_command_want(repository->ssh, remote_head, repository->head);
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

	gitt_unpack_end(&repository->unpack);
	gitt_command_end(repository->ssh);

	return 0;

err1:
	gitt_unpack_end(&repository->unpack);
err0:
	gitt_command_end(repository->ssh);
	return -GITT_ERRNO_INVAL;
}

int gitt_repository_update_head(struct gitt_repository *repository)
{
	struct gitt_ssh* ssh;
	int ret;

	ssh = gitt_command_start_upload(repository->url, repository->privkey);
	if (!ssh)
		return -GITT_ERRNO_INVAL;

	ret = gitt_command_get_head(ssh, repository->head, repository->refs);
	if (ret)
		goto err;

	ret = gitt_command_say_byebye(ssh);
	if (ret)
		goto err;

	gitt_command_end(ssh);
	gitt_log_debug("Head updated: %s\n", repository->head);

	if (!strlen(repository->refs))
		return -GITT_ERRNO_INVAL;

	return 0;

err:
	gitt_command_end(ssh);
	return -GITT_ERRNO_INVAL;
}

int gitt_repository_end(struct gitt_repository *repository)
{
	repository->privkey = NULL;
	repository->url = NULL;
	repository->buf = NULL;
	repository->buf_len = 0;

	return 0;
}
