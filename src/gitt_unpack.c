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
#include <zlib.h>
#include <gitt_unpack.h>
#include <gitt_log.h>

/**
 * @brief Initialization handle
 *
 * @param unpack
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_unpack_init(struct gitt_unpack *unpack)
{
	if (!unpack->buf || unpack->buf_len == 0 ||
	    !unpack->zbuf || unpack->zbuf_len == 0) {
		gitt_log_error("Buffer cannot be empty or length equal to 0\n");
		return -1;
	}

	if (!unpack->obj_dump) {
		gitt_log_error("obj_dump cannot be empty\n");
		return -1;
	}

	unpack->valid_len = 0;
	unpack->state = 0;
	unpack->offset = 0;
	gitt_sha1_init(&unpack->sha1);

	return 0;
}

static void gitt_unpack_propel(struct gitt_unpack *unpack, uint16_t len)
{
	uint16_t copy_size = unpack->valid_len - len;
	uint16_t index = 0;

	if (len == 0)
		return;

	gitt_sha1_update(&unpack->sha1, unpack->buf, len);

	while (index < copy_size) {
		unpack->buf[index] = unpack->buf[index + len];
		index++;
	}

	unpack->valid_len -= len;
	unpack->offset += len;
}

static int gitt_unpack_obj_step(struct gitt_unpack *unpack)
{
	uint32_t obj_size = 0;
	uint8_t obj_type;
	uint8_t offset;
	uint8_t index = 0;
	uint16_t tmp;
	uLong length;
	int err;

	if (unpack->valid_len <= 2)
		return -1;

	/* Object type */
	obj_type = unpack->buf[index] >> 4 & 0x7;
	if (!obj_type) {
		gitt_log_error("Invalid object type\n");
		return -1;
	}
	gitt_log_debug("Object type: %s\n", gitt_obj_types[obj_type]);

	obj_size = unpack->buf[index] & 0xf;
	index++;

	/* Object size */
	offset = 4;
	while ((index < 5) && (index < unpack->valid_len) &&
	       (unpack->buf[index - 1] & 0x80)) {
		obj_size |= (unpack->buf[index] & 0x7f) << offset;
		offset += 7;
		index++;
	}

	if (unpack->buf[index - 1] & 0x80) {
		if (index < 5)
			gitt_log_debug("Need more data\n");
		else
			gitt_log_error("Object too big\n");
		return -1;
	}
	gitt_log_debug("Object size: %u\n", obj_size);
	gitt_log_debug("Object offset: %u\n", unpack->offset);

	if (obj_size > unpack->zbuf_len - index) {
		gitt_log_error("Uncompress output buffer does not have enough space\n");
		return -1;
	}

	/* We don't deal with OFS_DELTA, so we skip its special parts directly. */
	if (obj_type == 6) {
		/* Skip offset size */
		while (index < unpack->valid_len && unpack->buf[index] & 0x80)
			index++;

		if (unpack->buf[index] & 0x80)
			return -1;

		index++;
	}

	/* Try to uncompress */
	tmp = index + 1;
	while (tmp < unpack->valid_len) {
		length = unpack->zbuf_len;

		err = uncompress((Bytef *)unpack->zbuf, &length,
				 (Bytef *)(unpack->buf + index),
				 (uLong)(tmp - index));
		/* Uncompress done */
		if (!err) {
			if (unpack->obj_dump) {
				if (obj_size == length) {
					struct gitt_obj obj;

					obj.type = (uint8_t)obj_type;
					obj.size = (uint16_t)length;
					obj.data = (char *)unpack->zbuf;

					unpack->obj_dump(&obj);
				} else {
					gitt_log_error("The uncompress data size is inconsistent with that in the record\n");
				}
			}

			unpack->number--;
			gitt_unpack_propel(unpack, tmp);
			return 0;
		}
		tmp++;
	}

	return -1;
}

static int gitt_unpack_try_work(struct gitt_unpack *unpack)
{
	switch (unpack->state) {
	case 0:
		if (unpack->valid_len < 12)
			return -1;
		/*
		 * File header
		 * | 4byte magic | 4byte version | 4byte mumber of objects |
		 */
		if (unpack->buf[0] != 'P' || unpack->buf[1] != 'A' ||
		    unpack->buf[2] != 'C' || unpack->buf[3] != 'K') {
			gitt_log_error("Not a valid pack file\n");
			return -1;
		}

		unpack->version = unpack->buf[4] << 24 |
				  unpack->buf[5] << 16 |
				  unpack->buf[6] << 8 |
				  unpack->buf[7];
		unpack->number = unpack->buf[8] << 24 |
				 unpack->buf[9] << 16 |
				 unpack->buf[10] << 8 |
				 unpack->buf[11];

		gitt_unpack_propel(unpack, 12);

		if (unpack->work)
			unpack->work(unpack);

		unpack->state = 1;
		break;
	case 1:
		/* Unpack objects */
		if(gitt_unpack_obj_step(unpack))
			return -1;
		if (!unpack->number)
			unpack->state = 2;
		break;
	case 2:
		if (unpack->valid_len < 20)
			return -1;

		/* 20byte SHA-1 */
		if (unpack->verify) {
			bool pass;
			uint8_t sha1[20];
			int ret;

			ret = gitt_sha1_digest(&unpack->sha1, sha1);
			if (ret)
				pass = false;
			else
				pass = (bool) !memcmp(unpack->buf, sha1, sizeof(sha1));

			unpack->verify(pass, &unpack->sha1);
		}

		unpack->state = 3;
		gitt_unpack_propel(unpack, 20);
		break;
	case 3:
		return -1;
	}

	return 0;
}

/**
 * @brief Add data
 *
 * @param unpack handle
 * @param data
 * @param size
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_unpack_update(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	uint16_t copy_size;
	uint16_t done_size = 0;
	uint16_t valid_size = size;
	int ret;

	/* Copy and try */
	do {
		copy_size = unpack->buf_len - unpack->valid_len;
		copy_size = valid_size < copy_size ? valid_size : copy_size;
		memcpy(unpack->buf + unpack->valid_len, data + done_size, copy_size);
		unpack->valid_len += copy_size;

		ret = gitt_unpack_try_work(unpack);
		if (unpack->valid_len == unpack->buf_len) {
			gitt_log_error("Unpacking error, please increase buffer\n");
			return -1;
		}

		done_size += copy_size;
		valid_size -= copy_size;

	} while (valid_size);

	/* If ret is 0, we can continue to try */
	while (!ret)
		ret = gitt_unpack_try_work(unpack);

	return 0;
}
