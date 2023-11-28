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

#include <gitt_pack.h>
#include <gitt_log.h>
#include <gitt_errno.h>
#include <gitt_commit.h>

#define GITT_PACK_STATE_INIT		0x00
#define GITT_PACK_STATE_STOP		0xff

static int gitt_pack_data_update(struct gitt_pack *pack, uint8_t *buf, uint16_t size)
{
	int ret;

	ret = gitt_sha1_update(&pack->sha1, buf, size);
	if (ret)
		return ret;

	if (pack->data_dump) {
		ret = pack->data_dump(pack, buf, size);
		if (ret)
			return ret;
	}

	return 0;
}

int gitt_pack_init(struct gitt_pack *pack)
{
	int ret;

	if (!pack->buf || pack->buf_len < 20) {
		gitt_log_error("Buffe cannot be empty and the length cannot be less than 20\n");
		return -GITT_ERRNO_NOMEM;
	}

	gitt_sha1_init(&pack->sha1);
	pack->state = GITT_PACK_STATE_INIT;

	/* 4byte magic */
	pack->buf[0] = 'P';
	pack->buf[1] = 'A';
	pack->buf[2] = 'C';
	pack->buf[3] = 'K';

	/* 4byte version */
	pack->buf[4] = 0;
	pack->buf[5] = 0;
	pack->buf[6] = 0;
	pack->buf[7] = 2;

	/* 4byte mumber of objects */
	pack->buf[8] = 0;
	pack->buf[9] = 0;
	pack->buf[10] = 0;
	pack->buf[11] = pack->obj_num;

	/* Dump */
	ret = gitt_pack_data_update(pack, pack->buf, 12);
	if (ret)
		return ret;

	return 0;
}

static int gitt_obj_data_dump(void *p, uint8_t *buf, uint16_t size, bool end)
{
	struct gitt_pack *pack = (struct gitt_pack *)p;
	uint16_t in_size = size;
	uint16_t out_size = pack->buf_len;
	int ret;

	ret = gitt_zlib_compress_update(&pack->zlib, buf, &in_size, pack->buf,
					&out_size, end);
	if (ret)
		return ret;

	if (out_size) {
		/* Dump data */
		ret = gitt_pack_data_update(pack, pack->buf, out_size);
		if (ret)
			return ret;
	}

	return 0;
}

int gitt_pack_update(struct gitt_pack *pack, struct gitt_obj *obj)
{
	int ret;
	uint8_t obj_head[3];
	uint8_t head_len = 0;

	if (!pack->obj_num)
		goto done;

	if (pack->state >= pack->obj_num) {
		gitt_log_error("Pack fail\n");
		return -GITT_ERRNO_INVAL;
	}

	obj_head[0] = (obj->type & 0x7) << 4;
	obj_head[0] |= obj->size & 0xf;
	obj->size >>= 4;
	head_len++;

	if (obj->size) {
		obj_head[0] |= 0x80;
		obj_head[1] = obj->size & 0x7f;
		head_len++;
		obj->size >>= 7;
	}

	if (obj->size) {
		obj_head[1] |= 0x80;
		obj_head[2] = obj->size & 0x7f;
		head_len++;
	}

	/* Dump head */
	ret = gitt_pack_data_update(pack, obj_head, head_len);
	if (ret)
		return ret;

	/* Build object */
	if (obj->type == GITT_OBJ_TYPE_COMMIT) {
		ret = gitt_zlib_compress_init(&pack->zlib);
		if (ret)
			return ret;

		ret = gitt_commit_build(gitt_obj_data_dump, pack,
					(struct gitt_commit *)obj->data);
		gitt_zlib_compress_end(&pack->zlib);

		if (ret)
			return ret;
	} else {
		gitt_log_error("Unsupported object type\n");
		return -GITT_ERRNO_INVAL;
	}

	pack->state++;

done:
	/* If done, add SHA-1 */
	if (pack->state == pack->obj_num) {
		ret = gitt_sha1_digest(&pack->sha1, pack->buf);
		if (ret)
			return ret;

		ret = pack->data_dump(pack, pack->buf, 20);
		if (ret)
			return ret;

		pack->state = GITT_PACK_STATE_STOP;
	}

	return 0;
}

void gitt_pack_end(struct gitt_pack *pack)
{
	pack->state = GITT_PACK_STATE_STOP;
}
