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
#include <gitt_repertory.h>
#include <gitt_commit.h>

#define PRIVKEY_PATH	"/home/huxiang/.ssh/id_ed25519"
#define REPERTORY_URL	"git@github.com:huxiangjs/git_things.git"

static void __replace(char src, char tag, char *buf, int buf_size)
{
	int i;

	for (i = 0; i < buf_size; i++)
		buf[i] = buf[i] == src ? tag : buf[i];
}

static void gitt_repertory_commit_dump(struct gitt_commit *commit)
{
	printf("tree.sha1      : %s\n", commit->tree.sha1      );
	printf("parent.sha1    : %s\n", commit->parent.sha1    );
	printf("author.date    : %s\n", commit->author.date    );
	printf("author.email   : %s\n", commit->author.email   );
	printf("author.name    : %s\n", commit->author.name    );
	printf("author.zone    : %s\n", commit->author.zone    );
	printf("committer.date : %s\n", commit->committer.date );
	printf("committer.email: %s\n", commit->committer.email);
	printf("committer.name : %s\n", commit->committer.name );
	printf("committer.zone : %s\n", commit->committer.zone );
	printf("message        : ");
	__replace('\n', ' ', commit->message, strlen(commit->message));
	printf("%s\n", commit->message);
	printf("\n");
}

int main(int args, char *argv[])
{
	int ret;
	char buffer[4096];
	uint8_t buf[4096*6];
	FILE *file;
	struct gitt_repertory repertory;

	/* Read key file */
	file = fopen(PRIVKEY_PATH, "rb");
	if (!file)
		return -1;
	ret = fread(buffer, 1, sizeof(buffer) - 1, file);
	if (ret <= 0) {
		fclose(file);
		return -1;
	}
	buffer[ret] = '\0';
	printf("%s\n", buffer);
	fclose(file);

	repertory.privkey = buffer;
	repertory.url = REPERTORY_URL;
	repertory.buf = buf;
	repertory.buf_len = sizeof(buf);
	repertory.commit_dump = gitt_repertory_commit_dump;
	ret = gitt_repertory_init(&repertory);
	if (ret) {
		printf("An error occurred while initializing the repertory\n");
		return -1;
	}

	// ret = gitt_repertory_update_head(&repertory);
	ret = gitt_repertory_clone(&repertory);
	if (ret) {
		printf("An error occurred while clone the repertory\n");
		return -1;
	}

	printf("Test end\n");

	return 0;
}
