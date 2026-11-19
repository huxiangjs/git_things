# :fire: Git Things

[English](README.md) | [简体中文](README_zh.md)

Git Things is a library implemented in pure C, referred to as GITT below. It was originally developed to enable serverless IoT device connectivity. It can convert instructions from the operator into git commits, and the device side then pulls the repository to perform control operations. Using this library, you will no longer have to spend time and money maintaining your own cloud server. We can directly use GitHub as our data exchange server.

So let's forget about other servers and let's have fun!

:)

[![Linux CI](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml/badge.svg)](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml)


## :rocket: Advantages
* Implemented in pure C, portable across platforms.
* Very few third-party dependencies (currently only zlib).
* Very small memory footprint.
* No dynamic memory allocation, friendly to MCUs, and will not cause memory fragmentation.

## :zap: Notice (Very important)
* **DO NOT USE A REPOSITORY THAT ALREADY CONTAINS DATA!** (GITT will clear the historical data in the repository)
* Only the git protocol is currently supported; the https protocol is not supported.

## :memo: License
* MIT License

## :art: Try it on Linux
* Please see [Examples](examples/README.md)

## :sparkles: Tested code hosting platforms
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: Invitation card
* If you find GITT useful, please join us in developing and maintaining it. :bug:
