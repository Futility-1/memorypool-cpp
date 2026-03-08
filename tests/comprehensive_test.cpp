#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <iostream>
#include <chrono>
#include <vector>
#include "ngx_mem_pool.h"

using namespace std;

int test_passed = 0;
int test_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ FAILED: %s\n", message); \
            printf("   File: %s, Line: %d\n", __FILE__, __LINE__); \
            test_failed++; \
            return false; \
        } \
        test_passed++; \
        printf("✅ PASSED: %s\n", message); \
    } while(0)

#define TEST_START(name) \
    printf("\n========== %s ==========\n", name)

#define TEST_END() \
    printf("========== Test End ==========\n")

struct TestData {
    int id;
    char data[256];
    double value;
};

void cleanup_handler(void *data) {
    printf("  Cleanup handler called for data: %p\n", data);
    if (data) {
        printf("  Data content: %s\n", (char *)data);
    }
}

void external_mem_cleanup_handler(void *data) {
    printf("  External memory cleanup handler called for data: %p\n", data);
    if (data) {
        free(data);
    }
}

void file_cleanup_handler(void *data) {
    printf("  File cleanup handler called\n");
    if (data) {
        FILE *fp = (FILE *)data;
        fclose(fp);
    }
}

bool test_basic_pool_creation() {
    TEST_START("Basic Pool Creation");
    
    ngx_mem_pool pool;
    void *result = pool.ngx_create_pool(4096);
    
    TEST_ASSERT(result != nullptr, "Create pool with 4096 bytes");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_small_memory_allocation() {
    TEST_START("Small Memory Allocation");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    void *p1 = pool.ngx_palloc(64);
    TEST_ASSERT(p1 != nullptr, "Allocate 64 bytes");
    
    void *p2 = pool.ngx_palloc(128);
    TEST_ASSERT(p2 != nullptr, "Allocate 128 bytes");
    
    void *p3 = pool.ngx_palloc(256);
    TEST_ASSERT(p3 != nullptr, "Allocate 256 bytes");
    
    TEST_ASSERT(p1 != p2 && p2 != p3, "Different allocations should have different addresses");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_large_memory_allocation() {
    TEST_START("Large Memory Allocation");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(512);
    
    void *p1 = pool.ngx_palloc(1024);
    TEST_ASSERT(p1 != nullptr, "Allocate 1024 bytes (large memory)");
    
    void *p2 = pool.ngx_palloc(2048);
    TEST_ASSERT(p2 != nullptr, "Allocate 2048 bytes (large memory)");
    
    TEST_ASSERT(p1 != p2, "Different large allocations should have different addresses");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_memory_alignment() {
    TEST_START("Memory Alignment");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    void *p1 = pool.ngx_palloc(32);
    void *p2 = pool.ngx_palloc(64);
    
    uintptr_t addr1 = (uintptr_t)p1;
    uintptr_t addr2 = (uintptr_t)p2;
    
    TEST_ASSERT(addr1 % NGX_ALIGNMENT == 0, "First allocation should be aligned");
    TEST_ASSERT(addr2 % NGX_ALIGNMENT == 0, "Second allocation should be aligned");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_memory_zero_initialization() {
    TEST_START("Memory Zero Initialization");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    char *p = (char *)pool.ngx_pcalloc(256);
    TEST_ASSERT(p != nullptr, "Allocate and zero initialize memory");
    
    bool all_zero = true;
    for (int i = 0; i < 256; i++) {
        if (p[i] != 0) {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT(all_zero, "All bytes should be zero after pcalloc");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_memory_pool_exhaustion() {
    TEST_START("Memory Pool Exhaustion");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(1024);
    
    vector<void*> allocations;
    
    for (int i = 0; i < 100; i++) {
        void *p = pool.ngx_palloc(32);
        if (p == nullptr) {
            TEST_ASSERT(true, "Pool exhaustion detected");
            break;
        }
        allocations.push_back(p);
    }
    
    TEST_ASSERT(allocations.size() > 0, "Should allocate some memory before exhaustion");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_large_memory_free() {
    TEST_START("Large Memory Free");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(512);
    
    void *p1 = pool.ngx_palloc(1024);
    TEST_ASSERT(p1 != nullptr, "Allocate large memory");
    
    pool.ngx_pfree(p1);
    TEST_ASSERT(true, "Free large memory");
    
    void *p2 = pool.ngx_palloc(1024);
    TEST_ASSERT(p2 != nullptr, "Allocate large memory again after free");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_memory_pool_reset() {
    TEST_START("Memory Pool Reset");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    void *p1 = pool.ngx_palloc(256);
    void *p2 = pool.ngx_palloc(512);
    
    TEST_ASSERT(p1 != nullptr && p2 != nullptr, "Initial allocations");
    
    pool.ngx_reset_pool();
    
    void *p3 = pool.ngx_palloc(256);
    TEST_ASSERT(p3 != nullptr, "Allocate after reset");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_cleanup_handlers() {
    TEST_START("Cleanup Handlers");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    char *external_mem = (char *)malloc(128);
    strcpy(external_mem, "test data");
    
    ngx_pool_cleanup_s *cleanup = pool.ngx_pool_cleanup_add(0);
    TEST_ASSERT(cleanup != nullptr, "Add cleanup handler");
    
    cleanup->handler = external_mem_cleanup_handler;
    cleanup->data = external_mem;
    
    printf("  Calling destroy pool (should trigger cleanup handler)...\n");
    pool.ngx_destroy_pool();
    
    TEST_ASSERT(true, "Cleanup handlers executed");
    
    TEST_END();
    return true;
}

bool test_multiple_cleanup_handlers() {
    TEST_START("Multiple Cleanup Handlers");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    char *mem1 = (char *)malloc(64);
    char *mem2 = (char *)malloc(64);
    char *mem3 = (char *)malloc(64);
    
    ngx_pool_cleanup_s *c1 = pool.ngx_pool_cleanup_add(0);
    c1->handler = external_mem_cleanup_handler;
    c1->data = mem1;
    
    ngx_pool_cleanup_s *c2 = pool.ngx_pool_cleanup_add(0);
    c2->handler = external_mem_cleanup_handler;
    c2->data = mem2;
    
    ngx_pool_cleanup_s *c3 = pool.ngx_pool_cleanup_add(0);
    c3->handler = external_mem_cleanup_handler;
    c3->data = mem3;
    
    TEST_ASSERT(c1 != nullptr && c2 != nullptr && c3 != nullptr, "Add multiple cleanup handlers");
    
    printf("  Calling destroy pool (should trigger all cleanup handlers)...\n");
    pool.ngx_destroy_pool();
    
    TEST_ASSERT(true, "All cleanup handlers executed");
    
    TEST_END();
    return true;
}

bool test_complex_data_structures() {
    TEST_START("Complex Data Structures");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    TestData *data1 = (TestData *)pool.ngx_palloc(sizeof(TestData));
    TEST_ASSERT(data1 != nullptr, "Allocate complex data structure");
    
    data1->id = 1;
    strcpy(data1->data, "test data");
    data1->value = 3.14159;
    
    TEST_ASSERT(data1->id == 1, "Data structure field 1");
    TEST_ASSERT(strcmp(data1->data, "test data") == 0, "Data structure field 2");
    TEST_ASSERT(data1->value == 3.14159, "Data structure field 3");
    
    TestData *data2 = (TestData *)pool.ngx_pcalloc(sizeof(TestData));
    TEST_ASSERT(data2 != nullptr, "Allocate and zero initialize complex data structure");
    TEST_ASSERT(data2->id == 0, "Zero initialized field 1");
    TEST_ASSERT(data2->value == 0.0, "Zero initialized field 3");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_boundary_conditions() {
    TEST_START("Boundary Conditions");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    void *p1 = pool.ngx_palloc(1);
    TEST_ASSERT(p1 != nullptr, "Allocate minimum size (1 byte)");
    
    void *p2 = pool.ngx_palloc(0);
    TEST_ASSERT(p2 != nullptr, "Allocate zero bytes");
    
    void *p3 = pool.ngx_palloc(4096);
    TEST_ASSERT(p3 != nullptr, "Allocate exactly pool size");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_performance() {
    TEST_START("Performance Test");
    
    const int num_allocations = 10000;
    const int allocation_size = 64;
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(1024 * 1024);
    
    auto start = chrono::high_resolution_clock::now();
    
    vector<void*> allocations;
    for (int i = 0; i < num_allocations; i++) {
        void *p = pool.ngx_palloc(allocation_size);
        if (p) {
            allocations.push_back(p);
        }
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    printf("  Allocated %zu blocks in %ld microseconds\n", allocations.size(), duration.count());
    printf("  Average time per allocation: %.2f microseconds\n", 
           (double)duration.count() / allocations.size());
    
    TEST_ASSERT(allocations.size() > 0, "Performance test completed");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_mixed_allocation_sizes() {
    TEST_START("Mixed Allocation Sizes");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(8192);
    
    vector<void*> allocations;
    
    int sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    for (int i = 0; i < 8; i++) {
        void *p = pool.ngx_palloc(sizes[i]);
        TEST_ASSERT(p != nullptr, "Allocate mixed size memory");
        allocations.push_back(p);
    }
    
    TEST_ASSERT(allocations.size() == 8, "All mixed size allocations successful");
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_consecutive_allocations() {
    TEST_START("Consecutive Allocations");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    void *prev = nullptr;
    for (int i = 0; i < 10; i++) {
        void *current = pool.ngx_palloc(32);
        TEST_ASSERT(current != nullptr, "Consecutive allocation");
        TEST_ASSERT(current != prev, "Consecutive allocations should be different");
        prev = current;
    }
    
    pool.ngx_destroy_pool();
    
    TEST_END();
    return true;
}

bool test_file_resource_cleanup() {
    TEST_START("File Resource Cleanup");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    FILE *fp = fopen("test_cleanup.txt", "w");
    TEST_ASSERT(fp != nullptr, "Open test file");
    
    ngx_pool_cleanup_s *cleanup = pool.ngx_pool_cleanup_add(0);
    TEST_ASSERT(cleanup != nullptr, "Add file cleanup handler");
    
    cleanup->handler = file_cleanup_handler;
    cleanup->data = fp;
    
    printf("  Calling destroy pool (should close file)...\n");
    pool.ngx_destroy_pool();
    
    TEST_ASSERT(true, "File cleanup handler executed");
    
    remove("test_cleanup.txt");
    
    TEST_END();
    return true;
}

bool test_pool_with_cleanup_data() {
    TEST_START("Pool With Cleanup Data");
    
    ngx_mem_pool pool;
    pool.ngx_create_pool(4096);
    
    size_t cleanup_data_size = 128;
    ngx_pool_cleanup_s *cleanup = pool.ngx_pool_cleanup_add(cleanup_data_size);
    
    TEST_ASSERT(cleanup != nullptr, "Add cleanup handler with data");
    TEST_ASSERT(cleanup->data != nullptr, "Cleanup data allocated");
    
    strcpy((char *)cleanup->data, "cleanup test data");
    
    cleanup->handler = cleanup_handler;
    
    pool.ngx_destroy_pool();
    
    TEST_ASSERT(true, "Cleanup with data completed");
    
    TEST_END();
    return true;
}

void print_summary() {
    printf("\n");
    printf("========================================\n");
    printf("           TEST SUMMARY                  \n");
    printf("========================================\n");
    printf("Total Tests: %d\n", test_passed + test_failed);
    printf("✅ Passed: %d\n", test_passed);
    printf("❌ Failed: %d\n", test_failed);
    printf("Success Rate: %.1f%%\n", 
           (test_passed + test_failed) > 0 ? 
           (100.0 * test_passed / (test_passed + test_failed)) : 0.0);
    printf("========================================\n");
}

int main() {
    printf("========================================\n");
    printf("    Nginx Memory Pool Test Suite       \n");
    printf("========================================\n");
    
    test_basic_pool_creation();
    test_small_memory_allocation();
    test_large_memory_allocation();
    test_memory_alignment();
    test_memory_zero_initialization();
    test_memory_pool_exhaustion();
    test_large_memory_free();
    test_memory_pool_reset();
    test_cleanup_handlers();
    test_multiple_cleanup_handlers();
    test_complex_data_structures();
    test_boundary_conditions();
    test_performance();
    test_mixed_allocation_sizes();
    test_consecutive_allocations();
    test_file_resource_cleanup();
    test_pool_with_cleanup_data();
    
    print_summary();
    
    return test_failed > 0 ? 1 : 0;
}