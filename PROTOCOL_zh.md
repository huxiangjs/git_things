# GITT 实现的 git 协议

[English](PROTOCOL.md) | [简体中文](PROTOCOL_zh.md)

介绍 GITT 在 ssh 通道之上发送和接收的数据格式，以及它如何解析。
ssh 层本身（libssh，`examples/gitt_ssh_impl.c`）不在本文范围；本文从
ssh 通道建立完成那一刻开始。

GITT 实现的是 git "smart" 协议，**仅支持 v1**，与两个服务端程序
交互：

* **fetch（拉取）**：`git-upload-pack <repository path>`
* **push**：`git-receive-pack <repository path>`

两个交互都以服务端的 **ref 广告（advertisement）** 开始，线上所有
数据都用 **pkt-line** 分包。

## 1. Pkt-line 分包

协议里的每一行都是一个 pkt-line：

```
+------------------+----------+
| 4 位十六进制     | 载荷     |
| (长度)           |          |
+------------------+----------+
```

* 长度 = 载荷长度 **加 4**（长度字段自身占 4 字节），写成 4 位
  十六进制数字。GITT 写小写，两种大小写都接受。
* 长度为 `0000` 的是 **flush** 包：标志一行流的结束。
* ref 的能力（capabilities）用 NUL 字节与其余部分分隔：
  `<sha> <refname> <capabilities>\0`。能力只出现在广告的第一行；
  GITT 会读到它们但不使用其内容。

## 2. Fetch（拉取）— git-upload-pack

```
client                                        server (git-upload-pack)
   |
   |  ssh 通道建立，命令: "git-upload-pack <path>"
   |
   |<-- 广告 (advertisement)
   |     <40-hex-sha> HEAD <capabilities>\0
   |     <40-hex-sha> refs/heads/master
   |     ...
   |     0000
   |
   |--> want <40-hex-sha> <capabilities>
   |--> deepen 1
   |--> 0000
   |
   |<-- shallow <40-hex-sha>
   |<-- 0000
   |
   |--> done
   |
   |<-- NAK
   |<-- side-band 流
   |     (通道 1) pack 字节 ...
   |     (通道 2) progress ...
   |     0000
```

### 2.1 Ref 广告（server → client）

* 第一行携带 HEAD 的 SHA-1 和服务端能力（NUL 分隔的列表；GITT
  忽略其内容）。
* 后续行列出仓库的 ref。GITT 保留 SHA-1 与 HEAD 相同的那一行（HEAD
  所指向的分支），之后用它作为 fetch 和 push 的 ref。
* **空仓库**：旧版 git 直接发 flush（`0000`）；新版 git 先发一行
  伪 ref：`<40 个 0> capabilities^{} <capabilities>\0`。GITT 把
  null head 视为空仓库；拉取到此结束（客户端发一个 flush 并关闭
  连接）。

### 2.2 协商（client → server）

```
want <40-hex-sha> <capabilities>
[have <40-hex-sha>]
deepen 1
0000
```

* `want` — 远端 head 的 SHA-1，后面跟 GITT 的 fetch 能力：
  ` multi_ack no-progress side-band-64k ofs-delta shallow agent=git/2.34.1`
  （开头的空格是第一个能力前的分隔符）。
* `have` — 协议代码支持但**从不发送**：客户端是无状态的，两次
  fetch 之间不保留任何对象，不可能声称自己有什么。
* `deepen 1` — 浅拉取，只要 tip 那一个 commit（相当于
  `git fetch --depth=1`）。GITT 永远只恢复 tip 树，从不需要更早的
  对象。
* `0000` — flush 标志协商阶段结束。flush 必须位于 want/have/deepen
  **之后**；flush 之后再发 `have` 是协议错误。
* `thin-pack` 被**刻意不声明**：pack 必须是自包含的（不能有缺失的
  delta base），因为客户端不保留对象来补齐缺失部分。

### 2.3 浅边界回复（server → client）

```
shallow <40-hex-sha>
...
0000
```

针对客户端的 flush，服务端通告浅边界的 commit 及它自己的 flush。
GITT 消费到 flush 为止，然后继续。

### 2.4 `done`（client → server）

```
done
```

### 2.5 结果（server → client）

* 一行普通的 `NAK`（或 `ACK`）确认请求。
* 然后是 **side-band-64k** 流：每个 pkt-line 的第一个字节是通道号：
  * 通道 `1`（data）— packfile 的字节。GITT 剥掉通道字节，把剩余
    部分交给 pack 解析器（§4）。
  * 通道 `2`（progress）— 人类可读的进度文本；GITT 只在 debug 构建
    里打日志。
  * 外层 flush `0000` 标志流结束。

## 3. Push — git-receive-pack

```
client                                        server (git-receive-pack)
   |
   |  ssh 通道建立，命令: "git-receive-pack <path>"
   |
   |<-- 广告 (advertisement)（格式同 fetch）
   |
   |--> <old-sha> <new-sha> <refname>\0<capabilities>
   |--> 0000
   |
   |--> 原始 packfile（不做 pkt 分包）
   |
   |<-- side-band 流
   |     (通道 1) 0012: ok refs/heads/master
   |                  0014: unpack <message>
   |                  0000 (内层 flush)
   |     0000
```

### 3.1 Ref 广告（server → client）

格式同 fetch。空仓库时第一个 pkt 就是 flush；客户端此时用 40 个 0
作为 old-id 来创建 ref。

### 3.2 Ref 更新请求（client → server）

一个 pkt-line，然后 flush：

```
<old-sha> <new-sha> <refname>\0report-status-v2 side-band-64k object-format=sha1 agent=git/2.34.1
0000
```

* `<old-sha>` — 当前远端 head（从广告里读到的），创建新 ref 时为
  40 个 0；
* `<new-sha>` — GITT 即将发送的 commit 的 SHA-1（进程内计算）；
* `<refname>` — 要更新的 ref：HEAD 指向的那个（从广告里读到），或
  上次会话记住的那个，或默认的 `refs/heads/master`；
* 能力：`report-status-v2`（扩展状态报告）、`side-band-64k`、
  `object-format=sha1`（本次交互用 SHA-1 而非 SHA-256）、
  `agent=...`。

如果远端 head 自广告之后已经变化（并发 push），服务端会拒绝更新。
GITT 随后拉取新 head 并重试，最多 `GITT_TRY_NUMBER` 次（默认 5）。

### 3.3 Packfile（client → server）

packfile 紧跟在 flush 之后，以**原始字节**发送 — 不做 pkt-line
分包（git 协议双向传 pack 数据都是这样）。格式见 §4。

### 3.4 状态报告（server → client）

side-band 流，分包方式同 fetch：

* 通道 `1`（data）— 一个 pkt-line 流（report-status-v2 的载荷）：
  ```
  0012  ok refs/heads/master
  0014  unpack <unpack 状态消息>
  0000  (内层 flush: 本 ref 的报告结束)
  ```
  被拒绝的更新以 `ng` 代替 `ok`，后面跟原因。GITT 只检查第一条
  报告行的前两个字节（`ok` / `ng`）。
* 通道 `2`（progress）— 忽略（只打 debug 日志）。
* 外层 `0000` 标志流结束。

## 4. Packfile 格式

```
+----------+-------------+----------------------+
| PACK     | version 2   | 对象数               |   12 字节头
| 4 字节   | 4 字节大端  | 4 字节大端           |
+----------+-------------+----------------------+
| 对象 0   | 对象 1 | ... | 对象 N-1            |
+------------------------------------------------+
| SHA-1 校验和 (20 字节)                         |
+------------------------------------------------+
```

### 4.1 对象条目

```
| type/size 变长整数 | [base 引用] | zlib 流 |
```

* **type/size 变长整数** — 第一字节：
  `| 1bit 续位 | 3bit 类型 | 4bit 长度 |`；每个续位字节再贡献 7 位
  长度（小端 7-bit 分组）。
  对象类型：`1` commit、`2` tree、`3` blob、`4` tag、`6`
  OFS_DELTA、`7` REF_DELTA（`0` 非法）。
* **base 引用**（仅 delta 条目有）：
  * OFS_DELTA — 从本条目起始到 base 条目起始的偏移，变长编码：
    `value = ((value + 1) << 7) | next_byte`，最后一个字节的高位为 0。
  * REF_DELTA — base 对象的 20 字节 SHA-1。
* **zlib 流** — 对象内容。GITT 直接解压进用户 buffer，所以单个
  对象必须能完整放进 buffer。

### 4.2 GITT 写出什么（push）

从不写 delta — 只有 commit / tree / blob 完整对象。对象计数器只有
1 字节（≤ 255）；实际一次 push pack 最多 25 个对象（文件 + 树 + 1
commit）。最后一个对象之后，GITT 追加前面全部内容的 20 字节
SHA-1。

### 4.3 GITT 读什么（pull）

完整对象和两种 delta。每个收到的对象都保留在内存对象存储里，delta
条目按引用解析（OFS_DELTA 按 pack 内偏移，REF_DELTA 按 SHA-1）。
解析后的对象取 base 的类型。

对象 id 按 `SHA-1("<type> <size>\0<content>")` 计算 — type 用解析
后的类型名（如 `blob`），size 用最终大小。tag 对象会被解压、算 id、
存入存储，但永远不会被引用（GITT 不构建 tag）。

### 4.4 Delta 指令（读侧）

解压后，delta 载荷为：

```
| source size | result size | 指令 ... |
```

* 两个 size — 小端，每字节 7 位，除最后一个字节外每字节高位为 1
  （source 最多 4 字节，result 最多 3 字节）。
* 指令，直到 `0x00` 字节为止：
  * `0x80 | n` — 原样插入接下来 `n` 个字节；
  * 否则 — 从 base 拷贝：
    * 低 4 位 — 后面跟几个小端偏移字节（0–3）；
    * 高 4 位 — 大小码：`0` → 65536，`1`–`13` → 就是大小本身，
      `14` → 下一个 1 字节是大小，`15` → 接下来 2 字节（小端）是大小。

## 5. 对象内容格式

### 5.1 Commit

```
tree <40-hex>
parent <40-hex>
author <name> <<email>> <unix-秒> <+hhmm>
committer <name> <<email>> <unix-秒> <+hhmm>

<message>
```

* `parent` — push 时刻的远端 head（向空仓库的第一次 push 没有
  parent 行）。
* author 和 committer 携带**同一个**设备身份，和普通 `git commit`
  一样：name = 设备名（或 `UNKNOWN`），email =
  `<device-id>@<GITT_VERSION>.GITT`（如
  `0000000000000000@1_00.GITT`）。
* date / zone — 来自 `get_date` / `get_zone` 回调；未设置时用内置
  默认值。
* message — 可选（允许空 message）。整个对象在一个 zlib 流里压缩，
  因此 message 上限为 buffer − 11 字节（最坏情况：不可压缩数据）。

### 5.2 Tree

每个文件或子目录一个 entry：

```
<mode> <name>\0<20 字节 sha>
```

* mode — `100644`（文件）或 `40000`（子目录）；
* entry 按名称排序（git 的要求）；子目录的 SHA-1 是其 tree 对象的
  SHA-1；
* 文件的 blob SHA-1 在构建 pack 前进程内计算：
  `SHA-1("blob <size>\0<content>")`。

### 5.3 Blob

文件的原始内容，无头部。

## 6. 能力一览

**Fetch**（client → server，在 `want` 行上）：

| 能力 | 含义 |
|---|---|
| `multi_ack` | 服务端可以回 NAK/ACK 行（GITT 两种都处理） |
| `no-progress` | 服务端不应发送进度文本（GITT 收到也能容忍） |
| `side-band-64k` | pack 用 side-band 通道包装（1 = data，2 = progress） |
| `ofs-delta` | 服务端可以使用 OFS_DELTA 条目（GITT 支持） |
| `shallow` | 客户端可以发送 `deepen` |
| `agent=git/2.34.1` | 版本字符串（装饰用） |
| `thin-pack` | 被**刻意不声明** → pack 自包含 |

**Push**（client → server，在更新行上）：

| 能力 | 含义 |
|---|---|
| `report-status-v2` | 扩展状态报告（每个 ref 的 `ok`/`ng` + `unpack` 行） |
| `side-band-64k` | 报告用 side-band 通道包装 |
| `object-format=sha1` | 本次交互用 SHA-1 而非 SHA-256 |
| `agent=git/2.34.1` | 版本字符串（装饰用） |
