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
#include <gitt_sha1.h>
#include <gitt_obj.h>
#include <gitt_zlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct gitt_unpack;

typedef void (*gitt_unpack_header)(uint32_t *version, uint32_t *number);
typedef void (*gitt_unpack_obj)(struct gitt_obj *obj);
typedef void (*gitt_unpack_verify)(bool pass, struct gitt_sha1 *sha1);

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
};

int gitt_unpack_init(struct gitt_unpack *unpack);
int gitt_unpack_update(struct gitt_unpack *unpack, uint8_t *data, uint16_t size);
void gitt_unpack_end(struct gitt_unpack *unpack);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_UNPACK_H_ */
