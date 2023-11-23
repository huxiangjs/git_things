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
#define DEBUG
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
	if (!unpack->buf || unpack->buf_len == 0) {
		gitt_log_error("Buffer cannot be empty or length equal to 0\n");
		return -1;
	}

	if (!unpack->obj_dump) {
		gitt_log_error("obj_dump cannot be empty\n");
		return -1;
	}

	unpack->version = 0;
	unpack->number = 0;
	unpack->pack_state = 0;
	gitt_sha1_init(&unpack->sha1);

	return 0;
}

static int gitt_unpack_obj_step(struct gitt_unpack *unpack)
{
#if 0
	uint32_t obj_size;
	uint8_t obj_type;
	uint8_t offset;
	uint8_t index = 0;
	uint16_t length;
	int ret;

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
	length = unpack->valid_len - index;
	ret = gitt_unpack_try_uncompress(unpack, unpack->buf + index, &length);
	if (ret >= 0) {
		if (unpack->obj_dump) {
			if (obj_size == ret) {
				struct gitt_obj obj;

				obj.type = (uint8_t)obj_type;
				obj.size = (uint16_t)ret;
				obj.data = (char *)unpack->zbuf;

				unpack->obj_dump(&obj);
			} else {
				gitt_log_error("The uncompress data size is inconsistent with that in the record\n");
			}
		}

		unpack->number--;
		gitt_unpack_propel(unpack, index + length);
		return 0;
	}
#endif

	return -1;
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
	uint16_t index = 0;
	int ret;
	const char *magic = "PACK";

	if (unpack->pack_state == 0xff)
		return -1;

	if (!size)
		goto out;

	/*
	 * Using a state machine to parse byte by byte
	 *
	 * File header:
	 * | 4byte magic | 4byte version | 4byte mumber of objects |
	 */

	/* state: 0 ~ 3 */
	while (index < size && unpack->pack_state < 4) {
		if (data[index] != magic[unpack->pack_state]) {
			unpack->pack_state = 0xff;
			return -1;
		}
		index++;
		unpack->pack_state++;
	}

	/* state: 4 ~ 7 */
	while (index < size && unpack->pack_state < 8) {
		unpack->version <<= 8;
		unpack->version |= data[index];
		index++;
		unpack->pack_state++;
	}

	/* state: 8 ~ 11 */
	while (index < size && unpack->pack_state < 12) {
		unpack->number <<= 8;
		unpack->number |= data[index];
		index++;
		unpack->pack_state++;
	}

	if (unpack->pack_state == 12 && unpack->work)
		unpack->work(unpack);

	// TODO: for test
	if (unpack->pack_state == 12)
		unpack->pack_state = 13;
#if 0
	case 12:
		/* Unpack objects */
		ret = gitt_unpack_obj_step(unpack);
		if (ret < 0) {
			unpack->pack_state = 14;
			return -1;
		} else if (ret > 0) {

		}
		if (!unpack->number)
			unpack->pack_state = 2;
		break;
	case 13:

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

		unpack->pack_state = 3;

		break;
	case 14:
		return -1;
	}
#endif

out:
	return 0;
}
