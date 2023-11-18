# Tools
For testing and evaluating Git Things.

## Test

### SHA-1
* Build and test:
  ```shell
  $ make
  gcc -I../include ../src/sha1.c test_sha1.c -o test_sha1
  $ ./test_sha1.exe
  STR SHA1: 4274fab1162825dc1868487812b2dee9d5a41b0c
  BIN SHA1: 059217ea0faca8c1c25aa6849b9c37a5ee367998
  ```
* Reference python results:
  ```shell
  $ python python/refs_sha1.py
  STR SHA1: 4274fab1162825dc1868487812b2dee9d5a41b0c
  BIN SHA1: 059217ea0faca8c1c25aa6849b9c37a5ee367998
  ```
