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
#include <gitt_pack.h>
#include <gitt_unpack.h>
#include <gitt_errno.h>
#include <gitt_commit.h>
#include <gitt_sha1.h>

static FILE *file;

/* A copy of the pack in memory, for the verify step */
static uint8_t pack_buf[4096];
static uint16_t pack_len;

static struct gitt_commit ref_commit;
static int obj_match;		/* object id matches the commit id */
static int pack_ok;		/* pack checksum verified */

static int gitt_pack_data_dump(void *p, uint8_t *buf, uint16_t size)
{
	uint16_t i;
	int ret;

	(void)p;

	for (i = 0; i < size; i++)
		printf("%02x ", buf[i]);

	for (i = 0; i < size && pack_len < sizeof(pack_buf); i++)
		pack_buf[pack_len++] = buf[i];

	ret = fwrite(buf, 1, size, file);
	if (ret <= 0) {
		fprintf(stderr, "Error writing file\n");
		return -1;
	}

	return 0;
}

static void gitt_unpack_header_callback(uint32_t *version, uint32_t *number)
{
	printf("version:%u, number of objects:%u\n", *version, *number);
}

static void gitt_unpack_obj_callback(struct gitt_obj *obj)
{
	printf("* ");

	if (obj->sha1[0] && !strcmp(obj->sha1, ref_commit.id.sha1))
		obj_match = 1;
	else
		printf("object id %s does not match commit id %s\n",
		       obj->sha1, ref_commit.id.sha1);
}

static void gitt_unpack_verify_callback(bool pass, struct gitt_sha1 *sha1)
{
	char sha1_hex[41];
	int ret;

	ret = gitt_sha1_hexdigest(sha1, sha1_hex);
	if (ret) {
		printf("Test fail: hexdigest %d\n", ret);
		return;
	}
	printf("Pack checksum: %s (%s)\n", sha1_hex,
	       pass ? "verified" : "MISMATCH");

	if (pass)
		pack_ok = 1;
}

/* Unpack the pack produced above and check the object hash */
static int test_pack_verify(void)
{
	struct gitt_unpack unpack = {0};
	uint8_t buffer[4096];
	int ret;

	printf("Verifying the pack by unpacking it...\n");

	unpack.buf = buffer;
	unpack.buf_len = sizeof(buffer);
	unpack.header_dump = gitt_unpack_header_callback;
	unpack.obj_dump = gitt_unpack_obj_callback;
	unpack.verify_dump = gitt_unpack_verify_callback;
	ret = gitt_unpack_init(&unpack);
	if (ret)
		return ret;

	ret = gitt_unpack_update(&unpack, pack_buf, pack_len);
	if (ret) {
		fprintf(stderr, "Test fail (unpack): %d\n", ret);
		return ret;
	}

	gitt_unpack_end(&unpack);
	gitt_unpack_store_clear(&unpack);

	if (!pack_ok || !obj_match) {
		printf("Test fail: the unpacked pack does not verify\n");
		return -1;
	}

	printf("Pack verified: checksum ok, unpacked commit matches (id %s)\n",
	       ref_commit.id.sha1);
	return 0;
}

static int test_pack(void)
{
	int ret;
	struct gitt_obj obj;
	struct gitt_pack pack;
	uint8_t buffer[4096];
	struct gitt_commit commit = {0};
	char hexdigest[41];

	file = fopen("pack-test.pack", "wb");
	if (!file) {
		fprintf(stderr, "Error opening file\n");
		return -1;
	}

	/* Initialize commit */
	commit.tree.sha1       = "4b825dc642cb6eb9a060e54bf8d69288fbee4904";
	commit.parent.sha1     = "1e5d56c3b90714c7761a3c77d4d67aa12c3ae13a";
	commit.author.date     = "170098713";
	commit.author.email    = "huxiangjs1@foxmail.com";
	commit.author.name     = "Hoozz1";
	commit.author.zone     = "+080";
	commit.committer.date  = "170098714";
	commit.committer.email = "huxiangjs2@foxmail.com";
	commit.committer.name  = "Hoozz2";
	commit.committer.zone  = "+081";
	commit.message         = "Init for test";
	ret = gitt_commit_sha1_update(&commit);
	if (ret) {
		printf("Update SHA-1 fail\n");
		return ret;
	}
	printf("Commit id: %s\n", commit.id.sha1);

	/* Keep a copy for the verify step */
	memcpy(&ref_commit, &commit, sizeof(ref_commit));

	/* Initialize header */
	pack.buf = buffer;
	pack.buf_len = sizeof(buffer);
	pack.obj_num = 1;
	pack.data_dump = gitt_pack_data_dump;
	ret = gitt_pack_init(&pack);
	if (ret) {
		printf("Pack init fail\n");
		return ret;
	}

	/* Update to pack */
	obj.type = GITT_OBJ_TYPE_COMMIT;
	obj.data = &commit;
	obj.size = gitt_commit_length(&commit);
	ret = gitt_pack_update(&pack, &obj);
	if (ret) {
		printf("Pack update fail\n");
		return ret;
	}

	ret = gitt_sha1_hexdigest(&pack.sha1, hexdigest);
	if (ret) {
		printf("SH1-A hexdigest fail\n");
		return ret;
	}
	printf("\n");
	printf("SH1-A: %s\n", hexdigest);

	fclose(file);

	gitt_pack_end(&pack);

	ret = test_pack_verify();
	if (ret)
		return ret;

	printf("Test end\n");

	return 0;
}

int main(int args, char *argv[])
{
	/*
	 * Doc: https://git-scm.com/book/zh/v2
	 *
	 * [git index-pack -v pack-test.pack]  Can generate idx index file.
	 * [git verify-pack -v pack-test.pack] You can view pack information.
	 */
	return test_pack() ? 1 : 0;
}
