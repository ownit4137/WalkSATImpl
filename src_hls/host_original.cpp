#include "xcl2.hpp"
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#include "mm.h"

const int SIZE = 2048;

void mm_sw( std::vector<DTYPE, aligned_allocator<DTYPE> > A, std::vector<DTYPE, aligned_allocator<DTYPE> > B, std::vector<DTYPE, aligned_allocator<DTYPE> > & AB){
//void mm_sw( std::vector<DTYPE, aligned_allocator<DTYPE> > At, std::vector<DTYPE, aligned_allocator<DTYPE> > B, std::vector<DTYPE, aligned_allocator<DTYPE> > & AB){

#pragma omp parallel
	{
		int tid = omp_get_thread_num();
		if( tid == 0 ){
			int nthreads = omp_get_num_threads();
			std::cout << "Running OpenMP with " << nthreads << " threads...\n";
		}
	}

	DTYPE sum = 0;
#pragma omp parallel for
	for(int i = 0; i < SIZE; i++){
		for(int j = 0; j<SIZE; j++){
			sum = 0;
			for(int k = 0; k < SIZE; k++){
				sum = sum + A[i*SIZE+k] * B[k*SIZE+j];
				//sum = sum + At[k*SIZE+i] * B[k*SIZE+j];
			}
			AB[i*SIZE+j] = sum;
		}
	}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }
    std::string binaryFile = argv[1];
    cl_int err;
    cl::Context context;
    cl::Kernel krnl_mm;
    cl::CommandQueue q;
    

	std::vector<DTYPE, aligned_allocator<DTYPE> > A(SIZE*SIZE); 
	//std::vector<DTYPE, aligned_allocator<DTYPE> > At(SIZE*SIZE); 
	std::vector<DTYPE, aligned_allocator<DTYPE> > B(SIZE*SIZE); 
	std::vector<DTYPE, aligned_allocator<DTYPE> > AB_sw(SIZE*SIZE); 
	std::vector<DTYPE, aligned_allocator<DTYPE> > AB_hw(SIZE*SIZE); 

	srand(time(NULL));

	for(int i = 0; i < SIZE; i++){
		for(int j = 0; j < SIZE; j++){
				A[i*SIZE+j] = rand() % 8;
				//At[i*SIZE+j] = rand() % 8;
				B[i*SIZE+j] = rand() % 8;

				AB_sw[i*SIZE+j] = 0;
				AB_hw[i*SIZE+j] = 0;
		}
	}
	printf("Done initializing vectors\n");

	
	
	std::cout << "Running SW MM...\n";
	mm_sw(A, B, AB_sw);
	//mm_sw(At, B, AB_sw);
	printf("Done\n");

	
	
	
    // OPENCL HOST CODE AREA START
    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    auto devices = xcl::get_xil_devices();
    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, krnl_mm = cl::Kernel(program, "mm", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }
    
    
    
    
    

    // Allocate Buffer in Global Memory
    // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
    // Device-to-host communication
    OCL_CHECK(err, cl::Buffer buffer_A(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(DTYPE)*SIZE*SIZE, A.data(), &err));
    //OCL_CHECK(err, cl::Buffer buffer_At(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(DTYPE)*SIZE*SIZE, At.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_B(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(DTYPE)*SIZE*SIZE, B.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_AB(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(DTYPE)*SIZE*SIZE, AB_hw.data(), &err));

    int matrix_size = SIZE;
    OCL_CHECK(err, err = krnl_mm.setArg(0, buffer_A));
    //OCL_CHECK(err, err = krnl_mm.setArg(0, buffer_At));
    OCL_CHECK(err, err = krnl_mm.setArg(1, buffer_B));
    OCL_CHECK(err, err = krnl_mm.setArg(2, buffer_AB));
    OCL_CHECK(err, err = krnl_mm.setArg(3, matrix_size));

	OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_A, buffer_B}, 0 /* 0 means from host*/));
	//OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_At, buffer_B}, 0 /* 0 means from host*/));
	q.finish();
	
	std::cout << "Running FPGA MM...\n";
	
	auto start = std::chrono::steady_clock::now();
	OCL_CHECK(err, err = q.enqueueTask(krnl_mm));
	q.finish();
	auto end = std::chrono::steady_clock::now();
	
	std::cout << "Done.\n";
	double exec_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	double gops = double(SIZE) * SIZE * SIZE * 2 / (exec_time);
	std::cout << "Time: " << exec_time*1e-9 << ", GOPS: " << gops << std::endl;
	
	
	OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_AB}, CL_MIGRATE_MEM_OBJECT_HOST));
	q.finish();


	int err_cnt = 0;
	for(int i = 0; i<SIZE; i++){
		for(int j = 0; j<SIZE; j++){
			if(AB_sw[i*SIZE+j] != AB_hw[i*SIZE+j]) {
				err_cnt++;
				if( err_cnt == 1 ){
					printf("i:%d j:%d sw:%d hw:%d\n", i, j, AB_sw[i*SIZE+j], AB_hw[i*SIZE+j] );
				}
			}
		}
	}

	if(err_cnt != 0){
		printf("FAILED! Error count : %d\n", err_cnt);
	}
	else{
		printf("PASSED!\n");
	}

	return EXIT_SUCCESS;
}



//#include "xcl2.hpp"
//#include <stdio.h>
//#include <stdlib.h>
//#include <algorithm>
//#include <vector>
//#define SIZE 512
//typedef short DTYPE;
//
//void mm_sw(std::vector<short, aligned_allocator<short>>& A, 
//			std::vector<short, aligned_allocator<short>>& B, 
//			std::vector<short, aligned_allocator<short> >& AB){
//	DTYPE sum = 0;
//
//	for(int i = 0; i < SIZE; i++){
//		for(int j = 0; j < SIZE; j++){
//			sum = 0;
//			for(int k = 0; k < SIZE; k++){
//				sum = sum + A[i * SIZE + k] * B[k * SIZE + j];
//			}
//			AB[i * SIZE + j] = sum;
//		}
//	}
//}
//
//int main(int argc, char** argv) {
//    if (argc != 2) {
//        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
//        return EXIT_FAILURE;
//    }
//
//    std::string binaryFile = argv[1];
//    size_t vector_size_bytes = sizeof(short) * SIZE * SIZE;
//    cl_int err;
//    cl::Context context;
//    cl::Kernel krnl_vector_add;
//    cl::CommandQueue q;
//    
//    
//    // Data Initialization
//    std::vector<short, aligned_allocator<short> > A(SIZE * SIZE);
//    std::vector<short, aligned_allocator<short> > B(SIZE * SIZE);
//    std::vector<short, aligned_allocator<short> > AB_hw(SIZE * SIZE);
//    std::vector<short, aligned_allocator<short> > AB_sw(SIZE * SIZE);
////    std::generate(A.begin(), A.end(), std::rand() % 8);
////    std::generate(B.begin(), B.end(), std::rand() % 8);
////    std::generate(AB_hw.begin(), AB_hw.end(), 0);
////    std::generate(AB_sw.begin(), AB_sw.end(), 0);
//    for(int i = 0; i < SIZE * SIZE; i++){
//    	int rn = rand() % 8;
//    	A[i] = rn;
//    	B[i] = rn;
//    	AB_hw[i] = 0;
//    	AB_sw[i] = 0;
//    }
//
//    
//    /**********************/
//    // OPENCL HOST CODE AREA START
//    // get_xil_devices() is a utility API which will find the xilinx
//    // platforms and will return list of devices connected to Xilinx platform
//    auto devices = xcl::get_xil_devices();
//    // read_binary_file() is a utility API which will load the binaryFile
//    // and will return the pointer to file buffer.
//    auto fileBuf = xcl::read_binary_file(binaryFile);
//    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
//    bool valid_device = false;
//    for (unsigned int i = 0; i < devices.size(); i++) {
//        auto device = devices[i];
//        // Creating Context and Command Queue for selected Device
//        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
//        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
//        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
//        cl::Program program(context, {device}, bins, nullptr, &err);
//        if (err != CL_SUCCESS) {
//            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
//        } else {
//            std::cout << "Device[" << i << "]: program successful!\n";
//            OCL_CHECK(err, krnl_vector_add = cl::Kernel(program, "vadd", &err));
//            valid_device = true;
//            break; // we break because we found a valid device
//        }
//    }
//    if (!valid_device) {
//        std::cout << "Failed to program any device found, exit!\n";
//        exit(EXIT_FAILURE);
//    }
//    /**********************/
//    
//    
//    
//    // Funtion Input
//    int size = SIZE;
//    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes, A.data(), &err));
//    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, vector_size_bytes, B.data(), &err));
//    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, vector_size_bytes, AB_hw.data(), &err));
//    OCL_CHECK(err, err = krnl_vector_add.setArg(0, buffer_in1));
//    OCL_CHECK(err, err = krnl_vector_add.setArg(1, buffer_in2));
//    OCL_CHECK(err, err = krnl_vector_add.setArg(2, buffer_output));
//    OCL_CHECK(err, err = krnl_vector_add.setArg(3, size));
//    
//    
//    
//    // Launch Kernel
//    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2}, 0 /* 0 means from host*/));
//    q.finish();
//
//    auto start = std::chrono::steady_clock::now();
//    OCL_CHECK(err, err = q.enqueueTask(krnl_vector_add));
//    auto end = std::chrono::steady_clock::now();
//    q.finish();
//    
//    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
//    q.finish();
//    
//   
//    
//    
//    // Compare the result
//    mm_sw(A, B, AB_sw);
//
//	int fail = 0;
//	for(int i = 0; i < SIZE * SIZE; i++){
//		if (AB_sw[i] != AB_hw[i]){
//			fail = 1;
//		}
//	}
//
//	if (fail == 1) {
//		printf("Failed\n");
//	}
//	else {
//		printf("Passed\n");
//	}
//	
//	double exec_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//	double gops = double(SIZE) * SIZE * SIZE * 2 / (exec_time);
//	printf("Time: %f, GOP: %f\n", exec_time * 1e-9, gops);
//    return 0;
//}

//DTYPE A[SIZE][SIZE], B[SIZE][SIZE], AB_sw[SIZE][SIZE], AB_hw[SIZE][SIZE];
//
//void mm_sw(DTYPE A[SIZE][SIZE], DTYPE B[SIZE][SIZE], DTYPE out[SIZE][SIZE]){
//	DTYPE sum = 0;
//
//	for(int i = 0; i < SIZE; i++){
//		for(int j = 0; j < SIZE; j++){
//			sum = 0;
//			for(int k = 0; k < SIZE; k++){
//				sum = sum + A[i][k] * B[k][j];
//			}
//			out[i][j] = sum;
//		}
//	}
//}
//
//int main(){
//	init_matrices:
//	for(int i = 0; i < SIZE; i++){
//		for(int j = 0; j < SIZE; j++){
//			A[i][j] = rand() % 8;
//			B[i][j] = rand() % 8;
//			AB_sw[i][j] = 0;
//			AB_hw[i][j] = 0;
//		}
//	}
//
//	auto start = std::chrono::steady_clock::now();
//	mm(A, B, AB_hw, SIZE);
//	auto end = std::chrono::steady_clock::now();
//
//	mm_sw(A, B, AB_sw);
//
//	int fail = 0;
//	for(int i = 0; i < SIZE; i++){
//		for(int j = 0; j < SIZE; j++){
//			if (AB_sw[i][j] != AB_hw[i][j]){
//				fail = 1;
//			}
//		}
//	}
//
//	if (fail == 1){
//		printf("Failed\n");
//	}
//	else{
//		printf("Passed\n");
//	}
//	double exec_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//	double gops = double(SIZE) * SIZE * SIZE * 2 / (exec_time);
//	printf("Time: %f, GOPS: %f\n", exec_time * 1e-9, gops);
//
//}

