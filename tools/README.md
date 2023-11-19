# Tools
For testing and evaluating Git Things.

## Test

### SHA-1
* Build and test:
  ```shell
  $ make
  $ make test_sha1
  gcc -I../include -I../third_party/zlib ../src/gitt_sha1.c test_sha1.c -o test_sha1
  $ ./test_sha1
  STR SHA1: ac56d9346cb1f8950d141426c5cf4f63749e18fb
  BIN SHA1: 059217ea0faca8c1c25aa6849b9c37a5ee367998
  ```
* Reference python results:
  ```shell
  $ python python/refs_sha1.py
  STR SHA1: ac56d9346cb1f8950d141426c5cf4f63749e18fb
  BIN SHA1: 059217ea0faca8c1c25aa6849b9c37a5ee367998
  ```

### ZLIB
* Build and test:
  ```shell
  $ make test_zlib
  gcc -I../include -I../third_party/zlib test_zlib.c ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../third_party/zlib/deflate.c ../third_party/  zlib/infback.c ../third_party/zlib/inffast.c ../third_party/zlib/inflate.c ../third_party/zlib/inftrees.c ../third_party/zlib/trees.c ../third_party/zlib/  zutil.c ../third_party/zlib/compress.c ../third_party/zlib/uncompr.c -o test_zlib
  $ ./test_zlib
  zlib version: 1.3.0.1-motley
  78 9c 0b c9 c8 2c 56 00 a2 44 85 e2 92 a2 cc bc 74 85 b4 fc 22 85 92 d4 e2 12 20 5b 11 00 9b 22 0a 73 (34byte)
  This is a string for testing! (29byte)
  ```
* Reference python results:
  ```shell
  $ python python/refs_zlib.py
  zlib version: 1.2.11
  78 9c 0b c9 c8 2c 56 00 a2 44 85 e2 92 a2 cc bc 74 85 b4 fc 22 85 92 d4 e2 12 20 5b 11 00 9b 22 0a 73 (34byte)
  This is a string for testing! (29byte)
  ```
