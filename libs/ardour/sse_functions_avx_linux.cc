/*
 * Copyright (C) 2015 Paul Davis <paul@linuxaudiosystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ardour/mix.h"

#include <immintrin.h>
#include <xmmintrin.h>

#ifndef __AVX__
#error "__AVX__ must be eanbled for this module to work"
#endif

#define IS_ALIGNED_TO(ptr, bytes) (((uintptr_t)ptr) % (bytes) == 0)

#ifdef __cplusplus
#define C_FUNC extern "C"
#else
#define C_FUNC
#endif

/**
 * External funcions
 */
C_FUNC void
x86_sse_mix_buffers_with_gain(float *dst, const float *src, uint32_t nframes, float gain);

C_FUNC void
x86_sse_mix_buffers_no_gain(float *dst, const float *src, uint32_t nframes);

/**
 * Local functions
 */

static inline __m256 avx_abs_ps(__m256 x);
static inline float avx_getmax_ps(__m256 vmax);
static inline float avx_getmin_ps(__m256 vmin);

static void
x86_sse_avx_mix_buffers_with_gain_unaligned(float *dst, const float *src, uint32_t nframes, float gain);

static void
x86_sse_avx_mix_buffers_with_gain_aligned(float *dst, const float *src, uint32_t nframes, float gain);

static void
x86_sse_avx_mix_buffers_no_gain_unaligned(float *dst, const float *src, uint32_t nframes);

static void
x86_sse_avx_mix_buffers_no_gain_aligned(float *dst, const float *src, uint32_t nframes);

/**
 * Module implementation
 */

/**
 * @brief x86-64 AVX optimized routine for compute peak procedure
 * @param src Pointer to source buffer
 * @param nframes Number of frames to process
 * @param current Current peak value
 * @return float New peak value
 */
C_FUNC float
x86_sse_avx_compute_peak(const float *src, uint32_t nframes, float current)
{
	const __m256 ABS_MASK = _mm256_set1_ps(-0.0F);

	// Broadcast the current max value to all elements of the YMM register
	__m256 vcurrent = _mm256_broadcast_ss(&current);

	// Compute single min/max of unaligned portion until alignment is reached
	while ((((intptr_t)src) % 32 != 0) && nframes > 0) {
		__m256 vsrc;

		vsrc = _mm256_broadcast_ss(src);
		vsrc = _mm256_andnot_ps(ABS_MASK, vsrc);
		vcurrent = _mm256_max_ps(vcurrent, vsrc);

		++src;
		--nframes;
	}

	// Process the aligned portion 32 samples at a time
	while (nframes >= 32) {
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)src + 64), _mm_hint(0));
#else
		__builtin_prefetch(src + 64, 0, 0);
#endif
		__m256 vsrc1, vsrc2, vsrc3, vsrc4;

		vsrc1 = _mm256_load_ps(src + 0 );
		vsrc2 = _mm256_load_ps(src + 8 );
		vsrc3 = _mm256_load_ps(src + 16);
		vsrc4 = _mm256_load_ps(src + 24);

		vsrc1 = _mm256_andnot_ps(ABS_MASK, vsrc1);
		vsrc2 = _mm256_andnot_ps(ABS_MASK, vsrc2);
		vsrc3 = _mm256_andnot_ps(ABS_MASK, vsrc3);
		vsrc4 = _mm256_andnot_ps(ABS_MASK, vsrc4);

		vcurrent = _mm256_max_ps(vcurrent, vsrc1);
		vcurrent = _mm256_max_ps(vcurrent, vsrc2);

		src += 32;
		nframes -= 32;
	}

	// Process the aligned portion 16 samples at a time
	while (nframes >= 16) {
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)src + 64), _mm_hint(0));
#else
		__builtin_prefetch(src + 64, 0, 0);
#endif
		__m256 vsrc1, vsrc2;
		vsrc1 = _mm256_load_ps(src + 0);
		vsrc2 = _mm256_load_ps(src + 8);

		vsrc1 = _mm256_andnot_ps(ABS_MASK, vsrc1);
		vsrc2 = _mm256_andnot_ps(ABS_MASK, vsrc2);

		vcurrent = _mm256_max_ps(vcurrent, vsrc1);
		vcurrent = _mm256_max_ps(vcurrent, vsrc2);

		src += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		__m256 vsrc;

		vsrc = _mm256_load_ps(src);
		vsrc = _mm256_andnot_ps(ABS_MASK, vsrc);
		vcurrent = _mm256_max_ps(vcurrent, vsrc);

		src += 8;
		nframes -= 8;
	}

	// If there are still some left 4 to 8 samples, process them below
	while (nframes > 0) {
		__m256 vsrc;

		vsrc = _mm256_broadcast_ss(src);
		vsrc = _mm256_andnot_ps(ABS_MASK, vsrc);
		vcurrent = _mm256_max_ps(vcurrent, vsrc);

		++src;
		--nframes;
	}

	// Get the current max from YMM register
	current = avx_getmax_ps(vcurrent);

	// zero upper 128 bit of 256 bit ymm register to avoid penalties using non-AVX instructions
	_mm256_zeroupper();
	return current;
}

/**
 * @brief x86-64 AVX optimized routine for find peak procedure
 * @param src Pointer to source buffer
 * @param nframes Number of frames to process
 * @param[in,out] minf Current minimum value, updated
 * @param[in,out] maxf Current maximum value, updated
 */
C_FUNC void
x86_sse_avx_find_peaks(const float *src, uint32_t nframes, float *minf, float *maxf)
{
	// Broadcast the current min and max values to all elements of the YMM register
	__m256 vmin = _mm256_broadcast_ss(minf);
	__m256 vmax = _mm256_broadcast_ss(maxf);

	// Compute single min/max of unaligned portion until alignment is reached
	while ((((intptr_t)src) % 32 != 0) && nframes > 0) {
		__m256 vsrc;

		vsrc = _mm256_broadcast_ss(src);
		vmax = _mm256_max_ps(vmax, vsrc);
		vmin = _mm256_min_ps(vmin, vsrc);

		++src;
		--nframes;
	}

	// Process the aligned portion 32 samples at a time
	while (nframes >= 32) {
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)src + 128), _mm_hint(0));
#else
		__builtin_prefetch(src + 128, 0, 0);
#endif
		__m256 vsrc1, vsrc2, vsrc3, vsrc4;
		vsrc1 = _mm256_load_ps(src + 0 );
		vsrc2 = _mm256_load_ps(src + 8 );
		vsrc3 = _mm256_load_ps(src + 16);
		vsrc4 = _mm256_load_ps(src + 24);

		vmax = _mm256_max_ps(vmax, vsrc1);
		vmax = _mm256_max_ps(vmax, vsrc2);
		vmax = _mm256_max_ps(vmax, vsrc3);
		vmax = _mm256_max_ps(vmax, vsrc4);

		vmin = _mm256_min_ps(vmin, vsrc1);
		vmin = _mm256_min_ps(vmin, vsrc2);
		vmin = _mm256_min_ps(vmin, vsrc3);
		vmin = _mm256_min_ps(vmin, vsrc4);

		src += 32;
		nframes -= 32;
	}

	// Process the remaining samples 16 at a time
	while (nframes >= 16) {
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)src + 64), _mm_hint(0));
#else
		__builtin_prefetch(src + 64, 0, 0);
#endif

		__m256 vsrc1, vsrc2;
		vsrc1 = _mm256_load_ps(src + 0);
		vsrc2 = _mm256_load_ps(src + 8);

		vmax = _mm256_max_ps(vmax, vsrc1);
		vmin = _mm256_min_ps(vmin, vsrc1);

		vmax = _mm256_max_ps(vmax, vsrc2);
		vmin = _mm256_min_ps(vmin, vsrc2);

		src += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		__m256 vsrc;

		vsrc = _mm256_load_ps(src);
		vmax = _mm256_max_ps(vmax, vsrc);
		vmin = _mm256_min_ps(vmin, vsrc);

		src += 8;
		nframes -= 8;
	}

	// If there are still some left 4 to 8 samples, process them one at a time.
	while (nframes > 0) {
		__m256 vsrc;

		vsrc = _mm256_broadcast_ss(src);
		vmax = _mm256_max_ps(vmax, vsrc);
		vmin = _mm256_min_ps(vmin, vsrc);

		++src;
		--nframes;
	}

	// Get min and max of the YMM registers
	float vminf = avx_getmin_ps(vmin);
	float vmaxf = avx_getmax_ps(vmax);

	*minf = vminf;
	*maxf = vmaxf;

	// zero upper 128 bit of 256 bit ymm register to avoid penalties using non-AVX instructions
	_mm256_zeroupper();
}

/**
 * @brief x86-64 AVX optimized routine for apply gain routine
 * @param[in,out] dst Pointer to the destination buffer, which gets updated
 * @param nframes Number of frames (or samples) to process
 * @param gain Gain to apply
 */
C_FUNC void
x86_sse_avx_apply_gain_to_buffer(float *dst, uint32_t nframes, float gain)
{
	if (nframes) {
		__m128 g1 = _mm_set1_ps(gain);
		// Here comes the horror, poor-man's loop unrolling
		switch (((intptr_t)dst) % 32) {
		case 0:
		default:
			// Buffer is aligned, skip to the next section of aligned
			break;
		case 4:
			_mm_store_ss(dst, _mm_mul_ss(g1, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 8:
			_mm_store_ss(dst, _mm_mul_ss(g1, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 12:
			_mm_store_ss(dst, _mm_mul_ss(g1, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 16:
			// This is a special case where pointer is 16 byte aligned
			// for a XMM load/store operation.
			_mm_store_ps(dst, _mm_mul_ps(g1, _mm_load_ps(dst)));
			dst += 4;
			nframes -= 4;
			break;
		case 20:
			_mm_store_ss(dst, _mm_mul_ss(g1, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 24:
			_mm_store_ss(dst, _mm_mul_ss(g1, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 28:
			_mm_store_ss(dst, _mm_mul_ss(g1, _mm_load_ss(dst)));
			++dst;
			--nframes;
		}
	} else {
		return;
	}

	// Load gain vector to all elements of YMM register
	__m256 vgain = _mm256_set1_ps(gain);

	// Process the aligned portion 32 samples at a time
	while (nframes >= 32)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (32 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(dst + (32 * sizeof(float)), 0, 0);
#endif
		__m256 d0, d1, d2, d3;
		d0 = _mm256_load_ps(dst + 0 );
		d1 = _mm256_load_ps(dst + 8 );
		d2 = _mm256_load_ps(dst + 16);
		d3 = _mm256_load_ps(dst + 24);

		d0 = _mm256_mul_ps(vgain, d0);
		d1 = _mm256_mul_ps(vgain, d1);
		d2 = _mm256_mul_ps(vgain, d2);
		d3 = _mm256_mul_ps(vgain, d3);

		_mm256_store_ps(dst + 0 , d0);
		_mm256_store_ps(dst + 8 , d1);
		_mm256_store_ps(dst + 16, d2);
		_mm256_store_ps(dst + 24, d3);

		dst += 32;
		nframes -= 32;
	}

	// Process the remaining samples 16 at a time
	while (nframes >= 16)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (16 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(dst + (16 * sizeof(float)), 0, 0);
#endif
		__m256 d0, d1;
		d0 = _mm256_load_ps(dst + 0 );
		d1 = _mm256_load_ps(dst + 8 );

		d0 = _mm256_mul_ps(vgain, d0);
		d1 = _mm256_mul_ps(vgain, d1);

		_mm256_store_ps(dst + 0 , d0);
		_mm256_store_ps(dst + 8 , d1);

		dst += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		_mm256_store_ps(dst, _mm256_mul_ps(vgain, _mm256_load_ps(dst)));
		dst += 8;
		nframes -= 8;
	}


	// There's a penalty going away from AVX mode to SSE mode. This can
	// be avoided by ensuring to the CPU that rest of the routine is no
	// longer interested in the upper portion of the YMM register.

	_mm256_zeroupper(); // zeros the upper portion of YMM register

	if (nframes) {
		__m128 g2 = _mm_set1_ps(gain);
		switch (nframes % 8) {
		case 0:
		default:
			// No more samples left, break out
			break;
		case 7:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 6:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 5:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 4:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 3:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 2:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		case 1:
			_mm_store_ss(dst, _mm_mul_ss(g2, _mm_load_ss(dst)));
			++dst;
			--nframes;
		}
	}
}

/**
 * @brief x86-64 AVX optimized routine for mixing buffer with gain.
 *
 * This function may choose SSE over AVX if the pointers are aligned
 * to 16 byte boundary instead of 32 byte boundary to reduce time to
 * process.
 *
 * @param[in,out] dst Pointer to destination buffer, which gets updated
 * @param[in] src Pointer to source buffer (not updated)
 * @param nframes Number of samples to process
 * @param gain Gain to apply
 */
C_FUNC void
x86_sse_avx_mix_buffers_with_gain(float *dst, const float *src, uint32_t nframes, float gain)
{
	if (IS_ALIGNED_TO(dst, 32) && IS_ALIGNED_TO(src, 32)) {
		// Pointers are both aligned to 32 bit boundaries, this can be processed with AVX
		x86_sse_avx_mix_buffers_with_gain_aligned(dst, src, nframes, gain);
	} else if (IS_ALIGNED_TO(dst, 16) && IS_ALIGNED_TO(src, 16)) {
		// This can still be processed with SSE
		x86_sse_mix_buffers_with_gain(dst, src, nframes, gain);
	} else {
		// Pointers are unaligned, so process them with unaligned load/store AVX
		x86_sse_avx_mix_buffers_with_gain_unaligned(dst, src, nframes, gain);
	}
}

/**
 * @brief x86-64 AVX optimized routine for mixing buffer with no gain.
 *
 * This function may choose SSE over AVX if the pointers are aligned
 * to 16 byte boundary instead of 32 byte boundary to reduce time to
 * process.
 *
 * @param[in,out] dst Pointer to destination buffer, which gets updated
 * @param[in] src Pointer to source buffer (not updated)
 * @param nframes Number of samples to process
 */
C_FUNC void
x86_sse_avx_mix_buffers_no_gain(float *dst, const float *src, uint32_t nframes)
{
	if (IS_ALIGNED_TO(dst, 32) && IS_ALIGNED_TO(src, 32)) {
		// Pointers are both aligned to 32 bit boundaries, this can be processed with AVX
		x86_sse_avx_mix_buffers_no_gain_aligned(dst, src, nframes);
	} else if (IS_ALIGNED_TO(dst, 16) && IS_ALIGNED_TO(src, 16)) {
		// This can still be processed with SSE
		x86_sse_mix_buffers_no_gain(dst, src, nframes);
	} else {
		// Pointers are unaligned, so process them with unaligned load/store AVX
		x86_sse_avx_mix_buffers_no_gain_unaligned(dst, src, nframes);
	}
}

C_FUNC void
x86_sse_avx_copy_vector(float *dst, const float *src, uint32_t nframes)
{
	(void) memcpy(dst, src, nframes * sizeof(float));
}

/**
 * Local helper functions
 */

static void
x86_sse_avx_mix_buffers_with_gain_unaligned(float *dst, const float *src, uint32_t nframes, float gain)
{
	// Load gain vector to all elements of YMM register
	__m256 vgain = _mm256_set1_ps(gain);

	// Process portion 32 samples at a time
	while (nframes >= 32)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (32 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (32 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (32 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (32 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1, s2, s3;
		__m256 d0, d1, d2, d3;

		// Load sources
		s0 = _mm256_loadu_ps(src + 0 );
		s1 = _mm256_loadu_ps(src + 8 );
		s2 = _mm256_loadu_ps(src + 16);
		s3 = _mm256_loadu_ps(src + 24);

		// Load destinations
		d0 = _mm256_loadu_ps(dst + 0 );
		d1 = _mm256_loadu_ps(dst + 8 );
		d2 = _mm256_loadu_ps(dst + 16);
		d3 = _mm256_loadu_ps(dst + 24);

		// src = src * gain
		s0 = _mm256_mul_ps(vgain, s0);
		s1 = _mm256_mul_ps(vgain, s1);
		s2 = _mm256_mul_ps(vgain, s2);
		s3 = _mm256_mul_ps(vgain, s3);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);
		d2 = _mm256_add_ps(d2, s2);
		d3 = _mm256_add_ps(d3, s3);

		// Store result
		_mm256_storeu_ps(dst + 0 , d0);
		_mm256_storeu_ps(dst + 8 , d1);
		_mm256_storeu_ps(dst + 16, d2);
		_mm256_storeu_ps(dst + 24, d3);

		// Update pointers and counters
		src += 32;
		dst += 32;
		nframes -= 32;
	}

	// Process the remaining samples 16 at a time
	while (nframes >= 16)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (16 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (16 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (16 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (16 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1;
		__m256 d0, d1;

		// Load sources
		s0 = _mm256_loadu_ps(src + 0);
		s1 = _mm256_loadu_ps(src + 8);

		// Load destinations
		d0 = _mm256_loadu_ps(dst + 0);
		d1 = _mm256_loadu_ps(dst + 8);

		// src = src * gain
		s0 = _mm256_mul_ps(vgain, s0);
		s1 = _mm256_mul_ps(vgain, s1);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);

		// Store result
		_mm256_storeu_ps(dst + 0, d0);
		_mm256_storeu_ps(dst + 8, d1);

		// Update pointers and counters
		src += 16;
		dst += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		__m256 s0, d0;
		// Load sources
		s0 = _mm256_loadu_ps(src);
		// Load destinations
		d0 = _mm256_loadu_ps(dst);
		// src = src * gain
		s0 = _mm256_mul_ps(vgain, s0);
		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		// Store result
		_mm256_storeu_ps(dst, d0);
		// Update pointers and counters
		src+= 8;
		dst += 8;
		nframes -= 8;
	}


	// There's a penalty going away from AVX mode to SSE mode. This can
	// be avoided by ensuring the CPU that rest of the routine is no
	// longer interested in the upper portion of the YMM register.

	_mm256_zeroupper(); // zeros the upper portion of YMM register

	if (nframes) {
		__m128 vgain = _mm_set1_ps(gain);
		__m128 vsrc;
		__m128 vdst;

		switch (nframes % 8) {
		case 0:
		default:
			// No more samples left, break out
			break;
		case 7:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 6:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 5:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 4:
			vsrc = _mm_loadu_ps(src);
			vdst = _mm_loadu_ps(dst);
			vsrc = _mm_mul_ps(vgain, vsrc);
			vdst = _mm_add_ps(vdst, vsrc);
			_mm_storeu_ps(dst, vdst);
			src += 4;
			dst += 4;
			nframes -= 4;
			break;
		case 3:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 2:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 1:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		}
	}
}

static void
x86_sse_avx_mix_buffers_with_gain_aligned(float *dst, const float *src, uint32_t nframes, float gain)
{
	// Load gain vector to all elements of YMM register
	__m256 vgain = _mm256_set1_ps(gain);

	// Process the aligned portion 32 samples at a time
	while (nframes >= 32)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (32 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (32 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (32 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (32 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1, s2, s3;
		__m256 d0, d1, d2, d3;

		// Load sources
		s0 = _mm256_load_ps(src + 0 );
		s1 = _mm256_load_ps(src + 8 );
		s2 = _mm256_load_ps(src + 16);
		s3 = _mm256_load_ps(src + 24);

		// Load destinations
		d0 = _mm256_load_ps(dst + 0 );
		d1 = _mm256_load_ps(dst + 8 );
		d2 = _mm256_load_ps(dst + 16);
		d3 = _mm256_load_ps(dst + 24);

		// src = src * gain
		s0 = _mm256_mul_ps(vgain, s0);
		s1 = _mm256_mul_ps(vgain, s1);
		s2 = _mm256_mul_ps(vgain, s2);
		s3 = _mm256_mul_ps(vgain, s3);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);
		d2 = _mm256_add_ps(d2, s2);
		d3 = _mm256_add_ps(d3, s3);

		// Store result
		_mm256_store_ps(dst + 0 , d0);
		_mm256_store_ps(dst + 8 , d1);
		_mm256_store_ps(dst + 16, d2);
		_mm256_store_ps(dst + 24, d3);

		// Update pointers and counters
		src += 32;
		dst += 32;
		nframes -= 32;
	}

	// Process the remaining samples 16 at a time
	while (nframes >= 16)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (16 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (16 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (16 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (16 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1;
		__m256 d0, d1;

		// Load sources
		s0 = _mm256_load_ps(src + 0);
		s1 = _mm256_load_ps(src + 8);

		// Load destinations
		d0 = _mm256_load_ps(dst + 0);
		d1 = _mm256_load_ps(dst + 8);

		// src = src * gain
		s0 = _mm256_mul_ps(vgain, s0);
		s1 = _mm256_mul_ps(vgain, s1);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);

		// Store result
		_mm256_store_ps(dst + 0, d0);
		_mm256_store_ps(dst + 8, d1);

		// Update pointers and counters
		src += 16;
		dst += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		__m256 s0, d0;
		// Load sources
		s0 = _mm256_load_ps(src + 0 );
		// Load destinations
		d0 = _mm256_load_ps(dst + 0 );
		// src = src * gain
		s0 = _mm256_mul_ps(vgain, s0);
		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		// Store result
		_mm256_store_ps(dst, d0);
		// Update pointers and counters
		src += 8;
		dst += 8;
		nframes -= 8;
	}


	// There's a penalty going from AVX mode to SSE mode. This can
	// be avoided by ensuring the CPU that rest of the routine is no
	// longer interested in the upper portion of the YMM register.

	_mm256_zeroupper(); // zeros the upper portion of YMM register

	if (nframes) {
		__m128 vgain = _mm_load_ss(&gain);
		__m128 vsrc;
		__m128 vdst;

		switch (nframes % 8) {
		case 0:
		default:
			// No more samples left, break out
			break;
		case 7:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 6:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 5:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 4:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 3:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 2:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 1:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vsrc = _mm_mul_ss(vgain, vsrc);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		}
	}
}


static void
x86_sse_avx_mix_buffers_no_gain_unaligned(float *dst, const float *src, uint32_t nframes)
{
	// Process portion 32 samples at a time
	while (nframes >= 32)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (32 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (32 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (32 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (32 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1, s2, s3;
		__m256 d0, d1, d2, d3;

		// Load sources
		s0 = _mm256_loadu_ps(src + 0 );
		s1 = _mm256_loadu_ps(src + 8 );
		s2 = _mm256_loadu_ps(src + 16);
		s3 = _mm256_loadu_ps(src + 24);

		// Load destinations
		d0 = _mm256_loadu_ps(dst + 0 );
		d1 = _mm256_loadu_ps(dst + 8 );
		d2 = _mm256_loadu_ps(dst + 16);
		d3 = _mm256_loadu_ps(dst + 24);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);
		d2 = _mm256_add_ps(d2, s2);
		d3 = _mm256_add_ps(d3, s3);

		// Store result
		_mm256_storeu_ps(dst + 0 , d0);
		_mm256_storeu_ps(dst + 8 , d1);
		_mm256_storeu_ps(dst + 16, d2);
		_mm256_storeu_ps(dst + 24, d3);

		// Update pointers and counters
		src += 32;
		dst += 32;
		nframes -= 32;
	}

	// Process the remaining samples 16 at a time
	while (nframes >= 16)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (16 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (16 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (16 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (16 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1;
		__m256 d0, d1;

		// Load sources
		s0 = _mm256_loadu_ps(src + 0);
		s1 = _mm256_loadu_ps(src + 8);

		// Load destinations
		d0 = _mm256_loadu_ps(dst + 0);
		d1 = _mm256_loadu_ps(dst + 8);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);

		// Store result
		_mm256_storeu_ps(dst + 0, d0);
		_mm256_storeu_ps(dst + 8, d1);

		// Update pointers and counters
		src += 16;
		dst += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		__m256 s0, d0;
		// Load sources
		s0 = _mm256_loadu_ps(src);
		// Load destinations
		d0 = _mm256_loadu_ps(dst);
		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		// Store result
		_mm256_storeu_ps(dst, d0);
		// Update pointers and counters
		src+= 8;
		dst += 8;
		nframes -= 8;
	}

	// There's a penalty going away from AVX mode to SSE mode. This can
	// be avoided by ensuring the CPU that rest of the routine is no
	// longer interested in the upper portion of the YMM register.

	_mm256_zeroupper(); // zeros the upper portion of YMM register

	if (nframes) {
		__m128 vsrc;
		__m128 vdst;

		switch (nframes % 8) {
		case 0:
		default:
			// No more samples left, break out
			break;
		case 7:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 6:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 5:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 4:
			vsrc = _mm_loadu_ps(src);
			vdst = _mm_loadu_ps(dst);
			vdst = _mm_add_ps(vdst, vsrc);
			_mm_storeu_ps(dst, vdst);
			src += 4;
			dst += 4;
			nframes -= 4;
			break;
		case 3:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 2:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 1:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		}
	}
}

static void
x86_sse_avx_mix_buffers_no_gain_aligned(float *dst, const float *src, uint32_t nframes)
{
	// Process the aligned portion 32 samples at a time
	while (nframes >= 32)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (32 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (32 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (32 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (32 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1, s2, s3;
		__m256 d0, d1, d2, d3;

		// Load sources
		s0 = _mm256_load_ps(src + 0 );
		s1 = _mm256_load_ps(src + 8 );
		s2 = _mm256_load_ps(src + 16);
		s3 = _mm256_load_ps(src + 24);

		// Load destinations
		d0 = _mm256_load_ps(dst + 0 );
		d1 = _mm256_load_ps(dst + 8 );
		d2 = _mm256_load_ps(dst + 16);
		d3 = _mm256_load_ps(dst + 24);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);
		d2 = _mm256_add_ps(d2, s2);
		d3 = _mm256_add_ps(d3, s3);

		// Store result
		_mm256_store_ps(dst + 0 , d0);
		_mm256_store_ps(dst + 8 , d1);
		_mm256_store_ps(dst + 16, d2);
		_mm256_store_ps(dst + 24, d3);

		// Update pointers and counters
		src += 32;
		dst += 32;
		nframes -= 32;
	}

	// Process the remaining samples 16 at a time
	while (nframes >= 16)
	{
#if defined(COMPILER_MSVC) || defined(COMPILER_MINGW)
		_mm_prefetch(((char *)dst + (16 * sizeof(float))), _mm_hint(0));
		_mm_prefetch(((char *)src + (16 * sizeof(float))), _mm_hint(0));
#else
		__builtin_prefetch(src + (16 * sizeof(float)), 0, 0);
		__builtin_prefetch(dst + (16 * sizeof(float)), 0, 0);
#endif
		__m256 s0, s1;
		__m256 d0, d1;

		// Load sources
		s0 = _mm256_load_ps(src + 0);
		s1 = _mm256_load_ps(src + 8);

		// Load destinations
		d0 = _mm256_load_ps(dst + 0);
		d1 = _mm256_load_ps(dst + 8);

		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		d1 = _mm256_add_ps(d1, s1);

		// Store result
		_mm256_store_ps(dst + 0, d0);
		_mm256_store_ps(dst + 8, d1);

		// Update pointers and counters
		src += 16;
		dst += 16;
		nframes -= 16;
	}

	// Process the remaining samples 8 at a time
	while (nframes >= 8) {
		__m256 s0, d0;
		// Load sources
		s0 = _mm256_load_ps(src + 0 );
		// Load destinations
		d0 = _mm256_load_ps(dst + 0 );
		// dst = dst + src
		d0 = _mm256_add_ps(d0, s0);
		// Store result
		_mm256_store_ps(dst, d0);
		// Update pointers and counters
		src += 8;
		dst += 8;
		nframes -= 8;
	}

	// There's a penalty going from AVX mode to SSE mode. This can
	// be avoided by ensuring the CPU that rest of the routine is no
	// longer interested in the upper portion of the YMM register.

	_mm256_zeroupper(); // zeros the upper portion of YMM register

	if (nframes) {
		__m128 vsrc;
		__m128 vdst;

		switch (nframes % 8) {
		case 0:
		default:
			// No more samples left, break out
			break;
		case 7:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 6:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 5:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 4:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 3:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 2:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		case 1:
			vsrc = _mm_load_ss(src);
			vdst = _mm_load_ss(dst);
			vdst = _mm_add_ss(vdst, vsrc);
			_mm_store_ss(dst, vdst);
			++src;
			++dst;
			--nframes;
		}
	}
}

/**
 * @brief Compute the absolute value of a packed float 8x register
 * @param x Packed float 8x register
 * @return __m256 Absolute value
 */
static inline __m256 avx_abs_ps(__m256 x)
{
	const __m256 abs_mask = _mm256_set1_ps(-0.0F);
	return _mm256_andnot_ps(abs_mask, x);
}

/**
 * @brief Get the maximum value of packed float register
 * @param vmax Packed float 8x register
 * @return float Maximum value
 */
static inline float avx_getmax_ps(__m256 vmax)
{
	__m256 tmp;
	tmp = _mm256_shuffle_ps(vmax, vmax, _MM_SHUFFLE(2, 3, 0, 1));
	vmax = _mm256_max_ps(tmp, vmax);
	tmp = _mm256_shuffle_ps(vmax, vmax, _MM_SHUFFLE(1, 0, 3, 2));
	vmax = _mm256_max_ps(tmp, vmax);
	tmp = _mm256_permute2f128_ps(vmax, vmax, 1);
	vmax = _mm256_max_ps(tmp, vmax);
	return _mm256_cvtss_f32(vmax);
}

/**
 * @brief Get the minimum value of packed float register
 * @param vmax Packed float 8x register
 * @return float Minimum value
 */
static inline float avx_getmin_ps(__m256 vmin)
{
	__m256 tmp;
	tmp = _mm256_shuffle_ps(vmin, vmin, _MM_SHUFFLE(2, 3, 0, 1));
	vmin = _mm256_min_ps(tmp, vmin);
	tmp = _mm256_shuffle_ps(vmin, vmin, _MM_SHUFFLE(1, 0, 3, 2));
	vmin = _mm256_min_ps(tmp, vmin);
	tmp = _mm256_permute2f128_ps(vmin, vmin, 1);
	vmin = _mm256_min_ps(tmp, vmin);
	return _mm256_cvtss_f32(vmin);
}
