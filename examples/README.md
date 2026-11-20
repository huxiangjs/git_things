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
     Home directory: /home/huxiang
     ...... (Log is omitted here)
     Usage:
         init:  init [repository] <privkey>  -- Initialize GITT
         exit:  exit                        -- Exit GITT
      history:  history                     -- View historical events
       commit:  commit [event]              -- Commit an event (push)
         loop:  loop <time>                 -- Loop to get events
     pushfile:  pushfile <dir> [msg]        -- Back up files of a directory
     pullfile:  pullfile [dir] [full]       -- Restore the latest file snapshot
      shallow:  shallow <on/off>            -- Enable/disable shallow pull
         help:  help                        -- Show help
     GITT# init git@github.com:huxiangjs/gitt_example.git
     Repository URL: git@github.com:huxiangjs/gitt_example.git
     Privkey path: /home/huxiang/.ssh/id_ed25519
     Private key loading completed

     !! Note: Initializing will connect to the repository below and
     sync the remote head. The remote data will not be modified.
     Repository: git@github.com:huxiangjs/gitt_example.git
     Please confirm whether to continue? (no/yes): yes

     Initialize...
     Initialize result: Successful
     HEAD: 3e1df6385da0edf2965f9cb9880e5a1a984f3c46
     Refs: refs/heads/main
     Device name: Example Device
     Device id:   0000000000000000

     GITT# loop
     Interval time: 5 second
     Entering the loop, you can press any key to end it
     ```
  3. On the sending end, perform the following operations:
     ```shell
     $ cd git_things/examples/
     $ ./gitt
     Home directory: /home/huxiang
     ...... (Log is omitted here)
     Usage:
         init:  init [repository] <privkey>  -- Initialize GITT
         exit:  exit                        -- Exit GITT
      history:  history                     -- View historical events
       commit:  commit [event]              -- Commit an event (push)
         loop:  loop <time>                 -- Loop to get events
     pushfile:  pushfile <dir> [msg]        -- Back up files of a directory
     pullfile:  pullfile [dir] [full]       -- Restore the latest file snapshot
      shallow:  shallow <on/off>            -- Enable/disable shallow pull
         help:  help                        -- Show help
     GITT# init git@github.com:huxiangjs/gitt_example.git
     Repository URL: git@github.com:huxiangjs/gitt_example.git
     Privkey path: /home/huxiang/.ssh/id_ed25519
     Private key loading completed

     !! Note: Initializing will connect to the repository below and
     sync the remote head. The remote data will not be modified.
     Repository: git@github.com:huxiangjs/gitt_example.git
     Please confirm whether to continue? (no/yes): yes

     Initialize...
     Initialize result: Successful
     HEAD: 3e1df6385da0edf2965f9cb9880e5a1a984f3c46
     Refs: refs/heads/main
     Device name: Example Device
     Device id:   0000000000000000

     GITT# commit This is test event!!
     Please wait...
     Commit event result: Successful

     GITT#
     ```
  4. On the receiving end, you will see the following log:
     ```shell
     Example Device <0000000000000000> 1701617598 +0800:   This is test event!!
     ```

* File backup / restore
  1. On the sending end, back up a local directory to the repository:
     ```shell
     GITT# pushfile ./my_dir "backup of my_dir"
     Please wait...
     Push file result: Successful
     ```
  2. On the receiving end, restore the latest snapshot to a local directory:
     ```shell
     GITT# pullfile ./restored
     Please wait...
     Pull file result: Successful (files in ./restored)
     ```
     `pullfile` does a shallow fetch by default (only the latest snapshot); pass `full` to fetch the whole history:
     ```shell
     GITT# pullfile ./restored full
     ```

## Limits (for porting)
* The example's buffer is `GITT_CONFIG_BUF_SIZE` (4096) from [gitt_config.h](gitt_config.h), so the largest file GITT itself can pull back is 4095 bytes (`GITT_CONFIG_MAX_OBJECT` follows the buffer size).
* Every tunable limit and default of the library is a macro of [gitt_config.h](gitt_config.h) — the global config the GITT core includes. When porting to a microcontroller, provide your own gitt_config.h on the include path and size the RAM from it (each entry notes where the cap is enforced in the library); the protocol-fixed limits are summarized in the READMEs' Limits sections.
