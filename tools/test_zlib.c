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

/*
 * Compress a buffer using deflate in 30 byte chunks.
 * Returns 0 on success; on success *out_size is the number of
 * compressed bytes written to out.
 */
static int gitt_zlib_compress(uint8_t *in, uint16_t in_size,
			      uint8_t *out, uint16_t out_cap, uint16_t *out_size)
{
	int ret;
	/*
	 * The in-repo zlib (1.3.0.1-motley) has a nonstandard z_stream
	 * layout: the structure must be zero initialized.
	 */
	struct gitt_zlib zlib = {0};
	uint16_t consumed_in = 0;
	uint16_t written_out = 0;

	ret = gitt_zlib_compress_init(&zlib);
	if (ret)
		return ret;

	/*
	 * Simulate segmented compress. The update functions take the
	 * available byte counts in in_size/out_size and report the
	 * bytes consumed/written in them on exit.
	 */
	while (consumed_in < in_size) {
		uint16_t in_chunk = in_size - consumed_in;
		uint16_t out_chunk = out_cap - written_out;

		in_chunk = in_chunk < 30 ? in_chunk : 30;

		ret = gitt_zlib_compress_update(&zlib, in + consumed_in, &in_chunk,
				out + written_out, &out_chunk,
				(bool)(in_size - consumed_in <= 30));
		if (ret)
			break;
		consumed_in += in_chunk;
		written_out += out_chunk;
	}

	printf("Compressed %u bytes into %u bytes\n",
	       consumed_in, written_out);

	gitt_zlib_compress_end(&zlib);

	*out_size = written_out;
	return ret;
}

/*
 * Decompress a zlib stream of in_size bytes in 30 byte chunks.
 * Returns 0 if the whole stream was decompressed into out;
 * on success *out_size is the number of decompressed bytes.
 */
static int gitt_zlib_decompress(uint8_t *in, uint16_t in_size,
				uint8_t *out, uint16_t out_cap, uint16_t *out_size)
{
	int ret;
	struct gitt_zlib zlib = {0};
	uint16_t consumed_in = 0;
	uint16_t written_out = 0;

	ret = gitt_zlib_decompress_init(&zlib);
	if (ret)
		return ret;

	/*
	 * Simulate segmented decompress. The stream may end in the middle
	 * of a chunk: the next call would then fail (Z_BUF_ERROR), but
	 * the output is already complete, so just stop feeding.
	 */
	while (consumed_in < in_size) {
		uint16_t in_chunk = in_size - consumed_in;
		uint16_t out_chunk = out_cap - written_out;

		in_chunk = in_chunk < 30 ? in_chunk : 30;

		ret = gitt_zlib_decompress_update(&zlib, in + consumed_in, &in_chunk,
						  out + written_out, &out_chunk);
		if (ret)
			break;
		consumed_in += in_chunk;
		written_out += out_chunk;
	}

	printf("Decompressed %u bytes into %u bytes\n",
	       consumed_in, written_out);

	gitt_zlib_decompress_end(&zlib);

	*out_size = written_out;
	return ret;
}

int main(int argc, char *argv[])
{
	uint8_t in[1024];
	uint8_t out[1024];
	uint16_t in_size = sizeof(in);
	uint16_t compressed = 0;
	uint16_t decompressed = 0;
	uint16_t i;
	int pass = 1;

	printf("zlib version: %s\n", zlibVersion());

	/* Fill the input buffer with some data */
	for (i = 0; i < in_size; i++)
		in[i] = i % 256;
	printf("%02x %02x ... %02x %02x\n",
	       in[0], in[1], in[in_size-2], in[in_size-1]);

	gitt_zlib_compress(in, in_size, out, sizeof(out), &compressed);

	memset(in, 0, sizeof(in));
	gitt_zlib_decompress(out, compressed, in, sizeof(in), &decompressed);

	printf("%02x %02x ... %02x %02x\n",
	       in[0], in[1], in[decompressed ? decompressed-2 : 0],
	       in[decompressed ? decompressed-1 : 0]);

	/* The decompressed data must be the original data */
	if (decompressed != in_size) {
		printf("FAIL: size mismatch: %u, expected %u\n",
		       decompressed, in_size);
		pass = 0;
	}
	for (i = 0; i < decompressed && i < in_size; i++) {
		if (in[i] != (uint8_t)(i % 256)) {
			printf("FAIL: data mismatch at %u: got %02x, expected %02x\n",
			       i, in[i], (uint8_t)(i % 256));
			pass = 0;
			break;
		}
	}

	printf("%s\n", pass ? "Test pass" : "Test FAIL");
	return pass ? 0 : 1;
}
