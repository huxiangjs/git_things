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

#ifndef __GITT_SHA1_H_
#define __GITT_SHA1_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct gitt_sha1 {
	uint32_t digest[5];
	uint32_t low;
	uint32_t high;
	uint8_t block[64];
	int block_index;
	int computed;
	int corrupted;
};

void gitt_sha1_init(struct gitt_sha1 *handle);
int gitt_sha1_digest(struct gitt_sha1 *handle, uint8_t digest[20]);
int gitt_sha1_update(struct gitt_sha1 *handle, uint8_t *data, uint32_t size);
int gitt_sha1_hexdigest(struct gitt_sha1 *handle, char hexdigest[41]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_SHA1_H_ */
