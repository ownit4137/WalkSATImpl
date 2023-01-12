typedef short DTYPE;
const int M = 256;

extern "C"{
void vadd(DTYPE* A, DTYPE* B, DTYPE* AB, int N){
#pragma HLS INTERFACE mode=m_axi bundle=m0 port=A
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=B
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=AB

	
	DTYPE AB_block[M][M];
	DTYPE B_line[M];
#pragma HLS ARRAY_PARTITION variable=AB_block type=complete dim=2
#pragma HLS ARRAY_PARTITION variable=B_line type=complete dim=1

	
	for (int ib = 0; ib < N / M; ib++){
	for (int jb = 0; jb < N / M; jb++){
		// init AB_block
		for (int i = 0; i < M; i++){
			for(int j = 0; j < M; j++){
				AB_block[i][j] = 0;
			}
		}
	for (int kb = 0; kb < N / M; kb++){
		for (int k = 0; k < M; k++){
			for (int j = 0; j < M; j++){
				B_line[j] = B[(kb * M + k) * N + jb * M + j];
			}
			i_loop: for (int i = 0; i < M; i++){
#pragma HLS PIPELINE II=1
				DTYPE Atemp = A[(ib * M + i) * N + kb * M + k];
				j_loop: for (int j = 0; j < M; j++){
#pragma HLS unroll
					AB_block[i][j] += Atemp * B_line[j];
				}
			}
		}
	}
	// AB_block to AB
	for (int i = 0; i < M; i++){
		for(int j = 0; j < M; j++){
			AB[(ib * M + i) * N + jb * M + j] = AB_block[i][j];
		}
	}

	// ib jb loop
	}
	}
}
}



//extern "C"{
//
//void mm(hls::vector<DTYPE, DSIZE>* At, hls::vector<DTYPE, DSIZE>* B, hls::vector<DTYPE, DSIZE>* AB, int N){
//	
//	
//	// pragma m_axi
//
//	DTYPE AB_block[M][M];
//	DTYPE B_line[M];
//#pragma HLS ARRAY_PARTITION variable=AB_block type=complete dim=2
//#pragma HLS ARRAY_PARTITION variable=B_line type=complete dim=1
//
//	for (int ib = 0; ib < N / M; ib++){
//	for (int jb = 0; jb < N / M; jb++){
//		// init AB_block
//		for (int i = 0; i < M; i++){
//			for(int j = 0; j < M; j++){
//				AB_block[i][j] = 0;
//			}
//		}
//	for (int kb = 0; kb < N / M; kb++){
//		for (int k = 0; k < M; k++){
//			for (int j = 0; j < M; j++){
//				B_line[j] = B[(kb * M + k) * N + jb * M + j];
//			}
//			i_loop: for (int i = 0; i < M; i++){
//#pragma HLS PIPELINE II=1
//				DTYPE Atemp = A[(ib * M + i) * N + kb * M + k];
//				j_loop: for (int j = 0; j < M; j++){
//#pragma HLS unroll
//					AB_block[i][j] += Atemp * B_line[j];
//				}
//			}
//		}
//	}
//	// AB_block to AB
//	for (int i = 0; i < M; i++){
//		for(int j = 0; j < M; j++){
//			AB[(ib * M + i) * N + jb * M + j] = AB_block[i][j];
//		}
//	}
//
//	// ib jb loop
//	}
//	}
//}
//}


//
//#pragma HLS INTERFACE mode=m_axi bundle=m0 port=At
//#pragma HLS INTERFACE mode=m_axi bundle=m1 port=B
//#pragma HLS INTERFACE mode=m_axi bundle=m1 port=AB
//	
//#pragma HLS DATAFLOW
//	hls::stream<hls::vector<DTYPE, DSIZE>> AStreamWide("AStreamWide");
//	hls::stream<DTYPE> AStream("AStream");
//	hls::stream<hls::vector<DTYPE, DSIZE>> BStream("BStream");
//	hls::stream<hls::vector<DTYPE, DSIZE>> ABStream("ABStream");
//	
//	ReadAt(At, AStreamWide, M);
//	ChangeA_Rate(AStreamWide, AStream, M);
//	ReadB(B, BStream, M);
//	Comp(AStream, BStream, ABStream, M);
//	WriteAB(ABStream, AB, M);