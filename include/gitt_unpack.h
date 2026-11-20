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

#ifndef __GITT_UNPACK_H_
#define __GITT_UNPACK_H_

#include <stdint.h>
#include <stdbool.h>
#include <gitt_config.h>
#include <gitt_sha1.h>
#include <gitt_obj.h>
#include <gitt_zlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * The pack_state values (a byte-at-a-time state machine):
 *
 *   0 ~ 11  pack header (4byte magic, 4byte version, 4byte count)
 *   12      object entries
 *   13 ~ 32 the trailing 20byte pack SHA-1
 *   33      the SHA-1 is complete
 *   0xff    finished (or aborted)
 */
#define GITT_UNPACK_PACK_STATE_OBJECTS	12
#define GITT_UNPACK_PACK_STATE_SHA	13
#define GITT_UNPACK_PACK_STATE_SHA_END	33

/*
 * The obj_state values (the state of one object entry):
 *
 *   0   waiting for the first byte of the entry
 *   4   reading the size continuation bytes
 *   18  size complete: reading the delta base reference (if any)
 *   38  reading the zlib payload
 */
#define GITT_UNPACK_OBJ_STATE_SIZE	4
#define GITT_UNPACK_OBJ_STATE_DELTA	18
#define GITT_UNPACK_OBJ_STATE_DATA	38

struct gitt_unpack;

typedef void (*gitt_unpack_header)(uint32_t *version, uint32_t *number);
typedef void (*gitt_unpack_obj)(struct gitt_obj *obj);
typedef void (*gitt_unpack_verify)(bool pass, struct gitt_sha1 *sha1);

/*
 * One received object, kept in the store so that a later delta entry
 * can be resolved against it.
 */
struct gitt_unpack_obj_entry {
	uint32_t offset;
	uint8_t type;
	uint32_t size;
	uint8_t sha1[20];
	uint8_t *data;
};

struct gitt_unpack {
	uint8_t *buf;
	uint16_t buf_len;
	uint16_t valid_len;
	gitt_unpack_header header_dump;
	gitt_unpack_obj obj_dump;
	gitt_unpack_verify verify_dump;
	uint8_t pack_state;
	uint8_t obj_state;
	uint32_t version;
	uint32_t number;
	struct gitt_zlib zlib;
	struct gitt_obj obj;
	struct gitt_sha1 sha1;

	/* Position of the current object entry in the packfile */
	uint32_t pack_pos;
	uint32_t obj_start;

	/* Base reference of a delta entry */
	uint32_t ofs_base;
	uint8_t ref_base[20];
	uint8_t delta_base_idx;

	/* Object store (delta bases) */
	uint8_t store_num;
	struct gitt_unpack_obj_entry store[GITT_UNPACK_MAX_OBJECTS];
};

int gitt_unpack_init(struct gitt_unpack *unpack);
int gitt_unpack_update(struct gitt_unpack *unpack, uint8_t *data, uint16_t size);
void gitt_unpack_end(struct gitt_unpack *unpack);
void gitt_unpack_store_clear(struct gitt_unpack *unpack);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_UNPACK_H_ */
