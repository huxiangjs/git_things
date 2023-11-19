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
	  -I../third_party/zlib

.PHONY: all clean

OBJS := test_sha1 test_zlib

all: $(OBJS)

clean:
	rm -rf *.o *.a *.out $(OBJS)

# Test for SHA1
SHA1_SRCS := ../src/gitt_sha1.c test_sha1.c
test_sha1: $(SHA1_SRCS)
	$(CC) $(CFLAGS) $^ -o $@

# Test for zlib
ZLIB_SRCS := test_zlib.c \
	     ../third_party/zlib/adler32.c \
	     ../third_party/zlib/crc32.c \
	     ../third_party/zlib/deflate.c \
	     ../third_party/zlib/infback.c \
	     ../third_party/zlib/inffast.c \
	     ../third_party/zlib/inflate.c \
	     ../third_party/zlib/inftrees.c \
	     ../third_party/zlib/trees.c \
	     ../third_party/zlib/zutil.c \
	     ../third_party/zlib/compress.c \
	     ../third_party/zlib/uncompr.c

test_zlib: $(ZLIB_SRCS)
	$(CC) $(CFLAGS) $^ -o $@
