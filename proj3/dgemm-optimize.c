#include<emmintrin.h>
int minimum(int i, int j) {
	if (i < j) {
		return i;
	}
	else {
		return j;
	}
}

void dgemm(int m, int n, float *A, float *C)
{
	// FILL-IN 
		//cache blocking
	//int i, j, k, kk, jj;
	//double sum;
	//int block = 16;//lower block size gives a faster result for some reason may be to small array size
	//int size = m * n;
	//for (i = 0; i < m; i++)
	//	for (j = 0; j < m; j++)// only works if these r both m for some reason
	//		C[i + j * m] = 0.0;

	//for (kk = 0; kk < n; kk += block) {
	//	for (jj = 0; jj < m; jj += block) {
	//		for (i = 0; i < m; i++) {
	//			for (k = kk; k < minimum(n, kk + block); k++) {
	//				for (j = jj; j < minimum(m, jj + block); j++) {//maybe add-1
	//					C[i + j * m] += A[i + k * m] * A[j + k * m];
	//				}
	//			}
	//		}
	//	}
	//}


	//unroll
	// remove or reduce iterations to improve speed of loop
	//going to unroll the naive function
	//int k;
	//float z;
	//for (int i = 0; i < m; i++) {
	//	for (int j = 0; j < n; j++) {

	//		k = 0;
	//		z = A[i + j * m];

	//		if (m % 4 != 0) {// i want it to unroll 4 times
	//			switch (m % 4) {
	//			case 3:
	//				C[i + 2 * m] += z * A[2 + j * m];
	//			case 2:
	//				C[i + 1 * m] += z * A[1 + j * m];
	//			default:
	//				C[i] += z * A[j*m];
	//			}
	//			k = m % 4;
	//		}
	//		do{
	//			C[i + k * m] += z * A[k + j * m];
	//			C[i + (k + 1) * m] += z * A[(k + 1) + j * m];
	//			C[i + (k + 2) * m] += z * A[(k + 2) + j * m];
	//			C[i + (k + 3) * m] += z * A[(k + 3) + j * m];
	//			k += 4;
	//		} while (k < m);
	//	}
	//}

	//sse
	__m128 a1, a2, result, c, add;
	for (int k = 0; k < n; k++)
		for (int j = 0; j < m; j++)
			for (int i = 0; i < m; i++)
			{

				a1 = _mm_load_ss(A + (i + k * m));
				a2 = _mm_load_ss(A + (j + k * m));
				c = _mm_load_ss(C + (i + j * m));
				result = _mm_mul_ss(a1, a2);
				add = _mm_add_ss(c, result);
				_mm_store_ss(C + (i + j * m), add);
			}

}
	//sse2
			//__m128 a1, a2, result,c,add;
			//int i, j, k, k2, j2;
			//double sum;
			//int block = 32;//lower block size gives a faster result for some reason may be to small array size
			//for (k2 = 0; k2 < n; k2 += block) {
			//	int min1 = minimum(n, k2 + block);
			//	for (j2 = 0; j2 < m; j2 += block) {
			//	int 	min2=minimum(m, j2 + block);
			//		for (i = 0; i < m; i++) {
			//			for (k = k2; k < min1; k++) {
			//				a1 = _mm_load_ss(A + (i + k * m));

			//				for (j = j2; j < min2; j++) {//maybe add-1
			//					a2 = _mm_load_ss(A + (j + k * m));
			//					c = _mm_load_ss(C + (i + j * m));
			//					result = _mm_mul_ss(a1, a2);
			//					add = _mm_add_ss(c, result);
			//					_mm_store_ss(C + (i + j * m), add);
			//				}
			//			}
			//		}
			//	}
			//}

	////reordering
	//	for (int k = 0; k < n; k++)
	//		for (int j = 0; j < m; j++)
	//			for (int i = 0; i < m; i++)

	//			C[i + j * m] += A[i + k * m] * A[j + k * m];


