/*
 * MIT License
 *
 * Copyright (c) 2023 Hoozz <huxiangjs@foxmail.com>
 *
 * Permission is hereby granted, free of charge, for any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * GITT global configuration.
 *
 * The GITT core includes <gitt_config.h>: every configurable limit and
 * default of the library comes from this file. When porting the library
 * to a microcontroller, provide your own gitt_config.h on the include
 * path - this file is the reference config, consumed by the example.
 *
 * Porting checklist:
 *  1. Pick the user buffer size first - it is the largest knob and
 *     bounds the largest file / object that can be pulled back.
 *  2. Check the push / pull limits against your data (files, paths,
 *     history depth).
 *  3. Set the device identity and the ssh url field sizes for your
 *     network.
 *
 * Limits not listed here are fixed by the git protocol or by the C
 * types; see the READMEs' Limits sections for the full picture.
 */

#ifndef __GITT_CONFIG_H_
#define __GITT_CONFIG_H_

/*
 * ===================================================================
 * The user buffer - the largest knob
 * ===================================================================
 * One contiguous buffer the application provides for the whole
 * session: the pack work area on push and the unpack work area on
 * pull.
 *
 * - the library requires at least 20 bytes (a pack ends with a 20
 *   byte SHA-1 checksum)
 * - pushing blobs needs at least about 1035 bytes (a
 *   GITT_PACK_BLOB_CHUNK compression chunk plus the ~11 byte overhead
 *   of a stored zlib block)
 *
 * used in: the application (example: main.c)
 */
#define GITT_CONFIG_BUF_SIZE			4096

/*
 * Largest single object (file, tree, commit) the application can pull
 * back, bytes: the decompressed object is written into the user
 * buffer (src/gitt_unpack.c)
 */
#define GITT_CONFIG_MAX_OBJECT		(GITT_CONFIG_BUF_SIZE - 1)

/*
 * ===================================================================
 * Push side (gitt_push / gitt_commit_files)
 * ===================================================================
 */

/* Files per commit. used in: include/gitt.h */
#define GITT_MAX_FILES			16

/*
 * Longest file path, bytes including the NUL (63 chars).
 * used in: include/gitt.h
 */
#define GITT_FILE_PATH_SIZE		64

/*
 * Longest file name, bytes including the NUL (63 chars).
 * used in: include/gitt_tree.h
 */
#define GITT_TREE_NAME_SIZE		64

/*
 * Entries (files plus subdirectories) per tree. used in:
 * include/gitt_tree.h
 */
#define GITT_TREE_MAX_ENTRIES		16

/*
 * Subtrees per commit, excluding the root tree (a commit carries at
 * most the root plus this many trees). used in: src/gitt.c
 */
#define GITT_MAX_TREES			8

/*
 * Sibling subdirectories per directory. used in: src/gitt.c
 */
#define GITT_MAX_DIR_CHILDREN		16

/*
 * The input chunk size for compressing a blob. The working buffer
 * must hold the worst case of one chunk, i.e. a stored zlib block:
 * chunk plus about 11 bytes. used in: src/gitt_pack.c
 */
#define GITT_PACK_BLOB_CHUNK		1024

/*
 * Push retries when the remote head moved in between (the push
 * returns GITT_ERRNO_RETRY). used in: src/gitt.c
 */
#define GITT_TRY_NUMBER			5

/*
 * ===================================================================
 * Pull side (gitt_pull / gitt_history)
 * ===================================================================
 */

/*
 * Objects kept in the object store per pull (all types combined;
 * bounds the fetchable history: commits + trees + blobs <= this).
 * used in: include/gitt_unpack.h
 */
#define GITT_UNPACK_MAX_OBJECTS		64

/*
 * Files in the pulled tip tree (total, all levels). used in:
 * include/gitt_repository.h
 */
#define GITT_REPOSITORY_MAX_FILES	16

/*
 * Non-root trees in the pulled tip tree. used in:
 * include/gitt_repository.h
 */
#define GITT_REPOSITORY_MAX_TREES	8

/*
 * Longest file path of a pulled file, bytes including the NUL (63
 * chars). used in: include/gitt_repository.h
 */
#define GITT_REPOSITORY_MAX_PATH	64

/*
 * Longest ref name, bytes including the NUL (31 chars, e.g.
 * "refs/heads/master"). used in: include/gitt_repository.h and
 * src/gitt_command.c
 */
#define GITT_REPOSITORY_REF_SIZE	32

/*
 * The ref created on the first push when the remote has no refs yet.
 * used in: src/gitt_repository.c
 */
#define GITT_DEFAULT_REF		"refs/heads/master"

/*
 * ===================================================================
 * Device identity
 * ===================================================================
 */

/*
 * Device name / id, bytes including the NUL (31 chars each).
 * used in: include/gitt_device.h
 */
#define GITT_DEVICE_NAME_SIZE		32
#define GITT_DEVICE_ID_SIZE		32

/*
 * The author fields of a commit built from a device: the email is
 * built from the device id with GITT_EMAIL_FORMAT, and the name /
 * email fall back to GITT_UNKNOWN_* when the device is unset.
 * used in: src/gitt.c
 */
#define GITT_EMAIL_FORMAT		"%s@%s.GITT"
#define GITT_UNKNOWN_EMAIL		"UNKNOWN"
#define GITT_UNKNOWN_NAME		"UNKNOWN"

/*
 * The committer fields of every commit GITT builds. used in: src/gitt.c
 */
#define GITT_COMMITTER_ID		"https://github.com/huxiangjs/git_things.git"
#define GITT_COMMITTER_NAME		"GITT"

/*
 * ===================================================================
 * ssh
 * ===================================================================
 */

/*
 * ssh url field sizes, bytes including the NUL. The command buffer
 * for "git-receive-pack '<repository>'" is derived from the
 * repository size (src/gitt_ssh.c). used in: include/gitt_ssh.h
 */
#define GITT_SSH_USER_SIZE		16
#define GITT_SSH_HOST_SIZE		32
#define GITT_SSH_PORT_SIZE		8
#define GITT_SSH_REPOSITORY_SIZE	64

#endif /* __GITT_CONFIG_H_ */
