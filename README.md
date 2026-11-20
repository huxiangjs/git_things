# :fire: Git Things

[English](README.md) | [简体中文](README_zh.md)

Git Things is a library implemented in pure C, referred to as GITT below. It was originally developed to enable serverless IoT device connectivity. It can convert instructions from the operator into git commits, and the device side then pulls the repository to perform control operations. Using this library, you will no longer have to spend time and money maintaining your own cloud server. We can directly use GitHub as our data exchange server.

Besides recording events, GITT can back up a device's files to the repository as real git objects (blob / tree / commit) and restore them later — the result cross-checks cleanly with a plain `git clone` / `git fsck`.

So let's forget about other servers and let's have fun! :)

[![Linux CI](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml/badge.svg)](https://github.com/huxiangjs/git_things/actions/workflows/linux.yml)


## :rocket: Advantages
* Implemented in pure C, portable across platforms.
* Only one third-party dependency: zlib (vendored as a git submodule).
* No git binary needed on the device: GITT speaks the git "smart" protocol (v1) over ssh by itself — packfiles are created and parsed in-process, delta objects (OFS_DELTA / REF_DELTA) included.
* Very small memory footprint: the protocol core is allocation-free; a pull keeps at most 64 received objects in memory and `gitt_commit_files()` uses one small tree pool (at most 8 trees).

## :zap: Notice (Very important)
* **Message-only commits become the new root of the branch.** `gitt_push()` always creates a commit without a parent, so the previous history is left as dangling commits and `git log` from HEAD shows only the newest one. Do not point GITT at a repository whose complete `git log` history must be preserved. (`gitt_commit_files()` does keep the history: its parent is the remote head.)
* Only the git (ssh) protocol is supported; the https protocol is not supported.
* The device side is stateless: it keeps no objects between sessions, so every pull is a fresh snapshot of the tip tree.
* Limits: at most 16 files per commit (`GITT_MAX_FILES`); a single object (e.g. a file) must fit in the user buffer — with the 4096-byte buffer that is 4095 bytes per file; a pull keeps at most 64 objects in memory (`GITT_UNPACK_MAX_OBJECTS`).

## :package: What it can do
* **Events**: an instruction is recorded as a message-only commit with `gitt_push()`; a device receives it through the `remote_event` callback after `gitt_pull()` or `gitt_history()`.
* **File backup / restore**: `gitt_commit_files()` pushes a directory tree as real git objects; `gitt_pull()` restores the files through the `file_dump` callback (shallow mode: the latest snapshot only; full mode: the whole history).

## :book: API overview
* `gitt_init(g)` — connect once and sync the remote head
* `gitt_pull(g)` — fetch the remote state (shallow: latest snapshot; full: whole history)
* `gitt_push(g, message)` — push a message-only commit (becomes the new root of the branch)
* `gitt_commit_files(g, files, file_num, message)` — push a commit with a real file tree
* `gitt_history(g)` — fetch the whole history
* `gitt_end(g)` — release everything

Optional callbacks on `struct gitt`: `get_date` / `get_zone` (commit timestamps), `remote_event` (a received commit), `file_dump` (a restored file).

## :art: Try it on Linux
* Please see [Examples](examples/README.md)
* Tests: `cd tools && make test` (SHA-1, zlib, pack, unpack) and `make test_files && ./test_files` (a full push/pull round-trip against a local git server, cross-checked with `git fsck` and `git clone`). See [tools/README.md](tools/README.md).

## :sparkles: Tested code hosting platforms
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: Invitation card
* If you find GITT useful, please join us in developing and maintaining it. :bug:
