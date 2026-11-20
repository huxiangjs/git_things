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
#include <stdlib.h>
#include <stdio.h>
#include <gitt_unpack.h>
#include <gitt_log.h>
#include <gitt_errno.h>

#define GITT_UNPACK_STATE_INIT		0x00
#define GITT_UNPACK_STATE_STOP		0xff

/*
 * The object store
 */

static struct gitt_unpack_obj_entry *gitt_unpack_store_find_by_offset(struct gitt_unpack *unpack,
								      uint32_t offset)
{
	uint8_t i;

	for (i = 0; i < unpack->store_num; i++)
		if (unpack->store[i].offset == offset)
			return &unpack->store[i];

	return NULL;
}

static struct gitt_unpack_obj_entry *gitt_unpack_store_find_by_sha(struct gitt_unpack *unpack,
								    uint8_t sha1[20])
{
	uint8_t i;

	for (i = 0; i < unpack->store_num; i++)
		if (!memcmp(unpack->store[i].sha1, sha1, 20))
			return &unpack->store[i];

	return NULL;
}

static int gitt_unpack_store_add(struct gitt_unpack *unpack, uint32_t offset,
				 uint8_t type, uint32_t size,
				 uint8_t *data, uint8_t sha1[20])
{
	struct gitt_unpack_obj_entry *entry;

	if (unpack->store_num >= GITT_UNPACK_MAX_OBJECTS) {
		gitt_log_error("Object store is full\n");
		return -GITT_ERRNO_NOMEM;
	}

	entry = &unpack->store[unpack->store_num++];
	entry->offset = offset;
	entry->type = type;
	entry->size = size;
	memcpy(entry->sha1, sha1, 20);
	entry->data = NULL;
	if (size) {
		entry->data = (uint8_t *)malloc(size);
		if (!entry->data) {
			/* Roll back: leave the store as it was */
			unpack->store_num--;
			return -GITT_ERRNO_NOMEM;
		}
		memcpy(entry->data, data, size);
	}

	return 0;
}

/*
 * Delta (OFS_DELTA / REF_DELTA) resolution
 *
 * The delta payload (already decompressed) starts with:
 * | <source size> | <result size> | <delta instructions> ...
 *
 * Both sizes are little-endian, 7 bits per byte, the high bit is set
 * on every byte except the last one.
 */

static int gitt_delta_decode_size(const uint8_t *p, uint16_t size,
				  uint16_t max_bytes, uint16_t *pos,
				  uint32_t *out)
{
	uint32_t value = 0;
	uint16_t i;
	int shift = 0;

	for (i = 0; i < max_bytes; i++) {
		uint8_t b;

		if (*pos + i >= size)
			return -GITT_ERRNO_INVAL;
		b = p[*pos + i];
		value |= (uint32_t)(b & 0x7f) << shift;
		if (!(b & 0x80)) {
			*pos += i + 1;
			*out = value;
			return 0;
		}
		shift += 7;
	}

	return -GITT_ERRNO_INVAL;
}

static int gitt_delta_apply(const uint8_t *base, uint32_t base_size,
			    const uint8_t *delta, uint32_t delta_size,
			    uint8_t *out, uint32_t out_cap, uint32_t *res)
{
	uint32_t src_size;
	uint32_t result_size;
	uint16_t pos = 0;
	uint32_t written = 0;
	int ret;

	ret = gitt_delta_decode_size(delta, (uint16_t)delta_size, 4, &pos, &src_size);
	if (ret) {
		gitt_log_error("Invalid delta: source size\n");
		return ret;
	}
	if (src_size != base_size) {
		gitt_log_error("Delta base size mismatch: %u != %u\n", src_size, base_size);
		return -GITT_ERRNO_INVAL;
	}

	ret = gitt_delta_decode_size(delta, (uint16_t)delta_size, 3, &pos, &result_size);
	if (ret) {
		gitt_log_error("Invalid delta: result size\n");
		return ret;
	}
	if (result_size + 1 > out_cap) {
		gitt_log_error("Uncompress output buffer does not have enough space\n");
		return -GITT_ERRNO_NOMEM;
	}

	while (pos < delta_size) {
		uint8_t op = delta[pos++];

		if (!op)
			break;

		if (op & 0x80) {
			/* Insert: 0x80 | <number of literal bytes> */
			uint32_t n = op & 0x7f;

			if (n == 0 || (uint32_t)pos + n > delta_size) {
				gitt_log_error("Invalid delta: insert\n");
				return -GITT_ERRNO_INVAL;
			}
			if (written + n > result_size)
				return -GITT_ERRNO_INVAL;
			memcpy(out + written, delta + pos, n);
			written += n;
			pos += (uint16_t)n;
		} else {
			/* Copy: | 4bit offset size | 4bit copy size | */
			uint32_t off_bytes = op & 0x0f;
			uint32_t size_bits = (op >> 4) & 0x0f;
			uint32_t cp_off = 0;
			uint32_t cp_size = 0;
			uint32_t i;

			for (i = 0; i < off_bytes; i++) {
				if ((uint32_t)pos + i >= delta_size)
					return -GITT_ERRNO_INVAL;
				cp_off |= (uint32_t)delta[pos + i] << (8 * i);
			}
			pos += (uint16_t)off_bytes;

			switch (size_bits) {
			case 14:
				if ((uint32_t)pos >= delta_size)
					return -GITT_ERRNO_INVAL;
				cp_size = delta[pos];
				pos += 1;
				break;
			case 15:
				if ((uint32_t)pos + 1 >= delta_size)
					return -GITT_ERRNO_INVAL;
				cp_size = (uint32_t)delta[pos] |
					  ((uint32_t)delta[pos + 1] << 8);
				pos += 2;
				break;
			default:
				cp_size = size_bits;
				break;
			}
			if (!cp_size)
				cp_size = 0x10000;

			if (cp_off + cp_size > base_size) {
				gitt_log_error("Invalid delta: copy out of range\n");
				return -GITT_ERRNO_INVAL;
			}
			if (written + cp_size > result_size)
				return -GITT_ERRNO_INVAL;
			memcpy(out + written, base + cp_off, cp_size);
			written += cp_size;
		}
	}

	if (written != result_size) {
		gitt_log_error("Invalid delta: size mismatch %u != %u\n",
			       written, result_size);
		return -GITT_ERRNO_INVAL;
	}

	*res = written;
	return 0;
}

/*
 * Resolve the just-decompressed object (in unpack->buf):
 * - if it is a delta, resolve it against the base object from the store
 * - compute the object id (SHA-1 of "<type> <size>\0<content>")
 * - keep it in the store for later delta resolution
 */
static int gitt_unpack_obj_resolve(struct gitt_unpack *unpack)
{
	struct gitt_unpack_obj_entry *base = NULL;
	uint8_t type = unpack->obj.type;
	uint8_t sha1_bin[20];
	uint32_t res_size = 0;
	char front_str[24];
	struct gitt_sha1 handle;
	int ret;

	if (type == GITT_OBJ_TYPE_OFS_DELTA) {
		if (unpack->ofs_base >= unpack->pack_pos) {
			gitt_log_error("OFS_DELTA base is out of range\n");
			return -GITT_ERRNO_INVAL;
		}
		base = gitt_unpack_store_find_by_offset(unpack,
				unpack->pack_pos - unpack->ofs_base);
	} else if (type == GITT_OBJ_TYPE_REF_DELTA) {
		base = gitt_unpack_store_find_by_sha(unpack, unpack->ref_base);
	}

	if (base) {
		uint8_t *payload;

		if (!base->data) {
			gitt_log_error("Delta base object is empty\n");
			return -GITT_ERRNO_INVAL;
		}

		/*
		 * The result is written into unpack->buf, so the delta
		 * payload (also in unpack->buf) must be copied out first.
		 */
		payload = (uint8_t *)malloc(unpack->obj.size ? unpack->obj.size : 1);
		if (!payload)
			return -GITT_ERRNO_NOMEM;
		memcpy(payload, unpack->buf, unpack->obj.size);

		ret = gitt_delta_apply(base->data, base->size, payload,
				       unpack->obj.size, unpack->buf,
				       unpack->buf_len, &res_size);
		free(payload);
		if (ret)
			return ret;

		gitt_log_debug("Delta resolved: %u bytes\n", res_size);
		unpack->obj.size = res_size;
		unpack->obj.type = base->type;
	}

	/* Compute the object id */
	gitt_sha1_init(&handle);
	ret = sprintf(front_str, "%s %u", GITT_OBJ_STR(unpack->obj.type),
		      unpack->obj.size);
	if (ret <= 0)
		return -GITT_ERRNO_INVAL;
	ret = gitt_sha1_update(&handle, (uint8_t *)front_str, ret + 1);
	if (ret)
		return ret;
	ret = gitt_sha1_update(&handle, unpack->buf, unpack->obj.size);
	if (ret)
		return ret;
	ret = gitt_sha1_digest(&handle, sha1_bin);
	if (ret)
		return ret;
	ret = gitt_sha1_hexdigest(&handle, unpack->obj.sha1);
	if (ret)
		return ret;
	gitt_log_debug("Object id: %s\n", unpack->obj.sha1);

	/* Keep it in the store so a later delta can use it as a base */
	return gitt_unpack_store_add(unpack, unpack->obj_start,
				     unpack->obj.type, unpack->obj.size,
				     unpack->buf, sha1_bin);
}

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
		gitt_log_error("Buffer cannot be empty and the length cannot be less than 20\n");
		return -GITT_ERRNO_INVAL;
	}

	unpack->pack_state = GITT_UNPACK_STATE_INIT;
	unpack->obj_state = GITT_UNPACK_STATE_INIT;
	unpack->pack_pos = 0;
	unpack->obj_start = 0;
	unpack->ofs_base = 0;
	unpack->delta_base_idx = 0;
	unpack->store_num = 0;
	gitt_sha1_init(&unpack->sha1);

	return 0;
}

static int gitt_unpack_header_step(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	const char *magic = "PACK";
	uint16_t index = 0;

	/*
	 * File header:
	 * | 4byte magic | 4byte version | 4byte number of objects |
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
			return -GITT_ERRNO_INVAL;
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

	/* State: 8 ~ 11  (4byte number of objects) */
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
		if (index < size && unpack->obj_state == GITT_UNPACK_STATE_INIT) {
			ret = gitt_zlib_decompress_init(&unpack->zlib);
			if (ret) {
				unpack->pack_state = GITT_UNPACK_STATE_STOP;
				return ret;
			}
			unpack->valid_len = 0;
			unpack->delta_base_idx = 0;
			unpack->ofs_base = 0;
			unpack->obj_start = unpack->pack_pos + index;

			/* Object type */
			unpack->obj.type = data[index] >> 4 & 0x7;
			if (!unpack->obj.type) {
				gitt_log_error("Invalid object type\n");
				goto fail;
			}
			gitt_log_debug("Object type: %s\n", GITT_OBJ_STR(unpack->obj.type));

			unpack->obj.size = data[index] & 0xf;
			unpack->obj_state = GITT_UNPACK_OBJ_STATE_SIZE;
			if (!(data[index] & 0x80)) {
				/* The size fits in the first byte */
				unpack->obj_state = GITT_UNPACK_OBJ_STATE_DELTA;
				gitt_log_debug("Object size: %u\n", unpack->obj.size);
			}

			index++;
		}

		/* High bits of size */
		while (index < size && unpack->obj_state < GITT_UNPACK_OBJ_STATE_DELTA) {
			uint8_t b = data[index];

			unpack->obj.size |= (uint32_t)(b & 0x7f) << unpack->obj_state;

			index++;
			if (b & 0x80)
				unpack->obj_state += 7;
			else
				unpack->obj_state = GITT_UNPACK_OBJ_STATE_DELTA;

			/* End check */
			if (unpack->obj_state == GITT_UNPACK_OBJ_STATE_DELTA) {
				if (b & 0x80) {
					gitt_log_error("Object too big\n");
					goto fail;
				}
				if (unpack->obj.size > unpack->buf_len - 1) {
					gitt_log_error("Uncompress output buffer does not have enough space\n");
					goto fail;
				}
				gitt_log_debug("Object size: %u\n", unpack->obj.size);
			}
		}

		/* Delta base reference (OFS_DELTA: offset, REF_DELTA: SHA-1) */
		if (index < size && unpack->obj_state == GITT_UNPACK_OBJ_STATE_DELTA) {
			if (unpack->obj.type == GITT_OBJ_TYPE_OFS_DELTA) {
				/*
				 * Offset encoding:
				 * | 7bit value | 7bit value | ...
				 * value = ((value + 1) << 7) | (next & 0x7f)
				 */
				uint8_t b = data[index++];

				if (!unpack->delta_base_idx)
					unpack->ofs_base = b & 0x7f;
				else
					unpack->ofs_base = ((unpack->ofs_base + 1) << 7) |
							    (b & 0x7f);
				if (b & 0x80)
					unpack->delta_base_idx++;
				else {
					unpack->obj_state = GITT_UNPACK_OBJ_STATE_DATA;
					gitt_log_debug("OFS_DELTA base offset: %u\n",
							unpack->ofs_base);
				}
			} else if (unpack->obj.type == GITT_OBJ_TYPE_REF_DELTA) {
				/* 20byte SHA-1 */
				unpack->ref_base[unpack->delta_base_idx++] = data[index++];
				if (unpack->delta_base_idx == 20)
					unpack->obj_state = GITT_UNPACK_OBJ_STATE_DATA;
			} else {
				/* Do nothing, jump to the next step */
				unpack->obj_state = GITT_UNPACK_OBJ_STATE_DATA;
			}
		}

		/* Decompress the data compressed by zlib */
		if (index < size && unpack->obj_state == GITT_UNPACK_OBJ_STATE_DATA) {
			in_size = size - index;
			out_size = unpack->buf_len - unpack->valid_len;
			ret = gitt_zlib_decompress_update(&unpack->zlib, data + index, &in_size,
							unpack->buf + unpack->valid_len, &out_size);
			if (ret)
				goto fail;
			unpack->valid_len += out_size;

			/*
			 * Check whether decompression has been completed.
			 *
			 * The stream may end in the middle of a chunk: the
			 * following call (with the next object's bytes as
			 * input) returns Z_STREAM_END consuming and
			 * producing nothing, which is the completion signal.
			 * If that call makes no progress while the object is
			 * not complete, the compressed stream ended before
			 * producing the declared number of bytes (corrupt
			 * object): fail instead of looping forever.
			 */
			if (in_size == 0 && unpack->obj.size == unpack->valid_len) {
				gitt_log_debug("Decompress has been completed\n");
				gitt_zlib_decompress_end(&unpack->zlib);
				/* Anyway, we add the terminator to it */
				unpack->buf[unpack->obj.size] = '\0';

				/* Resolve the object (delta) and store it */
				ret = gitt_unpack_obj_resolve(unpack);
				if (ret) {
					unpack->pack_state = GITT_UNPACK_STATE_STOP;
					return ret;
				}

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
			} else if (!in_size && !out_size) {
				gitt_log_error("Compressed stream ended before "
					 "producing all object data\n");
				goto fail;
			}

			index += in_size;
		}
	} while (index < size && unpack->pack_state == 12);

	return index;

fail:
	gitt_zlib_decompress_end(&unpack->zlib);
	unpack->pack_state = GITT_UNPACK_STATE_STOP;
	return -GITT_ERRNO_INVAL;
}

static int gitt_unpack_verify_step(struct gitt_unpack *unpack, uint8_t *data, uint16_t size)
{
	int ret;
	uint16_t index = 0;
	uint8_t sha1[20];
	bool pass;

	/* Initialization */
	if (unpack->pack_state == GITT_UNPACK_PACK_STATE_SHA)
		unpack->valid_len = 0;

	/* 20byte SHA-1 */
	while (index < size && unpack->pack_state < GITT_UNPACK_PACK_STATE_SHA_END) {
		unpack->buf[unpack->valid_len] = data[index];
		unpack->valid_len++;
		unpack->pack_state++;
		index++;
	}

	/* Get result */
	if (unpack->pack_state == GITT_UNPACK_PACK_STATE_SHA_END) {
		ret = gitt_sha1_digest(&unpack->sha1, sha1);
		if (ret) {
			gitt_log_error("SHA-1 digest fail\n");
			return -GITT_ERRNO_INVAL;
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
	int consumed;

	/* If an error occurs or has been completed, processing will not continue */
	if (unpack->pack_state == GITT_UNPACK_STATE_STOP)
		return -GITT_ERRNO_INVAL;

	/*
	 * Using a state machine to parse byte by byte
	 */

	/* State: 0 ~ 11 */
	if (size && unpack->pack_state < 12) {
		consumed = gitt_unpack_header_step(unpack, data, size);
		if (consumed < 0) {
			return consumed;
		} else if (consumed > 0) {
			ret = gitt_sha1_update(&unpack->sha1, data, consumed);
			if (ret) {
				gitt_log_error("SHA-1 update fail\n");
				return ret;
			}
			data += consumed;
			size -= consumed;
			unpack->pack_pos += consumed;
		}
	}

	/* State: 12 */
	if (size && unpack->pack_state == GITT_UNPACK_PACK_STATE_OBJECTS) {
		consumed = gitt_unpack_obj_step(unpack, data, size);
		if (consumed < 0) {
			return consumed;
		} else if (consumed > 0) {
			ret = gitt_sha1_update(&unpack->sha1, data, consumed);
			if (ret) {
				gitt_log_error("SHA-1 update fail\n");
				return ret;
			}
			data += consumed;
			size -= consumed;
			unpack->pack_pos += consumed;
		}
	}

	/* State: 13 ~ 32 */
	if (size && unpack->pack_state < GITT_UNPACK_PACK_STATE_SHA_END) {
		consumed = gitt_unpack_verify_step(unpack, data, size);
		if (consumed < 0)
			return consumed;
		data += consumed;
		size -= consumed;
		unpack->pack_pos += consumed;
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
		gitt_zlib_decompress_end(&unpack->zlib);
	}

	unpack->pack_state = GITT_UNPACK_STATE_STOP;
}

/*
 * Free the object store (call after a finished or aborted unpack)
 */
void gitt_unpack_store_clear(struct gitt_unpack *unpack)
{
	uint8_t i;

	for (i = 0; i < unpack->store_num; i++) {
		free(unpack->store[i].data);
		unpack->store[i].data = NULL;
	}
	unpack->store_num = 0;
}
