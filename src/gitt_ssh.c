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
#include <gitt_ssh.h>
#include <gitt_errno.h>

/* Please implement the following interfaces according to your system type */
struct gitt_ssh* gitt_ssh_alloc_impl(void);
void gitt_ssh_free_impl(struct gitt_ssh *ssh);
int gitt_ssh_connect_impl(struct gitt_ssh *ssh, struct gitt_ssh_url *ssh_url,
			  const char *exec, const char *privkey);
int gitt_ssh_read_impl(struct gitt_ssh *ssh, char *buf, int size);
int gitt_ssh_write_impl(struct gitt_ssh *ssh, char *buf, int size);
void gitt_ssh_disconnect_impl(struct gitt_ssh *ssh);

/**
 * @brief Pad in url information
 *
 * @param ssh_url
 * @param url
 * @return int 0: Good
 * @return int -1: Error
 */
static int gitt_ssh_url_pad(struct gitt_ssh_url *ssh_url, const char *url)
{
	int i, j;

	/* User */
	j = 0;
	i = 0;
	while (url[i] && url[i] != '@' && j < sizeof(ssh_url->user))
		ssh_url->user[j++] = url[i++];
	if (j == sizeof(ssh_url->user))
		return -GITT_ERRNO_INVAL;
	ssh_url->user[j] = '\0';
	i++;

	/* Host */
	j = 0;
	while (url[i] && url[i] != ':' && j < sizeof(ssh_url->host))
		ssh_url->host[j++] = url[i++];
	if (j == sizeof(ssh_url->host))
		return -GITT_ERRNO_INVAL;
	ssh_url->host[j] = '\0';
	i++;

	/* Repository */
	j = 0;
	while (url[i] && j < sizeof(ssh_url->repository))
		ssh_url->repository[j++] = url[i++];
	if (j == sizeof(ssh_url->repository))
		return -GITT_ERRNO_INVAL;
	ssh_url->repository[j] = '\0';
	i++;

	/* Port: fixed to 22*/
	strcpy(ssh_url->port, "22");

	return 0;
}

struct gitt_ssh* gitt_ssh_alloc(void)
{
	return gitt_ssh_alloc_impl();
}

void gitt_ssh_free(struct gitt_ssh *ssh)
{
	if (ssh)
		gitt_ssh_free_impl(ssh);
}

/**
 * @brief Connect to ssh
 *
 * @param ssh Handle
 * @param url The ssh address of the git repository.
 * 	      For example: "git@github.com:huxiangjs/git_things.git"
 * @param exec Commands to be executed.
 * 	       For example: "git-receive-pack"
 * @param privkey private key
 * @return int 0: Good
 * @return int -1: Error
 */
int gitt_ssh_connect(struct gitt_ssh *ssh, const char *url, const char *exec,
		     const char *privkey)
{
	struct gitt_ssh_url ssh_url;
	int err;
	char buffer[96];

	if (!ssh || !url || !exec || !privkey)
		return -GITT_ERRNO_INVAL;

	err = gitt_ssh_url_pad(&ssh_url, url);
	if (err)
		return err;
	gitt_log_debug("User: %s\n", ssh_url.user);
	gitt_log_debug("Host: %s\n", ssh_url.host);
	gitt_log_debug("Port: %s\n", ssh_url.port);
	gitt_log_debug("Repository: %s\n", ssh_url.repository);

	snprintf(buffer, sizeof(buffer), "%s '%s'", exec, ssh_url.repository);

	return gitt_ssh_connect_impl(ssh, &ssh_url, buffer, privkey);
}

int gitt_ssh_read(struct gitt_ssh *ssh, char *buf, int size)
{
	if (!ssh)
		return -GITT_ERRNO_INVAL;

	return gitt_ssh_read_impl(ssh, buf, size);
}

int gitt_ssh_write(struct gitt_ssh *ssh, char *buf, int size)
{
	if (!ssh)
		return -GITT_ERRNO_INVAL;

	return gitt_ssh_write_impl(ssh, buf, size);
}

void gitt_ssh_disconnect(struct gitt_ssh *ssh)
{
	if (!ssh)
		return;

	gitt_ssh_disconnect_impl(ssh);
}
