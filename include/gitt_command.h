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

#ifndef __GITT_COMMAND_H_
#define __GITT_COMMAND_H_

#include <stdint.h>
#include <gitt_ssh.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int (*gitt_command_pack_dump)(void *param, char *data, int size);

struct gitt_ssh* gitt_command_start_receive(const char *url, const char *privkey);
struct gitt_ssh* gitt_command_start_upload(const char *url, const char *privkey);
int gitt_command_get_head(struct gitt_ssh* ssh, char sha1_hex[41]);
void gitt_command_end(struct gitt_ssh* ssh);
int gitt_command_say_byebye(struct gitt_ssh* ssh);
int gitt_command_want(struct gitt_ssh* ssh, char want_sha1[41], char have_sha1[41]);
int gitt_command_get_pack(struct gitt_ssh* ssh, gitt_command_pack_dump dump, void *param);
int gitt_command_set_pack(struct gitt_ssh* ssh, const char *head, const char *id, const char *refs);
int gitt_command_write_pack(struct gitt_ssh* ssh, uint8_t *buf, uint16_t size);
int gitt_command_get_state(struct gitt_ssh* ssh);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_COMMAND_H_ */
