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
 *
 * refs: https://blog.csdn.net/mlynb/article/details/110570191
 * If the reference URL is not the original author, please contact me
 * and make corrections, thanks!
 *
 */

#include <gitt_sha1.h>

#define sha1_circular_shift(bits, word) \
	(((word) << (bits)) | ((word) >> (32 - (bits))))

/**
 * @brief Initialization handle
 *
 * @param handle
 */
void gitt_sha1_init(struct gitt_sha1 *handle)
{
	handle->low = 0;
	handle->high = 0;
	handle->block_index = 0;

	handle->digest[0] = 0x67452301;
	handle->digest[1] = 0xefcdab89;
	handle->digest[2] = 0x98badcfe;
	handle->digest[3] = 0x10325476;
	handle->digest[4] = 0xc3d2e1f0;

	handle->computed = 0;
	handle->corrupted = 0;
}

static void gitt_sha1_proc_block(struct gitt_sha1 *handle)
{
	const uint32_t k[] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};
	int t;
	uint32_t temp;
	uint32_t w[80];
	uint32_t a, b, c, d, e;

	for (t = 0; t < 16; t++) {
		w[t] = ((uint32_t) handle->block[t * 4]) << 24;
		w[t] |= ((uint32_t) handle->block[t * 4 + 1]) << 16;
		w[t] |= ((uint32_t) handle->block[t * 4 + 2]) << 8;
		w[t] |= ((uint32_t) handle->block[t * 4 + 3]);
	}

	for (t = 16; t < 80; t++)
		w[t] = sha1_circular_shift(1, w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16]);

	a = handle->digest[0];
	b = handle->digest[1];
	c = handle->digest[2];
	d = handle->digest[3];
	e = handle->digest[4];

	for (t = 0; t < 20; t++) {
		temp =  sha1_circular_shift(5, a) + ((b & c) |
			((~b) & d)) + e + w[t] + k[0];
		e = d;
		d = c;
		c = sha1_circular_shift(30, b);
		b = a;
		a = temp;
	}

	for (t = 20; t < 40; t++) {
		temp = sha1_circular_shift(5, a) + (b ^ c ^ d) + e + w[t] + k[1];
		e = d;
		d = c;
		c = sha1_circular_shift(30, b);
		b = a;
		a = temp;
	}

	for (t = 40; t < 60; t++) {
		temp = sha1_circular_shift(5, a) + ((b & c) | (b & d) |
		       (c & d)) + e + w[t] + k[2];
		e = d;
		d = c;
		c = sha1_circular_shift(30, b);
		b = a;
		a = temp;
	}

	for (t = 60; t < 80; t++) {
		temp = sha1_circular_shift(5, a) + (b ^ c ^ d) + e + w[t] + k[3];
		e = d;
		d = c;
		c = sha1_circular_shift(30, b);
		b = a;
		a = temp;
	}

	handle->digest[0] = (handle->digest[0] + a);
	handle->digest[1] = (handle->digest[1] + b);
	handle->digest[2] = (handle->digest[2] + c);
	handle->digest[3] = (handle->digest[3] + d);
	handle->digest[4] = (handle->digest[4] + e);
	handle->block_index = 0;
}

/**
 * @brief Enter data for processing
 *
 * @param handle
 * @param data data pointer
 * @param size data size
 * @return int  0: no error
 * @return int -1: error
 */
int gitt_sha1_update(struct gitt_sha1 *handle, uint8_t *data, uint32_t size)
{
	if (!size)
		return -1;

	if (handle->computed || handle->corrupted) {
		handle->corrupted = 1;
		return -1;
	}

	while (size-- && !handle->corrupted) {
		handle->block[handle->block_index++] = *data;
		handle->low += 8;

		if (handle->low == 0) {
			handle->high++;
			if (handle->high == 0)
				handle->corrupted = 1;
		}

		if (handle->block_index == 64)
			gitt_sha1_proc_block(handle);

		data++;
	}

	return 0;
}

static void gitt_sha1_pad(struct gitt_sha1 *handle)
{
	if (handle->block_index > 55) {
		handle->block[handle->block_index++] = 0x80;
		while (handle->block_index < 64)
			handle->block[handle->block_index++] = 0;

		gitt_sha1_proc_block(handle);

		while (handle->block_index < 56)
			handle->block[handle->block_index++] = 0;
	} else {
		handle->block[handle->block_index++] = 0x80;
		while (handle->block_index < 56)
			handle->block[handle->block_index++] = 0;
	}

	handle->block[56] = (handle->high >> 24 ) & 0xff;
	handle->block[57] = (handle->high >> 16 ) & 0xff;
	handle->block[58] = (handle->high >> 8 ) & 0xff;
	handle->block[59] = (handle->high) & 0xff;
	handle->block[60] = (handle->low >> 24 ) & 0xff;
	handle->block[61] = (handle->low >> 16 ) & 0xff;
	handle->block[62] = (handle->low >> 8 ) & 0xff;
	handle->block[63] = (handle->low) & 0xff;

	gitt_sha1_proc_block(handle);
}

/**
 * @brief Get digest results
 *
 * @param handle
 * @param digest 20byte
 * @return int  0: no error
 * @return int -1: error
 */
int gitt_sha1_digest(struct gitt_sha1 *handle, uint8_t digest[20])
{
	int i;

	if (handle->corrupted)
		return -1;

	if (!handle->computed) {
		gitt_sha1_pad(handle);
		handle->computed = 1;
	}

	for (i = 0; i < 20; i++)
		digest[i] = (handle->digest[i >> 2] >> (24 - 8 * (i & 0x3))) & 0xff;

	return 0;
}

/**
 * @brief Get hex digest results
 *
 * @param handle
 * @param digest 41byte (includes '\0' terminator)
 * @return int  0: no error
 * @return int -1: error
 */
int gitt_sha1_hexdigest(struct gitt_sha1 *handle, char hexdigest[41])
{
	uint8_t digest[20];
	const char *tables = "0123456789abcdef";
	int retval;
	int i;

	retval = gitt_sha1_digest(handle, digest);
	if (retval)
		return retval;

	for (i = 0; i < 20; i++) {
		hexdigest[i << 1] = tables[(digest[i] >> 4) & 0xf];
		hexdigest[(i << 1) + 1] = tables[digest[i] & 0xf];
	}
	hexdigest[40] = '\0';

	return 0;
}
