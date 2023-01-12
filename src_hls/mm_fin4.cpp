#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"

const int DSIZE = 64/sizeof(DTYPE);

extern "C" {


/*
 * ReadAt
 */
void ReadAt(hls::vector<DTYPE, DSIZE> *At, 
		hls::stream<hls::vector<DTYPE, DSIZE>>& AStreamWide, int N){
	//At -> AStreamWide
	for(int ib = 0; ib < N/M; ib++) {
	for(int jb = 0; jb < N/M; jb++) {
	for(int kb = 0; kb < N/M; kb++) {
	for(int k = 0; k < M; k++) {
#pragma HLS pipeline II = 1
	for(int ii = 0; ii < M / DSIZE; ii++) {
#pragma HLS unroll
		AStreamWide.write(At[((kb*M+k)*N+ib*M)/DSIZE + ii]);
	}}}}}
}



/*
 * ReadB
 */
void ReadB(hls::vector<DTYPE, DSIZE> *B, 
		hls::stream<hls::vector<DTYPE, DSIZE>>& BStream, int N){
	// B -> BStream
	for(int ib = 0; ib < N/M; ib++) {
	for(int jb = 0; jb < N/M; jb++) {
	for(int kb = 0; kb < N/M; kb++) {
	for(int k = 0; k < M; k++) {
#pragma HLS pipeline II = 1
	for(int jj = 0; jj < M / DSIZE; jj++) {
#pragma HLS unroll
		BStream.write(B[((kb*M+k)*N+jb*M)/DSIZE + jj]);
	}}}}}
}



/*
 * WriteAB
 */
void WriteAB(hls::stream<hls::vector<DTYPE, DSIZE>>& ABStream, 
		hls::vector<DTYPE, DSIZE> *AB, int N){
	// ABStream -> AB
	for(int ib = 0; ib < N / M; ib++){
	for(int jb = 0; jb < N / M; jb++){
	for(int i = 0; i < M; i++) {
#pragma HLS pipeline II = 1
	for(int j = 0; j < M/DSIZE; j++) {
#pragma HLS unroll
		AB[((ib*M+i)*N+jb*M)/DSIZE + j] = ABStream.read();
	}}}}
}



/*
 * ChangeA_Rate
 */
void ChangeA_Rate(hls::stream<hls::vector<DTYPE, DSIZE>>& AStreamWide, hls::stream<DTYPE>& AStream, int N){
	for(int ib = 0; ib < N/M; ib++){
	for(int jb = 0; jb < N/M; jb++){
	for(int kb = 0; kb < N/M; kb++){
		for(int k = 0; k < M; k++){
		for(int ii = 0; ii < M/DSIZE; ii++){
#pragma HLS pipeline II = 1
			hls::vector<DTYPE, DSIZE> A_temp = AStreamWide.read();
			for(int i = 0; i < DSIZE; i++){
#pragma HLS pipeline II=1;
				AStream.write(A_temp[i]);
			}
		}}
	}}}
}



/*
 * Comp
 */
void Comp(hls::stream<DTYPE>& AStream, 
		hls::stream<hls::vector<DTYPE, DSIZE>>& BStream, 
		hls::stream<hls::vector<DTYPE, DSIZE>>& ABStream, int N){
	DTYPE AB_block[M][M];
	#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=AB_block
	DTYPE At_line[M];
	#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=At_line
	DTYPE B_line[M];
	#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=B_line
	
	// Computation
	for(int ib = 0; ib < N/M; ib++) {
	for(int jb = 0; jb < N/M; jb++) {
		// init loop
		for(int i = 0; i < M; i++) {
#pragma HLS pipeline II = 1
		for(int j = 0; j < M; j++) {
#pragma HLS unroll
			AB_block[i][j] = 0;
		}}
		
	for(int kb = 0; kb < N/M; kb++) {
	for(int k = 0; k < M; k++) {
		
		// from Bstream vec(DSIZE) -> line #M
		for(int jj = 0; jj < M/DSIZE; jj++) {
#pragma HLS pipeline II = 1
			hls::vector<DTYPE, DSIZE> B_temp = BStream.read();
		for(int j = 0; j < DSIZE; j++) {
#pragma HLS unroll
			B_line[jj*DSIZE + j] = B_temp[j];
		}}
		
		// from Astream short -> line #M
		for(int ii = 0; ii < M; ii++){
#pragma HLS unroll
			At_line[ii] = AStream.read();
		}
		
		// main cal loop
		for(int i = 0; i < M; i++) {
#pragma HLS pipeline II=1
		for(int j = 0; j < M; j++) {
#pragma HLS unroll
			AB_block[i][j] += At_line[i] * B_line[j];
		}}
	}}
	
	// write matrix AB[M][M] -> ABStream vec(DSIZE)
	for(int i = 0; i < M; i++) {
		for(int jj = 0; jj < M/DSIZE; jj++) {
	#pragma HLS pipeline II = 1
			hls::vector<DTYPE, DSIZE> AB_temp;
			for(int j = 0; j < DSIZE; j++) {
	#pragma HLS unroll
				AB_temp[j] = AB_block[i][jj * DSIZE + j];
			}
			ABStream.write(AB_temp);
		}}
	}}
}



/*
 * mm
 */
void mm(hls::vector<DTYPE, DSIZE> *At, hls::vector<DTYPE, DSIZE> *B, 
		hls::vector<DTYPE, DSIZE> *AB, int N) {
#pragma HLS INTERFACE mode=m_axi bundle=m0 port=At 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=B 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=AB 
	
#pragma HLS DATAFLOW
	
	hls::stream<hls::vector<DTYPE, DSIZE>> AStreamWide("AStreamWide");
	hls::stream<DTYPE> AStream("AStream");
	hls::stream<hls::vector<DTYPE, DSIZE>> BStream("BStream");
	hls::stream<hls::vector<DTYPE, DSIZE>> ABStream("ABStream");
	
	ReadAt(At, AStreamWide, N);
	ChangeA_Rate(AStreamWide, AStream, N);
	ReadB(B, BStream, N);
	Comp(AStream, BStream, ABStream, N);
	WriteAB(ABStream, AB, N);
}
}