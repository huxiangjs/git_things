# :fire: Git Things

[English](README.md) | [简体中文](README_zh.md)

Git Things 是一个用纯 C 语言实现的库，下文简称为 GITT。它最初是为了实现无云服务的 IoT 设备连接而开发的。它可以将操作端发出的指令转换为 git commit，然后设备端拉取仓库来执行控制操作。使用本库后，你不再需要花费时间和金钱去维护自己的云服务器，可以直接使用 GitHub 作为数据交换服务器。

除了记录事件，GITT 还能把设备上的文件以真实的 git 对象（blob / tree / commit）形式备份到仓库，之后再拉取恢复——结果可以直接用普通的 `git clone` / `git fsck` 校验。

那么，忘掉其他服务器，来享受乐趣吧！

:)

[![Linux CI](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml/badge.svg)](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml)


## :rocket: 优点
* 纯 C 语言实现，可移植到各平台。
* 第三方依赖仅一个：zlib（以 git submodule 形式内置）。
* 设备端无需 git 程序：GITT 自己通过 ssh 实现 git "smart" 协议（v1）——packfile 在进程内创建和解析，支持 delta 对象（OFS_DELTA / REF_DELTA）。
* 内存占用极小：协议核心不做动态分配；一次拉取最多在内存中保留 64 个接收到的对象，`gitt_commit_files()` 只用一个很小的树池（最多 8 棵树）。

## :zap: 注意事项（非常重要）
* **纯消息提交会成为分支的新根节点。** `gitt_push()` 创建的提交没有 parent，因此旧的历史会变成 dangling commit，从 HEAD 看 `git log` 只显示最新的那一个。不要把 GITT 指向一个必须完整保留 `git log` 历史的仓库。（`gitt_commit_files()` 会保留历史：它的 parent 是远端 head。）
* 目前仅支持 git（ssh）协议，不支持 https 协议。
* 设备端是无状态的：会话之间不保留任何对象，因此每次拉取都是 tip 树的一份全新快照。
* 限制：一次提交最多 16 个文件（`GITT_MAX_FILES`）；单个对象（例如一个文件）必须能装进用户 buffer（4096 字节的 buffer 即每文件最大 4095 字节）；一次拉取最多在内存中保留 64 个对象（`GITT_UNPACK_MAX_OBJECTS`）。

## :package: 能做什么
* **事件**：用 `gitt_push()` 把一条指令记录为纯消息提交；设备端在 `gitt_pull()` 或 `gitt_history()` 之后通过 `remote_event` 回调收到它。
* **文件备份 / 恢复**：`gitt_commit_files()` 把一个目录树以真实的 git 对象推送上去；`gitt_pull()` 通过 `file_dump` 回调恢复文件（shallow 模式：只要最新快照；full 模式：全部历史）。

## :book: API 一览
* `gitt_init(g)` —— 连接一次并同步远端 head
* `gitt_pull(g)` —— 拉取远端状态（shallow：最新快照；full：全部历史）
* `gitt_push(g, message)` —— 推送一个纯消息提交（成为分支的新根节点）
* `gitt_commit_files(g, files, file_num, message)` —— 推送一个带真实文件树的提交
* `gitt_history(g)` —— 拉取全部历史
* `gitt_end(g)` —— 释放所有资源

`struct gitt` 上的可选回调：`get_date` / `get_zone`（提交时间戳）、`remote_event`（收到一个提交）、`file_dump`（恢复一个文件）。

## :art: 在 Linux 上试用
* 请参阅[示例](examples/README_zh.md)
* 测试：`cd tools && make test`（SHA-1、zlib、pack、unpack）和 `make test_files && ./test_files`（针对本地 git 服务器的完整推送/拉取往返，用 `git fsck` 和 `git clone` 交叉校验）。见 [tools/README_zh.md](tools/README_zh.md)。

## :sparkles: 已测试可用的代码托管平台
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: 邀请
* 如果你觉得 GITT 有用，欢迎加入我们的开发与维护。:bug:
