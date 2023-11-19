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
#include <gitt_ssh.h>

#define PRIVKEY_PATH	"/home/huxiang/.ssh/id_ed25519"

int main(int args, char *argv[])
{
	struct gitt_ssh* ssh = gitt_ssh_alloc();
	int err;
	char buffer[4096];
	FILE *file;

	if (!ssh)
		return -1;

	/* Read key file */
	file = fopen(PRIVKEY_PATH, "rb");
	if (!file)
		goto out0;
	err = fread(buffer, 1, sizeof(buffer) - 1, file);
	if (err <= 0) {
		fclose(file);
		return -1;
	}
	buffer[err] = '\0';
	printf("%s\n", buffer);
	fclose(file);

	/* Connect */
	err = gitt_ssh_connect(ssh, "git@github.com:huxiangjs/git_things.git",
			       "git-receive-pack", buffer);
	if (err)
		goto out0;

	/* Read */
	err = gitt_ssh_read(ssh, buffer, sizeof(buffer));
	// while (err > 0) {
	if (err > 0) {
		printf("%.*s\n", err, buffer);
		// err = ssh_channel_read(ssh->channel, buffer, sizeof(buffer), 0);
	}

	gitt_ssh_disconnect(ssh);

	printf("Done\n");

	return 0;

out0:
	gitt_ssh_free(ssh);
	return -1;
}
