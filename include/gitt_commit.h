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

#ifndef __GITT_COMMIT_H_
#define __GITT_COMMIT_H_

#include <stdint.h>
#include <gitt_obj.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GITT_COMMIT_AUTO_BASE		"-"

struct gitt_commit_id {
	char sha1[41];
};

struct gitt_commit_tree {
	char *sha1;
};

struct gitt_commit_parent {
	char *sha1;
};

struct gitt_commit_author {
	char *name;
	char *email;
	char *date;
	char *zone;
};

struct gitt_commit_committer {
	char *name;
	char *email;
	char *date;
	char *zone;
};

struct gitt_commit {
	struct gitt_commit_id id;
	struct gitt_commit_tree tree;
	struct gitt_commit_parent parent;
	struct gitt_commit_author author;
	struct gitt_commit_committer committer;
	char *message;
};

int gitt_commit_parse(char *buf, uint16_t size, struct gitt_commit *commit);
int gitt_commit_build(gitt_obj_data dump, void *p, struct gitt_commit *commit);
uint16_t gitt_commit_length(struct gitt_commit *commit);
int gitt_commit_sha1_update(struct gitt_commit *commit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_COMMIT_H_ */
