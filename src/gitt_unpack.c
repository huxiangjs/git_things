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

#define GITT_UNPACK_STATE_INIT		0x00
#define GITT_UNPACK_STATE_STOP		0xff

/**
 * @brief Initialization handle
 *
 * @param unpack
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_unpack_init(struct gitt_unpack *unpack)
{
	if (!unpack->buf || unpack->buf_len < 20) {
		gitt_log_error("Buffe cannot be empty and the length cannot be less than 20\n");
		return -1;
	}

	unpack->pack_state = GITT_UNPACK_STATE_INIT;
	unpack->obj_state = GITT_UNPACK_STATE_INIT;
	gitt_sha1_init(&unpack->sha1);

	return 0;
}

static int gitt_unpack_header_step(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	const char *magic = "PACK";
	uint16_t index = 0;

	/*
	 * File header:
	 * | 4byte magic | 4byte version | 4byte mumber of objects |
	 */

	/* State: 0  (Initialization) */
	if (unpack->pack_state == 0) {
		unpack->version = 0;
		unpack->number = 0;
	}

	/* State: 0 ~ 3  (4byte magic) */
	while (index < size && unpack->pack_state < 4) {
		if (data[index] != magic[unpack->pack_state]) {
			unpack->pack_state = GITT_UNPACK_STATE_STOP;
			gitt_log_error("File format is incorrect\n");
			return -1;
		}
		index++;
		unpack->pack_state++;
	}

	/* State: 4 ~ 7  (4byte version) */
	while (index < size && unpack->pack_state < 8) {
		unpack->version <<= 8;
		unpack->version |= data[index];
		index++;
		unpack->pack_state++;
	}

	/* State: 8 ~ 11  (4byte mumber of objects) */
	while (index < size && unpack->pack_state < 12) {
		unpack->number <<= 8;
		unpack->number |= data[index];
		index++;
		unpack->pack_state++;
	}

	/* Callback */
	if (unpack->pack_state == 12 && unpack->header_dump)
		unpack->header_dump(&unpack->version, &unpack->number);

	return index;
}

static int gitt_unpack_obj_step(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	uint16_t index = 0;
	uint16_t in_size;
	uint16_t out_size;
	int ret;

	do {
		/* First byte:   | 1bit flag | 3bit type | 4bit length | */
		if (index < size && unpack->obj_state == 0) {
			ret = gitt_zlib_decompress_init(&unpack->zlib);
			if (ret) {
				unpack->pack_state = GITT_UNPACK_STATE_STOP;
				return ret;
			}
			unpack->valid_len = 0;

			/* Object type */
			unpack->obj.type = data[index] >> 4 & 0x7;
			if (!unpack->obj.type) {
				gitt_log_error("Invalid object type\n");
				goto fail;
			}
			gitt_log_debug("Object type: %s\n", gitt_obj_types[unpack->obj.type]);

			unpack->obj.size = data[index] & 0xf;
			unpack->obj_state = 4;
			if (!(data[index] & 0x80)) {
				unpack->obj_state += 7 * 2;
				gitt_log_debug("Object size: %u\n", unpack->obj.size);
			}

			index++;
		}

		/* High bit of size */
		while (index < size && unpack->obj_state < 18) {
			/* Object size */
			unpack->obj.size |= (data[index] & 0x7f) << unpack->obj_state;

			if (data[index] & 0x80)
				unpack->obj_state += 7;
			else
				unpack->obj_state = 18;

			/* End check */
			if (unpack->obj_state == 18) {
				if (data[index] & 0x80) {
					gitt_log_error("Object too big\n");
					goto fail;
				}
				gitt_log_debug("Object size: %u\n", unpack->obj.size);

				if (unpack->obj.size > unpack->buf_len) {
					gitt_log_error("Uncompress output buffer does not have enough space\n");
					goto fail;
				}
			}

			index++;
		}

		/* We don't deal with OFS_DELTA, so we skip its special part directly */
		if (index < size && unpack->obj_state == 18) {
			if (unpack->obj.type == 6) {
				while ((index < size) && (data[index] & 0x80))
					index++;
				if ((index < size) && !(data[index] & 0x80)) {
					unpack->obj_state++;
					index++;
				}
			} else {
				unpack->obj_state++;
			}
		}

		/* Decompress the data compressed by zlib */
		if (index < size && unpack->obj_state == 19) {
			in_size = size - index;
			out_size = unpack->obj.size - unpack->valid_len;
			ret = gitt_zlib_decompress_update(&unpack->zlib, data + index, &in_size,
							unpack->buf + unpack->valid_len, &out_size);
			if (ret)
				goto fail;
			unpack->valid_len += out_size;

			/* Check whether decompression has been completed */
			if (in_size == 0 && unpack->obj.size == unpack->valid_len) {
				gitt_log_debug("Decompress has been completed\n");
				gitt_zlib_compress_end(&unpack->zlib);
				unpack->obj.data = (char *)unpack->buf;
				unpack->number--;
				unpack->obj_state = GITT_UNPACK_STATE_INIT;

				/* Callback */
				if (unpack->obj_dump)
					unpack->obj_dump(&unpack->obj);

				/* Check whether unpack has been completed */
				if (!unpack->number) {
					gitt_log_debug("Unpack has been completed\n");
					unpack->pack_state++;
				}
			}

			index += in_size;
		}
	} while (index < size && unpack->pack_state == 12);

	return index;

fail:
	gitt_zlib_compress_end(&unpack->zlib);
	unpack->pack_state = GITT_UNPACK_STATE_STOP;
	return -1;
}

static int gitt_unpack_verify_step(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	int ret;
	uint16_t index = 0;
	uint8_t sha1[20];
	bool pass;

	/* Initialization */
	if (unpack->pack_state == 13)
		unpack->valid_len = 0;

	/* 20byte SHA-1 */
	while (index < size && unpack->pack_state < 33) {
		unpack->buf[unpack->valid_len] = data[index];
		unpack->valid_len++;
		unpack->pack_state++;
		index++;
	}

	/* Get result */
	if (unpack->pack_state == 33) {
		ret = gitt_sha1_digest(&unpack->sha1, sha1);
		if (ret) {
			gitt_log_error("SHA-1 digest fail\n");
			return -1;
		}

		/* Callback */
		pass = (bool)!memcmp(unpack->buf, sha1, sizeof(sha1));
		if (unpack->verify_dump)
			unpack->verify_dump(pass, &unpack->sha1);

		/* You're lucky, it's all done! :) */
		unpack->pack_state = GITT_UNPACK_STATE_STOP;
	}

	return index;
}

/**
 * @brief Update data and unpack
 *
 * @param unpack handle
 * @param data data point
 * @param size data size
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_unpack_update(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	int ret;
	int cost;

	/* If an error occurs or has been completed, processing will not continue */
	if (unpack->pack_state == GITT_UNPACK_STATE_STOP)
		return -1;

	/*
	 * Using a state machine to parse byte by byte
	 */

	/* State: 0 ~ 11 */
	if (size && unpack->pack_state < 12) {
		cost = gitt_unpack_header_step(unpack, data, size);
		if (cost < 0) {
			return cost;
		} else if (cost > 0) {
			ret = gitt_sha1_update(&unpack->sha1, data, cost);
			if (ret) {
				gitt_log_error("SHA-1 update fail\n");
				return ret;
			}
			data += cost;
			size -= cost;
		}
	}

	/* State: 12 */
	if (size && unpack->pack_state == 12) {
		cost = gitt_unpack_obj_step(unpack, data, size);
		if (cost < 0) {
			return cost;
		} else if (cost > 0) {
			ret = gitt_sha1_update(&unpack->sha1, data, cost);
			if (ret) {
				gitt_log_error("SHA-1 update fail\n");
				return ret;
			}
			data += cost;
			size -= cost;
		}
	}

	/* State: 13 ~ 32 */
	if (size && unpack->pack_state < 33) {
		cost = gitt_unpack_verify_step(unpack, data, size);
		if (cost < 0)
			return cost;
		data += cost;
		size -= cost;
	}

	return 0;
}

/**
 * @brief Calling during unpacking can end this operation
 *
 * @param unpack handle
 */
void gitt_unpack_end(struct gitt_unpack *unpack)
{
	if (unpack->pack_state == GITT_UNPACK_STATE_STOP)
		return;

	if (unpack->obj_state != GITT_UNPACK_STATE_INIT) {
		unpack->obj_state = GITT_UNPACK_STATE_INIT;
		gitt_zlib_compress_end(&unpack->zlib);
	}

	unpack->pack_state = GITT_UNPACK_STATE_STOP;
}
