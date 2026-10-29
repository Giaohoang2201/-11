/*
 * 과제 11: 삽입 정렬과 쉘 정렬의 성능 비교
 *
 * 컴파일: gcc -O2 -o sort_compare sort_compare.c
 * 실행  : ./sort_compare            (과제 요구 출력 형식)
 *         ./sort_compare --gaps     (추가: 가장 큰 n에서 간격별 비교 횟수 출력)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_SIZES 6
#define SEED 20261029ULL   /* 재현 가능한 실험을 위한 고정 시드 */

static const int SIZES[NUM_SIZES] = {1000, 2000, 4000, 8000, 16000, 20000};

/* 플랫폼에 상관없이 같은 데이터를 만들기 위한 xorshift64 난수 생성기 */
static unsigned long long rng_state = SEED;
static unsigned long long next_rand(void) {
    unsigned long long x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

/* 간격 h로 삽입 정렬을 수행하고 데이터 값 비교 횟수를 반환한다. */
static long long gap_insertion_sort(int *a, int n, int h) {
    long long cnt = 0;
    for (int i = h; i < n; i++) {
        int key = a[i];
        int j = i - h;
        while (j >= 0) {
            cnt++;                      /* 데이터 값 비교 (a[j] > key) 1회 */
            if (a[j] > key) {
                a[j + h] = a[j];
                j -= h;
            } else {
                break;
            }
        }
        a[j + h] = key;
    }
    return cnt;
}

/* [1] 삽입 정렬 (간격 1) */
static long long insertion_sort(int *a, int n) {
    return gap_insertion_sort(a, n, 1);
}

/* [2] 쉘 정렬 - 기본 간격: n/2, n/4, ..., 1 */
static long long shell_sort_original(int *a, int n, int verbose) {
    long long total = 0;
    for (int h = n / 2; h >= 1; h /= 2) {
        long long c = gap_insertion_sort(a, n, h);
        if (verbose) printf("  h=%6d  comps=%8lld  (%.2f per element)\n", h, c, (double)c / n);
        total += c;
    }
    return total;
}

/* [3] 쉘 정렬 - Knuth 간격: h(1)=1, h(k+1)=3h(k)+1 (n보다 작은 값, 큰 값부터 적용) */
static long long shell_sort_knuth(int *a, int n, int verbose) {
    int gaps[64];
    int k = 0;
    for (int h = 1; h < n; h = 3 * h + 1) gaps[k++] = h;

    long long total = 0;
    for (int i = k - 1; i >= 0; i--) {
        long long c = gap_insertion_sort(a, n, gaps[i]);
        if (verbose) printf("  h=%6d  comps=%8lld  (%.2f per element)\n", gaps[i], c, (double)c / n);
        total += c;
    }
    return total;
}

/* 오름차순 정렬 여부 확인 */
static int is_sorted(const int *a, int n) {
    for (int i = 0; i + 1 < n; i++)
        if (a[i] > a[i + 1]) return 0;
    return 1;
}

int main(int argc, char **argv) {
    int show_gaps = (argc > 1 && strcmp(argv[1], "--gaps") == 0);

    for (int s = 0; s < NUM_SIZES; s++) {
        int n = SIZES[s];
        int *original = malloc(sizeof(int) * n);
        int *work = malloc(sizeof(int) * n);
        if (!original || !work) {
            fprintf(stderr, "메모리 할당 실패\n");
            return 1;
        }

        /* 원본 데이터 생성 (0 ~ 1,000,000) - 측정 대상 아님 */
        for (int i = 0; i < n; i++) original[i] = (int)(next_rand() % 1000001ULL);

        /* --gaps 모드는 가장 큰 n에서만 간격별 결과를 보여준다 */
        int verbose = show_gaps && (s == NUM_SIZES - 1);
        if (show_gaps && !verbose) { free(original); free(work); continue; }

        long long c_ins, c_org, c_knu;

        memcpy(work, original, sizeof(int) * n);          /* 복사 - 측정 제외 */
        c_ins = insertion_sort(work, n);
        if (!is_sorted(work, n)) { fprintf(stderr, "삽입 정렬 오류 (n=%d)\n", n); return 1; }

        memcpy(work, original, sizeof(int) * n);
        if (verbose) printf("Original (n=%d)\n", n);
        c_org = shell_sort_original(work, n, verbose);
        if (!is_sorted(work, n)) { fprintf(stderr, "쉘 정렬(기본) 오류 (n=%d)\n", n); return 1; }

        memcpy(work, original, sizeof(int) * n);
        if (verbose) printf("Knuth (n=%d)\n", n);
        c_knu = shell_sort_knuth(work, n, verbose);
        if (!is_sorted(work, n)) { fprintf(stderr, "쉘 정렬(Knuth) 오류 (n=%d)\n", n); return 1; }

        printf("Data Size : %d\n\n", n);
        printf("Insertion Sort       : %lld comparisons\n", c_ins);
        printf("Shell Sort (Original): %lld comparisons\n", c_org);
        printf("Shell Sort (Knuth)   : %lld comparisons\n\n", c_knu);

        free(original);
        free(work);
    }

    if (!show_gaps) printf("모든 정렬 결과가 오름차순으로 올바르게 정렬되었습니다.\n");
    return 0;
}
