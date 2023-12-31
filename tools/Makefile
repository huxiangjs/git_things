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
	  -DGITT_LOG_TO_STDIO \
	  -O2 \
	  -Wall \

.PHONY: all clean

OBJS := test_sha1 test_zlib test_unpack test_pack

all: $(OBJS)

clean:
	rm -rf *.o *.a *.out $(OBJS)


# Test for SHA1
SHA1_SRCS := ../src/gitt_sha1.c test_sha1.c
test_sha1: $(SHA1_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Test for zlib
ZLIB_SRCS := test_zlib.c
ZLIB_SRCS += ../src/gitt_zlib.c
ZLIB_SRCS += ../third_party/zlib/adler32.c
ZLIB_SRCS += ../third_party/zlib/crc32.c
ZLIB_SRCS += ../third_party/zlib/deflate.c
# ZLIB_SRCS += ../third_party/zlib/infback.c
ZLIB_SRCS += ../third_party/zlib/inffast.c
ZLIB_SRCS += ../third_party/zlib/inflate.c
ZLIB_SRCS += ../third_party/zlib/inftrees.c
ZLIB_SRCS += ../third_party/zlib/trees.c
ZLIB_SRCS += ../third_party/zlib/zutil.c
# ZLIB_SRCS += ../third_party/zlib/compress.c
# ZLIB_SRCS += ../third_party/zlib/uncompr.c

test_zlib: $(ZLIB_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Test for unpack
UNPACK_SRCS := test_unpack.c
UNPACK_SRCS += ../src/gitt_sha1.c
UNPACK_SRCS += ../src/gitt_unpack.c
UNPACK_SRCS += ../src/gitt_misc.c
UNPACK_SRCS += ../src/gitt_zlib.c
UNPACK_SRCS += ../third_party/zlib/adler32.c
UNPACK_SRCS += ../third_party/zlib/crc32.c
UNPACK_SRCS += ../third_party/zlib/deflate.c
# UNPACK_SRCS += ../third_party/zlib/infback.c
UNPACK_SRCS += ../third_party/zlib/inffast.c
UNPACK_SRCS += ../third_party/zlib/inflate.c
UNPACK_SRCS += ../third_party/zlib/inftrees.c
UNPACK_SRCS += ../third_party/zlib/trees.c
UNPACK_SRCS += ../third_party/zlib/zutil.c
# UNPACK_SRCS += ../third_party/zlib/compress.c
# UNPACK_SRCS += ../third_party/zlib/uncompr.c

test_unpack: $(UNPACK_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Test for pack
PACK_SRCS := test_pack.c
PACK_SRCS += ../src/gitt_sha1.c
PACK_SRCS += ../src/gitt_pack.c
PACK_SRCS += ../src/gitt_commit.c
PACK_SRCS += ../src/gitt_misc.c
PACK_SRCS += ../src/gitt_zlib.c
PACK_SRCS += ../third_party/zlib/adler32.c
PACK_SRCS += ../third_party/zlib/crc32.c
PACK_SRCS += ../third_party/zlib/deflate.c
PACK_SRCS += ../third_party/zlib/inffast.c
PACK_SRCS += ../third_party/zlib/inflate.c
PACK_SRCS += ../third_party/zlib/inftrees.c
PACK_SRCS += ../third_party/zlib/trees.c
PACK_SRCS += ../third_party/zlib/zutil.c

test_pack: $(PACK_SRCS)
	$(CC) $(CFLAGS) $^ -o $@
