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
* 限制：见下文"限制（移植）"一节。可配置的限制与默认值是 core 所 include 的全局配置 `<gitt_config.h>` 的宏——[examples/gitt_config.h](examples/gitt_config.h) 是参考配置，移植时由移植者提供自己的文件。

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

## :ruler: 限制（移植）

库的全部限制。可配置的那些是 core 所 include 的全局配置 `<gitt_config.h>` 的宏（移植时由移植者提供自己的文件；[examples/gitt_config.h](examples/gitt_config.h) 是参考配置，每条都注明了限制在库里的真实出处）。其余由 git 协议或 C 类型限定，不可更改。以示例的 4096 字节 buffer 为例汇总如下：

| 项目 | 限制 | 出处 |
|---|---|---|
| 每次提交的文件数 | 16 | `GITT_MAX_FILES` |
| 文件路径 / 文件名长度 | 63 字符 | `GITT_FILE_PATH_SIZE` / `GITT_TREE_NAME_SIZE` |
| blob 大小（push） | 65535 字节 | `uint16_t` 大小字段 |
| 每个目录的兄弟子目录数 | 16 | `GITT_MAX_DIR_CHILDREN` |
| 每次提交的树数（不含根树） | 8 | `GITT_MAX_TREES` |
| 单个树的 entry 数（文件 + 子目录） | 16 | `GITT_TREE_MAX_ENTRIES` |
| 提交 message | buffer − 11 字节（≈4085） | message 整体是一个 zlib block |
| 一个 pack 的对象数 | 255（实际 ≤ 25） | `uint8_t` 计数器 |
| push 冲突时的重试次数 | 5 | `GITT_TRY_NUMBER` |
| 一次拉取内存中保留的对象数（各类型合计） | 64 | `GITT_UNPACK_MAX_OBJECTS` |
| 单个拉取对象的大小（文件、树、提交） | buffer − 1 字节（4095） | 解压直接写进用户 buffer |
| tip 树里的文件总数（所有层级） | 16 | `GITT_REPOSITORY_MAX_FILES` |
| tip 树里的非根树数 | 8 | `GITT_REPOSITORY_MAX_TREES` |
| 拉取文件的路径长度 | 63 字符 | `GITT_REPOSITORY_MAX_PATH` |
| 拉取深度 depth | 255（目前：0 = 全量，1 = shallow） | `uint8_t` |

历史中的提交数量没有硬性上限：受 64 个对象拉取存储的限制，可拉取的历史满足 提交数 + 树数 + blob 数 ≤ 64。

其他移植说明：
* 用户 buffer 至少 20 字节；push blob 需要约 1035 字节（1024 字节的压缩分块 + 约 11 字节 zlib 开销）。示例使用 4096 字节。
* ref 名 ≤ 31 字符；设备名 / 设备 id 各 ≤ 31 字符；提交日期 ≤ 15、时区 ≤ 7。
* 仅支持 ssh 上的 git 协议（端口 22）；tag 对象可以解析但不会被 push。

## :art: 在 Linux 上试用
* 请参阅[示例](examples/README_zh.md)
* 测试：`cd tools && make test`（SHA-1、zlib、pack、unpack）和 `make test_files && ./test_files`（针对本地 git 服务器的完整推送/拉取往返，用 `git fsck` 和 `git clone` 交叉校验）。见 [tools/README_zh.md](tools/README_zh.md)。

## :sparkles: 已测试可用的代码托管平台
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: 邀请
* 如果你觉得 GITT 有用，欢迎加入我们的开发与维护。:bug:
