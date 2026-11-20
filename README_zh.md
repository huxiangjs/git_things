# :fire: Git Things

[English](README.md) | [简体中文](README_zh.md)

Git Things 是一个用纯 C 语言实现的库，下文简称为 GITT。它最初是为了实现无云服务的 IoT 设备连接而开发的。操作端把文件（即对设备的指令）推送到 git 仓库，设备端定期拉取仓库得到最新的文件快照。使用本库后，你不再需要花费时间和金钱去维护自己的云服务器，可以直接使用 GitHub 作为数据交换服务器。

每个文件都存储为真实的 git 对象（blob / tree / commit），因此仓库可以直接用普通的 `git clone` / `git fsck` 校验。

那么，忘掉其他服务器，来享受乐趣吧！

:)

[![Linux CI](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml/badge.svg)](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml)


## :rocket: 优点
* 纯 C 语言实现，可移植到各平台。
* 第三方依赖仅一个：zlib（以 git submodule 形式内置）。
* 设备端无需 git 程序：GITT 自己通过 ssh 实现 git "smart" 协议（v1）——packfile 在进程内创建和解析，支持 delta 对象（OFS_DELTA / REF_DELTA）。线上格式（发出的请求、收到的数据）见 [PROTOCOL_zh.md](PROTOCOL_zh.md)（[English](PROTOCOL.md)）。
* 内存占用极小：协议核心不做动态分配；一次拉取最多在内存中保留 64 个接收到的对象，`gitt_commit_files()` 只用一个很小的树池（最多 8 棵树）。

## :zap: 注意事项（非常重要）
* **每个提交都携带真实的文件树。** 每次 push 都把新提交的 parent 设为远端 head，因此历史是完整的（`git log` 一切正常）。
* 目前仅支持 git（ssh）协议，不支持 https 协议。
* 设备端是无状态的：会话之间不保留任何对象，因此每次拉取都是 tip 树的一份全新快照（depth 1）。
* **每次传输都是全量快照，不做增量。** push 时每个文件都以完整内容上传——服务端按 SHA-1 去重，没变过的文件只消耗流量、不占存储。pull 时下载的是整个 tip 树，包括没变过的文件（无状态客户端做不了增量拉取）。
* 限制：见下文"限制（移植）"一节。可配置的限制与默认值是 core 所 include 的全局配置 `<gitt_config.h>` 的宏——[examples/gitt_config.h](examples/gitt_config.h) 是参考配置，移植时由移植者提供自己的文件。

## :package: 能做什么
* **文件备份 / 恢复**：`gitt_commit_files()` 把一个目录树以真实的 git 对象推送上去；`gitt_pull()` 拉取 tip 树的全新快照（depth 1）并通过 `file_dump` 回调恢复文件。设备端可以定期轮询 `gitt_pull()`，跟踪某个文件（比如状态文件）的变化。

## :book: API 一览
* `gitt_init(g)` —— 连接一次并同步远端 head
* `gitt_pull(g)` —— 拉取 tip 树的全新快照（depth 1）并恢复文件
* `gitt_commit_files(g, files, file_num, message)` —— 推送一个带真实文件树的提交
* `gitt_end(g)` —— 释放所有资源

`struct gitt` 上的可选回调：`get_date` / `get_zone`（提交时间戳）、`file_dump`（恢复一个文件）。

## :pushpin: 限制（移植）

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
| 拉取深度 depth | 1（永远是 tip 树） | 设计固定 |

拉取永远只要 tip 树（depth 1）：64 个对象的拉取存储限制的是单次快照的大小（提交 + 树 + blob ≤ 64）。

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
