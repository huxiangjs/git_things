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

#ifndef __GITT_REPERTORY_H_
#define __GITT_REPERTORY_H_

#include <gitt_unpack.h>
#include <gitt_commit.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*gitt_repertory_commit)(struct gitt_commit *commit);

struct gitt_repertory {
	struct gitt_unpack unpack;
	char *url;
	char *privkey;
	char head_sha1[41];
	uint8_t *buf;
	uint16_t buf_len;
	gitt_repertory_commit commit_dump;
};

int gitt_repertory_init(struct gitt_repertory *repertory);
int gitt_repertory_clone(struct gitt_repertory *repertory);
int gitt_repertory_push(struct gitt_repertory *repertory);
int gitt_repertory_pull(struct gitt_repertory *repertory);
int gitt_repertory_update_head(struct gitt_repertory *repertory);
int gitt_repertory_end(struct gitt_repertory *repertory);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_REPERTORY_H_ */
