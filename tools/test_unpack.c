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
#include <gitt_ssh.h>
#include <gitt_unpack.h>
#include <gitt_errno.h>

#define PACK_PATH	"../.git/objects/pack/pack-xxxxx.pack"

static void gitt_unpack_obj_callback(struct gitt_obj *obj)
{
#if 0
	printf("%s: type:%s, size:%d\n", __func__, GITT_OBJ_STR(obj->type), obj->size);
	if (obj->type == 1 || obj->type == 2)
		printf("%.*s\n", obj->size, obj->data);
#else
	printf("*");
#endif
}

static void gitt_unpack_header_callback(uint32_t *version, uint32_t *number)
{
	printf("version:%u, number of objects:%u; ", *version, *number);
}

static void gitt_unpack_verify_callback(bool pass, struct gitt_sha1 *sha1)
{
	char sha1_hex[41];
	int ret;

	printf(" verify %s, ", pass ? "pass" : "not pass");

	ret = gitt_sha1_hexdigest(sha1, sha1_hex);
	if (!ret)
		printf("SHA-1: %s\n", sha1_hex);
}

static void test_unpack(uint8_t *buf, uint16_t len)
{
	struct gitt_unpack unpack = {0};
	uint8_t buffer[4096*2];
	uint16_t count;
	uint16_t blk_size;
	uint16_t need_size;
	int err;

	blk_size = 1;
	while (blk_size <= len) {
		printf("[%8d] ", blk_size);

		unpack.buf = buffer;
		unpack.buf_len = sizeof(buffer);
		unpack.header_dump = gitt_unpack_header_callback;
		unpack.obj_dump = gitt_unpack_obj_callback;
		unpack.verify_dump = gitt_unpack_verify_callback;
		err = gitt_unpack_init(&unpack);
		if (err)
			break;

		count = 0;
		while (count < len) {
			need_size = len - count;
			need_size = need_size < blk_size ? need_size : blk_size;
			err = gitt_unpack_update(&unpack, buf + count, need_size);
			if (err) {
				fprintf(stderr, "Test fail: %d\n", err);
				break;
			}
			count += need_size;
		}

		gitt_unpack_end(&unpack);
		blk_size++;
	}

	printf("Test end\n");
}

static int test_unpack_from_file(void)
{
	FILE *file;
	int ret;
	uint8_t buffer[40960];

	file = fopen(PACK_PATH, "rb");
	if (!file) {
		fprintf(stderr, "Error opening file\n");
		return -1;
	}
	ret = fread(buffer, 1, sizeof(buffer) - 1, file);
	if (ret <= 0) {
		fclose(file);
		return -1;
	}
	printf("File size: %dbyte\n", ret);

	test_unpack(buffer, (uint16_t)ret);

	return 0;
}

int main(int args, char *argv[])
{
	test_unpack_from_file();

	return 0;
}
