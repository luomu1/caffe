#include <vector>


#include "caffe/common.hpp"
#include "caffe/syncedmem.hpp"
#include "caffe/util/device_alternate.hpp"
#include "caffe/util/math_functions.hpp"

#include "gtest/gtest.h"
#include "caffe/test/test_caffe_main.hpp"

#ifdef USE_GREENTEA
#include "caffe/greentea/greentea.hpp"
#include "caffe/greentea/greentea_math_functions.hpp"
#endif

namespace caffe {

class SyncedMemoryTest : public ::testing::Test {
};

TEST_F(SyncedMemoryTest, TestInitialization) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  EXPECT_EQ(mem.head(), SyncedMemory::UNINITIALIZED);
  EXPECT_EQ(mem.size(), 10);
  SyncedMemory* p_mem = new SyncedMemory(10 * sizeof(float),
                                         Caffe::GetDefaultDevice(),
                                         dtypeof<float>());
  EXPECT_EQ(p_mem->size(), 10 * sizeof(float));
  delete p_mem;
}

#ifndef CPU_ONLY  // GPU test

TEST_F(SyncedMemoryTest, TestAllocationCPUGPU) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  EXPECT_TRUE(mem.cpu_data());
  EXPECT_TRUE(mem.gpu_data());
  EXPECT_TRUE(mem.mutable_cpu_data());
  EXPECT_TRUE(mem.mutable_gpu_data());
}

#endif

TEST_F(SyncedMemoryTest, TestAllocationCPU) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  EXPECT_TRUE(mem.cpu_data());
  EXPECT_TRUE(mem.mutable_cpu_data());
}

#ifndef CPU_ONLY  // GPU test

TEST_F(SyncedMemoryTest, TestAllocationGPU) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  EXPECT_TRUE(mem.gpu_data());
  EXPECT_TRUE(mem.mutable_gpu_data());
}

#endif

TEST_F(SyncedMemoryTest, TestCPUWrite) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  void* cpu_data = mem.mutable_cpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::HEAD_AT_CPU);
  caffe_memset(mem.size(), 1, cpu_data);
  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<char*>(cpu_data))[i], 1);
  }
  // do another round
  cpu_data = mem.mutable_cpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::HEAD_AT_CPU);
  caffe_memset(mem.size(), 2, cpu_data);
  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<char*>(cpu_data))[i], 2);
  }
}

#ifndef CPU_ONLY  // GPU test

TEST_F(SyncedMemoryTest, TestGPURead) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  void* cpu_data = mem.mutable_cpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::HEAD_AT_CPU);
  caffe_memset(mem.size(), 1, cpu_data);
  const void* gpu_data = mem.gpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::SYNCED);
  // check if values are the same
  char* recovered_value = new char[10];

  device *dc = Caffe::GetDefaultDevice();

  if (dc->backend() == BACKEND_CUDA) {
#ifdef USE_CUDA
    caffe_gpu_memcpy(10, gpu_data, recovered_value);
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    viennacl::ocl::context &ctx = viennacl::ocl::get_context(dc->id());
    greentea_gpu_memcpy(10, (cl_mem) gpu_data, 0, recovered_value, &ctx);
#endif  // USE_GREENTEA
  }

  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<char*>(recovered_value))[i], 1);
  }
  // do another round
  cpu_data = mem.mutable_cpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::HEAD_AT_CPU);
  caffe_memset(mem.size(), 2, cpu_data);
  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<char*>(cpu_data))[i], 2);
  }
  gpu_data = mem.gpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::SYNCED);
  // check if values are the same

  if (dc->backend() == BACKEND_CUDA) {
#ifdef USE_CUDA
    caffe_gpu_memcpy(10, gpu_data, recovered_value);
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    viennacl::ocl::context &ctx = viennacl::ocl::get_context(dc->id());
    greentea_gpu_memcpy(10, (cl_mem) gpu_data, 0, recovered_value, &ctx);
#endif  // USE_GREENTEA
  }

  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<char*>(recovered_value))[i], 2);
  }
  delete[] recovered_value;
}

TEST_F(SyncedMemoryTest, TestGPUWrite) {
  SyncedMemory mem(10, Caffe::GetDefaultDevice(), dtypeof<float>());
  void* gpu_data = mem.mutable_gpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::HEAD_AT_GPU);

  device *dc = Caffe::GetDefaultDevice();

  if (dc->backend() == BACKEND_CUDA) {
#ifdef USE_CUDA
    caffe_gpu_memset(mem.size(), 1, gpu_data);
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    greentea_memset(dc->id(), mem.size(), 1, (cl_mem) gpu_data, 0);
#endif  // USE_GREENTEA
  }

  const void* cpu_data = mem.cpu_data();
  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<const char*>(cpu_data))[i], 1);
  }
  EXPECT_EQ(mem.head(), SyncedMemory::SYNCED);

  gpu_data = mem.mutable_gpu_data();
  EXPECT_EQ(mem.head(), SyncedMemory::HEAD_AT_GPU);

  if (dc->backend() == BACKEND_CUDA) {
#ifdef USE_CUDA
    caffe_gpu_memset(mem.size(), 2, gpu_data);
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    greentea_memset(dc->id(), mem.size(), 2, (cl_mem) gpu_data, 0);
#endif  // USE_GREENTEA
  }

  cpu_data = mem.cpu_data();
  for (int_tp i = 0; i < mem.size(); ++i) {
    EXPECT_EQ((static_cast<const char*>(cpu_data))[i], 2);
  }
  EXPECT_EQ(mem.head(), SyncedMemory::SYNCED);
}

#endif

}  // namespace caffe
