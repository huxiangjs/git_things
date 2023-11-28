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

#ifndef __GITT_PACK_H_
#define __GITT_PACK_H_

#include <stdint.h>
#include <gitt_obj.h>
#include <gitt_sha1.h>
#include <gitt_zlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int (*gitt_pack_data)(void *p, uint8_t *buf, uint16_t size);

struct gitt_pack {
	uint8_t *buf;
	uint16_t buf_len;
	uint8_t obj_num;
	uint8_t state;
	struct gitt_sha1 sha1;
	gitt_pack_data data_dump;
	struct gitt_zlib zlib;
};

int gitt_pack_init(struct gitt_pack *pack);
int gitt_pack_update(struct gitt_pack *pack, struct gitt_obj *obj);
void gitt_pack_end(struct gitt_pack *pack);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_PACK_H_ */
