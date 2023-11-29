# Tools
For testing and evaluating Git Things.

## Test

### SHA-1
* Build and test:
  ```shell
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
  gcc -I../include -I../third_party/zlib -DGITT_LOG_TO_STDIO test_zlib.c ../src/  gitt_zlib.c ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../  third_party/zlib/deflate.c ../third_party/zlib/inffast.c ../third_party/zlib/  inflate.c ../third_party/zlib/inftrees.c ../third_party/zlib/trees.c ../  third_party/zlib/zutil.c -o test_zlib

  $ ./test_zlib 
  zlib version: 1.3.0.1-motley
  00 01 ... fe ff
  Compressed 1024 bytes into 286 bytes
  Compressed 1024 bytes into 286 bytes
  Decompressed 286 bytes into 1024 bytes
  Decompressed 286 bytes into 1024 bytes
  00 01 ... fe ff
  ```

### Unpack
* Build and test:
  ```shell
  $ make test_unpack
  gcc -I../include -I../third_party/zlib -DGITT_LOG_TO_STDIO -O2 -Wall  test_unpack.c ../src/gitt_sha1.c ../src/gitt_unpack.c ../src/gitt_obj.c ../src/gitt_zlib.c ../  third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../third_party/zlib/deflate.c ../third_party/zlib/inffast.c ../third_party/zlib/inflate.c ../third_party/zlib/  inftrees.c ../third_party/zlib/trees.c ../third_party/zlib/zutil.c -o test_unpack

  $ ./test_unpack
  File size: 9779byte
  [       1] version:2, number of objects:42; ****************************************** verify pass, SHA-1: a52896a8b8e6e3f91c5a941653eb7add97b8ae82
  ......
  [    9779] version:2, number of objects:42; ****************************************** verify pass, SHA-1: a52896a8b8e6e3f91c5a941653eb7add97b8ae82
  Test end
  ```

### Pack
* Build and test:
  ```shell
  $ make test_pack

  $ ./test_pack
  Commit id: 30a993f1fca9318abea1a77c65e3cb838701cb1a
  50 41 43 4b 00 00 00 02 00 00 00 01 9b 0d 78 9c 6d cd 4b 0a c2 30 14 85 e1 79 56 91 b9 20 b9 79 07 44 1c ea 32 6e   92 1b 5b b1 8d b4 29 94 ae de 0a 0e 1c 38 3a f0 c3 c7 69 13 11 d7 d1 4b 93 93 d5 32 45 4b 31 a0 b0 82 8c 8e c5 67   1b a4 f7 25 12 e9 20 34 7b e1 44 63 e3 40 26 1b 9b 54 0c c2 81 4e ce 59 40 b5 4f d6 d9 3a 44 90 49 21 81 42 86 4b   eb ea c4 af b5 6e 1b f0 53 b7 ac 3d 8e f7 c7 0c 97 52 d7 01 fb e7 31 d5 e1 cc c1 09 11 bc 03 c5 0f c2 0b b6 b7 a1   6f 8d be 50 fe 40 f9 1f ea 0f 04 c6 6e 63 df 78 d9 1f 1b cd ed 0d 64 1a 42 fc 44 2f 96 2d 5c 4b 9e 89 b0 fb 87 74   cd ab ca 7c 49 0f cc 92
  SH1-A: 442f962d5c4b9e89b0fb8774cdabca7c490fcc92
  Test end

  $ git index-pack -v pack-test.pack
  Indexing objects: 100% (1/1), done.
  442f962d5c4b9e89b0fb8774cdabca7c490fcc92

  $ git verify-pack -v pack-test.pack
  30a993f1fca9318abea1a77c65e3cb838701cb1a commit 219 166 12
  non delta: 1 object
  pack-test.pack: ok
  ```
