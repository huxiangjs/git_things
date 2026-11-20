# The git protocol as implemented by GITT

[English](PROTOCOL.md) | [简体中文](PROTOCOL_zh.md)

What GITT sends and receives over the ssh channel, and how it parses
it. The ssh layer itself (libssh, `examples/gitt_ssh_impl.c`) is out of
scope; this document starts at the moment the ssh channel is up.

GITT speaks the git "smart" protocol, **version 1 only**, with two
server programs:

* **fetch (pull)**: `git-upload-pack <repository path>`
* **push**: `git-receive-pack <repository path>`

Both exchanges begin with a **ref advertisement** from the server, and
everything on the wire is **pkt-line** framed.

## 1. Pkt-line framing

Every line of the protocol is a pkt-line:

```
+------------------+----------+
| 4 hex digits     | payload  |
| (length)         |          |
+------------------+----------+
```

* The length is the payload length **plus 4** (the length field itself),
  written as 4 hex digits. GITT writes them lowercase and accepts both
  cases.
* A length of `0000` is the **flush** pkt: it terminates a stream of
  lines.
* The capabilities of a ref are separated from the rest of the line by
  a NUL byte: `<sha> <refname> <capabilities>\0`. Capabilities appear
  only on the first line of an advertisement; GITT reads them but
  ignores their content.

## 2. Fetch (pull) — git-upload-pack

```
client                                        server (git-upload-pack)
   |
   |  ssh channel up, command: "git-upload-pack <path>"
   |
   |<-- advertisement
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
   |<-- side-band stream
   |     (channel 1) pack bytes ...
   |     (channel 2) progress ...
   |     0000
```

### 2.1 Ref advertisement (server → client)

* The first line carries the SHA-1 of HEAD plus the server's
  capabilities (a NUL-separated list; GITT ignores the content).
* The following lines list the repository's refs. GITT keeps the ref
  whose SHA-1 equals HEAD's (the branch HEAD points at); it is used as
  the ref to fetch and later to push.
* **Empty repository**: the first pkt is already the flush (`0000`).
  GITT treats the remote as having no head; the pull ends here (the
  client sends a flush and closes the connection).

### 2.2 Negotiation (client → server)

```
want <40-hex-sha> <capabilities>
[have <40-hex-sha>]
deepen 1
0000
```

* `want` — the SHA-1 of the remote head, followed by GITT's fetch
  capabilities:
  ` multi_ack no-progress side-band-64k ofs-delta shallow agent=git/2.34.1`
  (the leading space is the separator before the first capability).
* `have` — supported by the protocol code but **never sent**: the
  client is stateless and keeps no objects between fetches, so it can
  never claim to have one.
* `deepen 1` — a shallow fetch of exactly the tip commit (like
  `git fetch --depth=1`). GITT only ever restores the tip tree, so it
  never wants anything older.
* `0000` — the flush ends the negotiation phase. The flush must come
  **after** the want/have/deepen lines; a `have` after the flush is a
  protocol error.
* `thin-pack` is deliberately **not** advertised: the pack must be
  self-contained (no missing delta bases), because the client keeps no
  objects to fill in the missing parts.

### 2.3 Shallow boundary reply (server → client)

```
shallow <40-hex-sha>
...
0000
```

In answer to the client's flush, the server announces the shallow
boundary commit(s) and its own flush. GITT consumes everything up to
the flush before continuing.

### 2.4 `done` (client → server)

```
done
```

### 2.5 Result (server → client)

* A plain `NAK` line (or `ACK`) acknowledges the request.
* Then a **side-band-64k** stream: pkt-lines whose first byte is the
  channel number:
  * channel `1` (data) — the packfile's bytes. GITT strips the channel
    byte and feeds the rest to the pack parser (§4).
  * channel `2` (progress) — human-readable progress text; GITT only
    logs it in debug builds.
  * the outer flush `0000` ends the stream.

## 3. Push — git-receive-pack

```
client                                        server (git-receive-pack)
   |
   |  ssh channel up, command: "git-receive-pack <path>"
   |
   |<-- advertisement (same format as for fetch)
   |
   |--> <old-sha> <new-sha> <refname>\0<capabilities>
   |--> 0000
   |
   |--> raw packfile (no pkt framing)
   |
   |<-- side-band stream
   |     (channel 1) 0012: ok refs/heads/master
   |                  0014: unpack <message>
   |                  0000 (inner flush)
   |     0000
```

### 3.1 Ref advertisement (server → client)

Same line format as for fetch. For an empty repository the first pkt
is the flush; the client then creates the ref by advertising a
40-zero old-id.

### 3.2 Ref update request (client → server)

One pkt-line, then the flush:

```
<old-sha> <new-sha> <refname>\0report-status-v2 side-band-64k object-format=sha1 agent=git/2.34.1
0000
```

* `<old-sha>` — the current remote head (read from the
  advertisement), or 40 zeros when creating a new ref;
* `<new-sha>` — the SHA-1 of the commit GITT is about to send
  (computed in-process);
* `<refname>` — the ref to update: the one HEAD points at (from the
  advertisement), or the one remembered from a previous session, or
  the default `refs/heads/master`;
* capabilities: `report-status-v2` (extended status report),
  `side-band-64k`, `object-format=sha1` (this exchange uses SHA-1, not
  SHA-256), `agent=...`.

If the remote head has moved since the advertisement (a concurrent
push), the server rejects the update. GITT then pulls the new head and
retries, up to `GITT_TRY_NUMBER` times (default 5).

### 3.3 Packfile (client → server)

The packfile follows the flush as **raw bytes** — no pkt-line framing
(this is how the git protocol carries pack data in both directions).
Format in §4.

### 3.4 Status report (server → client)

A side-band stream with the same framing as for fetch:

* channel `1` (data) — a pkt-line stream (the report-status-v2
  payload):
  ```
  0012  ok refs/heads/master
  0014  unpack <unpack status message>
  0000  (inner flush: ends the report for this ref)
  ```
  A rejected update starts with `ng` instead of `ok`, followed by the
  reason. GITT inspects the first two bytes of the first report line
  (`ok` / `ng`).
* channel `2` (progress) — ignored (debug log only).
* the outer `0000` ends the stream.

## 4. Packfile format

```
+----------+-------------+----------------------+
| PACK     | version 2   | number of objects    |   12-byte header
| 4 byte   | 4 byte BE   | 4 byte BE            |
+----------+-------------+----------------------+
| object 0 | object 1 | ... | object N-1         |
+------------------------------------------------+
| SHA-1 checksum (20 byte)                       |
+------------------------------------------------+
```

### 4.1 Object entry

```
| type/size varint | [base reference] | zlib stream |
```

* **type/size varint** — first byte:
  `| 1bit continue | 3bit type | 4bit size |`; each continuation byte
  contributes 7 more size bits (little-endian 7-bit groups).
  Object types: `1` commit, `2` tree, `3` blob, `4` tag,
  `6` OFS_DELTA, `7` REF_DELTA (`0` is invalid).
* **base reference** (delta entries only):
  * OFS_DELTA — the offset from the start of this entry to the start
    of the base entry, as a varint: `value = ((value + 1) << 7) |
    next_byte`, the high bit of the last byte clear.
  * REF_DELTA — the 20-byte SHA-1 of the base object.
* **zlib stream** — the object content. GITT decompresses it straight
  into the user buffer, so one object must fit in the buffer
  completely.

### 4.2 What GITT writes (push)

Never deltas — only full objects of type commit / tree / blob. The
object counter is one byte (≤ 255); in practice a push pack holds at
most 25 objects (files + trees + 1 commit). After the last object,
GITT appends the 20-byte SHA-1 of everything before it.

### 4.3 What GITT reads (pull)

Full objects and both delta kinds. Every received object is kept in an
in-memory object store, and a delta entry is resolved against the base
it refers to (by pack offset for OFS_DELTA, by SHA-1 for REF_DELTA).
The resolved object takes the base's type.

The object id is computed as `SHA-1("<type> <size>\0<content>")` — the
resolved type name (e.g. `blob`) and the final size. Tag objects are
decompressed, id-computed and stored, but never referenced (GITT
builds no tags).

### 4.4 Delta instructions (read side)

After decompression, a delta payload is:

```
| source size | result size | instructions ... |
```

* both sizes — little-endian, 7 bits per byte, the high bit set on
  every byte except the last (at most 4 bytes for the source, 3 for
  the result).
* instructions, until a `0x00` byte:
  * `0x80 | n` — insert the next `n` bytes literally;
  * otherwise — copy from the base:
    * low 4 bits — how many little-endian offset bytes follow (0–3);
    * high 4 bits — the size code: `0` → 65536, `1`–`13` → the size
      itself, `14` → the next 1 byte is the size, `15` → the next 2
      bytes (LE) are the size.

## 5. Object content format

### 5.1 Commit

```
tree <40-hex>
parent <40-hex>
author <name> <<email>> <unix-seconds> <+hhmm>
committer <name> <<email>> <unix-seconds> <+hhmm>

<message>
```

* `parent` — the remote head at push time (the first push into an
  empty repository has no parent line).
* author and committer carry the **same** device identity, like a
  plain `git commit`: name = the device name (or `UNKNOWN`), email =
  `<device-id>@<GITT_VERSION>.GITT` (e.g.
  `0000000000000000@1_00.GITT`).
* date / zone — from the `get_date` / `get_zone` callbacks, or the
  built-in defaults when they are not set.
* message — optional (an empty message is allowed). The whole object
  is compressed in one zlib stream, which caps the message at
  buffer − 11 bytes (worst case, incompressible data).

### 5.2 Tree

One entry per file or subdirectory:

```
<mode> <name>\0<20-byte-sha>
```

* mode — `100644` (file) or `40000` (subdirectory);
* entries are sorted by name (a git requirement); a subdirectory's
  SHA-1 is the one of its tree object;
* files' blob SHA-1s are computed in-process as
  `SHA-1("blob <size>\0<content>")` before the pack is built.

### 5.3 Blob

The raw file content, no header.

## 6. Capabilities, in one place

**Fetch** (client → server, on the `want` line):

| capability | meaning |
|---|---|
| `multi_ack` | the server may reply with NAK/ACK lines (GITT handles both) |
| `no-progress` | the server should not send progress text (GITT tolerates it either way) |
| `side-band-64k` | the pack is wrapped in side-band channels (1 = data, 2 = progress) |
| `ofs-delta` | the server may use OFS_DELTA entries (GITT supports them) |
| `shallow` | the client may send `deepen` |
| `agent=git/2.34.1` | version string (cosmetic) |
| `thin-pack` | deliberately **absent** → self-contained pack |

**Push** (client → server, on the update line):

| capability | meaning |
|---|---|
| `report-status-v2` | extended status report (per-ref `ok`/`ng` + `unpack` lines) |
| `side-band-64k` | the report is wrapped in side-band channels |
| `object-format=sha1` | this exchange uses SHA-1, not SHA-256 |
| `agent=git/2.34.1` | version string (cosmetic) |
