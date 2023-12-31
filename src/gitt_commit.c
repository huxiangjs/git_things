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
#include <gitt_commit.h>
#include <gitt_log.h>
#include <gitt_sha1.h>
#include <string.h>
#include <gitt_errno.h>

static char *gitt_commit_find_line_end(char *buf, uint16_t size, char dir)
{
	uint16_t index;

	if (dir == 'D') {
		index = 0;
		while (index < size) {
			if (buf[index] == '\n')
				return buf + index;
			index++;
		}
	} else if (dir == 'U') {
		index = size;
		while (index > 0) {
			index--;
			if (buf[index] == '\n')
				return buf + index;
		}
	}

	return NULL;
}

static char *gitt_commit_find_line_char(char *buf, uint16_t size, char ch)
{
	while (size) {
		if (*buf == ch)
			return buf;
		else if (*buf == '\n')
			break;
		buf++;
		size--;
	}

	return NULL;
}

static int gitt_commit_sha1(char *buf, uint16_t size, struct gitt_commit_id *id)
{
	struct gitt_sha1 sha1;
	int ret;
	char front_str[16];

	gitt_sha1_init(&sha1);

	ret = sprintf(front_str, "commit %u", size);
	if (ret <= 0)
		return -GITT_ERRNO_INVAL;

	ret = gitt_sha1_update(&sha1, (uint8_t *)front_str, ret + 1);
	if (ret)
		return ret;

	ret = gitt_sha1_update(&sha1, (uint8_t *)buf, size);
	if (ret)
		return ret;

	ret = gitt_sha1_hexdigest(&sha1, id->sha1);
	if (ret)
		return ret;

	gitt_log_debug("SHA1: %s\n", id->sha1);
	return 0;
}

int gitt_commit_parse(char *buf, uint16_t size, struct gitt_commit *commit)
{
	char *empty_str = "";
	char *start;
	char *cur;
	char *next;
	int valid_size;

	if (gitt_commit_sha1(buf, size, &commit->id))
		return -GITT_ERRNO_INVAL;

	/* Padded with empty string */
	commit->tree.sha1 = empty_str;
	commit->parent.sha1 = empty_str;
	commit->author.date = empty_str;
	commit->author.email = empty_str;
	commit->author.name = empty_str;
	commit->author.zone = empty_str;
	commit->committer.date = empty_str;
	commit->committer.email = empty_str;
	commit->committer.name = empty_str;
	commit->committer.zone = empty_str;

	/* Parse */
	start = buf;
	valid_size = size;
	while (valid_size > 0) {
		cur = gitt_commit_find_line_char(start, valid_size, ' ');
		if (!cur)
			break;
		*cur = '\0';

		gitt_log_debug("Line type: %s\n", start);

		if (!strcmp(start, "tree")) {
			start += 5;
			valid_size -= 5;
			next = gitt_commit_find_line_end(start, valid_size, 'D');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			gitt_log_debug("SHA-1: %s\n", start);

			commit->tree.sha1 = start;
			valid_size -= next - start + 1;
			start = next + 1;
		} else if (!strcmp(start, "parent")) {
			start += 7;
			valid_size -= 7;
			next = gitt_commit_find_line_end(start, valid_size, 'D');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';

			commit->parent.sha1 = start;
			valid_size -= next - start + 1;
			start = next + 1;
		}  else if (!strcmp(start, "author")) {
			start += 7;
			valid_size -= 7;

			/* Name */
			next = gitt_commit_find_line_char(start, valid_size, '<');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*(next - 1) = '\0';
			commit->author.name = start;
			valid_size -= next - start + 1;
			start = next + 1;

			/* E-Mail */
			next = gitt_commit_find_line_char(start, valid_size, '>');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			commit->author.email = start;
			valid_size -= next - start + 2;
			start = next + 2;

			/* Date */
			next = gitt_commit_find_line_char(start, valid_size, ' ');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			commit->author.date = start;
			valid_size -= next - start + 1;
			start = next + 1;

			/* Zone */
			next = gitt_commit_find_line_end(start, valid_size, 'D');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			commit->author.zone = start;
			valid_size -= next - start + 1;
			start = next + 1;
		} else if (!strcmp(start, "committer")) {
			start += 10;
			valid_size -= 10;

			/* Name */
			next = gitt_commit_find_line_char(start, valid_size, '<');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*(next - 1) = '\0';
			commit->committer.name = start;
			valid_size -= next - start + 1;
			start = next + 1;

			/* E-Mail */
			next = gitt_commit_find_line_char(start, valid_size, '>');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			commit->committer.email = start;
			valid_size -= next - start + 2;
			start = next + 2;

			/* Date */
			next = gitt_commit_find_line_char(start, valid_size, ' ');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			commit->committer.date = start;
			valid_size -= next - start + 1;
			start = next + 1;

			/* Zone */
			next = gitt_commit_find_line_end(start, valid_size, 'D');
			if (!next)
				return -GITT_ERRNO_INVAL;
			*next = '\0';
			commit->committer.zone = start;
			valid_size -= next - start + 1;
			start = next + 1;
		} else {
			gitt_log_debug("break\n");
			valid_size -= cur - start + 1;
			start = cur + 1;
			break;
		}
	}

	valid_size--;
	start++;
	if (valid_size > 0) {
		gitt_log_debug("Parse message\n");
		commit->message = start;
	} else {
		gitt_log_debug("Message not found\n");
		return -GITT_ERRNO_INVAL;
	}

	return 0;
}

int gitt_commit_build(gitt_obj_data dump, void *p, struct gitt_commit *commit)
{
	int ret;
	int length;

	/* Tree line */
	length = strlen(commit->tree.sha1);
	if (length) {
		ret = dump(p, (uint8_t *)"tree ", 5, false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)commit->tree.sha1, length, false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)"\n", 1, false);
		if (ret)
			return ret;
	}

	/* Parent line */
	length = strlen(commit->parent.sha1);
	if (length) {
		ret = dump(p, (uint8_t *)"parent ", 7, false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)commit->parent.sha1, length, false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)"\n", 1, false);
		if (ret)
			return ret;
	}

	/* Author line */
	ret = dump(p, (uint8_t *)"author ", 7, false);
	if (ret)
		return ret;

	/* Author:Name */
	length = strlen(commit->author.name);
	ret = dump(p, (uint8_t *)commit->author.name, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)" <", 2, false);
	if (ret)
		return ret;

	/* Author:Email */
	length = strlen(commit->author.email);
	ret = dump(p, (uint8_t *)commit->author.email, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)"> ", 2, false);
	if (ret)
		return ret;

	/* Author:Date */
	length = strlen(commit->author.date);
	ret = dump(p, (uint8_t *)commit->author.date, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)" ", 1, false);
	if (ret)
		return ret;

	/* Author:Zone */
	length = strlen(commit->author.zone);
	ret = dump(p, (uint8_t *)commit->author.zone, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)"\n", 1, false);
	if (ret)
		return ret;

	/* Committer line */
	ret = dump(p, (uint8_t *)"committer ", 10, false);
	if (ret)
		return ret;

	/* Committer:Name */
	length = strlen(commit->committer.name);
	ret = dump(p, (uint8_t *)commit->committer.name, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)" <", 2, false);
	if (ret)
		return ret;

	/* Committer:Email */
	length = strlen(commit->committer.email);
	ret = dump(p, (uint8_t *)commit->committer.email, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)"> ", 2, false);
	if (ret)
		return ret;

	/* Committer:Date */
	length = strlen(commit->committer.date);
	ret = dump(p, (uint8_t *)commit->committer.date, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)" ", 1, false);
	if (ret)
		return ret;

	/* Committer:Zone */
	length = strlen(commit->committer.zone);
	ret = dump(p, (uint8_t *)commit->committer.zone, length, false);
	if (ret)
		return ret;

	ret = dump(p, (uint8_t *)"\n\n", 2, false);
	if (ret)
		return ret;

	/* Message */
	length = strlen(commit->message);
	ret = dump(p, (uint8_t *)commit->message, length, true);
	if (ret)
		return ret;

	return 0;
}

uint16_t gitt_commit_length(struct gitt_commit *commit)
{
	int length;
	uint16_t retval = 0;

	/* Tree line */
	length = strlen(commit->tree.sha1);
	if (length)
		retval += 5 + (uint16_t)length + 1;

	/* Parent line */
	length = strlen(commit->parent.sha1);
	if (length)
		retval += 7 + (uint16_t)length + 1;

	/* Author:Name */
	length = strlen(commit->author.name);
	retval += 7 + (uint16_t)length + 2;

	/* Author:Email */
	length = strlen(commit->author.email);
	retval += (uint16_t)length + 2;

	/* Author:Date */
	length = strlen(commit->author.date);
	retval += (uint16_t)length + 1;

	/* Author:Zone */
	length = strlen(commit->author.zone);
	retval += (uint16_t)length + 1;

	/* Committer:Name */
	length = strlen(commit->committer.name);
	retval += 10 + (uint16_t)length + 2;

	/* Committer:Email */
	length = strlen(commit->committer.email);
	retval += (uint16_t)length + 2;

	/* Committer:Date */
	length = strlen(commit->committer.date);
	retval += (uint16_t)length + 1;

	/* Committer:Zone */
	length = strlen(commit->committer.zone);
	retval += (uint16_t)length + 2;

	/* Message */
	length = strlen(commit->message);
	retval += (uint16_t)length;

	return retval;
}

static int gitt_obj_data_dump(void *p, uint8_t *buf, uint16_t size, bool end)
{
	return gitt_sha1_update((struct gitt_sha1 *)p, buf, size);
}

int gitt_commit_sha1_update(struct gitt_commit *commit)
{
	struct gitt_sha1 sha1;
	int ret;
	char front_str[16];

	gitt_sha1_init(&sha1);

	ret = sprintf(front_str, "commit %u", gitt_commit_length(commit));
	if (ret <= 0)
		return -GITT_ERRNO_INVAL;

	ret = gitt_sha1_update(&sha1, (uint8_t *)front_str, ret + 1);
	if (ret)
		return ret;

	ret = gitt_commit_build(gitt_obj_data_dump, &sha1, commit);
	if (ret)
		return ret;

	ret = gitt_sha1_hexdigest(&sha1, commit->id.sha1);
	if (ret)
		return ret;

	gitt_log_debug("SHA1: %s\n", commit->id.sha1);
	return 0;
}
