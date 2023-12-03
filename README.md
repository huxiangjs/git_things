# :fire: Git Things

Git Things is a library implemented in pure C language, referred to as GITT below. It was originally developed to enable cloud server-less IoT device connectivity. It can convert the instructions on the operating side into git commits, and then the device side pulls the repository to perform control operations. Using this library, you will no longer have to spend time and money maintaining your cloud server. We can directly use Github as our data exchange server.

So let's forget about other servers and let's have fun!

:)

## :rocket: Advantages
* Implemented in pure C language, portable across platforms.
* Very few third-party dependencies. (Currently only relying on zlib)
* Very small memory footprint.
* No memory dynamic allocation, friendly to MCU, will not cause memory fragmentation problem.

## :zap: Notice (Very important)
* **DON'T USE A REPOSITORY WITH DATA!** (GITT will clear historical data in the repository)
* Currently, only the git protocol is supported, and the https protocol is not supported.

## :memo: License
* MIT License

## :art: Try it on Linux
* Please see [Examples](examples/README.md)

## :sparkles: Tested and available code hosting platforms
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: Invitation card
* If you find GITT useful, please join us in developing and maintaining it. :bug:
