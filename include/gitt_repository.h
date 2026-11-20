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

#ifndef __GITT_REPOSITORY_H_
#define __GITT_REPOSITORY_H_

#include <stdint.h>
#include <gitt_unpack.h>
#include <gitt_commit.h>
#include <gitt_pack.h>
#include <gitt_ssh.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GITT_REPOSITORY_MAX_FILES	16
#define GITT_REPOSITORY_MAX_PATH	64
#define GITT_REPOSITORY_MAX_TREES	8

struct gitt_repository;

typedef void (*gitt_repository_commit)(struct gitt_repository *repository,
				      struct gitt_commit *commit);
typedef void (*gitt_repository_blob)(struct gitt_repository *repository,
				     char *path, uint8_t *data, uint32_t size);

/*
 * The file registry (path <-> SHA-1), built after a pull by walking
 * the trees in the object store. It maps a blob (identified by its
 * SHA-1) to the file path of the tip commit.
 */
struct gitt_repository_file {
	char path[GITT_REPOSITORY_MAX_PATH];
	char sha1[41];
};

struct gitt_repository {
	struct gitt_unpack unpack;
	struct gitt_pack pack;
	char *url;
	char *privkey;
	char head[41];
	char refs[32];
	uint8_t *buf;
	uint16_t buf_len;
	gitt_repository_commit commit_dump;
	gitt_repository_blob blob_dump;
	struct gitt_ssh* ssh;

	/* If set, the pull requests a shallow fetch (depth 1) */
	uint8_t shallow;

	/*
	 * Set after a pull successfully restored the file tree of the
	 * current head. The "already up to date" fast path only applies
	 * when the files of that head are actually available in this
	 * session (the client is stateless and keeps no objects).
	 */
	uint8_t files_loaded;

	/*
	 * The root tree of the tip commit, remembered while the pack
	 * streams in (only the commit that matches the head counts)
	 */
	char root_tree_sha1[41];

	/* File registry (path <-> SHA-1), rebuilt after every pull */
	uint8_t file_num;
	struct gitt_repository_file files[GITT_REPOSITORY_MAX_FILES];
	uint8_t tree_num;
	char tree_sha1[GITT_REPOSITORY_MAX_TREES][41];
	char tree_path[GITT_REPOSITORY_MAX_TREES][GITT_REPOSITORY_MAX_PATH];
};

int gitt_repository_init(struct gitt_repository *repository);
int gitt_repository_clone(struct gitt_repository *repository);
int gitt_repository_push_commit(struct gitt_repository *repository,
			       struct gitt_commit *commit,
			       struct gitt_obj *objs, uint8_t obj_num);
int gitt_repository_pull(struct gitt_repository *repository);
int gitt_repository_update_head(struct gitt_repository *repository);
int gitt_repository_end(struct gitt_repository *repository);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_REPOSITORY_H_ */
