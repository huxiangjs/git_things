# 在 Linux 上运行

[English](README.md) | [简体中文](README_zh.md)

## 基础环境
* 操作系统：
  ```shell
  $ cat /proc/version
  Linux version 5.19.0-32-generic (buildd@lcy02-amd64-026) (x86_64-linux-gnu-gcc (Ubuntu 11.3.0-1ubuntu1~22.04) 11.3.0, GNU ld (GNU Binutils for Ubuntu) 2.38) #33~22.04.  1-Ubuntu SMP PREEMPT_DYNAMIC Mon Jan 30 17:03:34 UTC 2
  ```
* 安装依赖：
  ```shell
  sudo apt-get install libssh-dev
  ```
* GCC 版本：
  ```shell
  $ gcc --version
  gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
  Copyright (C) 2021 Free Software Foundation, Inc.
  This is free software; see the source for copying conditions.  There is NO
  warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  ```

## 操作步骤
* 创建仓库（以 GitHub 为例）
  1. 打开 [GitHub](https://github.com)
  2. 登录你的账号，点击 "New repository"
  3. "Repository name" 填写你想要的名字，这里我们填 "gitt_example"
  4. 仓库类型设置为 "Private"（除非你想公开）
  5. "Choose a license" 选择 MIT License。（你也可以随便选，但务必确保仓库被正确初始化，这一点非常重要）
  6. 最后点击 "Create repository"。此时你就得到了一个初始化好的仓库。这里我们仓库的地址是：`git@github.com:huxiangjs/gitt_example.git`

* 将公钥添加到 GitHub（已添加的可跳过）
  1. 在你的主机上运行以下命令：`ssh-keygen -t ed25519 -C "your_email@example.com"`。完成后，你的公钥会生成在 `/home/you/.ssh/id_ed25519.pub`
  2. 进入 GitHub 设置页面，点击 "SSH and GPG keys"，再点击 "New SSH key"，添加你的公钥

* 编译示例
  ```shell
  git clone git@github.com:huxiangjs/git_things.git
  cd git_things/
  git submodule init
  git submodule update
  cd examples/
  make
  ```

* 简单测试
  1. 打开两个终端（一个用于发送，一个用于接收）
  2. 接收端执行以下操作：
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
  3. 发送端执行以下操作：
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
  4. 在接收端，你将会看到如下日志：
     ```shell
     Example Device <0000000000000000> 1701617598 +0800:   This is test event!!
     ```

* 文件备份 / 恢复
  1. 发送端把本地目录备份到仓库：
     ```shell
     GITT# pushfile ./my_dir "backup of my_dir"
     Please wait...
     Push file result: Successful
     ```
  2. 接收端把最新快照恢复到本地目录：
     ```shell
     GITT# pullfile ./restored
     Please wait...
     Pull file result: Successful (files in ./restored)
     ```
     `pullfile` 默认是 shallow 拉取（只要最新快照）；传 `full` 则拉取全部历史：
     ```shell
     GITT# pullfile ./restored full
     ```
