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

#include <gitt_log.h>
#include <gitt_zlib.h>

/* Check the return value of zlib functions */
void gitt_zlib_check(int ret)
{
	switch (ret) {
	case Z_OK:
		gitt_log_debug("OK\n");
		break;
	case Z_STREAM_END:
		gitt_log_debug("Stream end\n");
		break;
	case Z_NEED_DICT:
		gitt_log_debug("Need dictionary\n");
		break;
	case Z_ERRNO:
		gitt_log_error("Error\n");
		break;
	case Z_STREAM_ERROR:
		gitt_log_error("Stream error\n");
		break;
	case Z_DATA_ERROR:
		gitt_log_error("Data error\n");
		break;
	case Z_MEM_ERROR:
		gitt_log_error("Memory error\n");
		break;
	case Z_BUF_ERROR:
		gitt_log_error("Buffer error\n");
		break;
	case Z_VERSION_ERROR:
		gitt_log_error("Version error\n");
		break;
	default:
		gitt_log_error("Unknown error\n");
	}
}

int gitt_zlib_compress_init(struct gitt_zlib *zlib)
{
	int ret;

	/* Initialize the stream structure */
	zlib->stream.zalloc = Z_NULL;
	zlib->stream.zfree = Z_NULL;
	zlib->stream.opaque = Z_NULL;
	zlib->stream.avail_in = 0;
	zlib->stream.next_in = Z_NULL;

	/* Use the default compression level */
	ret = deflateInit(&zlib->stream, Z_DEFAULT_COMPRESSION);
	gitt_zlib_check(ret);
	if (ret != Z_OK)
		return ret;

	return 0;
}

int gitt_zlib_compress_update(struct gitt_zlib *zlib,
			      uint8_t *in, uint16_t *in_size,
			      uint8_t *out, uint16_t *out_size)
{
	int ret;

	/* Compress the input buffer in chunks */
	zlib->stream.avail_in = (uInt)*in_size;
	zlib->stream.next_in = (Bytef *)in;
	zlib->stream.avail_out = (uInt)*out_size;
	zlib->stream.next_out = (Bytef *)out;

	ret = deflate(&zlib->stream, Z_FINISH);
	if (ret != Z_OK && ret != Z_STREAM_END) {
		gitt_zlib_check(ret);
		*in_size = 0;
		*out_size = 0;
		return ret;
	}

	*in_size -= zlib->stream.avail_in;
	*out_size -= zlib->stream.avail_out;
	gitt_log_debug("cost in +%u bytes\n", *in_size);
	gitt_log_debug("cost out +%u bytes\n", *out_size);

	return 0;
}

void gitt_zlib_compress_end(struct gitt_zlib *zlib)
{
	/* Clean up */
	deflateEnd(&zlib->stream);
}

int gitt_zlib_decompress_init(struct gitt_zlib *zlib)
{
	int ret;

	/* Initialize the stream structure */
	zlib->stream.zalloc = Z_NULL;
	zlib->stream.zfree = Z_NULL;
	zlib->stream.opaque = Z_NULL;
	zlib->stream.avail_in = 0;
	zlib->stream.next_in = Z_NULL;

	ret = inflateInit(&zlib->stream);
	gitt_zlib_check(ret);
	if (ret != Z_OK)
		return ret;

	return 0;
}

int gitt_zlib_decompress_update(struct gitt_zlib *zlib,
				uint8_t *in, uint16_t *in_size,
				uint8_t *out, uint16_t *out_size)
{
	int ret;

	/* Decompress the input buffer in chunks */
	zlib->stream.avail_in = (uInt)*in_size;
	zlib->stream.next_in = (Bytef *)in;
	zlib->stream.avail_out = (uInt)*out_size;
	zlib->stream.next_out = (Bytef *)out;

	ret = inflate(&zlib->stream, Z_NO_FLUSH);
	if (ret != Z_OK && ret != Z_STREAM_END) {
		gitt_zlib_check(ret);
		*in_size = 0;
		*out_size = 0;
		return ret;
	}

	*in_size -= zlib->stream.avail_in;
	*out_size -= zlib->stream.avail_out;
	gitt_log_debug("cost in +%u bytes\n", *in_size);
	gitt_log_debug("cost out +%u bytes\n", *out_size);

	return 0;
}

void gitt_zlib_decompress_end(struct gitt_zlib *zlib)
{
	/* Clean up */
	inflateEnd(&zlib->stream);
}
