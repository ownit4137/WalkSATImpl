#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"

const int DSIZE = 64/sizeof(DTYPE);

extern "C" {

void mm(DTYPE *A, hls::vector<DTYPE, DSIZE> *B, hls::vector<DTYPE, DSIZE> *AB, int N) {
#pragma HLS INTERFACE mode=m_axi bundle=m0 port=A 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=B 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=AB 
	
	DTYPE AB_block[M][M];
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=AB_block
	DTYPE Bj[M];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=Bj

	
	// block mm start
	// block ij
	for(int ib = 0; ib < N/M; ib++) {
	for(int jb = 0; jb < N/M; jb++) {
		
	
	// init loop
	for(int i = 0; i < M; i++) {
#pragma HLS pipeline II = 1
	for(int j = 0; j < M; j++) {
#pragma HLS unroll
		AB_block[i][j] = 0;
	}}

	
	
	// inside block
	for(int kb = 0; kb < N/M; kb++) {
	for(int k = 0; k < M; k++) {
	
		
		// load b line
		for(int jj = 0; jj < M/DSIZE; jj++) {
#pragma HLS pipeline II = 1
			hls::vector<DTYPE, DSIZE> B_temp = B[((kb*M+k)*N+jb*M)/DSIZE + jj];
		for(int j = 0; j < DSIZE; j++) {
#pragma HLS unroll
			Bj[jj*DSIZE + j] = B_temp[j];
		}}
		
		
		// main cal loop
		for(int i = 0; i < M; i++) {
#pragma HLS pipeline II=1
			DTYPE Ai =  A[((ib*M+i)*N+kb*M)+k];
			for(int j = 0; j < M; j++) {
#pragma HLS unroll
				AB_block[i][j] += Ai * Bj[j];
			}
		}
	}}
	// inside block end

	
	// write
	for(int i = 0; i < M; i++) {
	for(int jj = 0; jj < M/DSIZE; jj++) {
#pragma HLS pipeline II = 1
		hls::vector<DTYPE, DSIZE> AB_temp;
		for(int j = 0; j < DSIZE; j++) {
#pragma HLS unroll
			AB_temp[j] = AB_block[i][jj * DSIZE + j];
		}
		AB[((ib*M+i)*N+jb*M)/DSIZE + jj] = AB_temp;
	}}
	
	
	}}
	// block ij end
}}



/*

[ver1]

void mm(DTYPE *A, DTYPE *B, DTYPE *AB, int N) {

		// load b line
		for(int j = 0; j < M; j++) {
#pragma HLS pipeline II = 1
			DTYPE B_temp = B[(kb*M+k)*N+jb*M+j];
			Bj[j] = B_temp;
		}




// write
	for(int i = 0; i < M; i++) {
	for(int j = 0; j < M; j++) {
#pragma HLS pipeline II = 1
		AB[(ib*M+i)*N+jb*M+j] = AB_block[i][j];
	}}








*/
