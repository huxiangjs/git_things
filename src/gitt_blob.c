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
#include <gitt_blob.h>
#include <gitt_sha1.h>
#include <gitt_log.h>
#include <gitt_errno.h>

/* SHA-1 hex string length, chars (no NUL) */
#define GITT_SHA1_HEX_SIZE	40

/*
 * The object id header "<type> <size>\0" (longest:
 * "ofs_delta 4294967295")
 */
#define GITT_OBJECT_HEADER_SIZE	24

int gitt_blob_sha1_update(const uint8_t *data, uint16_t size, char sha1[GITT_SHA1_HEX_SIZE + 1])
{
	struct gitt_sha1 handle;
	char front_str[GITT_OBJECT_HEADER_SIZE];
	int ret;

	gitt_sha1_init(&handle);

	ret = sprintf(front_str, "blob %u", size);
	if (ret <= 0)
		return -GITT_ERRNO_INVAL;

	ret = gitt_sha1_update(&handle, (uint8_t *)front_str, ret + 1);
	if (ret)
		return ret;

	ret = gitt_sha1_update(&handle, (uint8_t *)data, size);
	if (ret)
		return ret;

	ret = gitt_sha1_hexdigest(&handle, sha1);
	if (ret)
		return ret;

	gitt_log_debug("Blob SHA1: %s\n", sha1);
	return 0;
}
