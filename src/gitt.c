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
#include <gitt_log.h>
#include <gitt.h>
#include <gitt_type.h>
#include <gitt_commit.h>
#include <gitt_tree.h>
#include <gitt_errno.h>

#define GITT_DEFAULT_DATE		"0"
#define GITT_DEFAULT_ZONE		"+0000"
#define GITT_COMMITTER_EMAIL		"https://github.com/huxiangjs/git_things.git"
#define GITT_COMMITTER_NAME		"GITT"

#define GITT_UNKNOWN_EMAIL		"UNKNOWN"
#define GITT_UNKNOWN_NEME		"UNKNOWN"

#define GITT_EMAIL_FORMAT		"%s@%s.GITT"

#define GITT_TRY_NUMBER			5

static void gitt_repertory_commit_dump(struct gitt_repertory *repertory,
				       struct gitt_commit *commit)
{
	struct gitt *g = gitt_containerof(repertory, struct gitt, repertory);
	struct gitt_device device;

	/* Debug output */
	gitt_log_debug("id.sha1        : %s\n", commit->id.sha1        );
	gitt_log_debug("tree.sha1      : %s\n", commit->tree.sha1      );
	gitt_log_debug("parent.sha1    : %s\n", commit->parent.sha1    );
	gitt_log_debug("author.date    : %s\n", commit->author.date    );
	gitt_log_debug("author.email   : %s\n", commit->author.email   );
	gitt_log_debug("author.name    : %s\n", commit->author.name    );
	gitt_log_debug("author.zone    : %s\n", commit->author.zone    );
	gitt_log_debug("committer.date : %s\n", commit->committer.date );
	gitt_log_debug("committer.email: %s\n", commit->committer.email);
	gitt_log_debug("committer.name : %s\n", commit->committer.name );
	gitt_log_debug("committer.zone : %s\n", commit->committer.zone );
	gitt_log_debug("message        : ");
	gitt_log_debug("%s\n", commit->message);
	gitt_log_debug("\n");

	/* Fill name */
	if (strlen(commit->author.name) > GITT_DEVICE_NAME_SIZE - 1) {
		gitt_log_error("Name is too long\n");
		return;
	}
	strcpy(device.name, commit->author.name);

	/* Fill id */
	if (strlen(commit->author.email) > GITT_DEVICE_ID_SIZE - 1) {
		gitt_log_error("Email is too long\n");
		return;
	}
	strcpy(device.id, commit->author.email);

	/* Callback */
	if (g->remote_event) {
		g->remote_event(g, &device, commit->author.date,
				commit->author.zone, commit->message);
	}
}

int gitt_init(struct gitt *g)
{
	int ret;

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	g->repertory.privkey = g->privkey;
	g->repertory.url = g->url;
	g->repertory.buf = g->buf;
	g->repertory.buf_len = g->buf_len;
	g->repertory.commit_dump = gitt_repertory_commit_dump;

	ret = gitt_repertory_init(&g->repertory);
	if (ret) {
		gitt_log_error("Initializing the repertory fail\n");
		goto err0;
	}

	ret = gitt_repertory_update_head(&g->repertory);
	if (ret) {
		gitt_log_error("Update head fail\n");
		goto err1;
	}

	return 0;

err1:
	gitt_repertory_end(&g->repertory);
err0:
	return ret;
}

int gitt_update_event(struct gitt *g)
{
	int ret;

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	ret = gitt_repertory_pull(&g->repertory);
	if (ret) {
		gitt_log_error("Pull the repertory fail\n");
		return ret;
	}

	return 0;
}

int gitt_commit_event(struct gitt *g, char *data)
{
	int ret;
	int count;
	struct gitt_commit commit = {0};
	char date[16];
	char zone[8];
	char id[GITT_DEVICE_ID_SIZE + 10];

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	if (data == NULL || !strlen(data)) {
		gitt_log_error("Date cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	/* Initialize commit */
	commit.tree.sha1       = GITT_TREE_EMPTY_SHA1;
	commit.parent.sha1     = GITT_COMMIT_AUTO_BASE;

	/* Fill date */
	if (g->get_date && !g->get_date(date, sizeof(date))) {
		commit.author.date     = date;
		commit.committer.date  = date;
	} else {
		commit.author.date     = GITT_DEFAULT_DATE;
		commit.committer.date  = GITT_DEFAULT_DATE;
	}

	/* Fill zone */
	if (g->get_zone && !g->get_zone(zone, sizeof(zone))) {
		commit.author.zone     = zone;
		commit.committer.zone  = zone;
	} else {
		commit.author.zone     = GITT_DEFAULT_ZONE;
		commit.committer.zone  = GITT_DEFAULT_ZONE;
	}

	/* Fill name and email */
	if (strlen(g->device.id)) {
		sprintf(id, GITT_EMAIL_FORMAT, g->device.id, GITT_VERSION);
		commit.author.email    = id;
	} else {
		commit.author.email    = GITT_UNKNOWN_EMAIL;
	}
	commit.author.name     = strlen(g->device.name) ? g->device.name : GITT_UNKNOWN_NEME;
	commit.committer.email = GITT_COMMITTER_EMAIL;
	commit.committer.name  = GITT_COMMITTER_NAME;

	commit.message         = data;

	/* Try to commit */
	count = 0;
	do {
		ret = gitt_repertory_push_commit(&g->repertory, &commit);
		count++;

		if (ret == -GITT_ERRNO_RETRY) {
			gitt_log_info("Retry count: %d\n", count);

			/* Update */
			ret = gitt_update_event(g);
			if (ret)
				return ret;
		} else {
			gitt_log_debug("End code: %d\n", ret);
			break;
		}
	} while (count < GITT_TRY_NUMBER);

	return ret;
}

int gitt_history(struct gitt *g)
{
	int ret;

	if (g == NULL) {
		gitt_log_error("Pointer cannot be null\n");
		return -GITT_ERRNO_INVAL;
	}

	ret = gitt_repertory_clone(&g->repertory);
	if (ret)
		return ret;

	return 0;
}

void gitt_end(struct gitt *g)
{
	gitt_repertory_end(&g->repertory);

	g->get_date = NULL;
	g->get_zone = NULL;
	g->remote_event = NULL;
	g->url = NULL;
	g->privkey = NULL;
	g->buf = NULL;
	g->buf_len = 0;
}

char *gitt_version(void)
{
	return GITT_VERSION;
}
