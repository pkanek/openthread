/*
 *  Copyright (c) 2017, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <openthread/config.h>

#include "core/crypto/heap.hpp"

#include <stdlib.h>

#include "common/debug.hpp"
#include "crypto/aes_ccm.hpp"
#include "utils/wrap_string.h"

#include "test_platform.h"
#include "test_util.h"

/**
 * Verifies single variable allocating and freeing.
 *
 */
void TestAllocateSingle(void)
{
    ot::Crypto::Heap heap;

    {
        void *p = heap.CAlloc(1, 0);
        VerifyOrQuit(p == NULL, "TestAllocateSingle allocate 1 x 0 byte failed!\n");
        heap.Free(p);

        p = heap.CAlloc(0, 1);
        VerifyOrQuit(p == NULL, "TestAllocateSingle allocate 0 x 1 byte failed!\n");
        heap.Free(p);
    }

    for (size_t size = 1; size <= heap.GetCapacity(); ++size)
    {
        Log("%s allocating %zu bytes...", __func__, size);
        void *p = heap.CAlloc(1, size);
        VerifyOrQuit(p != NULL && !heap.IsClean(), "allocating failed!\n");
        memset(p, 0xff, size);
        heap.Free(p);
        VerifyOrQuit(heap.IsClean(), "freeing failed!\n");
    }
}

/**
 * Verifies randomly allocating and freeing variables.
 *
 * @param[in]   aSizeLimit  The maximum allocation size.
 * @param[in]   aSeed       The seed for generating random sizes.
 *
 */
void TestAllocateRandomly(size_t aSizeLimit, unsigned int aSeed)
{
    struct Node
    {
        Node *mNext;
        size_t mSize;
    };

    ot::Crypto::Heap heap;
    Node head;
    size_t nnodes = 0;

    srand(aSeed);

    Node *last = &head;

    do
    {
        size_t size = sizeof(Node) + static_cast<size_t>(rand()) % aSizeLimit;
        Log("TestAllocateRandomly allocating %zu bytes...", size);
        last->mNext = static_cast<Node *>(heap.CAlloc(1, size));

        // No more memory for allocation.
        if (last->mNext == NULL)
        {
            break;
        }

        VerifyOrQuit(last->mNext->mNext == NULL, "TestAllocateRandomly memory not initialized to zero!\n");
        last = last->mNext;
        last->mSize = size;
        ++nnodes;

        // 50% probability to randomly free a node.
        size_t freeIndex = static_cast<size_t>(rand()) % (nnodes * 2);

        if (freeIndex > nnodes)
        {
            freeIndex /= 2;

            Node *prev = &head;

            while (freeIndex--)
            {
                prev = prev->mNext;
            }

            Node *curr = prev->mNext;
            Log("TestAllocateRandomly freeing %zu bytes...", curr->mSize);
            prev->mNext = curr->mNext;
            heap.Free(curr);

            if (last == curr)
            {
                last = prev;
            }

            --nnodes;
        }
    }
    while (true);

    last = head.mNext;

    while (last)
    {
        Node *next = last->mNext;
        Log("TestAllocateRandomly freeing %zu bytes...", last->mSize);
        heap.Free(last);
        last = next;
    }

    VerifyOrQuit(heap.IsClean(), "TestAllocateRandomly heap not clean after freeing all!\n");
}

/**
 * Verifies allocating and free multiple variables.
 */
void TestAllocateMultiple(void)
{
    for (unsigned int seed = 0; seed < 10; ++seed)
    {
        size_t sizeLimit = (1 << seed);
        Log("TestAllocateRandomly(%zu, %u)...", sizeLimit, seed);
        TestAllocateRandomly(sizeLimit, seed);
    }
}

void RunTimerTests(void)
{
    TestAllocateSingle();
    TestAllocateMultiple();
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    RunTimerTests();
    printf("All tests passed\n");
    return 0;
}
#endif
