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
#include <gitt_repertory.h>
#include <gitt_commit.h>
#include <gitt_tree.h>
#include <gitt_errno.h>

#define PRIVKEY_PATH	"/home/huxiang/.ssh/id_ed25519"
#define REPERTORY_URL	"git@github.com:huxiangjs/gitt_test.git"

static void __replace(char src, char tag, char *buf, int buf_size)
{
	int i;

	for (i = 0; i < buf_size; i++)
		buf[i] = buf[i] == src ? tag : buf[i];
}

static void gitt_repertory_commit_dump(struct gitt_commit *commit)
{
	printf("id.sha1        : %s\n", commit->id.sha1        );
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
	int ret = 0;
	char buffer[4096];
	uint8_t *buf = (uint8_t *)malloc(0xffff);
	FILE *file;
	struct gitt_repertory repertory;

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

	repertory.privkey = buffer;
	repertory.url = REPERTORY_URL;
	repertory.buf = buf;
	repertory.buf_len = 0xffff;
	repertory.commit_dump = gitt_repertory_commit_dump;
	ret = gitt_repertory_init(&repertory);
	if (ret) {
		printf("An error occurred while initializing the repertory\n");
		goto end;
	}

#if 1
	ret = gitt_repertory_update_head(&repertory);
	if (ret) {
		printf("An error occurred while update head\n");
		goto end;
	}
#endif

#if 0
	strcpy(repertory.head_sha1, "695b24a097fb97cbe1b863c6bd119dbaf10a52a0");
#endif

#if 0
	ret = gitt_repertory_pull(&repertory);
	if (ret) {
		printf("An error occurred while pull the repertory\n");
		goto end;
	}
#endif

#if 0
	ret = gitt_repertory_clone(&repertory);
	if (ret) {
		printf("An error occurred while clone the repertory\n");
		goto end;
	}
#endif

#if 1
	{
		struct gitt_commit commit = {0};

		/* Initialize commit */
		commit.tree.sha1       = GITT_TREE_EMPTY_SHA1;
		commit.parent.sha1     = GITT_COMMIT_AUTO_BASE;
		commit.author.date     = "1700987156";
		commit.author.email    = "huxiangjs1@foxmail.com";
		commit.author.name     = "Hoozz1";
		commit.author.zone     = "+0800";
		commit.committer.date  = "1700987155";
		commit.committer.email = "huxiangjs2@foxmail.com";
		commit.committer.name  = "Hoozz2";
		commit.committer.zone  = "+0800";
		commit.message         = "Init for test\n"
					 "\n"
					 "Behind every successful man there s a lot of unsuccessful years.\n"
					 "I think success has no rules, but you can learn a lot from failure.  \n"
					 "There are no secrets to success. It is the result of preparation, hard work, and learning from failure. \n"
					 "Few things are impossible in themselves; and it is often for want of will, rather than of means, that man fails to succeed.   \n"
					 "Failure is the mother of success.\n"
					 "You have to believe in yourself. That s the secret of success. \n"
					 "Four short words sum up what has lifted most successfulindividuals above  the crowd: a little bit more.  \n"
					 "Try not to become a man of success but rather try to become a man of value. \n"
					 "Victory won t come to me unless I go to it. \n"
					 "Industry is the parent of success.\n"
					 "The man who has made up his mind to win will never say \"impossible\".  \n"
					 "Genius only means hard-working all one s life.   \n"
					 "Great works are performed not by strengh, but by perseverance.   \n"
					 "I have nothing to offer but blood, boil, tears and sweat. \n"
					 "For man is man and master of his fate.   \n"
					 "There is no such thing as a great talent without great will .\n"
					 "A strong man will struggle with the storms of fate. \n"
					 "Cease to struggle and you cease to live.\n"
					 "A thousand-li journey is started by taking the first step.   \n"
					 "Constant dripping wears away the stone.\n"
					 "A young idler, an old beggar. \n"
					 "No pain, no gain.\n"
					 "There is no royal road to learning. \n"
					 "The first step is always the hardest. \n"
					 "Well begun is half done. \n"
					 "Nothing is impossible to a willing heart. \n"
					 "Action speaks louder than words. \n"
					 "Business may be troublesome, but idleness is pernicious. \n"
					 "Never think yourself above business. \n"
					 "Do business, but be not a slave to it. \n"
					 "They who cannot do as they would, must do as theycan. \n";

		ret = gitt_repertory_push_commit(&repertory, &commit);
		if (ret) {
			printf("An error occurred while push commit the repertory\n");
			goto end;
		}
	}
#endif

end:
	printf("Test end. Result: %s\n", GITT_ERRNO_STR(ret));

	free(buf);

	return 0;
}
