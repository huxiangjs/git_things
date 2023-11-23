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
  $ make
  gcc -I../include -I../third_party/zlib -DGITT_LOG_TO_STDIO test_zlib.c ../src/  gitt_zlib.c ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../  third_party/zlib/deflate.c ../third_party/zlib/inffast.c ../third_party/zlib/  inflate.c ../third_party/zlib/inftrees.c ../third_party/zlib/trees.c ../  third_party/zlib/zutil.c -o test_zlib
  $ ./test_zlib 
  00 01 ... fe ff
  Compressed 1024 bytes into 426 bytes
  Compressed 1024 bytes into 426 bytes
  Decompressed 432 bytes into 1024 bytes
  Decompressed 432 bytes into 1024 bytes
  00 01 ... fe ff
  ```
