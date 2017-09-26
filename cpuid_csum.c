/*
 *  Checksum benchmark
 *
 *  Copyright (c) 2017 Alexander Graf
 *
 *  SPDX-License-Identifier:     GPL-2.0+
 */

#include "tsc.h"
#include <immintrin.h>
#include <string.h>

/*
 * The checksum calculation functions in this file were taken from
 *
 *  https://www.klittlepage.com/2013/12/10/accelerated-fix-processing-via-avx2-vector-instructions/
 */

#ifdef __AVX2__
#define CSUM avxChecksum
#else
#define CSUM naiveChecksum
#endif

/*
A baseline, cannonically correct FIX checksum calculation.
*/
int CSUM(const char * const target, size_t targetLength) {
	unsigned int checksum = 0;
	size_t i;

	for (i = 0; i < targetLength; ++i) {
		checksum += (unsigned int) target[i];
	}
	return checksum % 256;
}


#ifdef __AVX2__
/*
A modified version of avxChecksumV1 in which the sum is carried out, in 
parallel across 8 32-bit words on each vector pass. The results are folded 
together at the end to obtain the final checksum. Note that this implementation 
is not canonically correct w.r.t overflow behavior but no reasonable length 
FIX message will ever create a problem.
*/
int avxChecksumV2(const char * const target, size_t targetLength) {
	const __m256i zeroVec = _mm256_setzero_si256();
	const __m256i oneVec = _mm256_set1_epi16(1);
	__m256i accum = _mm256_setzero_si256();
	unsigned int checksum = 0;
	size_t offset = 0;

	if(targetLength >= 32) {
		for(; offset <= targetLength - 32; offset += 32) {
			__m256i vec = _mm256_loadu_si256(
			(const __m256i*)(target + offset));
			__m256i vl = _mm256_unpacklo_epi8(vec, zeroVec);
			__m256i vh = _mm256_unpackhi_epi8(vec, zeroVec);
			// There's no "add and unpack" instruction but multiplying by 
			// one has the same effect and gets us unpacking from 16-bits to 
			// 32 bits for free.
			accum = _mm256_add_epi32(accum, _mm256_madd_epi16(vl, oneVec));
			accum = _mm256_add_epi32(accum, _mm256_madd_epi16(vh, oneVec));
		}
	}

	for(; offset < targetLength; ++offset) {
		checksum += (unsigned int) target[offset];
	}

	// We could accomplish the same thing with horizontal add instructions as 
	// we did above but shifts and vertical adds have much lower instruction 
	// latency.
	accum = _mm256_add_epi32(accum, _mm256_srli_si256(accum, 4));
	accum = _mm256_add_epi32(accum, _mm256_srli_si256(accum, 8));
	return (_mm256_extract_epi32(accum, 0) + _mm256_extract_epi32(accum, 4) + 
		checksum) % 256;
}
#endif
