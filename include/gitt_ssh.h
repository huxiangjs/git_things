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

#ifndef __GITT_SSH_H_
#define __GITT_SSH_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct gitt_ssh;

struct gitt_ssh_url {
	char user[16];
	char host[32];
	char port[8];
	char repository[64];
};

struct gitt_ssh* gitt_ssh_alloc(void);
void gitt_ssh_free(struct gitt_ssh *ssh);
int gitt_ssh_connect(struct gitt_ssh *ssh, const char *url, const char *exec,
		     const char *privkey);
int gitt_ssh_read(struct gitt_ssh *ssh, char *buf, int size);
int gitt_ssh_write(struct gitt_ssh *ssh, char *buf, int size);
void gitt_ssh_disconnect(struct gitt_ssh *ssh);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_SSH_H_ */
