# 工具

[English](README.md) | [简体中文](README_zh.md)

用于测试和评估 Git Things 的工具。

## 测试

`make test` 按顺序运行下面四个单元测试（test_pack 在 test_unpack
之前，因为 test_unpack 校验的是 test_pack 写出的 pack）。

### SHA-1
* 构建并测试（与 python3 算出的参考值比对）：
  ```shell
  $ make test_sha1
  gcc -I../include -I../examples -I../third_party/zlib -DGITT_LOG_TO_STDIO -O2 -Wall -Wno-pointer-to-int-cast ../src/gitt_sha1.c test_sha1.c -o test_sha1

  $ ./test_sha1
  STR SHA1: ac56d9346cb1f8950d141426c5cf4f63749e18fb (ref: ac56d9346cb1f8950d141426c5cf4f63749e18fb)
  BIN SHA1: 059217ea0faca8c1c25aa6849b9c37a5ee367998 (ref: 059217ea0faca8c1c25aa6849b9c37a5ee367998)
  Test pass
  ```
* Python 参考脚本的结果：
  ```shell
  $ python3 python/refs_sha1.py
  STR SHA1: ac56d9346cb1f8950d141426c5cf4f63749e18fb
  BIN SHA1: 059217ea0faca8c1c25aa6849b9c37a5ee367998
  ```

### ZLIB
* 构建并测试：
  ```shell
  $ make test_zlib
  gcc -I../include -I../examples -I../third_party/zlib -DGITT_LOG_TO_STDIO -O2 -Wall -Wno-pointer-to-int-cast test_zlib.c ../src/gitt_zlib.c ../src/gitt_misc.c ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../third_party/zlib/deflate.c ../third_party/zlib/inffast.c ../third_party/zlib/inflate.c ../third_party/zlib/inftrees.c ../third_party/zlib/trees.c ../third_party/zlib/zutil.c -o test_zlib

  $ ./test_zlib
  zlib version: 1.3.0.1-motley
  00 01 ... fe ff
  Compressed 1024 bytes into 286 bytes
  Decompressed 286 bytes into 1024 bytes
  00 01 ... fe ff
  Test pass
  ```

### Pack
* 构建并测试（通过解包校验 pack）：
  ```shell
  $ make test_pack
  gcc -I../include -I../examples -I../third_party/zlib -DGITT_LOG_TO_STDIO -O2 -Wall -Wno-pointer-to-int-cast test_pack.c ../src/gitt_sha1.c ../src/gitt_pack.c ../src/gitt_commit.c ../src/gitt_tree.c ../src/gitt_unpack.c ../src/gitt_misc.c ../src/gitt_zlib.c ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../third_party/zlib/deflate.c ../third_party/zlib/inffast.c ../third_party/zlib/inflate.c ../third_party/zlib/inftrees.c ../third_party/zlib/trees.c ../third_party/zlib/zutil.c -o test_pack

  $ ./test_pack
  Commit id: 30a993f1fca9318abea1a77c65e3cb838701cb1a
  50 41 43 4b 00 00 00 02 00 00 00 01 9b 0d 78 9c 6d cd 4b 0a c2 30 14 85 e1 79 56 91 b9 20 b9 79 07 44 1c ea 32 6e 92 1b 5b b1 8d b4 29 94 ae de 0a 0e 1c 38 3a f0 c3 c7 69 13 11 d7 d1 4b 93 93 d5 32 45 4b 31 a0 b0 82 8c 8e c5 67 1b a4 f7 25 12 e9 20 34 7b e1 44 63 e3 40 26 1b 9b 54 0c c2 81 4e ce 59 40 b5 4f d6 d9 3a 44 90 49 21 81 42 86 4b eb ea c4 af b5 6e 1b f0 53 b7 ac 3d 8e f7 c7 0c 97 52 d7 01 fb e7 31 d5 e1 cc c1 09 11 bc 03 c5 0f c2 0b b6 b7 a1 6f 8d be 50 fe 40 f9 1f ea 0f 04 c6 6e 63 df 78 d9 1f 1b cd ed 0d 64 1a 42 fc 44 2f 96 2d 5c 4b 9e 89 b0 fb 87 74 cd ab ca 7c 49 0f cc 92
  SH1-A: 442f962d5c4b9e89b0fb8774cdabca7c490fcc92
  Verifying the pack by unpacking it...
  version:2, number of objects:1
  * Pack checksum: 442f962d5c4b9e89b0fb8774cdabca7c490fcc92 (verified)
  Pack verified: checksum ok, unpacked commit matches (id 30a993f1fca9318abea1a77c65e3cb838701cb1a)
  Test end

  $ git index-pack -v pack-test.pack
  Indexing objects: 100% (1/1), done.
  442f962d5c4b9e89b0fb8774cdabca7c490fcc92

  $ git verify-pack -v pack-test.pack
  30a993f1fca9318abea1a77c65e3cb838701cb1a commit 219 166 12
  non delta: 1 object
  pack-test.pack: ok
  ```

### Unpack
* 校验 test_pack 写出的 `pack-test.pack` 文件。构建并测试：
  ```shell
  $ make test_unpack
  gcc -I../include -I../examples -I../third_party/zlib -DGITT_LOG_TO_STDIO -O2 -Wall -Wno-pointer-to-int-cast test_unpack.c ../src/gitt_sha1.c ../src/gitt_unpack.c ../src/gitt_misc.c ../src/gitt_zlib.c ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c ../third_party/zlib/deflate.c ../third_party/zlib/inffast.c ../third_party/zlib/inflate.c ../third_party/zlib/inftrees.c ../third_party/zlib/trees.c ../third_party/zlib/zutil.c -o test_unpack

  $ ./test_unpack
  File size: 198byte
  [       1] version:2, number of objects:1; * verify pass, SHA-1: 442f962d5c4b9e89b0fb8774cdabca7c490fcc92
  ......
  [     198] version:2, number of objects:1; * verify pass, SHA-1: 442f962d5c4b9e89b0fb8774cdabca7c490fcc92
  Test end
  ```

### 文件往返测试
* 将一个目录树推送到本地 git 服务器（`gitt_commit_files`），再拉取回来
  （`gitt_pull`），逐字节比对。同时用真正的 git（`fsck`、`clone`）
  交叉校验仓库。
* 构建并测试：
  ```shell
  $ make test_files
  $ ./test_files
  ```
* 要求：本地 sshd（默认 22 端口）、PATH 中有 git、可用的私钥（默认
  `$HOME/.ssh/id_ed25519`，也可以把其他私钥作为第一个参数传入）。测试
  会自己在临时目录中创建并销毁一个 bare 仓库。
* 覆盖：拉取空仓库；三层目录、4095 字节二进制文件和空文件的树；full
  与 shallow 拉取；对推送结果的真实 `git clone`；第二次推送（修改、新增、
  删除文件）；仅消息的 commit（其树为空）。
