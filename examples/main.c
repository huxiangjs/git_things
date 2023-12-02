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
#include <malloc.h>
#include <gitt.h>
#include <gitt_errno.h>

#define PRIVKEY_PATH	"/home/huxiang/.ssh/id_ed25519"
#define REPERTORY_URL	"git@github.com:huxiangjs/gitt_test.git"

static void __replace(char src, char tag, char *buf, int buf_size)
{
	int i;

	for (i = 0; i < buf_size; i++)
		buf[i] = buf[i] == src ? tag : buf[i];
}

static void gitt_remote_event_callback(struct gitt *g, struct gitt_device *device,
				       char *date, char *zone, char *event)
{
	int len = strlen(event);

	printf("%s | %s | %s | %s | ", device->name, device->id, date, zone);

	__replace('\n', ' ', event, len);
	if (len < 32)
		printf("%s\n", event);
	else
		printf("%.*s...\n", 32, event);
}

int main(int args, char *argv[])
{
	int ret = 0;
	char buffer[4096];
	uint8_t *buf = (uint8_t *)malloc(0xffff);
	FILE *file;
	struct gitt g = {0};

	/* Read key file */
	file = fopen(PRIVKEY_PATH, "rb");
	if (!file)
		goto end;
	ret = fread(buffer, 1, sizeof(buffer) - 1, file);
	if (ret <= 0) {
		fclose(file);
		goto end;
	}
	buffer[ret] = '\0';
	printf("%s\n", buffer);
	fclose(file);

	g.privkey = buffer;
	g.url = REPERTORY_URL;
	g.buf = buf;
	g.buf_len = 0xffff;
	g.remote_event = gitt_remote_event_callback;
	ret = gitt_init(&g);
	if (ret)
		goto end;
	printf("Init result: %s\n", GITT_ERRNO_STR(ret));

	/* Set device info */
	strcpy(g.device.name, "TestDevice");
	strcpy(g.device.id, "000000000000");

	ret = gitt_history(&g);
	if (ret)
		goto end1;
	printf("History result: %s\n", GITT_ERRNO_STR(ret));

	ret = gitt_commit_event(&g, "Test event 2");
	if (ret)
		goto end1;
	printf("Commit event result: %s\n", GITT_ERRNO_STR(ret));

end1:
	gitt_end(&g);
end:
	printf("Test end. Result: %s\n", GITT_ERRNO_STR(ret));
	free(buf);
	return 0;
}
