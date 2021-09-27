#include "convolution.h"

#include <iostream>
#include <cuda.h>
#include <cuda_runtime.h>
#include <curand.h>
#include <thrust/device_vector.h>

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

#define DIM 256
#define MASK_WIDTH 3
#define TILE_WIDTH 4
#define RADIUS 1

__constant__ float mask[MASK_WIDTH][MASK_WIDTH];

__global__ void conv2d(float* input, float* output) {
  int bx = blockIdx.x; int by = blockIdx.y;
  int tx = threadIdx.x; int ty = threadIdx.y;
  
  // index of the element in the output array
  int x_o = bx * TILE_WIDTH + tx;
  int y_o = by * TILE_WIDTH + ty;

  __shared__ float N_ds[TILE_WIDTH][TILE_WIDTH];
  
  // load the tile to shared memory.
  if (0 <= x_o && x_o < DIM && 0 <= y_o && y_o < DIM)
    N_ds[ty][tx] = input[DIM * y_o + x_o];
  else
    N_ds[ty][tx] = 0;
  __syncthreads();
  
  // offset (0, 0, 0) by (-Radius, -Radius, -Radius) so that we start computing
  // at the top left corner of the mask
  int x_start = tx - RADIUS;
  int y_start = ty - RADIUS;
  float result = 0;
  
  for (int i = 0; i < MASK_WIDTH; i++)
    for (int j = 0; j < MASK_WIDTH; j++) {      
      int x_index = x_start + i;
      int y_index = y_start + j;
      
      // if the indices are within range of what we have in the tile
      // use the shared memory data
      if (   0 <= x_index && x_index < TILE_WIDTH 
          && 0 <= y_index && y_index < TILE_WIDTH)
        result += N_ds[y_index][x_index] * mask[j][i];
      // otherwise just go to global memory
      else {
        int x_global = bx * TILE_WIDTH + x_index;
        int y_global = by * TILE_WIDTH + y_index;
        
        // if we're trying to access something outside the actual matrix itself, take the value to be 0
        // (aka do nothing)
        if (   0 <= x_global && x_global < DIM 
            && 0 <= y_global && y_global < DIM)
          result += input[DIM * y_global + x_global] * mask[j][i];
      }
    } 
  
  if (x_o < DIM && y_o < DIM)
    output[DIM * y_o + x_o] = result;
  
  __syncthreads();
}


void GPU_fill_rand(float *A, int nr_rows_A, int nr_cols_A)
{
  // Create a pseudo-random number generator
  curandGenerator_t prng;
  curandCreateGenerator(&prng, CURAND_RNG_PSEUDO_XORWOW);

  // Set the seed for the random number generator using the system clock
  curandSetPseudoRandomGeneratorSeed(prng, (unsigned long long) clock());

  // Fill the array with random numbers on the device
  curandGenerateUniform(prng, A, nr_rows_A * nr_cols_A);
}


HeightmapGenerator::HeightmapGenerator(float* noiseValues) {
  output_h = (float*) malloc(DIM * DIM * sizeof(float));
  mask_h = (float*) malloc(MASK_WIDTH * MASK_WIDTH * sizeof(float));

  mask_h[0] = 1.0; mask_h[1] = 2.0; mask_h[2] = 1.0; 
  mask_h[3] = 2.0; mask_h[4] = 4.0; mask_h[5] = 2.0; 
  mask_h[6] = 1.0; mask_h[7] = 2.0; mask_h[8] = 1.0; 

  gpuErrchk(cudaMalloc((void**) &input_d, DIM * DIM * sizeof(float)));
  gpuErrchk(cudaMalloc((void**) &output_d, DIM * DIM * sizeof(float)));

  gpuErrchk(cudaMemset(input_d, 0, DIM * DIM * sizeof(float)));

  gpuErrchk(cudaMemcpyToSymbol(mask, mask_h, MASK_WIDTH * MASK_WIDTH * sizeof(float)));
};

void HeightmapGenerator::run() {
  GPU_fill_rand(input_d, DIM, DIM);
  gpuErrchk(cudaDeviceSynchronize());

  dim3 dimBlock(TILE_WIDTH, TILE_WIDTH, 1);
  dim3 dimGrid(ceil(1.0f * DIM / TILE_WIDTH), ceil(1.0f * DIM / TILE_WIDTH), 1.0f);

  conv2d<<<dimGrid, dimBlock>>>(input_d, output_d);
  gpuErrchk(cudaDeviceSynchronize());

  thrust::device_ptr<float> dev_ptr = thrust::device_pointer_cast(output_d);
  maxHeight = *(thrust::max_element(dev_ptr, dev_ptr + DIM * DIM));
  maxHeight = *(thrust::min_element(dev_ptr, dev_ptr + DIM * DIM));
  // thrust::device_vector<float> output_thrust;
  // output_thrust.data() = thrust::device_pointer_cast(output_d);
  // maxHeight = *(thrust::max_element(output_thrust.begin(), output_thrust.end()));
  // minHeight = *(thrust::min_element(output_thrust.begin(), output_thrust.end()));

  std::cerr << maxHeight << ", " << minHeight << std::endl;

  gpuErrchk(cudaMemcpy(output_h, output_d, DIM * DIM * sizeof(float), cudaMemcpyDeviceToHost));
};

HeightmapGenerator::~HeightmapGenerator() {
  free(mask_h);
  free(output_h);
  cudaFree(input_d);
  cudaFree(output_d);
}

float HeightmapGenerator::getMaxHeight() {
  return maxHeight;
}

float HeightmapGenerator::getMinHeight() {
  return minHeight;
}