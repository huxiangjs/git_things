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

#ifndef __GITT_ZLIB_H_
#define __GITT_ZLIB_H_

#include <stdint.h>
#include <stdbool.h>
#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct gitt_zlib {
	z_stream stream;
};

void gitt_zlib_check(int ret);
int gitt_zlib_compress_init(struct gitt_zlib *zlib);
int gitt_zlib_compress_update(struct gitt_zlib *zlib,
			      uint8_t *in, uint16_t *in_size,
			      uint8_t *out, uint16_t *out_size,
			      bool end);
void gitt_zlib_compress_end(struct gitt_zlib *zlib);
int gitt_zlib_decompress_init(struct gitt_zlib *zlib);
int gitt_zlib_decompress_update(struct gitt_zlib *zlib,
				uint8_t *in, uint16_t *in_size,
				uint8_t *out, uint16_t *out_size);
void gitt_zlib_decompress_end(struct gitt_zlib *zlib);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_ZLIB_H_ */
