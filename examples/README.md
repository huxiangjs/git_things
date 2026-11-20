# Running on Linux

[English](README.md) | [简体中文](README_zh.md)

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

## Steps
* Create a repository (using GitHub as an example)
  1. Go to [GitHub](https://github.com)
  2. Log in to your account and click "New repository"
  3. For "Repository name", set the name you want; here we use "gitt_example"
  4. Set the repository type to "Private" (unless you want to make it public)
  5. For "Choose a license", select the MIT License. (You can choose whatever you like, but make sure the repository is initialized correctly—this is very important)
  6. Finally, click "Create repository". You now have an initialized repository. The URL of our repository here is: `git@github.com:huxiangjs/gitt_example.git`

* Add your public key to GitHub (skip if you have already done this)
  1. Run the following command on your host: `ssh-keygen -t ed25519 -C "your_email@example.com"`. When it finishes, your public key will be generated at `/home/you/.ssh/id_ed25519.pub`
  2. Go to GitHub Settings, click "SSH and GPG keys", then click "New SSH key" and add your public key

* Build examples
  ```shell
  git clone git@github.com:huxiangjs/git_things.git
  cd git_things/
  git submodule init
  git submodule update
  cd examples/
  make
  ```

* Simple test
  1. Open two terminals (one for sending, one for receiving)
  2. On the receiving end, perform the following operations:
     ```shell
     $ cd git_things/examples/
     $ ./gitt
     ==================================================
       GITT v1_00 - Git Things
       serverless file sync over the git protocol
     ==================================================

       Commands:
         init <repository> [privkey]
           Initialize GITT
         loop [time] [dir]
           Poll the file snapshot, report file changes
         pushfile <dir> [msg]
           Back up the files of a directory
         pullfile [dir]
           Restore the latest file snapshot
         help
           Show this help
         exit
           Exit GITT
     GITT# init git@github.com:huxiangjs/gitt_example.git
       repository   git@github.com:huxiangjs/gitt_example.git
       private key  /home/huxiang/.ssh/id_ed25519
       key loaded (399 bytes)
       !! init connects to the repository and syncs the remote head;
       !! the remote data will not be modified.
       Continue? (y/N) y
       connecting...
       head         3e1df6385da0edf2965f9cb9880e5a1a984f3c46
       refs         refs/heads/main
       device       Example Device (0000000000000000)
       Ready.

     GITT# loop
       polling ./gitt_files every 5s - press any key to stop
     ```
  3. On the sending end, perform the following operations:
     ```shell
     $ cd git_things/examples/
     $ ./gitt
     ==================================================
       GITT v1_00 - Git Things
       serverless file sync over the git protocol
     ==================================================

       Commands:
         init <repository> [privkey]
           Initialize GITT
         loop [time] [dir]
           Poll the file snapshot, report file changes
         pushfile <dir> [msg]
           Back up the files of a directory
         pullfile [dir]
           Restore the latest file snapshot
         help
           Show this help
         exit
           Exit GITT
     GITT# init git@github.com:huxiangjs/gitt_example.git
       repository   git@github.com:huxiangjs/gitt_example.git
       private key  /home/huxiang/.ssh/id_ed25519
       key loaded (399 bytes)
       !! init connects to the repository and syncs the remote head;
       !! the remote data will not be modified.
       Continue? (y/N) y
       connecting...
       head         3e1df6385da0edf2965f9cb9880e5a1a984f3c46
       refs         refs/heads/main
       device       Example Device (0000000000000000)
       Ready.

     GITT# pushfile ./my_dir "backup of my_dir"
       pushing 2 file(s) from './my_dir' ... ok

     GITT#
     ```
  4. On the receiving end, the loop restores the files and reports the changes:
     ```shell
       [14:03:22] restored ./gitt_files/hello.txt (12 bytes)
       [14:03:22] added:   hello.txt
     ```

* File backup / restore
  1. On the sending end, back up a local directory to the repository:
     ```shell
     GITT# pushfile ./my_dir "backup of my_dir"
       pushing 2 file(s) from './my_dir' ... ok
     ```
  2. On the receiving end, restore the latest snapshot to a local directory:
     ```shell
     GITT# pullfile ./restored
       restoring the latest snapshot into ./restored
       [14:05:10] restored ./restored/hello.txt (12 bytes)
       ok (files in ./restored)
     ```

## Limits (for porting)
* The example's buffer is `GITT_CONFIG_BUF_SIZE` (4096) from [gitt_config.h](gitt_config.h), so the largest file GITT itself can pull back is 4095 bytes (`GITT_CONFIG_MAX_OBJECT` follows the buffer size).
* Every tunable limit and default of the library is a macro of [gitt_config.h](gitt_config.h) — the global config the GITT core includes. When porting to a microcontroller, provide your own gitt_config.h on the include path and size the RAM from it (each entry notes where the cap is enforced in the library); the protocol-fixed limits are summarized in the READMEs' Limits sections.
