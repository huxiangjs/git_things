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

#ifndef __GITT_H_
#define __GITT_H_

#include <gitt_repertory.h>
#include <gitt_device.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GITT_VERSION			"0_90"

struct gitt;

typedef int (*gitt_get_date)(char *buf, uint8_t size);
typedef int (*gitt_get_zone)(char *buf, uint8_t size);
typedef void (*gitt_remote_event)(struct gitt *g, struct gitt_device *device,
				 char *date, char *zone, char *event);

struct gitt {
	struct gitt_device device;
	struct gitt_repertory repertory;
	gitt_get_date get_date;
	gitt_get_zone get_zone;
	gitt_remote_event remote_event;
	char *url;
	char *privkey;
	uint8_t *buf;
	uint16_t buf_len;
};

int gitt_init(struct gitt *g);
int gitt_update_event(struct gitt *g);
int gitt_commit_event(struct gitt *g, char *data);
int gitt_history(struct gitt *g);
void gitt_end(struct gitt *g);
char *gitt_version(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_H_ */
