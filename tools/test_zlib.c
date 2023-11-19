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

int main(int args, char *argv[])
{
	Bytef tmp1[64] = "This is a string for testing!";
	Bytef tmp2[64] = {0};
	uLong tmp1_size;
	uLong tmp2_size;
	uLong i;
	int retval;

	printf("zlib version: %s\n", zlibVersion());

	/* Compress */
	tmp1_size = (uLong)strlen(tmp1);
	tmp2_size = sizeof(tmp2);
	retval = compress(tmp2, &tmp2_size, tmp1, tmp1_size);
	if (!retval) {
		for (i  = 0; i < tmp2_size; i++)
			printf("%02x ", tmp2[i]);
		printf("(%lubyte)\n", tmp2_size);
	} else {
		printf("compress error: %d\n", retval);
	}

	/* Uncompress */
	tmp1_size = sizeof(tmp1);
	memset(tmp1, 0, tmp1_size);
	retval = uncompress(tmp1, &tmp1_size, tmp2, tmp2_size);
	if (!retval)
		printf("%s (%lubyte)\n", (char *)tmp1, tmp1_size);
	else
		printf("uncompress error: %d\n", retval);

	return 0;
}
