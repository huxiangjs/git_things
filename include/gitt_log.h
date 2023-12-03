/*
 * MIT License
 *
 * Copyright (c) 2023 Hoozz <huxiangjs@foxmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
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

#ifndef __GITT_LOG_H_
#define __GITT_LOG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(GITT_LOG_TO_STDIO)
#include <stdio.h>

#define _gitt_log(file, ...) \
	do { \
		fprintf(file, __VA_ARGS__); \
		fflush(file); \
	} while(0)

#define _gitt_log_out(...) _gitt_log(stdout, __VA_ARGS__)
#define _gitt_log_err(...) _gitt_log(stderr, __VA_ARGS__)
#else
/*
 * Please implement the following interfaces according
 * to your system type
 */
void gitt_log_out_impl(const char *fmt, ...);
void gitt_log_err_impl(const char *fmt, ...);

#define _gitt_log_out gitt_log_out_impl
#define _gitt_log_err gitt_log_err_impl
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define gitt_log_debug _gitt_log_out
#else
void gitt_null(const char *fmt, ...);
#define gitt_log_debug(...) do {gitt_null(__VA_ARGS__);} while (0)
#endif

#define gitt_log_info _gitt_log_out
#define gitt_log_error _gitt_log_err

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GITT_LOG_H_ */
