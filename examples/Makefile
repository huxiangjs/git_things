# MIT License
#
# Copyright (c) 2023 Hoozz <huxiangjs@foxmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

CROSS_COMPILE :=
CC := $(CROSS_COMPILE)gcc

CFLAGS := -I../include \
	  -I../third_party/zlib \
	  -O2 \
	  -Wall

LIBRARY := -lssh

.PHONY: all clean

OBJS := examples

all: $(OBJS)

clean:
	rm -rf *.o *.a *.out $(OBJS)

EXAMPLES_SRCS := main.c
EXAMPLES_SRCS += gitt_log_impl.c
EXAMPLES_SRCS += gitt_ssh_impl.c
EXAMPLES_SRCS += ../src/gitt_ssh.c
EXAMPLES_SRCS += ../src/gitt_sha1.c
EXAMPLES_SRCS += ../src/gitt_unpack.c
EXAMPLES_SRCS += ../src/gitt_obj.c
EXAMPLES_SRCS += ../src/gitt_zlib.c
EXAMPLES_SRCS += ../third_party/zlib/adler32.c
EXAMPLES_SRCS += ../third_party/zlib/crc32.c
EXAMPLES_SRCS += ../third_party/zlib/deflate.c
# EXAMPLES_SRCS += ../third_party/zlib/infback.c
EXAMPLES_SRCS += ../third_party/zlib/inffast.c
EXAMPLES_SRCS += ../third_party/zlib/inflate.c
EXAMPLES_SRCS += ../third_party/zlib/inftrees.c
EXAMPLES_SRCS += ../third_party/zlib/trees.c
EXAMPLES_SRCS += ../third_party/zlib/zutil.c
# EXAMPLES_SRCS += ../third_party/zlib/compress.c
# EXAMPLES_SRCS += ../third_party/zlib/uncompr.c

examples: $(EXAMPLES_SRCS)
	$(CC) $(CFLAGS) $^ $(LIBRARY) -o $@
