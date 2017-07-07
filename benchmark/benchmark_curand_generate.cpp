// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

#include <boost/program_options.hpp>

#include <cuda_runtime.h>
#include <curand.h>

#define CUDA_CALL(x) do { if((x)!=cudaSuccess) { \
    printf("Error at %s:%d\n",__FILE__,__LINE__);\
    return exit(EXIT_FAILURE);}} while(0)
#define CURAND_CALL(x) do { if((x)!=CURAND_STATUS_SUCCESS) { \
    printf("Error at %s:%d\n",__FILE__,__LINE__);\
    return exit(EXIT_FAILURE);}} while(0)

#ifndef DEFAULT_RAND_N
const size_t DEFAULT_RAND_N = 1024 * 1024 * 128;
#endif

void run_benchmark(const size_t size, const size_t trials, const curandRngType rng_type)
{
    unsigned int * data;
    CUDA_CALL(cudaMalloc((void **)&data, size * sizeof(unsigned int)));

    curandGenerator_t generator;
    CURAND_CALL(curandCreateGenerator(&generator, rng_type));
    CUDA_CALL(cudaDeviceSynchronize());

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < trials; i++)
    {
        CURAND_CALL(curandGenerate(generator, (unsigned int *) data, size));
    }
    CUDA_CALL(cudaDeviceSynchronize());
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> elapsed = end-start;

    std::cout << "Throughput = "
              << trials * size * sizeof(unsigned int) / (elapsed.count()
                     * (1024.0/1000.0) * (1024.0/1000.0) * (1024.0/1000.0))
              << "GB/s, AvgTime (1 trial) = "
              << std::chrono::duration<double>(elapsed).count() / trials
              << ", Time (all) "
              << std::chrono::duration<double>(elapsed).count()
              << ", Size = " << size
              << std::endl;

    CURAND_CALL(curandDestroyGenerator(generator));
    CUDA_CALL(cudaFree(data));
}

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;
    po::options_description options("options");
    options.add_options()
        ("help", "show usage instructions")
        ("size", po::value<size_t>()->default_value(DEFAULT_RAND_N), "number of values")
        ("trials", po::value<size_t>()->default_value(20), "number of trials")
        ("engine", po::value<std::string>()->default_value("philox"), "random number engine")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    if(vm.count("help")) {
        std::cout << options << std::endl;
        return 0;
    }

    const size_t size = vm["size"].as<size_t>();
    const size_t trials = vm["trials"].as<size_t>();
    const std::string& engine = vm["engine"].as<std::string>();

    std::cout << "cuRAND:" << std::endl;
    if(engine == "philox" || engine == "all"){
        std::cout << "philox4x32_10:" << std::endl;
        run_benchmark(size, trials, CURAND_RNG_PSEUDO_PHILOX4_32_10);
        return 0;
    }
    std::cerr << "Error: unknown random number engine '" << engine << "'" << std::endl;
    return -1;
}