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
	  -I../examples \
	  -I../third_party/zlib \
	  -DGITT_LOG_TO_STDIO \
	  -O2 \
	  -Wall \
	  -Wno-pointer-to-int-cast \

.PHONY: all clean test test-files

OBJS := test_sha1 test_zlib test_unpack test_pack test_files

all: $(OBJS)

clean:
	rm -rf *.o *.a *.out $(OBJS)

# Run all the tests (test_pack first: test_unpack verifies the pack it writes)
test: all
	@rc=0; \
	for t in test_sha1 test_zlib test_pack test_unpack; do \
		echo "== $$t =="; \
		./$$t || rc=1; \
		echo; \
	done; \
	if [ $$rc -eq 0 ]; then echo "All tests passed"; else echo "Some tests FAILED"; fi; \
	exit $$rc

# The bundled zlib sources (shared by all the tests that use it)
ZLIB_SRCS := ../third_party/zlib/adler32.c
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


# Test for SHA1 (compares against the python reference in tools/python/refs_sha1.py)
SHA1_SRCS := ../src/gitt_sha1.c test_sha1.c
test_sha1: $(SHA1_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Test for zlib
ZLIB_TEST_SRCS := test_zlib.c
ZLIB_TEST_SRCS += ../src/gitt_zlib.c
ZLIB_TEST_SRCS += ../src/gitt_misc.c
ZLIB_TEST_SRCS += $(ZLIB_SRCS)

test_zlib: $(ZLIB_TEST_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Test for unpack
UNPACK_SRCS := test_unpack.c
UNPACK_SRCS += ../src/gitt_sha1.c
UNPACK_SRCS += ../src/gitt_unpack.c
UNPACK_SRCS += ../src/gitt_misc.c
UNPACK_SRCS += ../src/gitt_zlib.c
UNPACK_SRCS += $(ZLIB_SRCS)

test_unpack: $(UNPACK_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Test for pack (the pack is verified by unpacking it)
PACK_SRCS := test_pack.c
PACK_SRCS += ../src/gitt_sha1.c
PACK_SRCS += ../src/gitt_pack.c
PACK_SRCS += ../src/gitt_commit.c
PACK_SRCS += ../src/gitt_tree.c
PACK_SRCS += ../src/gitt_unpack.c
PACK_SRCS += ../src/gitt_misc.c
PACK_SRCS += ../src/gitt_zlib.c
PACK_SRCS += $(ZLIB_SRCS)

test_pack: $(PACK_SRCS)
	$(CC) $(CFLAGS) $^ -o $@


# Round-trip test for the file backup/restore.
# Needs a local sshd, git in PATH and the user's private key; it creates
# and destroys a bare repository in a temporary directory by itself.
FILES_SRCS := test_files.c
FILES_SRCS += ../examples/gitt_log_impl.c
FILES_SRCS += ../examples/gitt_ssh_impl.c
FILES_SRCS += ../src/gitt_ssh.c
FILES_SRCS += ../src/gitt_sha1.c
FILES_SRCS += ../src/gitt_unpack.c
FILES_SRCS += ../src/gitt_misc.c
FILES_SRCS += ../src/gitt_zlib.c
FILES_SRCS += ../src/gitt_command.c
FILES_SRCS += ../src/gitt_repository.c
FILES_SRCS += ../src/gitt_commit.c
FILES_SRCS += ../src/gitt_pack.c
FILES_SRCS += ../src/gitt_tree.c
FILES_SRCS += ../src/gitt_blob.c
FILES_SRCS += ../src/gitt.c
FILES_SRCS += $(ZLIB_SRCS)

test_files: $(FILES_SRCS)
	$(CC) $(CFLAGS) $^ -lssh -o $@

test-files: test_files
	./test_files
