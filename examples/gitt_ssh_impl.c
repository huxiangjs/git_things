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
#include <stdlib.h>
#include <libssh/libssh.h>
#include <gitt_ssh.h>
#include <gitt_errno.h>

struct gitt_ssh {
	ssh_session session;
	ssh_key privkey;
	ssh_channel channel;
};

struct gitt_ssh* gitt_ssh_alloc_impl(void)
{
	return (struct gitt_ssh *)malloc(sizeof(struct gitt_ssh));
}

void gitt_ssh_free_impl(struct gitt_ssh *ssh)
{
	free(ssh);
}

int gitt_ssh_connect_impl(struct gitt_ssh *ssh, struct gitt_ssh_url *ssh_url,
			  const char *exec, const char *privkey)
{
	int err;

	/* Open session and set options */
	ssh->session = ssh_new();
	if (ssh->session == NULL)
		goto out0;
	ssh_options_set(ssh->session, SSH_OPTIONS_USER, ssh_url->user);
	ssh_options_set(ssh->session, SSH_OPTIONS_HOST, ssh_url->host);
	ssh_options_set(ssh->session, SSH_OPTIONS_PORT_STR, ssh_url->port);
	// ssh_options_set(ssh->session, SSH_OPTIONS_LOG_VERBOSITY_STR, "3");

	/* Connect to server */
	err = ssh_connect(ssh->session);
	if (err != SSH_OK) {
		fprintf(stderr, "Error connecting to %s: %s\n",
			ssh_url->host, ssh_get_error(ssh->session));
		goto out1;
	}

	/*
	 * Import the private key. The caller (see main.c) reads the PEM key
	 * file into memory and passes its content as 'privkey'; libssh
	 * accepts the PEM-wrapped base64 in this call.
	 */
	err = ssh_pki_import_privkey_base64(privkey, NULL, NULL, NULL, &ssh->privkey);
	if (err != SSH_OK) {
		fprintf(stderr, "Error importing ed25519 private key: %s\n",
			ssh_get_error(ssh->session));
		goto out2;
	}

	/* Authenticate with the ed25519 private key */
	err = ssh_userauth_publickey(ssh->session, NULL, ssh->privkey);
	if (err != SSH_AUTH_SUCCESS) {
		fprintf(stderr, "Error authenticating with ed25519 private key: %s\n",
			ssh_get_error(ssh->session));
		goto out3;
	}

	/* Open channel */
	ssh->channel = ssh_channel_new(ssh->session);
	if (!ssh->channel)
		goto out3;
	err = ssh_channel_open_session(ssh->channel);
	if (err != SSH_OK)
		goto out4;
	err = ssh_channel_request_exec(ssh->channel, exec);
	if (err != SSH_OK) {
		fprintf(stderr, "Error executing '%s': %s\n", exec,
			ssh_get_error(ssh->session));
		goto out4;
	}

	return 0;

out4:
	ssh_channel_free(ssh->channel);
	ssh->channel = NULL;
out3:
	ssh_key_free(ssh->privkey);
out2:
	ssh_disconnect(ssh->session);
out1:
	ssh_free(ssh->session);
out0:
	return -GITT_ERRNO_INVAL;
}

/*
 * Optional: capture the raw protocol into the file given by the
 * GITT_CAPTURE environment variable. The file is opened once per
 * session (truncated on the first open) instead of being opened and
 * closed for every packet.
 */
static FILE *gitt_capture;

static FILE *gitt_capture_file(void)
{
	const char *path;

	if (gitt_capture)
		return gitt_capture;

	path = getenv("GITT_CAPTURE");
	if (!path)
		return NULL;

	gitt_capture = fopen(path, "wb");
	return gitt_capture;
}

int gitt_ssh_write_impl(struct gitt_ssh *ssh, char *buf, int size)
{
	FILE *cap = gitt_capture_file();

	if (cap)
		fwrite(buf, 1, size, cap);

	return ssh_channel_write(ssh->channel, buf, size);
}

int gitt_ssh_read_impl(struct gitt_ssh *ssh, char *buf, int size)
{
	int ret = ssh_channel_read(ssh->channel, buf, size, 0);
	FILE *cap;

	if (ret > 0 && (cap = gitt_capture_file()))
		fwrite(buf, 1, ret, cap);

	return ret;
}

void gitt_ssh_disconnect_impl(struct gitt_ssh *ssh)
{
	/* Free and disconnect */
	if (gitt_capture) {
		fclose(gitt_capture);
		gitt_capture = NULL;
	}
	ssh_channel_close(ssh->channel);
	ssh_disconnect(ssh->session);
	ssh_key_free(ssh->privkey);
	ssh_free(ssh->session);
}
