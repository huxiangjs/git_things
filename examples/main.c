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
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pwd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gitt.h>
#include <gitt_type.h>
#include <gitt_errno.h>
#include <time.h>
#include "gitt_config.h"

#define DEFAULT_PRIVKEY_PATH		"id_ed25519"

#define EXAMPLE_DEVICE_NAME		"Example Device"
#define EXAMPLE_DEVICE_ID		"0000000000000000"

#define EXAMPLE_STATE_INIT		0
#define EXAMPLE_STATE_RUN		1

#define DEFAULT_LOOP_TIME		5

/*
 * The user buffer. It doubles as the pack/unpack work area, so a
 * single object must fit in it: the largest file GITT can pull back
 * is GITT_CONFIG_MAX_OBJECT bytes. Sized in gitt_config.h.
 */

struct gitt_example {
	struct gitt g;
	char *home;
	char privkey[2048];
	char repository[64];
	char out_dir[128];
	uint8_t buffer[GITT_CONFIG_BUF_SIZE];
	int state;
};

typedef int (*cmd_func)(struct gitt_example *example, int args, char *argv[]);

struct cmd_item {
	char *name;
	cmd_func func;
	char *help;
};

/* Get user home */
static int get_home(struct gitt_example *example)
{
	uid_t uid = getuid();
	struct passwd *pw = getpwuid(uid);

	if (pw == NULL) {
		fprintf(stderr, "Cannot get user information\n");
		return -1;
	}
	printf("Home directory: %s\n", pw->pw_dir);
	example->home = pw->pw_dir;

	return 0;
}

static int print_warning(struct gitt_example *example)
{
	char buf[64] = {0};
	int ret = -1;

	printf("\n!! Note: Initializing will connect to the repository below and\n");
	printf("sync the remote head. The remote data will not be modified.\n");
	printf("Repository: %s\n", example->repository);
	printf("Please confirm whether to continue? (no/yes): ");

	if (scanf("%63s", buf) == 1 && !strcmp(buf, "yes"))
		ret = 0;

	/* Consume the rest of the line */
	{
		int c;
		while ((c = getchar()) != EOF && c != '\n');
	}

	printf("\n");

	return ret;
}

static void print_info(void)
{
	const char *license =
		"\n"
		"MIT License\n"
		"\n"
		"Copyright (c) 2023 Hoozz <huxiangjs@foxmail.com>\n"
		"\n"
		"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
		"of this software and associated documentation files (the \"Software\"), to deal\n"
		"in the Software without restriction, including without limitation the rights\n"
		"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
		"copies of the Software, and to permit persons to whom the Software is\n"
		"furnished to do so, subject to the following conditions:\n"
		"\n"
		"The above copyright notice and this permission notice shall be included in all\n"
		"copies or substantial portions of the Software.\n"
		"\n"
		"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
		"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
		"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
		"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
		"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
		"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
		"SOFTWARE.\n";

	puts(license);
	printf("GITT Version: %s\n", gitt_version());
}

static int gitt_get_date_impl(char *buf, uint8_t size)
{
	time_t cur_time;

	cur_time = time(NULL);
	if (cur_time == -1)
		return -1;

	if (snprintf(buf, size, "%ld", cur_time) >= size)
		return -1;

	return 0;
}

static int gitt_get_zone_impl(char *buf, uint8_t size)
{
	int h, m;
	char d;
	long zone;

	tzset();

	zone = timezone / -60;
	d = zone < 0 ? '-' : '+';
	h = zone / 60;
	m = zone % 60;

	if (snprintf(buf, size, "%c%02d%02d", d, h, m) >= size)
		return -1;

	return 0;
}

static int cmd_func_init(struct gitt_example *example, int args, char *argv[])
{
	int ret = 0;
	FILE *file;
	char privkey_path[256];

	/* A repository url is required */
	if (args < 2) {
		fprintf(stderr, "Invalid: Please enter the repository URL\n");
		return -1;
	}
	if (strlen(argv[1]) + 1 > sizeof(example->repository)) {
		fprintf(stderr, "Invalid: Repository URL too long\n");
		return -1;
	}
	strcpy(example->repository, argv[1]);
	printf("Repository URL: %s\n", argv[1]);

	if (args >= 3) {
		if (strlen(argv[2]) + 1 > sizeof(privkey_path)) {
			fprintf(stderr, "Privkey path too long\n");
			return -1;
		}
		strcpy(privkey_path, argv[2]);
	} else {
		if (snprintf(privkey_path, sizeof(privkey_path), "%s/.ssh/%s",
				example->home, DEFAULT_PRIVKEY_PATH) >= sizeof(privkey_path)) {
			fprintf(stderr, "Privkey path too long\n");
			return -1;
		}
	}
	printf("Privkey path: %s\n", privkey_path);

	/* Read key file */
	file = fopen(privkey_path, "rb");
	if (!file) {
		fprintf(stderr, "Cannot open file: %s\n", privkey_path);
		return -1;
	}

	ret = fread(example->privkey, 1, sizeof(example->privkey) - 1, file);
	if (ret <= 0) {
		fprintf(stderr, "File read fail\n");
		fclose(file);
		return -1;
	}
	example->privkey[ret] = '\0';
	printf("Private key loading completed\n");
	fclose(file);

	ret = print_warning(example);
	if (ret)
		return ret;

	/* Initialize */
	example->g.privkey = example->privkey;
	example->g.url = example->repository;
	example->g.buf = example->buffer;
	example->g.buf_len = sizeof(example->buffer);

	/* These two functions are optional, you can choose not to implement them */
	example->g.get_date = gitt_get_date_impl;
	example->g.get_zone = gitt_get_zone_impl;

	printf("Initialize...\n");
	ret = gitt_init(&example->g);
	printf("Initialize result: %s\n", GITT_ERRNO_STR(ret));
	if (ret)
		return ret;

	printf("HEAD: %s\n", example->g.repository.head);
	printf("Refs: %s\n", example->g.repository.refs);

	/* Set device info */
	strcpy(example->g.device.name, EXAMPLE_DEVICE_NAME);
	strcpy(example->g.device.id, EXAMPLE_DEVICE_ID);
	printf("Device name: %s\n", example->g.device.name);
	printf("Device id:   %s\n", example->g.device.id);

	example->state = EXAMPLE_STATE_RUN;

	return 0;
}

static void print_help(void);

static int cmd_func_help(struct gitt_example *example, int args, char *argv[])
{
	print_help();

	return 0;
}

/*
 * File backup / restore (files are stored as real git objects)
 */

/*
 * A file larger than the buffer cannot be pulled back by GITT (the
 * unpack writes the decompressed object into the buffer), so cap the
 * backup at the largest pullable size (gitt_config.h).
 */

/* Recursively load the files of a directory */
static int gitt_load_dir(const char *path, const char *prefix,
			 struct gitt_file *files, uint8_t *count)
{
	DIR *dir;
	struct dirent *ent;
	int ret = 0;

	dir = opendir(path);
	if (!dir) {
		fprintf(stderr, "Cannot open dir: %s\n", path);
		return -1;
	}

	while ((ent = readdir(dir))) {
		char full[256];
		char fprefix[80];
		struct stat st;
		FILE *file;
		char *data;
		long size;

		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;

		snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);
		if (stat(full, &st))
			continue;

		if (S_ISDIR(st.st_mode)) {
			snprintf(fprefix, sizeof(fprefix), "%s%s%s", prefix,
				 prefix[0] ? "/" : "", ent->d_name);
			ret = gitt_load_dir(full, fprefix, files, count);
			if (ret)
				break;
		} else if (S_ISREG(st.st_mode)) {
			if (*count >= GITT_MAX_FILES) {
				fprintf(stderr, "Too many files (max %d)\n",
					GITT_MAX_FILES);
				ret = -1;
				break;
			}
			/*
			 * A file that does not fit is an error: silently skipping
			 * it would back up an incomplete tree.
			 */
			if (st.st_size > GITT_CONFIG_MAX_OBJECT) {
				fprintf(stderr, "File too big (max %d): %s\n",
					GITT_CONFIG_MAX_OBJECT, full);
				ret = -1;
				break;
			}

			file = fopen(full, "rb");
			if (!file) {
				ret = -1;
				break;
			}
			data = (char *)malloc(st.st_size ? (size_t)st.st_size : 1);
			if (!data) {
				fclose(file);
				ret = -1;
				break;
			}
			size = (long)fread(data, 1, (size_t)st.st_size, file);
			fclose(file);
			if (size != st.st_size) {
				fprintf(stderr, "Cannot read file: %s\n", full);
				free(data);
				ret = -1;
				break;
			}

			snprintf(files[*count].path, sizeof(files[*count].path),
				 "%s%s%s", prefix, prefix[0] ? "/" : "", ent->d_name);
			files[*count].data = (uint8_t *)data;
			files[*count].size = (uint16_t)size;
			(*count)++;
		}
	}

	closedir(dir);
	return ret;
}

/* Create the parent directories of a file path (ignore existing) */
static void gitt_mkdir_parent(char *path)
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

static void gitt_file_callback(struct gitt *g, char *path,
			       uint8_t *data, uint32_t size)
{
	struct gitt_example *example = gitt_containerof(g, struct gitt_example, g);
	char full[256];
	FILE *file;

	snprintf(full, sizeof(full), "%s/%s", example->out_dir, path);
	gitt_mkdir_parent(full);

	file = fopen(full, "wb");
	if (!file) {
		fprintf(stderr, "Cannot create file: %s\n", full);
		return;
	}
	if (fwrite(data, 1, size, file) != size)
		fprintf(stderr, "Error writing file: %s\n", full);
	fclose(file);
	printf("  restored %s (%u bytes)\n", full, size);
}

static int cmd_func_pushfile(struct gitt_example *example, int args, char *argv[])
{
	struct gitt_file files[GITT_MAX_FILES];
	uint8_t count = 0;
	char message[128] = "";
	int index;
	int ret;

	if (example->state != EXAMPLE_STATE_RUN) {
		fprintf(stderr, "Invalid: Please use the initialization command to initialize first\n");
		return -1;
	}

	if (args < 2) {
		fprintf(stderr, "Invalid: Please enter the directory to back up\n");
		return -1;
	}

	ret = gitt_load_dir(argv[1], "", files, &count);
	if (ret)
		return -1;
	if (!count) {
		fprintf(stderr, "No files found in %s\n", argv[1]);
		return -1;
	}

	for (index = 2; index < args; index++) {
		int n = snprintf(message + strlen(message),
				sizeof(message) - strlen(message),
				"%s%s", strlen(message) ? " " : "", argv[index]);

		if (n < 0 || (size_t)n >= sizeof(message)) {
			fprintf(stderr, "Invalid: Message too long\n");
			return -1;
		}
	}

	printf("Please wait...\n");
	ret = gitt_commit_files(&example->g, files, count,
				message[0] ? message : NULL);
	printf("Push file result: %s\n", GITT_ERRNO_STR(ret));

	for (index = 0; index < count; index++)
		free(files[index].data);

	if (ret)
		return -1;

	return 0;
}

static int cmd_func_pullfile(struct gitt_example *example, int args, char *argv[])
{
	int ret;

	if (example->state != EXAMPLE_STATE_RUN) {
		fprintf(stderr, "Invalid: Please use the initialization command to initialize first\n");
		return -1;
	}

	if (args >= 2 && strlen(argv[1]) + 1 <= sizeof(example->out_dir))
		strcpy(example->out_dir, argv[1]);
	else
		strcpy(example->out_dir, "./gitt_files");

	mkdir(example->out_dir, 0755);

	example->g.file_dump = gitt_file_callback;

	printf("Please wait...\n");
	ret = gitt_pull(&example->g);
	printf("Pull file result: %s (files in %s)\n",
	       GITT_ERRNO_STR(ret), example->out_dir);
	if (ret)
		return -1;

	return 0;
}

/* Detect any key press */
static int hit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if (ch != EOF)
		return ch;

	return 0;
}

/*
 * Poll the file snapshot
 *
 * Every interval the loop pulls the latest snapshot (a fresh depth-1
 * fetch of the tip tree) into the out directory and reports the file
 * changes against the previous poll. A file is identified by its path
 * and its blob SHA-1 (the file registry of the repository), so no file
 * content is kept between polls.
 */

struct loop_file {
	char path[GITT_REPOSITORY_MAX_PATH];
	char sha1[41];
};

static struct loop_file loop_prev_files[GITT_REPOSITORY_MAX_FILES];
static uint8_t loop_prev_num;

static void loop_report_changes(struct gitt *g)
{
	struct gitt_repository *r = &g->repository;
	uint8_t i, j;
	int changed = 0;

	for (i = 0; i < r->file_num; i++) {
		for (j = 0; j < loop_prev_num; j++)
			if (!strcmp(loop_prev_files[j].path, r->files[i].path))
				break;
		if (j == loop_prev_num) {
			printf("  added:   %s\n", r->files[i].path);
			changed = 1;
		} else if (strcmp(loop_prev_files[j].sha1, r->files[i].sha1)) {
			printf("  changed: %s\n", r->files[i].path);
			changed = 1;
		}
	}
	for (j = 0; j < loop_prev_num; j++) {
		for (i = 0; i < r->file_num; i++)
			if (!strcmp(loop_prev_files[j].path, r->files[i].path))
				break;
		if (i == r->file_num) {
			printf("  deleted: %s\n", loop_prev_files[j].path);
			changed = 1;
		}
	}
	if (!changed)
		printf("  (no change)\n");

	for (i = 0; i < r->file_num; i++) {
		strcpy(loop_prev_files[i].path, r->files[i].path);
		strcpy(loop_prev_files[i].sha1, r->files[i].sha1);
	}
	loop_prev_num = r->file_num;
}

static int cmd_func_loop(struct gitt_example *example, int args, char *argv[])
{
	int interval = DEFAULT_LOOP_TIME;
	int ret;
	int count = 0;
	int running = 1;

	if (example->state != EXAMPLE_STATE_RUN) {
		fprintf(stderr, "Invalid: Please use the initialization command to initialize first\n");
		return -1;
	}

	if (args >= 2)
		sscanf(argv[1], "%d", &interval);
	if (interval < 1)
		interval = 1;

	if (args >= 3 && strlen(argv[2]) + 1 <= sizeof(example->out_dir))
		strcpy(example->out_dir, argv[2]);
	else
		strcpy(example->out_dir, "./gitt_files");

	mkdir(example->out_dir, 0755);
	example->g.file_dump = gitt_file_callback;

	loop_prev_num = 0;

	printf("Polling %s every %d second(s), press any key to end\n",
	       example->out_dir, interval);

	while (running) {
		ret = gitt_pull(&example->g);
		if (ret)
			printf("Pull fail: %s\n", GITT_ERRNO_STR(ret));
		else
			loop_report_changes(&example->g);

		count = 0;
		while (running && count < interval) {
			if (hit())
				running = 0;
			else
				sleep(1);
			count++;
		}
	}

	printf("\nExit loop\n");

	return 0;
}

static int cmd_func_exit(struct gitt_example *example, int args, char *argv[])
{
	gitt_end(&example->g);
	example->state = EXAMPLE_STATE_INIT;

	return -2;
}

/* Menu */
static struct cmd_item cmd_list[] = {
	{"init",     cmd_func_init,     "init [repository] <privkey>  -- Initialize GITT"},
	{"exit",     cmd_func_exit,     "exit                         -- Exit GITT"},
	{"loop",     cmd_func_loop,     "loop <time> [dir]            -- Poll the file snapshot, report file changes"},
	{"pushfile", cmd_func_pushfile, "pushfile <dir> [msg]         -- Back up files of a directory"},
	{"pullfile", cmd_func_pullfile, "pullfile [dir]               -- Restore the latest file snapshot"},
	{"help",     cmd_func_help,     "help                         -- Show help"},
};

static void print_help(void)
{
	int index;

	printf("\nUsage:\n");
	for (index = 0; index < sizeof(cmd_list)/sizeof(cmd_list[0]); index++)
		printf("%8s:  %s\n", cmd_list[index].name, cmd_list[index].help);
}

/* Parse parameters in a line */
static int read_parse_line(char *buf, int size, char *argv[])
{
	int index;
	int ret;
	char last = '\0';
	int args;
	int loop;

	index = 0;
	args = 0;
	loop = 1;
	while (loop && index < size && args < 10) {
		ret = fread(buf + index, 1, 1, stdin);
		if (!ret)
			return 0;	/* EOF */
		if (buf[index] == '\n')
			loop = 0;

		if (buf[index] == ' ' || buf[index] == '\n')
			buf[index] = '\0';

		if (last == '\0' && buf[index] != '\0')
			argv[args] = buf + index;
		else if (last != '\0' && buf[index] == '\0')
			args++;

		if (buf[index] != '\0' || last != '\0') {
			last = buf[index];
			index++;
		} else {
			last = buf[index];
		}
	}

	/* A line longer than the buffer is truncated (index == size) */
	if (index >= size)
		index = size - 1;
	buf[index] = '\0';

	return args;
}

/* Enter */
int main(int args, char *argv[])
{
	struct gitt_example example = {0};
	int ret;
	char *arg_list[10];
	char input[1024];
	int index;

	ret = get_home(&example);
	if (ret)
		return ret;

	print_info();
	print_help();

	while (ret != -2) {
		printf("GITT# ");
		ret = read_parse_line(input, sizeof(input), arg_list);
		if (!ret) {
			/* EOF: stop (piped input exhausted); empty line: re-prompt */
			if (feof(stdin))
				break;
			continue;
		}

		/* Exec */
		for (index = 0; index < sizeof(cmd_list)/sizeof(cmd_list[0]); index++) {
			if (!strcmp(arg_list[0], cmd_list[index].name)) {
				ret = cmd_list[index].func(&example, ret, arg_list);
				if (ret == -1)
					print_help();
				printf("\n");
				break;
			}
		}
		if (index >= sizeof(cmd_list)/sizeof(cmd_list[0])) {
			fprintf(stderr, "Unknown command: %s\n", arg_list[0]);
			print_help();
		}
	}

	printf("Exited\n");

	return 0;
}
