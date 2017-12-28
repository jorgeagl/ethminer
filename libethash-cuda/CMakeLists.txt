find_package(CUDA REQUIRED)

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};--disable-warnings;--ptxas-options=-v;-use_fast_math;-lineinfo)

list(APPEND CUDA_NVCC_FLAGS_RELEASE -O3)
list(APPEND CUDA_NVCC_FLAGS_DEBUG -G)

if(COMPUTE AND (COMPUTE GREATER 0))
	list(APPEND CUDA_NVCC_FLAGS "-gencode arch=compute_${COMPUTE},code=sm_${COMPUTE}")
else()
	set(
		CUDA_NVCC_FLAGS
		${CUDA_NVCC_FLAGS}
		"-gencode arch=compute_30,code=sm_30"
		"-gencode arch=compute_35,code=sm_35"
		"-gencode arch=compute_50,code=sm_50"
		"-gencode arch=compute_52,code=sm_52"
		"-gencode arch=compute_53,code=sm_53"
		"-gencode arch=compute_60,code=sm_60"
		"-gencode arch=compute_61,code=sm_61"
		"-gencode arch=compute_62,code=sm_62"
		"-gencode arch=compute_70,code=sm_70"
	)
endif()

set(
	sources
	cuda_helper.h
	CUDAMiner.cpp CUDAMiner.h
	dagger_shared.cuh
	dagger_shuffled.cuh
	ethash_cuda_miner_kernel.cu
	ethash_cuda_miner_kernel.h
	ethash_cuda_miner_kernel_globals.h
	fnv.cuh
	keccak.cuh
	keccak_u64.cuh
)

cuda_add_library(ethash-cuda STATIC ${sources})
target_link_libraries(ethash-cuda ethcore ethash hwmon)
target_include_directories(ethash-cuda PUBLIC ${CUDA_INCLUDE_DIRS})
target_include_directories(ethash-cuda PRIVATE .. ${CMAKE_CURRENT_BINARY_DIR})
