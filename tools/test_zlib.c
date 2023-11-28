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

#include <stdio.h>
#include <string.h>
#include <gitt_zlib.h>

/* Compress a buffer using deflate */
static int gitt_zlib_compress(uint8_t *in, uint16_t *in_size,
			      uint8_t *out, uint16_t *out_size)
{
	int ret;
	struct gitt_zlib zlib;
	uint16_t cost_in;
	uint16_t cost_out;
	uint16_t cost_in_count = 0;
	uint16_t cost_out_count = 0;

	ret = gitt_zlib_compress_init(&zlib);
	if (ret)
		return ret;

	/* Simulate segmented compress */
	while (!ret && cost_in_count < *in_size) {
		cost_in = *in_size - cost_in_count;
		cost_in = cost_in < 30 ? cost_in : 30;
		cost_out = *out_size - cost_out_count;
		ret = gitt_zlib_compress_update(&zlib, in + cost_in_count, &cost_in,
						out + cost_out_count, &cost_out,
						(bool)(cost_in_count + cost_in == *in_size));
		if (!cost_in)
			break;
		cost_in_count += cost_in;
		cost_out_count += cost_out;
	}

	/* Number of bytes in the output buffer */
	printf("Compressed %lu bytes into %lu bytes\n",
	       zlib.stream.total_in, zlib.stream.total_out);
	printf("Compressed %u bytes into %u bytes\n",
	       cost_in_count, cost_out_count);

	gitt_zlib_compress_end(&zlib);

	return 0;
}

/* Decompress a buffer using inflate */
static int gitt_zlib_decompress(uint8_t *in, uint16_t *in_size,
				uint8_t *out, uint16_t *out_size)
{
	int ret;
	struct gitt_zlib zlib;
	uint16_t cost_in;
	uint16_t cost_out;
	uint16_t cost_in_count = 0;
	uint16_t cost_out_count = 0;

	ret = gitt_zlib_decompress_init(&zlib);
	if (ret)
		return ret;

	/* Simulate segmented decompress */
	while (!ret && cost_in_count < *in_size) {
		cost_in = *in_size - cost_in_count;
		cost_in = cost_in < 30 ? cost_in : 30;
		cost_out = *out_size - cost_out_count;
		ret = gitt_zlib_decompress_update(&zlib, in + cost_in_count, &cost_in,
						  out + cost_out_count, &cost_out);
		if (!cost_in)
			break;
		cost_in_count += cost_in;
		cost_out_count += cost_out;
	}

	/* Number of bytes in the output buffer */
	printf("Decompressed %lu bytes into %lu bytes\n",
	       zlib.stream.total_in, zlib.stream.total_out);
	printf("Decompressed %u bytes into %u bytes\n",
	       cost_in_count, cost_out_count);

	gitt_zlib_decompress_end(&zlib);

	return 0;
}

int main(int argc, char *argv[])
{
	uint8_t in[1024];
	uint8_t out[1024];
	uint16_t in_size = sizeof(in);
	uint16_t out_size = sizeof(out);
	uint16_t i;

	printf("zlib version: %s\n", zlibVersion());

	/* Fill the input buffer with some data */
	for (i = 0; i < in_size; i++)
		in [i] = i % 256;
	printf("%02x %02x ... %02x %02x\n",
	       in[0], in[1], in[in_size-2], in[in_size-1]);

	gitt_zlib_compress(in, &in_size, out, &out_size);

	memset(in, 0, sizeof(in));

	// zlib_decompress(out, &out_size, in, &in_size);
	in_size = sizeof(in);
	out_size = sizeof(out);
	gitt_zlib_decompress(out, &out_size, in, &in_size);

	printf("%02x %02x ... %02x %02x\n",
	       in[0], in[1], in[in_size-2], in[in_size-1]);

	return 0;
}
