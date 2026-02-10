#include <gtest/gtest.h>
#include "mymem.h"

TEST(MyMallocTest, BasicAllocation) {
    void* a = my_malloc(15);
    void* b = my_malloc(180);

    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    my_free(a);
    my_free(b);
}

TEST(MyMallocTest, ReuseFreedBlocks) {
    void* a = my_malloc(15);
    void* b = my_malloc(15);

    my_free(a);
    void* c = my_malloc(15);

    // Freed block 'a' should be reused
    ASSERT_EQ(a, c);

    my_free(b);
    my_free(c);
}

TEST(MyMallocTest, InvalidSize) {
    void* a = my_malloc(16);  // unsupported
    ASSERT_EQ(a, nullptr);
}

TEST(MyMallocTest, FreeNull) {
    my_free(nullptr);  // should do nothing
    SUCCEED();
}

TEST(MyAllocator, MultipleAlloc) {
    void* blocks[4];
    for (int i = 0; i < 4; ++i) {
        blocks[i] = my_malloc(15);
        ASSERT_NE(blocks[i], nullptr);
    }

    for (int i = 0; i < 4; ++i) {
        my_free(blocks[i]);
    }
}

TEST(MyAllocator, MixedAllocFree) {
    void* a = my_malloc(15);
    void* b = my_malloc(180);
    void* c = my_malloc(15);
    void* d = my_malloc(180);

    my_free(c);
    my_free(b);
    my_free(a);
    my_free(d);
}

TEST(MyAllocator, StressTest) {
    const int n = 1000;
    void* blocks[1000];

    for (int i = 0; i < n; ++i) {
        blocks[i] = my_malloc(15);
        ASSERT_NE(blocks[i], nullptr);
    }

    for (int i = 0; i < n; ++i) {
        my_free(blocks[i]);
    }
}

#ifdef ALLOCATOR_DOUBLE_FREE_AWARE
TEST(MyAllocator, InvalidFree) {
    void* a = my_malloc(15);
    my_free(a);

    // Should abort on double free
    EXPECT_DEATH(my_free(a), "");
}
#endif

