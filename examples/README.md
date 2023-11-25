# Example of running on linux

## Basic environment
* OS:
  ```shell
  $ cat /proc/version
  Linux version 5.19.0-32-generic (buildd@lcy02-amd64-026) (x86_64-linux-gnu-gcc (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0, GNU ld (GNU Binutils for Ubuntu) 2.38) #33~22.04.  1-Ubuntu SMP PREEMPT_DYNAMIC Mon Jan 30 17:03:34 UTC 2
  ```
* Install dependencies:
  ```shell
  sudo apt-get install libssh-dev
  ```
* GCC version:
  ```shell
  $ gcc --version
  gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
  Copyright (C) 2021 Free Software Foundation, Inc.
  This is free software; see the source for copying conditions.  There is NO
  warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ```

## How to use?
  ```shell
  make
  ./gitt
  ```
