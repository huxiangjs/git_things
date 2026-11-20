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
* Limits: see the Limits section below. The tunable limits and defaults are the macros of the global config `<gitt_config.h>` that the core includes — [examples/gitt_config.h](examples/gitt_config.h) is the reference config, and a port provides its own.

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

## :ruler: Limits (porting)

All limits of the library. The tunable ones are the macros of the global config `<gitt_config.h>` which the core includes (a port provides its own file; [examples/gitt_config.h](examples/gitt_config.h) is the reference, each entry notes where the cap is enforced). The rest are fixed by the git protocol or by C types and cannot be changed. Summary, for the example's 4096-byte buffer:

| What | Limit | Enforced by |
|---|---|---|
| Files per commit | 16 | `GITT_MAX_FILES` |
| File path / file name length | 63 chars | `GITT_FILE_PATH_SIZE` / `GITT_TREE_NAME_SIZE` |
| Blob size (push) | 65535 bytes | `uint16_t` size field |
| Sibling subdirectories per directory | 16 | `GITT_MAX_DIR_CHILDREN` |
| Trees per commit (excluding the root) | 8 | `GITT_MAX_TREES` |
| Entries per tree (files + subdirectories) | 16 | `GITT_TREE_MAX_ENTRIES` |
| Commit message | buffer − 11 bytes (≈4085) | the message is a single zlib block |
| Objects per pack | 255 (in practice ≤ 25) | `uint8_t` counter |
| Push retries on a remote-head conflict | 5 | `GITT_TRY_NUMBER` |
| Objects kept in memory per pull (all types) | 64 | `GITT_UNPACK_MAX_OBJECTS` |
| Size of one pulled object (file, tree, commit) | buffer − 1 bytes (4095) | decompressed straight into the buffer |
| Files in the tip tree (all levels) | 16 | `GITT_REPOSITORY_MAX_FILES` |
| Non-root trees in the tip tree | 8 | `GITT_REPOSITORY_MAX_TREES` |
| Path length of a pulled file | 63 chars | `GITT_REPOSITORY_MAX_PATH` |
| Fetch depth | 255 (today: 0 = full, 1 = shallow) | `uint8_t` |

There is no hard limit on the number of commits in the history: the 64-object pull store bounds the fetchable history (commits + trees + blobs ≤ 64).

Other porting notes:
* The user buffer must be at least 20 bytes; pushing blobs needs about 1035 bytes (a 1024-byte compression chunk plus ~11 bytes of zlib overhead). The example uses 4096.
* Ref names ≤ 31 chars; device name / id ≤ 31 chars; commit date ≤ 15, zone ≤ 7.
* Only the git protocol over ssh (port 22) is supported; tag objects can be parsed but are never pushed.

## :art: Try it on Linux
* Please see [Examples](examples/README.md)
* Tests: `cd tools && make test` (SHA-1, zlib, pack, unpack) and `make test_files && ./test_files` (a full push/pull round-trip against a local git server, cross-checked with `git fsck` and `git clone`). See [tools/README.md](tools/README.md).

## :sparkles: Tested code hosting platforms
* [GitHub](https://github.com)
* [Gitee](https://gitee.com/)

## :tada: Invitation card
* If you find GITT useful, please join us in developing and maintaining it. :bug:
