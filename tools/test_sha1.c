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
#include <gitt_sha1.h>

/*
 * Reference values, computed with python3 (see tools/python/refs_sha1.py):
 *
 *   import hashlib
 *   hashlib.sha1(b'This is a string for testing!').hexdigest()
 *
 *   sha1 = hashlib.sha1()
 *   for item in range(0, 256):
 *       sha1.update(bytearray(range(item+1)))
 *   sha1.hexdigest()
 */
#define REF_SHA1_STR	"ac56d9346cb1f8950d141426c5cf4f63749e18fb"
#define REF_SHA1_BIN	"059217ea0faca8c1c25aa6849b9c37a5ee367998"

static int test_for_string(void)
{
	const char *str = "This is a string for testing!";
	char hexdigest[41];
	struct gitt_sha1 sha1;
	int err;

	gitt_sha1_init(&sha1);

	err = gitt_sha1_update(&sha1, (uint8_t *)str, (uint32_t)strlen(str));
	if (err) {
		printf("ERROR: %d\n", __LINE__);
		return -1;
	}

	err = gitt_sha1_hexdigest(&sha1, hexdigest);
	if (err) {
		printf("ERROR: %d\n", __LINE__);
		return -1;
	}

	printf("STR SHA1: %s (ref: %s)\n", hexdigest, REF_SHA1_STR);

	return !strcmp(hexdigest, REF_SHA1_STR);
}

static int test_for_binary(void)
{
	uint8_t buffer[256] = {0};
	char hexdigest[41];
	struct gitt_sha1 sha1;
	int err;
	int i;

	gitt_sha1_init(&sha1);

	for (i = 0; i < 256; i++) {
		buffer[i] = (uint8_t)i;
		err = gitt_sha1_update(&sha1, buffer, (uint32_t)i + 1);
		if (err) {
			printf("ERROR: %d\n", __LINE__);
			return -1;
		}
	}

	err = gitt_sha1_hexdigest(&sha1, hexdigest);
	if (err) {
		printf("ERROR: %d\n", __LINE__);
		return -1;
	}

	printf("BIN SHA1: %s (ref: %s)\n", hexdigest, REF_SHA1_BIN);

	return !strcmp(hexdigest, REF_SHA1_BIN);
}

int main(int args, char *argv[])
{
	int ret = test_for_string();
	int ret2 = test_for_binary();

	printf("%s\n", (ret && ret2) ? "Test pass" : "Test FAIL");

	return (ret && ret2) ? 0 : 1;
}
