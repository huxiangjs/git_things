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

OBJS := gitt

all: $(OBJS)

clean:
	rm -rf *.o *.a *.out $(OBJS)

GITT_SRCS := main.c
GITT_SRCS += gitt_log_impl.c
GITT_SRCS += gitt_ssh_impl.c
GITT_SRCS += ../src/gitt_ssh.c
GITT_SRCS += ../src/gitt_sha1.c
GITT_SRCS += ../src/gitt_unpack.c
GITT_SRCS += ../src/gitt_obj.c
GITT_SRCS += ../src/gitt_zlib.c
GITT_SRCS += ../src/gitt_command.c
GITT_SRCS += ../src/gitt_repertory.c
GITT_SRCS += ../third_party/zlib/adler32.c
GITT_SRCS += ../third_party/zlib/crc32.c
GITT_SRCS += ../third_party/zlib/deflate.c
# GITT_SRCS += ../third_party/zlib/infback.c
GITT_SRCS += ../third_party/zlib/inffast.c
GITT_SRCS += ../third_party/zlib/inflate.c
GITT_SRCS += ../third_party/zlib/inftrees.c
GITT_SRCS += ../third_party/zlib/trees.c
GITT_SRCS += ../third_party/zlib/zutil.c
# GITT_SRCS += ../third_party/zlib/compress.c
# GITT_SRCS += ../third_party/zlib/uncompr.c

gitt: $(GITT_SRCS)
	$(CC) $(CFLAGS) $^ $(LIBRARY) -o $@
