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

static void gitt_obj_dump_callback(struct gitt_obj *obj)
{
	int ret;
	struct gitt_commit commit;
	struct gitt_unpack *unpack = gitt_containerof(obj, struct gitt_unpack, obj);
	struct gitt_repertory *repertory = gitt_containerof(unpack, struct gitt_repertory, unpack);

	gitt_log_debug("type:%s, size:%d\n", gitt_obj_types[obj->type], obj->size);

	if (obj->type == 1 && repertory->commit_dump) {
		ret = gitt_commit_parse(obj->data, obj->size, &commit);
		if (!ret)
			repertory->commit_dump(&commit);
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
		return -1;
	}

	if (!repertory->buf || repertory->buf_len < 20) {
		gitt_log_error("Buffe cannot be empty and the length cannot be less than 20\n");
		return -1;
	}

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
	repertory->head_sha1[0] = '\0';
	return gitt_repertory_pull(repertory);
}

int gitt_repertory_push(struct gitt_repertory *repertory)
{

	return 0;
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
	struct gitt_ssh* ssh;
	int ret;
	char remote_head[41];

	gitt_log_debug("Step: start\n");
	ssh = gitt_command_start_upload(repertory->url, repertory->privkey);
	if (!ssh)
		return -1;

	gitt_log_debug("Step: get remote head\n");
	ret = gitt_command_get_head(ssh, remote_head);
	if (ret)
		goto err0;

	/* Clone || Pull */
	if (!strlen(repertory->head_sha1)) {
		gitt_log_debug("Step: clone\n");

		ret = gitt_command_want(ssh, remote_head, NULL);
		if (ret)
			goto err0;

		/* Update head */
		memcpy(repertory->head_sha1, remote_head, sizeof(remote_head));
	} else if (memcmp(repertory->head_sha1, remote_head, sizeof(remote_head))) {
		gitt_log_debug("Step: pull\n");

		ret = gitt_command_want(ssh, remote_head, repertory->head_sha1);
		if (ret)
			goto err0;

		/* Update head */
		memcpy(repertory->head_sha1, remote_head, sizeof(remote_head));
	} else {
		gitt_log_debug("Already up to date\n");

		ret = gitt_command_say_byebye(ssh);
		if (ret)
			goto err0;

		gitt_command_end(ssh);
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

	gitt_log_debug("Step: get pack\n");
	ret = gitt_command_get_pack(ssh, gitt_command_pack_dump_callback, &repertory->unpack);
	if (ret)
		goto err1;

	gitt_unpack_end(&repertory->unpack);
	gitt_command_end(ssh);

	return 0;

err1:
	gitt_unpack_end(&repertory->unpack);
err0:
	gitt_command_end(ssh);
	return -1;
}

int gitt_repertory_update_head(struct gitt_repertory *repertory)
{
	struct gitt_ssh* ssh;
	int ret;

	ssh = gitt_command_start_upload(repertory->url, repertory->privkey);
	if (!ssh)
		return -1;

	ret = gitt_command_get_head(ssh, repertory->head_sha1);
	if (ret)
		goto err;

	ret = gitt_command_say_byebye(ssh);
	if (ret)
		goto err;

	gitt_command_end(ssh);
	gitt_log_debug("SHA-1 updated: %s\n", repertory->head_sha1);

	return 0;

err:
	gitt_command_end(ssh);
	return -1;
}

int gitt_repertory_end(struct gitt_repertory *repertory)
{
	repertory->privkey = NULL;
	repertory->url = NULL;
	repertory->buf = NULL;
	repertory->buf_len = 0;

	return 0;
}
