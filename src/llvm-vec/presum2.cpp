/*
 * prescan.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: teng
 */

#include <immintrin.h>
#include <omp.h>
#include <iostream>
using namespace std;


__m128 scan_SSE(__m128 x) {
    x = _mm_add_ps(x, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(x), 4)));
    x = _mm_add_ps(x, _mm_shuffle_ps(_mm_setzero_ps(), x, 0x40));
    return x;
}

float pass1_SSE(float *a, float *s, const int n) {
    __m128 offset = _mm_setzero_ps();
    #pragma omp for schedule(static) nowait
    for (int i = 0; i < n / 4; i++) {
        __m128 x = _mm_load_ps(&a[4 * i]);
        __m128 out = scan_SSE(x);
        out = _mm_add_ps(out, offset);
        _mm_store_ps(&s[4 * i], out);
        offset = _mm_shuffle_ps(out, out, _MM_SHUFFLE(3, 3, 3, 3));
    }
    float tmp[4];
    _mm_store_ps(tmp, offset);
    return tmp[3];
}

void pass2_SSE(float *s, __m128 offset, const int n) {
    #pragma omp for schedule(static)
    for (int i = 0; i<n/4; i++) {
        __m128 tmp1 = _mm_load_ps(&s[4 * i]);
        tmp1 = _mm_add_ps(tmp1, offset);
        _mm_store_ps(&s[4 * i], tmp1);
    }
}

void scan_omp_SSEp2_SSEp1_chunk(float a[], float s[], int n) {
    float *suma;
    const int chunk_size = 1<<18;
    const int nchunks = n%chunk_size == 0 ? n / chunk_size : n / chunk_size + 1;
    //printf("nchunks %d\n", nchunks);
    #pragma omp parallel
    {
        const int ithread = omp_get_thread_num();
        const int nthreads = omp_get_num_threads();

        #pragma omp single
        {
            suma = new float[nthreads + 1];
            suma[0] = 0;
        }

        float offset2 = 0.0f;
        for (int c = 0; c < nchunks; c++) {
            const int start = c*chunk_size;
            const int chunk = (c + 1)*chunk_size < n ? chunk_size : n - c*chunk_size;
            suma[ithread + 1] = pass1_SSE(&a[start], &s[start], chunk);
            #pragma omp barrier
            #pragma omp single
            {
                float tmp = 0;
                for (int i = 0; i < (nthreads + 1); i++) {
                    tmp += suma[i];
                    suma[i] = tmp;
                }
            }
            __m128 offset = _mm_set1_ps(suma[ithread]+offset2);
            pass2_SSE(&s[start], offset, chunk);
            #pragma omp barrier
            offset2 = s[start + chunk-1];
        }
    }
    delete[] suma;
}
