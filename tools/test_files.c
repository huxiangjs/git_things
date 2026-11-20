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

/*
 * File round-trip test (Route B).
 *
 * A directory tree is pushed to a local git server with gitt_commit_files
 * (files become real blob/tree objects) and pulled back with
 * gitt_pull; the restored tree must be byte-for-byte identical to
 * the source. The repository is also cross-checked with real git
 * (fsck, log, clone), so the packs are verified to be valid git objects.
 *
 * Requirements: a local sshd (default port 22), git in PATH and a usable
 * private key (default: $HOME/.ssh/id_ed25519). The test creates and
 * destroys a bare repository in a temporary directory by itself.
 *
 * Usage:
 *   ./test_files [privkey]
 *
 * Exit code: 0 if all checks pass, 1 otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <gitt.h>
#include <gitt_errno.h>

#define TEST_MAX_FILES		16
#define TEST_PATH_SIZE		128
#define TEST_BUF_SIZE		4096

/* The file restored by a pull */
static char test_out_dir[256];

/*
 * One client (the same shape as examples/main.c)
 */
struct test_client {
	struct gitt g;
	char url[160];
	char privkey[2048];
	uint8_t buffer[TEST_BUF_SIZE];
};

static struct test_client client;

static int test_passed;
static int test_failed;

static void check(const char *name, int ok)
{
	printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
	if (ok)
		test_passed++;
	else
		test_failed++;
}

/* Date/zone for the commits */
static int get_date(char *buf, uint8_t size)
{
	time_t t = time(NULL);

	if (t == -1)
		return -1;
	if (snprintf(buf, size, "%ld", t) >= size)
		return -1;
	return 0;
}

static int get_zone(char *buf, uint8_t size)
{
	int zone = (int)(timezone / -60);

	if (snprintf(buf, size, "%c%02d%02d",
			zone < 0 ? '-' : '+', abs(zone) / 60, abs(zone) % 60)
			>= size)
		return -1;
	return 0;
}

/* Create the parent directories of a path (ignore existing ones) */
static void mkdir_parent(char *path)
{
	char *slash;

	for (slash = path + 1; *slash; slash++) {
		if (*slash == '/') {
			*slash = '\0';
			mkdir(path, 0755);
			*slash = '/';
		}
	}
}

static void client_file_dump(struct gitt *g, char *path,
			     uint8_t *data, uint32_t size)
{
	char full[256];
	FILE *file;

	(void)g;

	snprintf(full, sizeof(full), "%s/%s", test_out_dir, path);
	mkdir_parent(full);

	file = fopen(full, "wb");
	if (!file) {
		fprintf(stderr, "Error writing file: %s\n", full);
		return;
	}
	if (fwrite(data, 1, size, file) != size)
		fprintf(stderr, "Error writing file: %s\n", full);
	fclose(file);
}

/*
 * File system helpers
 */

static void write_file(const char *root, const char *rel,
		       const uint8_t *data, uint32_t size)
{
	char path[256];
	FILE *file;

	snprintf(path, sizeof(path), "%s/%s", root, rel);
	mkdir_parent(path);

	file = fopen(path, "wb");
	if (!file) {
		fprintf(stderr, "Error creating file: %s\n", path);
		exit(1);
	}
	if (data && fwrite(data, 1, size, file) != size)
		fprintf(stderr, "Error writing file: %s\n", path);
	fclose(file);
}

static void delete_file(const char *root, const char *rel)
{
	char path[256];

	snprintf(path, sizeof(path), "%s/%s", root, rel);
	unlink(path);
}

struct file_entry {
	char path[TEST_PATH_SIZE];
};

/* Collect the relative paths of all regular files below root */
static int list_files_rec(const char *root, const char *prefix,
			  struct file_entry *list, uint16_t *count,
			  uint16_t max)
{
	DIR *dir;
	struct dirent *ent;
	int ret = 0;

	dir = opendir(root);
	if (!dir)
		return -1;

	while ((ent = readdir(dir))) {
		char froot[256];
		char fprefix[TEST_PATH_SIZE];
		char fpath[TEST_PATH_SIZE];
		struct stat st;

		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")
		    || !strcmp(ent->d_name, ".git"))
			continue;

		snprintf(froot, sizeof(froot), "%s/%s", root, ent->d_name);
		if (stat(froot, &st))
			continue;

		if (S_ISDIR(st.st_mode)) {
			snprintf(fprefix, sizeof(fprefix), "%s%s%s", prefix,
				 prefix[0] ? "/" : "", ent->d_name);
			ret = list_files_rec(froot, fprefix, list, count, max);
			if (ret)
				break;
		} else if (S_ISREG(st.st_mode)) {
			if (*count >= max) {
				ret = -1;
				break;
			}
			snprintf(fpath, sizeof(fpath), "%s%s%s", prefix,
				 prefix[0] ? "/" : "", ent->d_name);
			snprintf(list[*count].path, sizeof(list[*count].path),
				 "%s", fpath);
			(*count)++;
		}
	}

	closedir(dir);
	return ret;
}

static int list_files(const char *root, struct file_entry *list,
		      uint16_t *count)
{
	int ret;

	*count = 0;
	ret = list_files_rec(root, "", list, count, 64);
	if (ret)
		return ret;

	/* Sort (strcmp order) so that two trees can be compared directly */
	for (uint16_t i = 1; i < *count; i++) {
		struct file_entry key = list[i];
		uint16_t j = i;

		while (j && strcmp(list[j-1].path, key.path) > 0) {
			list[j] = list[j-1];
			j--;
		}
		list[j] = key;
	}

	return 0;
}

static int files_identical(const char *a, const char *b)
{
	FILE *fa = fopen(a, "rb");
	FILE *fb = fopen(b, "rb");
	uint8_t ba[TEST_BUF_SIZE];
	uint8_t bb[TEST_BUF_SIZE];
	size_t n;

	if (!fa || !fb)
		return 0;

	while (1) {
		n = fread(ba, 1, sizeof(ba), fa);
		if (fread(bb, 1, n, fb) != n) {
			fclose(fa);
			fclose(fb);
			return 0;
		}
		if (!n)
			break;
		if (memcmp(ba, bb, n)) {
			fclose(fa);
			fclose(fb);
			return 0;
		}
	}

	fclose(fa);
	fclose(fb);
	return 1;
}

/* Compare two file trees: identical file sets, identical content */
static int trees_identical(const char *a, const char *b)
{
	struct file_entry la[64];
	struct file_entry lb[64];
	uint16_t na, nb;
	uint16_t i;

	if (list_files(a, la, &na) || list_files(b, lb, &nb))
		return 0;

	if (na != nb) {
		printf("  file count differs: %s has %u, %s has %u\n",
		       a, na, b, nb);
		for (i = 0; i < na; i++)
			printf("    a: %s\n", la[i].path);
		for (i = 0; i < nb; i++)
			printf("    b: %s\n", lb[i].path);
		return 0;
	}

	for (i = 0; i < na; i++) {
		char pa[256];
		char pb[256];

		if (strcmp(la[i].path, lb[i].path)) {
			printf("  file set differs: '%s' vs '%s'\n",
			       la[i].path, lb[i].path);
			return 0;
		}
		snprintf(pa, sizeof(pa), "%s/%s", a, la[i].path);
		snprintf(pb, sizeof(pb), "%s/%s", b, la[i].path);
		if (!files_identical(pa, pb)) {
			printf("  content differs: %s\n", la[i].path);
			return 0;
		}
	}

	return 1;
}

static uint16_t count_files(const char *root)
{
	struct file_entry list[64];
	uint16_t count;

	if (list_files(root, list, &count))
		return 0;
	return count;
}

/*
 * Load the files of a directory (for a push)
 */
static int load_dir(const char *root, const char *prefix,
		    struct gitt_file *files, uint8_t *count)
{
	DIR *dir;
	struct dirent *ent;
	int ret = 0;

	dir = opendir(root);
	if (!dir)
		return -1;

	while ((ent = readdir(dir))) {
		char froot[256];
		char fprefix[TEST_PATH_SIZE];
		struct stat st;
		FILE *file;
		uint8_t *data;
		uint32_t size;

		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		snprintf(froot, sizeof(froot), "%s/%s", root, ent->d_name);
		if (stat(froot, &st))
			continue;

		if (S_ISDIR(st.st_mode)) {
			snprintf(fprefix, sizeof(fprefix), "%s%s%s", prefix,
				 prefix[0] ? "/" : "", ent->d_name);
			ret = load_dir(froot, fprefix, files, count);
			if (ret)
				break;
		} else if (S_ISREG(st.st_mode)) {
			if (*count >= GITT_MAX_FILES) {
				ret = -1;
				break;
			}

			file = fopen(froot, "rb");
			if (!file) {
				ret = -1;
				break;
			}
			data = malloc(st.st_size ? (size_t)st.st_size : 1);
			if (!data) {
				fclose(file);
				ret = -1;
				break;
			}
			size = (uint32_t)fread(data, 1, st.st_size, file);
			fclose(file);
			if (size != (uint32_t)st.st_size) {
				free(data);
				ret = -1;
				break;
			}

			snprintf(files[*count].path,
				 sizeof(files[*count].path), "%s%s%s", prefix,
				 prefix[0] ? "/" : "", ent->d_name);
			files[*count].data = data;
			files[*count].size = (uint16_t)size;
			(*count)++;
		}
	}

	closedir(dir);
	return ret;
}

/*
 * Cross-checks with real git
 */

static int run_cmd(const char *cmd)
{
	int ret = system(cmd);

	if (ret == -1)
		return -1;
	return WEXITSTATUS(ret);
}

/*
 * Check that git fsck reports no broken references. 'dangling commit'
 * lines are tolerated: message-only commits carry no parent, which
 * orphans the previous history.
 */
static int git_fsck_no_missing(const char *repo, const char *workdir)
{
	char cmd[512];
	char out[256];
	FILE *file;
	char line[256];
	int missing = 0;
	int shown = 0;

	snprintf(out, sizeof(out), "%s/fsck_out.txt", workdir);
	snprintf(cmd, sizeof(cmd), "git -C %s fsck > %s 2>&1", repo, out);
	if (run_cmd(cmd))
		return 0;

	file = fopen(out, "r");
	if (!file)
		return 1;
	while (fgets(line, sizeof(line), file)) {
		if (!strncmp(line, "missing", 7))
			missing = 1;
		if (line[0] != '\n') {
			printf("  git fsck: %s", line);
			shown = 1;
		}
	}
	fclose(file);

	if (shown && !missing)
		printf("  (tolerated: no broken references)\n");

	return !missing;
}

static int git_commit_count(const char *repo)
{
	char cmd[512];
	FILE *pipe;
	char line[128];
	int count = 0;

	snprintf(cmd, sizeof(cmd), "git -C %s log --oneline", repo);
	pipe = popen(cmd, "r");
	if (!pipe)
		return -1;

	while (fgets(line, sizeof(line), pipe))
		count++;

	pclose(pipe);
	return count;
}

/*
 * The source tree
 */

/*
 * Round 1: five files, three directory levels, a 4095 byte binary
 * (the largest object the 4096 byte buffer can hold) and an empty file.
 */
static void build_tree_round1(const char *src)
{
	uint8_t bin[4095];
	uint32_t i;

	for (i = 0; i < sizeof(bin); i++)
		bin[i] = (uint8_t)((i * 31 + i / 13) % 256);

	write_file(src, "top.txt", (uint8_t *)"top file, round 1\n", 18);
	write_file(src, "a/b.txt", (uint8_t *)"file in a\n", 10);
	write_file(src, "a/b/c/deep.txt", (uint8_t *)"deep file\n", 10);
	write_file(src, "a/big.bin", bin, sizeof(bin));
	write_file(src, "empty.txt", NULL, 0);
}

/* Round 2: modify one file, add one, delete one */
static void build_tree_round2(const char *src)
{
	write_file(src, "top.txt", (uint8_t *)"top file, round 2, modified\n", 28);
	write_file(src, "a/b/c/new.txt", (uint8_t *)"a new file\n", 11);
	delete_file(src, "empty.txt");
}

/*
 * gitt_end clears every field of the struct, so a new session needs
 * all the settings applied again.
 */
static int client_reconnect(void)
{
	gitt_end(&client.g);

	client.g.url = client.url;
	client.g.privkey = client.privkey;
	client.g.buf = client.buffer;
	client.g.buf_len = sizeof(client.buffer);
	client.g.get_date = get_date;
	client.g.get_zone = get_zone;
	client.g.shallow = 1;
	client.g.file_dump = client_file_dump;

	return gitt_init(&client.g);
}

int main(int argc, char *argv[])
{
	struct gitt_file files[TEST_MAX_FILES];
	uint8_t count = 0;
	char key_path[256];
	char workdir[64];
	char src[80];
	char repo[80];
	char out[128];
	char cmd[512];
	char tmpl[64];
	struct passwd *pw;
	FILE *file;
	int ret;

	/* Private key */
	pw = getpwuid(getuid());
	if (!pw) {
		fprintf(stderr, "Cannot get user information\n");
		return 1;
	}
	if (argc >= 2)
		snprintf(key_path, sizeof(key_path), "%s", argv[1]);
	else
		snprintf(key_path, sizeof(key_path), "%s/.ssh/id_ed25519",
			 pw->pw_dir);

	file = fopen(key_path, "rb");
	if (!file) {
		fprintf(stderr, "Cannot open the private key: %s\n", key_path);
		return 1;
	}
	ret = (int)fread(client.privkey, 1, sizeof(client.privkey) - 1, file);
	fclose(file);
	if (ret <= 0) {
		fprintf(stderr, "Cannot read the private key\n");
		return 1;
	}
	client.privkey[ret] = '\0';

	/* A fresh temporary workspace with a fresh bare repository */
	snprintf(tmpl, sizeof(tmpl), "/tmp/gitt_files_test_XXXXXX");
	workdir[0] = '\0';
	if (!mkdtemp(tmpl)) {
		fprintf(stderr, "Cannot create the work directory\n");
		return 1;
	}
	snprintf(workdir, sizeof(workdir), "%s", tmpl);
	snprintf(src, sizeof(src), "%s/src", workdir);
	snprintf(repo, sizeof(repo), "%s/repo.git", workdir);

	if (run_cmd("git --version")) {
		fprintf(stderr, "git is not available\n");
		return 1;
	}
	snprintf(cmd, sizeof(cmd), "git init -q --bare -b master %s", repo);
	ret = run_cmd(cmd);
	if (ret) {
		fprintf(stderr, "Cannot initialize the bare repository\n");
		return 1;
	}

	printf("Repository: %s\n", repo);
	printf("Key:        %s\n", key_path);
	printf("Workdir:    %s\n\n", workdir);

	/* Connect */
	snprintf(client.url, sizeof(client.url), "%s@localhost:%s",
		 pw->pw_name, repo);
	client.g.url = client.url;
	client.g.privkey = client.privkey;
	client.g.buf = client.buffer;
	client.g.buf_len = sizeof(client.buffer);
	client.g.get_date = get_date;
	client.g.get_zone = get_zone;
	client.g.shallow = 1;

	ret = gitt_init(&client.g);
	if (ret) {
		fprintf(stderr, "gitt_init failed: %s\n", GITT_ERRNO_STR(ret));
		return 1;
	}

	/* 1. Pulling an empty repository must succeed with zero files */
	snprintf(out, sizeof(out), "%s/out_empty", workdir);
	mkdir(out, 0755);
	test_out_dir[0] = '\0';
	snprintf(test_out_dir, sizeof(test_out_dir), "%s", out);
	client.g.file_dump = client_file_dump;
	ret = gitt_pull(&client.g);
	check("pull an empty repository",
	      !ret && count_files(out) == 0);

	/* 2. Round 1: push a fresh tree to the empty repository */
	build_tree_round1(src);
	count = 0;
	load_dir(src, "", files, &count);
	ret = gitt_commit_files(&client.g, files, count, "round 1");
	for (int i = 0; i < count; i++)
		free(files[i].data);
	check("push round 1 (5 files, 3 levels, 4095B binary, empty file)",
	      !ret);
	check("server: 1 commit, git fsck clean",
	      git_commit_count(repo) == 1 && git_fsck_no_missing(repo, workdir));

	/*
	 * 3. A fresh full (non-shallow) pull restores the tree
	 *    byte-for-byte. The push above left this session with the
	 *    head already up to date, so a reconnection is needed for
	 *    the pull to actually fetch.
	 */
	int rc = client_reconnect();
	client.g.shallow = 0;
	snprintf(out, sizeof(out), "%s/out_full", workdir);
	mkdir(out, 0755);
	snprintf(test_out_dir, sizeof(test_out_dir), "%s", out);
	ret = gitt_pull(&client.g);
	check("full pull round 1 == source (byte-for-byte)",
	      !rc && !ret && trees_identical(src, out));

	/* 4. Shallow pull restores the tree byte-for-byte */
	client.g.shallow = 1;
	snprintf(out, sizeof(out), "%s/out_shallow", workdir);
	mkdir(out, 0755);
	snprintf(test_out_dir, sizeof(test_out_dir), "%s", out);
	ret = gitt_pull(&client.g);
	check("shallow pull round 1 == source (byte-for-byte)",
	      !ret && trees_identical(src, out));

	/* 5. A real git clone reproduces the pushed tree */
	snprintf(out, sizeof(out), "%s/clone", workdir);
	snprintf(cmd, sizeof(cmd), "git clone -q %s %s", repo, out);
	run_cmd(cmd);
	check("real git clone == source (byte-for-byte)",
	      trees_identical(src, out));

	/* 6. Round 2: modify + add + delete, push with a real parent */
	build_tree_round2(src);
	count = 0;
	load_dir(src, "", files, &count);
	ret = gitt_commit_files(&client.g, files, count, "round 2");
	for (int i = 0; i < count; i++)
		free(files[i].data);
	check("push round 2 (modify + add + delete)", !ret);
	check("server: 2 commits, git fsck clean",
	      git_commit_count(repo) == 2 && git_fsck_no_missing(repo, workdir));

	/* 7. Shallow pull of the modified tree */
	snprintf(out, sizeof(out), "%s/out_round2", workdir);
	mkdir(out, 0755);
	snprintf(test_out_dir, sizeof(test_out_dir), "%s", out);
	ret = gitt_pull(&client.g);
	check("shallow pull round 2 == source (byte-for-byte)",
	      !ret && trees_identical(src, out));

	/*
	 * 8. A message-only commit references the empty tree; the empty
	 *    tree object travels with the pack, so the remote repository
	 *    must stay intact (git fsck: no 'missing tree').
	 *
	 *    Note: message-only commits carry no parent
	 *    (GITT_COMMIT_NO_BASE), so after such a commit 'git log'
	 *    from HEAD shows only the message commit and the previous
	 *    history is left dangling. This check asserts that the push
	 *    succeeds and the repository stays intact.
	 */
	ret = gitt_push(&client.g, "an event");
	check("message-only commit, git fsck clean (no dangling empty tree)",
	      !ret && git_fsck_no_missing(repo, workdir));

	/*
	 * 9. Current semantics: after a message-only commit the tip tree
	 *    is empty, so a pull restores zero files.
	 */
	snprintf(out, sizeof(out), "%s/out_event", workdir);
	mkdir(out, 0755);
	snprintf(test_out_dir, sizeof(test_out_dir), "%s", out);
	ret = gitt_pull(&client.g);
	check("pull after a message-only commit restores the (empty) tip tree",
	      !ret && count_files(out) == 0);

	/* 10. History of the whole session */
	ret = gitt_history(&client.g);
	check("history", !ret);

	gitt_end(&client.g);

	/* Cleanup */
	snprintf(cmd, sizeof(cmd), "rm -rf %s", workdir);
	run_cmd(cmd);

	printf("\n%d passed, %d failed\n", test_passed, test_failed);
	printf("%s\n", test_failed ? "TEST FAIL" : "TEST PASS");

	return test_failed ? 1 : 0;
}
