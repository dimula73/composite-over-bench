#include <assert.h>

#include <cuda_runtime.h>

typedef unsigned char quint8;

__constant__ float uint8Max;
__constant__ float uint8MaxRec1;
__constant__ float uint8MaxRec2;


__global__ void emptyKernel(quint8 *src, quint8 *dst, quint8 *mask,
                           float opacity)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
}

__global__ void overKernel(quint8 *src, quint8 *dst, quint8 *mask,
                           float opacity)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    int pi = 4 * i;

    float src_c1 = src[pi];
    float src_c2 = src[pi + 1];
    float src_c3 = src[pi + 2];
    float src_a = src[pi + 3];

    float dst_c1 = dst[pi];
    float dst_c2 = dst[pi + 1];
    float dst_c3 = dst[pi + 2];
    float dst_a = dst[pi + 3];

    src_a *= float(mask[i]) * opacity * uint8MaxRec1;

    float new_a = dst_a + (uint8Max - dst_a) * src_a * uint8MaxRec1;

    float src_blend = src_a / new_a;

    dst_c1 += src_blend * (src_c1 - dst_c1);
    dst_c2 += src_blend * (src_c2 - dst_c2);
    dst_c3 += src_blend * (src_c3 - dst_c3);

    dst[pi] = dst_c1;
    dst[pi + 1] = dst_c2;
    dst[pi + 2] = dst_c3;
    dst[pi + 3] = new_a;
}

quint8 *d_src = 0;
quint8 *d_dst = 0;
quint8 *d_mask = 0;

void initCuda(int size)
{
    cudaMalloc(&d_src, 4 * size);
    cudaMalloc(&d_dst, 4 * size);
    cudaMalloc(&d_mask, size);
}

void freeCuda()
{
    cudaFree(d_src);
    cudaFree(d_dst);
    cudaFree(d_mask);
}

void compositePixelsCUDA(int size, quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    float h_uint8Max = 255.0f;
    float h_uint8MaxRec1 = 1.0f/255.0f;
    float h_uint8MaxRec2 = 1.0f/(255.0f * 255.0f);

    cudaMemcpyToSymbol(uint8Max, &h_uint8Max, sizeof(float));
    cudaMemcpyToSymbol(uint8MaxRec1, &h_uint8MaxRec1, sizeof(float));
    cudaMemcpyToSymbol(uint8MaxRec2, &h_uint8MaxRec2, sizeof(float));


    cudaMemcpy(d_src, src, 4 * size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_dst, dst, 4 * size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_mask, mask, size, cudaMemcpyHostToDevice);

    // let size be multiple of 640
    int threadsPerBlock = 640;
    int blocksPerGrid = size / threadsPerBlock;

    assert(size % threadsPerBlock == 0);

    overKernel<<<blocksPerGrid, threadsPerBlock>>>(d_src, d_dst, d_mask, float(opacity)/255.0f);

    //cudaDeviceSynchronize();

    cudaMemcpy(dst, d_dst, 4 * size, cudaMemcpyDeviceToHost);
}

void compositePixelsCUDADataTransfers(int size, quint8 *src, quint8 *dst, quint8 *mask, quint8 opacity)
{
    float h_uint8Max = 255.0f;
    float h_uint8MaxRec1 = 1.0f/255.0f;
    float h_uint8MaxRec2 = 1.0f/(255.0f * 255.0f);

    cudaMemcpyToSymbol(uint8Max, &h_uint8Max, sizeof(float));
    cudaMemcpyToSymbol(uint8MaxRec1, &h_uint8MaxRec1, sizeof(float));
    cudaMemcpyToSymbol(uint8MaxRec2, &h_uint8MaxRec2, sizeof(float));


    cudaMemcpy(d_src, src, 4 * size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_dst, dst, 4 * size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_mask, mask, size, cudaMemcpyHostToDevice);

    // let size be multiple of 640
    int threadsPerBlock = 640;
    int blocksPerGrid = size / threadsPerBlock;

    assert(size % threadsPerBlock == 0);

    emptyKernel<<<blocksPerGrid, threadsPerBlock>>>(d_src, d_dst, d_mask, float(opacity)/255.0f);

    //cudaDeviceSynchronize();

    cudaMemcpy(dst, d_dst, 4 * size, cudaMemcpyDeviceToHost);
}
