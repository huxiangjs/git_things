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
#include <gitt_repertory.h>
#include <gitt_errno.h>

static void gitt_obj_dump_callback(struct gitt_obj *obj)
{
	int ret;
	struct gitt_commit commit;
	struct gitt_unpack *unpack = gitt_containerof(obj, struct gitt_unpack, obj);
	struct gitt_repertory *repertory = gitt_containerof(unpack, struct gitt_repertory, unpack);

	if (obj->type == GITT_OBJ_TYPE_COMMIT && repertory->commit_dump) {
		ret = gitt_commit_parse(obj->data, obj->size, &commit);
		if (!ret)
			repertory->commit_dump(repertory, &commit);
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
 * @brief Initialization repertory
 *
 * @param repertory
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repertory_init(struct gitt_repertory *repertory)
{
	if (!repertory->privkey || !repertory->url) {
		gitt_log_error("Privkey and repertory cannot be empty\n");
		return -GITT_ERRNO_INVAL;
	}

	if (!repertory->buf || repertory->buf_len < 20) {
		gitt_log_error("Buffe cannot be empty and the length cannot be less than 20\n");
		return -GITT_ERRNO_INVAL;
	}

	repertory->head[0] = '\0';
	repertory->ssh = NULL;

	return 0;
}

/**
 * @brief Clone repertory
 *
 * @param repertory
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repertory_clone(struct gitt_repertory *repertory)
{
	repertory->head[0] = '\0';
	return gitt_repertory_pull(repertory);
}

static int gitt_pack_data_dump_callback(void *p, uint8_t *buf, uint16_t size)
{
	struct gitt_repertory *repertory = gitt_containerof(p, struct gitt_repertory, pack);

	gitt_log_debug("write pack: %ubyte\n", size);
	return gitt_command_write_pack(repertory->ssh, buf, size);
}

int gitt_repertory_push_commit(struct gitt_repertory *repertory, struct gitt_commit *commit)
{
	int ret;
	struct gitt_obj obj;
	char remote_head[41];
	char refs[32];

	gitt_log_debug("Start connecting\n");
	repertory->ssh = gitt_command_start_receive(repertory->url, repertory->privkey);
	if (!repertory->ssh)
		return -GITT_ERRNO_INVAL;

	gitt_log_debug("Get remote head\n");
	ret = gitt_command_get_head(repertory->ssh, remote_head, refs);
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
	ret = gitt_command_set_pack(repertory->ssh, remote_head, commit->id.sha1,
				    strlen(refs) ? refs : repertory->refs);
	if (ret)
		goto err0;

	/* Initialize header */
	repertory->pack.buf = repertory->buf;
	repertory->pack.buf_len = repertory->buf_len;
	repertory->pack.obj_num = 1;
	repertory->pack.data_dump = gitt_pack_data_dump_callback;
	ret = gitt_pack_init(&repertory->pack);
	if (ret)
		goto err0;

	/* Update to pack */
	obj.type = GITT_OBJ_TYPE_COMMIT;
	obj.data = commit;
	obj.size = gitt_commit_length(commit);
	ret = gitt_pack_update(&repertory->pack, &obj);
	if (ret)
		goto err1;

	gitt_pack_end(&repertory->pack);

	ret = gitt_command_get_state(repertory->ssh);
	if (ret)
		goto err0;

	gitt_command_end(repertory->ssh);

	/* Update head */
	memcpy(repertory->head, commit->id.sha1, sizeof(commit->id.sha1));
	if (strlen(refs))
		strcpy(repertory->refs, refs);
	gitt_log_debug("Head updated: %s\n", repertory->head);

	return 0;

err1:
	gitt_pack_end(&repertory->pack);
err0:
	gitt_command_end(repertory->ssh);
	return ret;
}

/**
 * @brief Pull repertory (Get new commits)
 *
 * @param repertory
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_repertory_pull(struct gitt_repertory *repertory)
{
	int ret;
	char remote_head[41];
	char refs[32];

	gitt_log_debug("Start connecting\n");
	repertory->ssh = gitt_command_start_upload(repertory->url, repertory->privkey);
	if (!repertory->ssh)
		return -GITT_ERRNO_INVAL;

	gitt_log_debug("Get remote head\n");
	ret = gitt_command_get_head(repertory->ssh, remote_head, refs);
	if (ret)
		goto err0;

	/* Clone || Pull */
	if (!strlen(repertory->head)) {
		gitt_log_debug("Start clone\n");

		ret = gitt_command_want(repertory->ssh, remote_head, NULL);
		if (ret)
			goto err0;

		/* Update head */
		memcpy(repertory->head, remote_head, sizeof(remote_head));
		if (strlen(refs))
			strcpy(repertory->refs, refs);
		gitt_log_debug("Head updated: %s\n", repertory->head);
	} else if (memcmp(repertory->head, remote_head, sizeof(remote_head))) {
		gitt_log_debug("Start pull\n");

		ret = gitt_command_want(repertory->ssh, remote_head, repertory->head);
		if (ret)
			goto err0;

		/* Update head */
		memcpy(repertory->head, remote_head, sizeof(remote_head));
		if (strlen(refs))
			strcpy(repertory->refs, refs);
		gitt_log_debug("Head updated: %s\n", repertory->head);
	} else {
		gitt_log_debug("Already up to date\n");

		ret = gitt_command_say_byebye(repertory->ssh);
		if (ret)
			goto err0;

		gitt_command_end(repertory->ssh);
		return 0;
	}

	/* Initialize Unpack and prepare to unpack */
	repertory->unpack.buf = repertory->buf;
	repertory->unpack.buf_len = repertory->buf_len;
	repertory->unpack.header_dump = gitt_unpack_header_dump_callback;
	repertory->unpack.obj_dump = gitt_obj_dump_callback;
	repertory->unpack.verify_dump = gitt_unpack_verify_dump_callback;
	ret = gitt_unpack_init(&repertory->unpack);
	if (ret)
		goto err0;

	gitt_log_debug("Get pack\n");
	ret = gitt_command_get_pack(repertory->ssh, gitt_command_pack_dump_callback,
				    &repertory->unpack);
	if (ret)
		goto err1;

	gitt_unpack_end(&repertory->unpack);
	gitt_command_end(repertory->ssh);

	return 0;

err1:
	gitt_unpack_end(&repertory->unpack);
err0:
	gitt_command_end(repertory->ssh);
	return -GITT_ERRNO_INVAL;
}

int gitt_repertory_update_head(struct gitt_repertory *repertory)
{
	struct gitt_ssh* ssh;
	int ret;

	ssh = gitt_command_start_upload(repertory->url, repertory->privkey);
	if (!ssh)
		return -GITT_ERRNO_INVAL;

	ret = gitt_command_get_head(ssh, repertory->head, repertory->refs);
	if (ret)
		goto err;

	ret = gitt_command_say_byebye(ssh);
	if (ret)
		goto err;

	gitt_command_end(ssh);
	gitt_log_debug("Head updated: %s\n", repertory->head);

	if (!strlen(repertory->refs))
		return -GITT_ERRNO_INVAL;

	return 0;

err:
	gitt_command_end(ssh);
	return -GITT_ERRNO_INVAL;
}

int gitt_repertory_end(struct gitt_repertory *repertory)
{
	repertory->privkey = NULL;
	repertory->url = NULL;
	repertory->buf = NULL;
	repertory->buf_len = 0;

	return 0;
}
