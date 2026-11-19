# :fire: Git Things

[English](README.md) | [简体中文](README_zh.md)

Git Things 是一个用纯 C 语言实现的库，下文简称为 GITT。它最初是为了实现无云服务的 IoT 设备连接而开发的。它可以将操作端发出的指令转换为 git commit，然后设备端拉取仓库来执行控制操作。使用本库后，你不再需要花费时间和金钱去维护自己的云服务器，可以直接使用 GitHub 作为数据交换服务器。

那么，忘掉其他服务器，来享受乐趣吧！

:)

[![Linux CI](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml/badge.svg)](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml)


## :rocket: 优点
* 纯 C 语言实现，可移植到各平台。
* 第三方依赖极少（目前仅依赖 zlib）。
* 内存占用极小。
* 无动态内存分配，对 MCU 友好，不会引起内存碎片问题。

## :zap: 注意事项（非常重要）
* **不要使用已有数据的仓库！**（GITT 会清空仓库中的历史数据）
* 目前仅支持 git 协议，不支持 https 协议。

## :memo: 许可证
* MIT License

## :art: 在 Linux 上试用
* 请参阅[示例](examples/README_zh.md)

## :sparkles: 已测试可用的代码托管平台
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: 邀请
* 如果你觉得 GITT 有用，欢迎加入我们的开发与维护。:bug:
