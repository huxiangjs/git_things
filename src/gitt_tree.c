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
#include <stdlib.h>
#include <gitt_tree.h>
#include <gitt_sha1.h>
#include <gitt_log.h>
#include <gitt_errno.h>

/* SHA-1 digest size, bytes; hex string length, chars (no NUL) */
#define GITT_SHA1_SIZE		20
#define GITT_SHA1_HEX_SIZE	40

/*
 * The object id header "<type> <size>\0" (longest:
 * "ofs_delta 4294967295")
 */
#define GITT_OBJECT_HEADER_SIZE	24

/*
 * Longest mode string of a tree entry (git tree format, fixed: 5 or
 * 6 chars, e.g. "40000", "100644")
 */
#define GITT_TREE_MODE_MAX	6

/*
 * File format of a tree object:
 * | <mode> | ' ' | <name> | '\0' | <20byte SHA-1> | (one line per entry)
 *
 * The entries must be sorted by name. A directory entry is sorted as if
 * its name had a '/' appended at the end.
 */

static inline int8_t gitt_tree_half_byte(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 0xa;
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 0xa;

	return -1;
}

static int gitt_tree_hex_to_bin(const char *hex, uint8_t *bin)
{
	int i;
	int8_t half_byte;

	for (i = 0; i < GITT_SHA1_SIZE; i++) {
		half_byte = gitt_tree_half_byte(hex[i * 2]);
		if (half_byte < 0)
			return -GITT_ERRNO_INVAL;
		bin[i] = half_byte << 4;
		half_byte = gitt_tree_half_byte(hex[i * 2 + 1]);
		if (half_byte < 0)
			return -GITT_ERRNO_INVAL;
		bin[i] |= half_byte;
	}

	return 0;
}

static int gitt_tree_entry_cmp(const void *a, const void *b)
{
	const struct gitt_tree_entry *ea = (const struct gitt_tree_entry *)a;
	const struct gitt_tree_entry *eb = (const struct gitt_tree_entry *)b;
	char key_a[GITT_TREE_NAME_SIZE + 1];
	char key_b[GITT_TREE_NAME_SIZE + 1];
	size_t len_a = strlen(ea->name);
	size_t len_b = strlen(eb->name);

	/* Directory entries are compared as if a '/' were appended */
	memcpy(key_a, ea->name, len_a);
	key_a[len_a] = (!strcmp(ea->mode, GITT_TREE_DIR_MODE)) ? '/' : '\0';
	memcpy(key_b, eb->name, len_b);
	key_b[len_b] = (!strcmp(eb->mode, GITT_TREE_DIR_MODE)) ? '/' : '\0';

	return strcmp(key_a, key_b);
}

void gitt_tree_sort(struct gitt_tree *tree)
{
	qsort(tree->entries, tree->entry_num, sizeof(tree->entries[0]),
	      gitt_tree_entry_cmp);
}

uint16_t gitt_tree_length(struct gitt_tree *tree)
{
	uint16_t length = 0;
	uint8_t i;

	for (i = 0; i < tree->entry_num; i++)
		length += (uint16_t)(strlen(tree->entries[i].mode) + 1 +
				     strlen(tree->entries[i].name) + 1 +
				     GITT_SHA1_SIZE);

	return length;
}

int gitt_tree_build(gitt_obj_data dump, void *p, struct gitt_tree *tree)
{
	uint8_t i;
	uint8_t sha1_bin[GITT_SHA1_SIZE];
	size_t name_len;
	int ret;

	if (!tree->entry_num) {
		/*
		 * An empty tree still needs a (empty) zlib stream,
		 * so trigger Z_FINISH with a zero-length part.
		 */
		return dump(p, (uint8_t *)"", 0, true);
	}

	for (i = 0; i < tree->entry_num; i++) {
		bool last = (i + 1 == tree->entry_num);

		name_len = strlen(tree->entries[i].name);

		ret = gitt_tree_hex_to_bin(tree->entries[i].sha1, sha1_bin);
		if (ret)
			return ret;

		/* The mode is 5 or 6 chars (e.g. "40000", "100644") */
		ret = dump(p, (uint8_t *)tree->entries[i].mode,
			   (uint16_t)strlen(tree->entries[i].mode), false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)" ", 1, false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)tree->entries[i].name, (uint16_t)name_len, false);
		if (ret)
			return ret;

		ret = dump(p, (uint8_t *)"\0", 1, false);
		if (ret)
			return ret;

		ret = dump(p, sha1_bin, sizeof(sha1_bin), last);
		if (ret)
			return ret;
	}

	return 0;
}

int gitt_tree_parse(char *buf, uint16_t size, struct gitt_tree *tree)
{
	uint16_t index = 0;
	uint8_t i;
	uint8_t sha1_bin[GITT_SHA1_SIZE];
	const char *tables = "0123456789abcdef";

	tree->entry_num = 0;
	tree->sha1[0] = '\0';

	while (index < size) {
		struct gitt_tree_entry *entry;

		if (tree->entry_num >= GITT_TREE_MAX_ENTRIES) {
			gitt_log_error("Too many tree entries\n");
			return -GITT_ERRNO_INVAL;
		}

		entry = &tree->entries[tree->entry_num];

		/* Mode: 5 or 6 chars (e.g. "40000", "100644"), then ' ' */
		if (index + GITT_TREE_MODE_MAX > size)
			return -GITT_ERRNO_INVAL;
		for (i = 0; i < GITT_TREE_MODE_MAX; i++) {
			if (buf[index] == ' ')
				break;
			entry->mode[i] = buf[index];
			index++;
		}
		entry->mode[i] = '\0';
		if (buf[index] != ' ')
			return -GITT_ERRNO_INVAL;
		index++;

		/* Name: until '\0' (a name that does not fit is an error) */
		for (i = 0; i < GITT_TREE_NAME_SIZE && index < size; i++, index++) {
			if (buf[index] == '\0')
				break;
			entry->name[i] = buf[index];
		}
		if (index >= size)
			return -GITT_ERRNO_INVAL;
		if (i == GITT_TREE_NAME_SIZE) {
			gitt_log_error("Tree entry name is too long\n");
			return -GITT_ERRNO_INVAL;
		}
		entry->name[i] = '\0';
		index++;

		/* SHA-1: 20 byte */
		if (index + GITT_SHA1_SIZE > size)
			return -GITT_ERRNO_INVAL;
		memcpy(sha1_bin, buf + index, sizeof(sha1_bin));
		index += GITT_SHA1_SIZE;

		for (i = 0; i < GITT_SHA1_SIZE; i++)
			snprintf(entry->sha1 + i * 2, 3, "%c%c",
				tables[sha1_bin[i] >> 4 & 0xf],
				tables[sha1_bin[i] & 0xf]);
		entry->sha1[GITT_SHA1_HEX_SIZE] = '\0';

		tree->entry_num++;
	}

	return 0;
}

static int gitt_tree_sha1_dump(void *p, uint8_t *buf, uint16_t size, bool end)
{
	return gitt_sha1_update((struct gitt_sha1 *)p, buf, size);
}

int gitt_tree_sha1_update(struct gitt_tree *tree)
{
	struct gitt_sha1 sha1;
	char front_str[GITT_OBJECT_HEADER_SIZE];
	int ret;

	/* The entries must be sorted before computing the SHA-1 */
	gitt_tree_sort(tree);

	gitt_sha1_init(&sha1);

	ret = sprintf(front_str, "tree %u", gitt_tree_length(tree));
	if (ret <= 0)
		return -GITT_ERRNO_INVAL;

	ret = gitt_sha1_update(&sha1, (uint8_t *)front_str, ret + 1);
	if (ret)
		return ret;

	ret = gitt_tree_build(gitt_tree_sha1_dump, &sha1, tree);
	if (ret)
		return ret;

	ret = gitt_sha1_hexdigest(&sha1, tree->sha1);
	if (ret)
		return ret;

	gitt_log_debug("Tree SHA1: %s\n", tree->sha1);
	return 0;
}
