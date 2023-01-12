#include "hls_vector.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "mm.h"

const int DSIZE = 64/sizeof(DTYPE);

extern "C" {

void mm(hls::vector<DTYPE, DSIZE> *At, hls::vector<DTYPE, DSIZE> *B, hls::vector<DTYPE, DSIZE> *AB, int N) {
#pragma HLS INTERFACE mode=m_axi bundle=m0 port=At 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=B 
#pragma HLS INTERFACE mode=m_axi bundle=m1 port=AB 
	
	DTYPE AB_block[M][M];
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=AB_block
	DTYPE At_line[M];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=At_line
	DTYPE B_line[M];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=B_line

	
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
		
			
		// load B line
		for(int jj = 0; jj < M/DSIZE; jj++) {
#pragma HLS pipeline II = 1
			hls::vector<DTYPE, DSIZE> B_temp = B[((kb*M+k)*N+jb*M)/DSIZE + jj];
		for(int j = 0; j < DSIZE; j++) {
#pragma HLS unroll
			B_line[jj*DSIZE + j] = B_temp[j];
		}}
		
		
		// load At line
		for(int ii = 0; ii < M/DSIZE; ii++) {
#pragma HLS pipeline II = 1
			hls::vector<DTYPE, DSIZE> At_temp = At[((kb*M+k)*N+ib*M)/DSIZE + ii];
		for(int i = 0; i < DSIZE; i++) {
#pragma HLS unroll
			At_line[ii*DSIZE + i] = At_temp[i];
		}}
		
					
		// main cal loop
		for(int i = 0; i < M; i++) {
#pragma HLS pipeline II=1			
		for(int j = 0; j < M; j++) {
#pragma HLS unroll
			AB_block[i][j] += At_line[i] * B_line[j];
		}}
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
