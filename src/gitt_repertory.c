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

#include <gitt_log.h>
#include <gitt_command.h>
#include <gitt_repertory.h>

static int gitt_command_pack_dump_callback(void *param, char *data, int size)
{
	struct gitt_unpack *unpack = (struct gitt_unpack *)param;

	return gitt_unpack_update(unpack, (uint8_t *)data, (uint16_t)size);
}

static void gitt_unpack_obj_callback(struct gitt_obj *obj)
{
	gitt_log_debug("type:%s, size:%d\n", gitt_obj_types[obj->type], obj->size);
	if (obj->type == 1 || obj->type == 2)
		gitt_log_debug("%.*s\n", obj->size, obj->data);
}

static void gitt_unpack_header_callback(uint32_t *version, uint32_t *number)
{
	gitt_log_debug("version:%u, number of objects:%u; ", *version, *number);
}

static void gitt_unpack_verify_callback(bool pass, struct gitt_sha1 *sha1)
{
	char sha1_hex[41];
	int ret;

	gitt_log_debug("verify %s, ", pass ? "pass" : "not pass");

	ret = gitt_sha1_hexdigest(sha1, sha1_hex);
	if (!ret)
		gitt_log_debug("SHA-1: %s\n", sha1_hex);
}

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

int gitt_repertory_clone(struct gitt_repertory *repertory)
{
	struct gitt_ssh* ssh;
	int ret;
	struct gitt_unpack unpack = {0};

	gitt_log_debug("Step: start\n");
	ssh = gitt_command_start_upload(repertory->url, repertory->privkey);
	if (!ssh)
		return -1;

	gitt_log_debug("Step: get head\n");
	ret = gitt_command_get_head(ssh, repertory->sha1);
	if (ret)
		goto err0;

	gitt_log_debug("Step: want\n");
	ret = gitt_command_want(ssh, repertory->sha1, NULL);
	if (ret)
		goto err0;

	/* Initialize Unpack and prepare to unpack */
	unpack.buf = repertory->buf;
	unpack.buf_len = repertory->buf_len;
	unpack.header_dump = gitt_unpack_header_callback;
	unpack.obj_dump = gitt_unpack_obj_callback;
	unpack.verify_dump = gitt_unpack_verify_callback;
	ret = gitt_unpack_init(&unpack);
	if (ret)
		goto err0;

	gitt_log_debug("Step: get pack\n");
	ret = gitt_command_get_pack(ssh, gitt_command_pack_dump_callback, &unpack);
	if (ret)
		goto err1;

	gitt_unpack_end(&unpack);
	gitt_command_end(ssh);
	gitt_log_debug("SHA-1 updated: %s\n", repertory->sha1);

	return 0;

err1:
	gitt_unpack_end(&unpack);
err0:
	gitt_command_end(ssh);
	return -1;
}

int gitt_repertory_push(struct gitt_repertory *repertory)
{

	return 0;
}

int gitt_repertory_pull(struct gitt_repertory *repertory)
{

	return 0;
}

int gitt_repertory_update_head(struct gitt_repertory *repertory)
{
	struct gitt_ssh* ssh;
	int ret;

	ssh = gitt_command_start_upload(repertory->url, repertory->privkey);
	if (!ssh)
		return -1;

	ret = gitt_command_get_head(ssh, repertory->sha1);
	if (ret)
		goto err;

	ret = gitt_command_say_byebye(ssh);
	if (ret)
		goto err;

	gitt_command_end(ssh);
	gitt_log_debug("SHA-1 updated: %s\n", repertory->sha1);

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
