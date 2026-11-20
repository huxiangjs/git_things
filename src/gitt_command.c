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
#include <gitt_errno.h>

/*
 * Pkt-line framing (git "smart" protocol, fixed): each line starts
 * with a 4 char hex length (including itself); a length of 0, i.e.
 * "0000", is the flush-pkt that ends a stream of lines
 */
#define GITT_PKT_LINE_SIZE	4
#define GITT_PKT_FLUSH		"0000"

/*
 * Shortest status payload ("ok" / "ng") in
 * gitt_command_get_push_status
 */
#define GITT_PKT_STATUS_MIN	2

/* SHA-1 hex string length, chars (no NUL) */
#define GITT_SHA1_HEX_SIZE	40

/*
 * The longest ref name GITT accepts, chars: the GITT_REPOSITORY_REF_SIZE
 * byte refs buffer minus the NUL
 */
#define GITT_PKT_REF_MAX	(GITT_REPOSITORY_REF_SIZE - 1)

/* Capability line of the fetch (upload-pack) request */
#define GITT_FETCH_CAPABILITIES	" multi_ack no-progress side-band-64k ofs-delta shallow agent=git/2.34.1\n"

/* Capability line of the push (receive-pack) request */
#define GITT_PUSH_CAPABILITIES	"report-status-v2 side-band-64k object-format=sha1 agent=git/2.34.1"

/* The server's shallow boundary lines start with this */
#define GITT_SHALLOW_PREFIX		"shallow "
#define GITT_SHALLOW_PREFIX_SIZE	(sizeof(GITT_SHALLOW_PREFIX) - 1)

struct line_data {
	char *data;
	uint16_t size;
};

/*
 * Side-band-64k: the first byte of a side-band packet selects the
 * channel: 1 = data (pack bytes / status messages), 2 = progress.
 */
#define GITT_SIDE_BAND_DATA		0x01
#define GITT_SIDE_BAND_PROGRESS		0x02

/*
 * The kind of a line read from an upload-pack (fetch) stream. A line
 * is either a side-band packet (DATA / PROGRESS) or a plain protocol
 * line (NAK, ACK, a shallow boundary).
 */
enum {
	GITT_FETCH_LINE_DATA = 0,
	GITT_FETCH_LINE_PROGRESS,
	GITT_FETCH_LINE_NAK,
	GITT_FETCH_LINE_ACK,
	GITT_FETCH_LINE_SHALLOW,
	GITT_FETCH_LINE_UNKNOWN = 0xff,
};

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
	return -GITT_ERRNO_INVAL;
}

static int gitt_command_get_line_length(struct gitt_ssh* ssh)
{
	int ret = 0;
	int8_t half_byte;
	uint8_t count = 0;
	char buf[GITT_PKT_LINE_SIZE];

	/* Read 4byte */
	ret = gitt_ssh_read(ssh, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		gitt_log_debug("Failed to read 4 bytes\n");
		return -GITT_ERRNO_INVAL;
	}

	ret = 0;
	while (count < GITT_PKT_LINE_SIZE) {
		ret <<= 4;
		half_byte = gitt_command_char_to_half_byte(buf[count]);
		if (half_byte < 0)
			return -GITT_ERRNO_INVAL;
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
	const char *end = GITT_PKT_FLUSH "\n";
	int ret;

	ret = gitt_ssh_write(ssh, (char *)end, GITT_PKT_LINE_SIZE + 1);
	if (ret != GITT_PKT_LINE_SIZE + 1) {
		gitt_log_debug("Saying goodbye went wrong\n");
		return -GITT_ERRNO_INVAL;
	}

	return 0;
}

/*
 * Whether a hex SHA-1 is null (all zeros). Newer git versions
 * advertise an empty repository with a "capabilities^{}" pseudo-ref
 * whose SHA-1 is null.
 */
static int gitt_command_is_null_hex(const char *hex)
{
	int i;

	for (i = 0; i < GITT_SHA1_HEX_SIZE; i++)
		if (hex[i] != '0')
			return 0;
	return 1;
}

int gitt_command_get_head(struct gitt_ssh* ssh,
			  char head[GITT_SHA1_HEX_SIZE + 1],
			  char refs[GITT_REPOSITORY_REF_SIZE])
{
	int ret;
	int length;
	int index;
	char buf[GITT_SHA1_HEX_SIZE];

	length = gitt_command_get_line_length(ssh);
	gitt_log_debug("First line length: %dbyte\n", length);
	if (length == 0) {
		/* Empty advertisement: the remote repository has no refs yet */
		gitt_log_debug("Remote repository is empty\n");
		head[0] = '\0';
		return 0;
	}
	if (length < GITT_PKT_LINE_SIZE + GITT_SHA1_HEX_SIZE)
		return -GITT_ERRNO_INVAL;
	length -= GITT_PKT_LINE_SIZE;

	/* Read SHA-1 */
	ret = gitt_ssh_read(ssh, head, GITT_SHA1_HEX_SIZE);
	if (ret != GITT_SHA1_HEX_SIZE)
		return -GITT_ERRNO_INVAL;
	head[GITT_SHA1_HEX_SIZE] = '\0';
	length -= GITT_SHA1_HEX_SIZE;
	gitt_log_debug("HEAD: %s\n", head);

	/*
	 * Empty repository: newer git versions advertise it with the
	 * "capabilities^{}" pseudo-ref whose SHA-1 is all zeros (older
	 * versions send a bare flush, handled above). The ref loop
	 * below immediately hits the flush.
	 */
	if (gitt_command_is_null_hex(head)) {
		gitt_log_debug("Remote repository is empty\n");
		head[0] = '\0';
	}

	/* We don't care about the rest of the data */
	while (length) {
		ret = sizeof(buf);
		ret = ret < length ? ret : length;
		ret = gitt_ssh_read(ssh, buf, ret);

		if (ret <= 0)
			return -GITT_ERRNO_INVAL;

		/* Replace: '\0' ==> '\n' */
		for (index = 0; index < ret; index++)
			if (buf[index] == '\0')
				buf[index] = '\n';

		gitt_log_debug("%.*s", ret, buf);

		length -= ret;
	}

	refs[0] = '\0';
	while (1) {
		length = gitt_command_get_line_length(ssh);
		gitt_log_debug("Line length: %dbyte\n", length);

		if (length == 0)
			break;
		else if (length < GITT_PKT_LINE_SIZE + GITT_SHA1_HEX_SIZE + 1 + 1)
			return -GITT_ERRNO_INVAL;
		length -= GITT_PKT_LINE_SIZE;

		/* Find refs */
		ret = gitt_ssh_read(ssh, buf, GITT_SHA1_HEX_SIZE);
		if (ret <= 0) {
			return -GITT_ERRNO_INVAL;
		} else if (ret == GITT_SHA1_HEX_SIZE && !refs[0] &&
		   !memcmp(buf, head, GITT_SHA1_HEX_SIZE)) {
			length -= ret;

			/* Skip a space */
			ret = gitt_ssh_read(ssh, buf, 1);
			if (ret != 1)
				return -GITT_ERRNO_INVAL;
			length -= ret;

			if (length > GITT_PKT_REF_MAX || length < 2) {
				gitt_log_error("Head name too long or too short\n");
				return -GITT_ERRNO_INVAL;
			}

			ret = gitt_ssh_read(ssh, refs, length);
			if (ret != length)
				return -GITT_ERRNO_INVAL;
			refs[ret - 1] = '\0';
			length -= ret;
			gitt_log_debug("Refs: %s\n", refs);
		} else {
			/* Debug output */
			gitt_log_debug("%.*s", ret, buf);
			length -= ret;
			while (length) {
				ret = sizeof(buf);
				ret = ret < length ? ret : length;
				ret = gitt_ssh_read(ssh, buf, ret);
				if (ret <= 0)
					return -GITT_ERRNO_INVAL;

				gitt_log_debug("%.*s", ret, buf);

				length -= ret;
			}
		}
	}

	return 0;
}

static void gitt_command_set_line_length(char *line, uint16_t length)
{
	uint8_t count;
	const char *tables = "0123456789abcdef";

	/*
	 * A pkt-line with no payload is the flush-pkt "0000". (A total
	 * length of 4 means 0 payload bytes, which is exactly that.)
	 */
	if (length == GITT_PKT_LINE_SIZE)
		length = 0;

	for (count = 0; count < GITT_PKT_LINE_SIZE; count++)
		line[count] = tables[(length >> (12 - 4 * count)) & 0xf];
}

static int gitt_command_line_write(struct gitt_ssh* ssh, struct line_data *line, uint8_t part)
{
	char buf[GITT_PKT_LINE_SIZE + 1] = {0};
	uint16_t length;
	int ret;
	uint8_t i;

	/* Total line size */
	length = 0;
	for (i = 0; i < part; i++)
		length += line[i].size;

	if (length)
		length += GITT_PKT_LINE_SIZE;

	/* Set the length of the line and write */
	gitt_command_set_line_length(buf, length);
	gitt_log_debug("%04x", length);
	ret = gitt_ssh_write(ssh, buf, GITT_PKT_LINE_SIZE);
	if (ret != GITT_PKT_LINE_SIZE) {
		gitt_log_debug("Error writing line, at %d\n", __LINE__);
		return -GITT_ERRNO_INVAL;
	}

	if (!length)
		goto out;

	/* Write each part of the line */
	for (i = 0; i < part; i++) {
		gitt_log_debug("%.*s", (int)line[i].size, line[i].data);
		ret = gitt_ssh_write(ssh, line[i].data, line[i].size);
		if (ret != line[i].size) {
			gitt_log_debug("Error writing line, at %d\n", __LINE__);
			return -GITT_ERRNO_INVAL;
		}
	}

out:
	return 0;
}

/*
 * Read and discard server pkt-lines until a flush (0000).
 *
 * The server answers the client's flush with the shallow boundary
 * line(s) and its own flush, which the client must consume before
 * sending 'done'.
 */
static int gitt_command_read_until_flush(struct gitt_ssh* ssh)
{
	char buf[64];
	int length;
	int ret;

	while (1) {
		length = gitt_command_get_line_length(ssh);
		if (length < 0)
			return -GITT_ERRNO_INVAL;
		if (length == 0)
			break;
		length -= GITT_PKT_LINE_SIZE;

		while (length) {
			ret = sizeof(buf);
			ret = ret < length ? ret : length;
			ret = gitt_ssh_read(ssh, buf, ret);
			if (ret <= 0)
				return -GITT_ERRNO_INVAL;
			gitt_log_debug("Server: %.*s\n", ret, buf);
			length -= ret;
		}
	}

	return 0;
}

int gitt_command_want(struct gitt_ssh* ssh,
		      char want_sha1[GITT_SHA1_HEX_SIZE + 1],
		      char have_sha1[GITT_SHA1_HEX_SIZE + 1])
{
	struct line_data line[3];
	int ret;

	/*
	 * v1 "smart" protocol negotiation (verified against git upload-pack):
	 *
	 *   want <sha> <capabilities>
	 *   [have <sha>]         (multi_ack)
	 *   deepen 1             (fetch only the tip commit, like 'git fetch --depth=1')
	 *   0000                 (flush: ends the negotiation phase)
	 *   <- [shallow <sha> ... 0000]  (the server's shallow boundary reply)
	 *   done
	 *
	 * The flush must come AFTER the want/have/deepen lines; 'have'
	 * before the flush is a protocol error for the server.
	 *
	 * 'thin-pack' is deliberately not advertised: the client keeps no
	 * objects between fetches, so the pack must be self-contained
	 * (no missing delta bases).
	 */

	line[0].data = "want ";
	line[0].size = 5;
	line[1].data = want_sha1;
	line[1].size = GITT_SHA1_HEX_SIZE;
	line[2].data = GITT_FETCH_CAPABILITIES;
	line[2].size = (uint16_t)strlen(line[2].data);
	ret = gitt_command_line_write(ssh, line, 3);
	if (ret)
		return ret;

	/* Incremental fetch: announce the objects the client already has */
	if (have_sha1 && have_sha1[0]) {
		line[0].data = "have ";
		line[0].size = 5;
		line[1].data = have_sha1;
		line[1].size = GITT_SHA1_HEX_SIZE;
		ret = gitt_command_line_write(ssh, line, 2);
		if (ret)
			return ret;
	}

	/*
	 * Shallow fetch: the server will send only the tip commit (like
	 * 'git fetch --depth=1'). The client only ever restores the tip
	 * tree, so anything older is not wanted.
	 */
	line[0].data = "deepen 1\n";
	line[0].size = 9;
	ret = gitt_command_line_write(ssh, line, 1);
	if (ret)
		return ret;

	/* Flush: ends the negotiation phase */
	ret = gitt_command_line_write(ssh, NULL, 0);
	if (ret)
		return ret;

	/*
	 * The server answers the flush with the shallow boundary line(s)
	 * and its own flush; consume them before sending 'done'.
	 */
	ret = gitt_command_read_until_flush(ssh);
	if (ret)
		return ret;

	line[0].data = "done\n";
	line[0].size = 5;
	ret = gitt_command_line_write(ssh, line, 1);
	if (ret)
		return ret;

	return 0;
}

int gitt_command_get_pack(struct gitt_ssh* ssh, gitt_command_pack_dump dump, void *param)
{
	char buf[32];
	int length;
	int ret;
	bool new_line;
	uint8_t type = GITT_FETCH_LINE_UNKNOWN;
	char *pbuf;
	int valid;

	while (1) {
		new_line = true;
		/* Read line length */
		length = gitt_command_get_line_length(ssh);

		if (length < 0)
			return -GITT_ERRNO_INVAL;
		else if (length == 0)
			break;
		length -= GITT_PKT_LINE_SIZE;

		while (length) {
			ret = sizeof(buf);
			ret = ret < length ? ret : length;
			ret = gitt_ssh_read(ssh, buf, ret);
			if (ret <= 0)
				return -GITT_ERRNO_INVAL;

			pbuf = buf;
			valid = ret;

			if (new_line) {
				/* Classify the line by its first byte */
				if (buf[0] == GITT_SIDE_BAND_DATA) {
					gitt_log_debug("Pack size: %dbyte\n", length);
					pbuf++;
					valid--;
					type = GITT_FETCH_LINE_DATA;
				} else if (buf[0] == GITT_SIDE_BAND_PROGRESS) {
					pbuf++;
					valid--;
					type = GITT_FETCH_LINE_PROGRESS;
				} else if (ret >= 3 && buf[0] == 'N' && buf[1] == 'A' && buf[2] == 'K') {
					gitt_log_debug("NAK\n");
					type = GITT_FETCH_LINE_NAK;
				} else if (ret >= 3 && buf[0] == 'A' && buf[1] == 'C' && buf[2] == 'K') {
					gitt_log_debug("ACK\n");
					type = GITT_FETCH_LINE_ACK;
				} else if (ret >= (int)GITT_SHALLOW_PREFIX_SIZE &&
					   !memcmp(buf, GITT_SHALLOW_PREFIX,
						   GITT_SHALLOW_PREFIX_SIZE)) {
					/* Shallow boundary announced by the server */
					gitt_log_debug("Shallow boundary: %.*s\n",
							valid - GITT_SHALLOW_PREFIX_SIZE,
							pbuf + GITT_SHALLOW_PREFIX_SIZE);
					type = GITT_FETCH_LINE_SHALLOW;
				} else {
					if (buf[0] >= '0' && buf[0] <= 'z')
						gitt_log_error("Unknown case: %.*s\n", valid, pbuf);
					else
						gitt_log_error("Unknown format: '0x%02x'\n", buf[0]);
					type = GITT_FETCH_LINE_UNKNOWN;
				}
			}

			/* Process this line of data */
			if (type == GITT_FETCH_LINE_DATA) {
				gitt_log_debug("+%dbyte\n", valid);
				if (dump && dump(param, pbuf, valid))
					return -GITT_ERRNO_INVAL;
			} else if (type == GITT_FETCH_LINE_PROGRESS) {
				gitt_log_debug("%.*s", valid, pbuf);
			}

			length -= ret;
			new_line = false;
		}
	}

	return 0;
}

int gitt_command_set_pack(struct gitt_ssh* ssh, const char *head, const char *id, const char *refs)
{
	struct line_data line[7];
	int ret;

	/* First line */
	line[0].data = (char *)head;
	line[0].size = GITT_SHA1_HEX_SIZE;
	line[1].data = " ";
	line[1].size = 1;
	line[2].data = (char *)id;
	line[2].size = GITT_SHA1_HEX_SIZE;
	line[3].data = " ";
	line[3].size = 1;
	line[4].data = (char *)refs;
	line[4].size = strlen(refs);
	line[5].data = "\0 ";
	line[5].size = 2;
	line[6].data = GITT_PUSH_CAPABILITIES;
	line[6].size = (uint16_t)strlen(line[6].data);
	ret = gitt_command_line_write(ssh, line, 7);
	if (ret)
		return ret;

	ret = gitt_command_line_write(ssh, NULL, 0);
	if (ret)
		return ret;

	return 0;
}

int gitt_command_write_pack(struct gitt_ssh* ssh, uint8_t *buf, uint16_t size)
{
	int ret;

	ret = gitt_ssh_write(ssh, (char *)buf, size);
	if (ret != size) {
		gitt_log_debug("Error writing pack\n");
		return -GITT_ERRNO_INVAL;
	}

	return 0;
}

int gitt_command_get_push_status(struct gitt_ssh* ssh)
{
	char buf[32];
	int length;
	int msg_len;
	int read_len;
	int ret;
	int retval = -GITT_ERRNO_RETRY;
	char type;

	while (1) {
		length = gitt_command_get_line_length(ssh);

		if (length < 0)
			return -GITT_ERRNO_INVAL;
		else if (length == 0)
			break;
		length -= GITT_PKT_LINE_SIZE;

		/* Read line type */
		ret = gitt_ssh_read(ssh, &type, 1);
		if (ret != 1)
			return -GITT_ERRNO_INVAL;
		length -= 1;

		while (length) {
			/* The status report arrives on the side-band data channel */
			if (type == GITT_SIDE_BAND_DATA) {
				msg_len = gitt_command_get_line_length(ssh);

				if (msg_len < 0) {
					return -GITT_ERRNO_INVAL;
				} else if(msg_len == 0) {
					length -= GITT_PKT_LINE_SIZE;
					continue;
				} else if (msg_len < GITT_PKT_LINE_SIZE + GITT_PKT_STATUS_MIN) {
					return -GITT_ERRNO_INVAL;
				}
				length -= msg_len;
				msg_len -= GITT_PKT_LINE_SIZE;
				buf[0] = '\0';
				buf[1] = '\0';

				read_len = msg_len > sizeof(buf) ? sizeof(buf) : msg_len;
				ret = gitt_ssh_read(ssh, buf, read_len);
				if (ret != read_len) {
					gitt_log_error("Data is incomplete, ret:%d\n", ret);
					return -GITT_ERRNO_INVAL;
				}
				gitt_log_debug("%.*s", ret, buf);
				msg_len -= ret;

				/* Remote return: 'ok' or 'ng' */
				if (retval && buf[0] == 'o' && buf[1] == 'k')
					retval = 0;

				/* Read the remaining data */
				read_len = msg_len > sizeof(buf) ? sizeof(buf) : msg_len;
				while (read_len) {
					ret = gitt_ssh_read(ssh, buf, read_len);
					if (ret != read_len) {
						gitt_log_error("Remaining data is incomplete, ret:%d\n", ret);
						return -GITT_ERRNO_INVAL;
					}
					gitt_log_debug("%.*s", ret, buf);
					msg_len -= ret;
					read_len = msg_len > sizeof(buf) ? sizeof(buf) : msg_len;
				}
			} else {
				ret = sizeof(buf);
				ret = ret < length ? ret : length;
				ret = gitt_ssh_read(ssh, buf, ret);
				if (ret <= 0)
					return -GITT_ERRNO_INVAL;
				gitt_log_debug("%.*s", ret, buf);

				length -= ret;
			}
		}
	}

	if (retval)
		gitt_log_error("Remote said: Not good\n");
	else
		gitt_log_debug("Remote said: OK\n");

	return retval;
}

void gitt_command_end(struct gitt_ssh* ssh)
{
	gitt_ssh_disconnect(ssh);
	gitt_ssh_free(ssh);
}
