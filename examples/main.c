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
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pwd.h>
#include <string.h>
#include <malloc.h>
#include <gitt.h>
#include <gitt_errno.h>

#define DEFAULT_PRIVKEY_PATH		"id_ed25519"

#define EXAMPLE_DEVICE_NAME		"Example Device"
#define EXAMPLE_DEVICE_ID		"0000000000000000"

#define EXAMPLE_STATE_INIT		0
#define EXAMPLE_STATE_RUN		1

#define DEFAULT_LOOP_TIME		5

struct gitt_example {
	struct gitt g;
	char *home;
	char privkey[2048];
	char repertory[64];
	uint8_t buffer[4096];
	int state;
};

typedef int (*cmd_func)(struct gitt_example *example, int args, char *argv[]);

struct cmd_item {
	char *name;
	cmd_func func;
	char *help;
};

static void gitt_replace(char src, char tag, char *buf, int buf_size)
{
	int i;

	for (i = 0; i < buf_size; i++)
		buf[i] = buf[i] == src ? tag : buf[i];
}

static void gitt_remote_event_callback(struct gitt *g, struct gitt_device *device,
				       char *date, char *zone, char *event)
{
	char buf[128];
	int len = strlen(event);

	sprintf(buf, "%s <%s> %s %s: ", device->name, device->id, date, zone);
	printf("%-64s", buf);

	gitt_replace('\n', ' ', event, len);
	if (len < 32)
		printf("%s\n", event);
	else
		printf("%.*s...\n", 32, event);
}

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

	printf("\n!! Warning: The following repertory will be formatted and all data will be lost.\n");
	printf("Repertory: %s\n", example->repertory);
	printf("Please confirm whether to continue? (no/yes): ");

	if (scanf("%s", buf) == 1 && !strcmp(buf, "yes"))
		ret = 0;

	while (fread(buf, 1, 1, stdin) == 1 && buf[0] == '\0');

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

static int cmd_func_init(struct gitt_example *example, int args, char *argv[])
{
	int ret = 0;
	FILE *file;
	char privkey_path[256];

	/* A repertory url is required */
	if (args < 2) {
		fprintf(stderr, "Invalid: Please enter the repertory URL\n");
		return -1;
	}
	if (strlen(argv[1]) + 1 > sizeof(example->repertory)) {
		fprintf(stderr, "Invalid: Repertory URL too long\n");
		return -1;
	}
	strcpy(example->repertory, argv[1]);
	printf("Repertory URL: %s\n", argv[1]);

	if (args >= 3) {
		if (strlen(argv[2]) + 1 > sizeof(privkey_path)) {
			fprintf(stderr, "Privkey path too long\n");
			return -1;
		}
		strcpy(privkey_path, argv[2]);
	} else {
		sprintf(privkey_path, "%s/.ssh/%s", example->home, DEFAULT_PRIVKEY_PATH);
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
	example->g.url = example->repertory;
	example->g.buf = example->buffer;
	example->g.buf_len = sizeof(example->buffer);
	example->g.remote_event = gitt_remote_event_callback;
	printf("Initialize...\n");
	ret = gitt_init(&example->g);
	printf("Initialize result: %s\n", GITT_ERRNO_STR(ret));
	if (ret)
		return ret;

	printf("HEAD: %s\n", example->g.repertory.head_sha1);

	/* Set device info */
	strcpy(example->g.device.name, EXAMPLE_DEVICE_NAME);
	strcpy(example->g.device.id, EXAMPLE_DEVICE_ID);
	printf("Device name: %s\n", example->g.device.name);
	printf("Device id:   %s\n", example->g.device.id);

	example->state = EXAMPLE_STATE_RUN;

	return 0;
}

static int cmd_func_history(struct gitt_example *example, int args, char *argv[])
{
	int ret;

	if (example->state != EXAMPLE_STATE_RUN) {
		fprintf(stderr, "Invalid: Please use the initialization command to initialize first\n");
		return -1;
	}

	printf("Please wait...\n");
	ret = gitt_history(&example->g);
	printf("History result: %s\n", GITT_ERRNO_STR(ret));
	if (ret)
		return -1;

	return 0;
}

static int cmd_func_commit(struct gitt_example *example, int args, char *argv[])
{
	int ret;
	char buff[1024];
	int count;
	int index;

	if (example->state != EXAMPLE_STATE_RUN) {
		fprintf(stderr, "Invalid: Please use the initialization command to initialize first\n");
		return -1;
	}

	if (args < 2) {
		fprintf(stderr, "Invalid: Please enter the event data\n");
		return -1;
	}

	count = 0;
	for (index = 1 ; index < args; index++)
		count += sprintf(buff + count, "%s ", argv[index]);

	printf("Please wait...\n");
	ret = gitt_commit_event(&example->g, buff);
	printf("Commit event result: %s\n", GITT_ERRNO_STR(ret));
	if (ret)
		return -1;

	return 0;
}

static void print_help(void);

static int cmd_func_help(struct gitt_example *example, int args, char *argv[])
{
	print_help();

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

static int cmd_func_loop(struct gitt_example *example, int args, char *argv[])
{
	int interval = DEFAULT_LOOP_TIME;
	int ret;

	if (example->state != EXAMPLE_STATE_RUN) {
		fprintf(stderr, "Invalid: Please use the initialization command to initialize first\n");
		return -1;
	}

	if (args >= 2)
		sscanf(argv[1], "%d", &interval);

	printf("Interval time: %d second\n", interval);
	printf("Entering the loop, you can press any key to end it\n");

	while (1) {
		ret = gitt_update_event(&example->g);
		// printf("Update event result: %s\n", GITT_ERRNO_STR(ret));
		if (ret)
			break;

		if (hit())
			break;

		sleep(interval);
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
	{"init",    cmd_func_init,    "init [repertory] <privkey>  -- Initialize GITT"},
	{"exit",    cmd_func_exit,    "exit                        -- Exit GITT"},
	{"history", cmd_func_history, "history                     -- View historical events"},
	{"commit",  cmd_func_commit,  "commit [event]              -- Commit a event"},
	{"loop",    cmd_func_loop,    "loop <time>                 -- Loop to get events"},
	{"help",    cmd_func_help,    "help                        -- Show help"},
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
		if (ret <= 0 || buf[index] == '\n')
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
		if (!ret)
			continue;

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
	}

	printf("Exited\n");

	return 0;
}
