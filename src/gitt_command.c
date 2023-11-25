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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <gitt_ssh.h>
#include <gitt_log.h>
#include <gitt_command.h>

static struct gitt_ssh* gitt_command_start(const char *url, const char *privkey, char *type)
{
	struct gitt_ssh* ssh = gitt_ssh_alloc();
	int ret;

	if (!ssh)
		return NULL;

	/* Connect */
	ret = gitt_ssh_connect(ssh, url, type, privkey);
	if (ret) {
		gitt_ssh_free(ssh);
		return NULL;
	}

	return ssh;
}

static inline int8_t gitt_command_char_to_half_byte(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 0xa;
	else if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 0xa;

	gitt_log_debug("Unknown character: '%c'\n", ch);
	return -1;
}

static int gitt_command_get_line_length(struct gitt_ssh* ssh)
{
	int ret = 0;
	int8_t half_byte;
	uint8_t count = 0;
	char buf[4];

	/* Read 4byte */
	ret = gitt_ssh_read(ssh, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		gitt_log_debug("Failed to read 4 bytes\n");
		return -1;
	}

	ret = 0;
	while (count < 4) {
		ret <<= 4;
		half_byte = gitt_command_char_to_half_byte(buf[count]);
		if (half_byte < 0)
			return -1;
		ret |= half_byte;
		count++;
	}

	return ret;
}

struct gitt_ssh* gitt_command_start_receive(const char *url, const char *privkey)
{
	return gitt_command_start(url, privkey, "git-receive-pack");
}

struct gitt_ssh* gitt_command_start_upload(const char *url, const char *privkey)
{
	return gitt_command_start(url, privkey, "git-upload-pack");
}

int gitt_command_say_byebye(struct gitt_ssh* ssh)
{
	const char *end = "0000\n";
	int ret;

	ret = gitt_ssh_write(ssh, (char *)end, 5);
	if (ret != 5) {
		gitt_log_debug("Saying goodbye went wrong\n");
		return -1;
	}

	return 0;
}

int gitt_command_get_head(struct gitt_ssh* ssh, char sha1_hex[41])
{
	int ret;
	int length;
	int index;
	char buf[32];

	length = gitt_command_get_line_length(ssh);
	gitt_log_debug("First line length: %dbyte\n", length);
	if (length < 4 + 40)
		return -1;

	/* Read SHA-1 */
	ret = gitt_ssh_read(ssh, sha1_hex, 40);
	if (ret != 40)
		return -1;
	sha1_hex[40] = '\0';
	gitt_log_debug("HEAD: %s\n", sha1_hex);

	/* We don't care about the rest of the data */
	length -= 40 + 4;
	while (length) {
		ret = sizeof(buf);
		ret = ret < length ? ret : length;
		ret = gitt_ssh_read(ssh, buf, ret);

		if (ret <= 0)
			return -1;

		/* Replace: '\0' ==> '\n' */
		for (index = 0; index < ret; index++)
			if (buf[index] == '\0')
				buf[index] = '\n';

		gitt_log_debug("%.*s", ret, buf);

		length -= ret;
	}

	while (1) {
		length = gitt_command_get_line_length(ssh);
		gitt_log_debug("Line length: %dbyte\n", length);

		if (length < 0)
			return -1;
		else if (length == 0)
			break;
		length -= 4;

		while (length) {
			ret = sizeof(buf);
			ret = ret < length ? ret : length;
			ret = gitt_ssh_read(ssh, buf, ret);
			if (ret <= 0)
				return -1;

			gitt_log_debug("%.*s", ret, buf);

			length -= ret;
		}
	}

	return 0;
}

static void gitt_command_set_line_length(char *line, uint16_t length)
{
	uint8_t count;
	const char *tables = "0123456789abcdef";

	if (length == 4)
		length = 0;

	for (count = 0; count < 4; count++)
		line[count] = tables[(length >> (12 - 4 * count)) & 0xf];
}

static void inline gitt_command_set_line_sha1(uint16_t offset, char *line, char *sha1)
{
	memcpy(line + offset, sha1, 40);
}

int gitt_command_want(struct gitt_ssh* ssh, char want_sha1[41], char have_sha1[41])
{
	char line1[] = "----want ---------------------------------------- "
		       "multi_ack_detailed side-band-64k thin-pack include-tag "
		       "ofs-delta deepen-since deepen-not agent=git/2.34.1\n";
	char line2[] = "----";
	char line3[] = "----have ----------------------------------------\n";
        char line4[] = "----done\n";
	int length;
	int ret;

	length = strlen(line1);
	gitt_command_set_line_length(line1, length);
	gitt_command_set_line_sha1(9, line1, want_sha1);
	gitt_log_debug(line1);
	ret = gitt_ssh_write(ssh, line1, length);
	if (ret != length) {
		gitt_log_debug("Error writing line, at %d\n", __LINE__);
		return -1;
	}

	length = strlen(line2);
	gitt_command_set_line_length(line2, length);
	gitt_log_debug(line2);
	ret = gitt_ssh_write(ssh, line2, length);
	if (ret != length) {
		gitt_log_debug("Error writing line, at %d\n", __LINE__);
		return -1;
	}

	if (have_sha1) {
		length = strlen(line3);
		gitt_command_set_line_length(line3, length);
		gitt_command_set_line_sha1(9, line3, want_sha1);
		gitt_log_debug(line3);
		ret = gitt_ssh_write(ssh, line3, length);
		if (ret != length) {
			gitt_log_debug("Error writing line, at %d\n", __LINE__);
			return -1;
		}
	}

	length = strlen(line4);
	gitt_command_set_line_length(line4, length);
	gitt_log_debug(line4);
	ret = gitt_ssh_write(ssh, line4, length);
	if (ret != length) {
		gitt_log_debug("Error writing line, at %d\n", __LINE__);
		return -1;
	}

	return 0;
}

int gitt_command_get_pack(struct gitt_ssh* ssh, gitt_command_pack_dump dump, void *param)
{
	char buf[32];
	int length;
	int ret;
	bool new_line;
	uint8_t type;
	char *pbuf;
	int valid;

	while (1) {
		new_line = true;
		length = gitt_command_get_line_length(ssh);

		if (length < 0)
			return -1;
		else if (length == 0)
			break;
		length -= 4;

		while (length) {
			ret = sizeof(buf);
			ret = ret < length ? ret : length;
			ret = gitt_ssh_read(ssh, buf, ret);
			if (ret <= 0)
				return -1;

			pbuf = buf;
			valid = ret;

			if (new_line) {
				if (buf[0] == 0x01) {
					pbuf++;
					valid--;
					type = 0x01;
				} else if (buf[0] == 0x02) {
					pbuf++;
					valid--;
					type = 0x02;
				} else if (ret >= 3 && buf[0] == 'N' && buf[1] == 'A' && buf[2] == 'K') {
					gitt_log_debug("NAK\n");
					type = 0x03;
				} else {
					gitt_log_error("Unknown format: '0x%02x'\n", buf[0]);
					type = 0xff;
				}
			}

			if (type == 0x01) {
				gitt_log_debug("+%dbyte\n", valid);
				if (dump && dump(param, pbuf, valid))
					return -1;
			} else if (type == 0x02) {
				gitt_log_debug("%.*s", valid, pbuf);
			}

			length -= ret;
			new_line = false;
		}
	}

	return 0;
}

void gitt_command_end(struct gitt_ssh* ssh)
{
	gitt_ssh_disconnect(ssh);
}