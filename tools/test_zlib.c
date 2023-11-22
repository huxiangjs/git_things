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
#include <zlib.h>

/* Check the return value of zlib functions */
static void zlib_check_ret(int ret) {
	switch (ret) {
	case Z_OK:
		fprintf(stdout, "OK\n");
		break;
	case Z_STREAM_END:
		fprintf(stdout, "Stream end\n");
		break;
	case Z_NEED_DICT:
		fprintf(stdout, "Need dictionary\n");
		break;
	case Z_ERRNO:
		fprintf(stderr, "Error\n");
		break;
	case Z_STREAM_ERROR:
		fprintf(stderr, "Stream error\n");
		break;
	case Z_DATA_ERROR:
		fprintf(stderr, "Data error\n");
		break;
	case Z_MEM_ERROR:
		fprintf(stderr, "Memory error\n");
		break;
	case Z_BUF_ERROR:
		fprintf(stderr, "Buffer error\n");
		break;
	case Z_VERSION_ERROR:
		fprintf(stderr, "Version error\n");
		break;
	default:
		fprintf(stderr, "Unknown error\n");
	}
}

/* Compress a buffer using deflate */
static int zlib_compress(unsigned char *in, size_t in_size,
			 unsigned char *out, size_t *out_size)
{
	int ret;
	z_stream strm;
	size_t have;

	/* Initialize the stream structure */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	/* Use the default compression level */
	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	zlib_check_ret(ret);
	if (ret != Z_OK)
		return ret;

	/* Compress the input buffer in chunks */
	strm.avail_in = in_size;
	strm.next_in = in;
	strm.avail_out = *out_size;
	strm.next_out = out;

	ret = deflate(&strm, Z_FINISH);
	zlib_check_ret(ret);
	if (ret == Z_STREAM_ERROR)
		return ret;

	/* Number of bytes in the output buffer */
	have = *out_size - strm.avail_out;
	fprintf(stdout, "Compressed %lu bytes into %lu bytes\n", in_size, have);

	/* Clean up */
	deflateEnd (&strm);
	*out_size = have;

	return Z_OK;
}

/* Decompress a buffer using inflate */
int zlib_decompress(unsigned char *in, size_t in_size,
			unsigned char *out, size_t *out_size)
{
	int ret;
	z_stream strm;
	size_t have;

	/* Initialize the stream structure */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	ret = inflateInit(&strm);
	zlib_check_ret(ret);
	if (ret != Z_OK)
		return ret;

	/* Decompress the input buffer in chunks */
	strm.avail_in = in_size;
	strm.next_in = in;
	strm.avail_out = *out_size;
	strm.next_out = out;

	ret = inflate(&strm, Z_NO_FLUSH);
	zlib_check_ret(ret);
	switch (ret) {
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
		inflateEnd(&strm);
		return ret;
	}

	/* Number of bytes in the output buffer */
	have = *out_size - strm.avail_out;
	fprintf(stdout, "Decompressed %lu bytes into %lu bytes\n", in_size, have);

	/* Clean up */
	inflateEnd (&strm);
	*out_size = have;

	return Z_OK;
}

int main(int argc, char *argv[])
{
	unsigned char in[4096];
	unsigned char out[4096];
	size_t in_size = sizeof(in);
	size_t out_size = sizeof(out);
	int i;

	/* Fill the input buffer with some data */
	for (i = 0; i < in_size; i++)
		in [i] = i % 256;

	zlib_compress(in, in_size, out, &out_size);
	zlib_decompress(out, out_size, in, &in_size);

	return 0;
}
