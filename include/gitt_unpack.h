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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct gitt_unpack;

typedef void (*gitt_obj_dump)(struct gitt_obj *obj);
typedef void (*gitt_unpack_work)(struct gitt_unpack *unpack);
typedef void (*gitt_unpack_verify)(bool pass, struct gitt_sha1 *sha1);

struct gitt_unpack {
	struct gitt_sha1 sha1;
	uint8_t *buf;
	uint16_t buf_len;
	gitt_obj_dump obj_dump;
	uint32_t version;
	uint32_t number;
	gitt_unpack_work work;
	gitt_unpack_verify verify;
	uint16_t valid_len;
	uint8_t state;
	uint16_t offset;
};

int gitt_unpack_init(struct gitt_unpack *unpack);
int gitt_unpack_update(struct gitt_unpack *unpack, uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_UNPACK_H_ */
