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

## Steps
* Create repository (Take GitHub as an example)
  1. Enter [GitHub](https://github.com)
  2. Login to your account and click "New Repository"
  3. "Repository name" sets the name you want, here we fill in "gitt_example"
  4. The repository type is set to "Private". (unless you want to make it public)
  5. "Choose a license" fill in the MIT License. (You can fill it in whatever you want, just make sure the repository is initialized correctly, this is very important)
  6. Finally, click "Create repository". At this time you will get an initialized repository. The URL of our repository here is: `git@github.com:huxiangjs/gitt_example.git`

* Add the public key to your GitHub (skip if you have already added it)
  1. Enter the following command on your host: `ssh-keygen -t ed25519 -C "your_email@example.com"`. After the operation is completed, your public key will be generated in `/home/you/.ssh/id_ed25519.pub`
  2. Enter GitHub setting, Click "SSH and GPG keys", then click "New SSH key" and add your public key

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
  1. Create two consoles (one for sending and one for receiving)
  2. The receiving end performs the following operations:
     ```shell
     $ cd git_things/examples/
     $ ./gitt
     Home directory: /home/huxiang
     ...... (Log is omitted here)
     Usage:
         init:  init [repository] <privkey>  -- Initialize GITT
         exit:  exit                        -- Exit GITT
      history:  history                     -- View historical events
       commit:  commit [event]              -- Commit a event
         loop:  loop <time>                 -- Loop to get events
         help:  help                        -- Show help
     GITT# init git@github.com:huxiangjs/gitt_example.git
     Repository URL: git@github.com:huxiangjs/gitt_example.git
     Privkey path: /home/huxiang/.ssh/id_ed25519
     Private key loading completed

     !! Warning: The following repository will be formatted and all data will    be   lost.
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
  3. The sending end does the following operations:
     ```shell
     $ cd git_things/examples/
     $ ./gitt
     Home directory: /home/huxiang
     ...... (Log is omitted here)
     Usage:
         init:  init [repository] <privkey>  -- Initialize GITT
         exit:  exit                        -- Exit GITT
      history:  history                     -- View historical events
       commit:  commit [event]              -- Commit a event
         loop:  loop <time>                 -- Loop to get events
         help:  help                        -- Show help
     GITT# init git@github.com:huxiangjs/gitt_example.git
     Repository URL: git@github.com:huxiangjs/gitt_example.git
     Privkey path: /home/huxiang/.ssh/id_ed25519
     Private key loading completed

     !! Warning: The following repository will be formatted and all data will be      lost.
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
  4. Check your receiving end, the following log will appear:
     ```shell
     Example Device <0000000000000000> 1701617598 +0800:   This is test event!!
     ```
