#include "engine/graphics/pipeline/vertex_buffer.h"
#include "engine/graphics/pipeline/vertex_buffer_globals.h"
#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_api/dd_manager.h"   // InitStruct macro

#include <string.h>
#include <stdio.h>

// uc_orig: VB_Init (fallen/DDEngine/Source/vertexbuffer.cpp)
void VB_Init()
{
    if (!TheVPool)
        TheVPool = MFnew<VertexBufferPool>();
}

// uc_orig: VB_Term (fallen/DDEngine/Source/vertexbuffer.cpp)
void VB_Term()
{
    MFdelete(TheVPool);
    TheVPool = NULL;
}

// uc_orig: VertexBuffer::VertexBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
VertexBuffer::VertexBuffer()
{
    m_LogSize = 0;
    m_LockedPtr = NULL;
    m_Prev = NULL;
    m_Next = NULL;
    m_TheBuffer = NULL;
}

// uc_orig: VertexBuffer::~VertexBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
VertexBuffer::~VertexBuffer()
{
    ASSERT(!m_Next);
    ASSERT(!m_Prev);

    if (m_LockedPtr) {
        Unlock();
    }

    if (m_TheBuffer) {
        m_TheBuffer->Release();
        m_TheBuffer = NULL;
    }
}

// uc_orig: VertexBuffer::Create (fallen/DDEngine/Source/vertexbuffer.cpp)
HRESULT VertexBuffer::Create(IDirect3D3* d3d, bool force_system, ULONG logsize)
{
    ASSERT(!m_LockedPtr);
    ASSERT(d3d);
    ASSERT(!m_TheBuffer);

    D3DVERTEXBUFFERDESC desc;

    InitStruct(desc);

    desc.dwCaps = force_system ? D3DVBCAPS_SYSTEMMEMORY : 0;
    desc.dwFVF = D3DFVF_TLVERTEX;
    desc.dwNumVertices = 1 << logsize;

    HRESULT res = d3d->CreateVertexBuffer(&desc, &m_TheBuffer, 0, NULL);

    if (FAILED(res)) {
        m_TheBuffer = NULL;
        return res;
    }

    m_LogSize = logsize;

    return DD_OK;
}

// uc_orig: VertexBuffer::Lock (fallen/DDEngine/Source/vertexbuffer.cpp)
D3DTLVERTEX* VertexBuffer::Lock()
{
    ASSERT(m_TheBuffer);
    ASSERT(!m_LockedPtr);

    HRESULT res = m_TheBuffer->Lock(DDLOCK_SURFACEMEMORYPTR | DDLOCK_NOSYSLOCK, (LPVOID*)&m_LockedPtr, NULL);

    if (FAILED(res)) {
        ASSERT(res != DDERR_SURFACELOST);
        m_LockedPtr = NULL;
        return NULL;
    }

    return m_LockedPtr;
}

// uc_orig: VertexBuffer::Unlock (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBuffer::Unlock()
{
    ASSERT(m_TheBuffer);
    ASSERT(m_LockedPtr);

    HRESULT res = m_TheBuffer->Unlock();

    if (FAILED(res)) {
        ASSERT(res != DDERR_SURFACELOST);
        ASSERT(0);
    }

    m_LockedPtr = NULL;
}

// uc_orig: VertexBufferPool::VertexBufferPool (fallen/DDEngine/Source/vertexbuffer.cpp)
VertexBufferPool::VertexBufferPool()
{
    m_D3D = NULL;
    m_SysMem = false;

    for (int ii = 0; ii < 16; ii++) {
        m_FreeList[ii] = NULL;
        m_BusyListLRU[ii] = NULL;
        m_BusyListMRU[ii] = NULL;
        m_Count[ii] = 0;
    }
}

// uc_orig: VertexBufferPool::~VertexBufferPool (fallen/DDEngine/Source/vertexbuffer.cpp)
VertexBufferPool::~VertexBufferPool()
{
    VertexBuffer* p;

    for (int ii = 0; ii < 16; ii++) {
        while (p = m_FreeList[ii]) {
            m_FreeList[ii] = p->m_Next;
            p->m_Next = NULL;
            p->m_Prev = NULL;
            delete p;
        }
        while (p = m_BusyListLRU[ii]) {
            m_BusyListLRU[ii] = p->m_Next;
            p->m_Next = NULL;
            p->m_Prev = NULL;
            delete p;
        }
    }
}

// Pre-allocates a variety of buffer sizes totaling ~48,000 vertices.
// uc_orig: VertexBufferPool::Create (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBufferPool::Create(IDirect3D3* d3d, bool force_system)
{
    static int Allocations[16] = { 0, 0, 0, 0, 0, 0, 128, 64, 32, 16, 8, 4, 0, 0, 0, 0 };

    ASSERT(!m_D3D);
    ASSERT(d3d);

    m_D3D = d3d;
    m_SysMem = force_system;

    for (int ii = 0; ii < 16; ii++) {
        for (int jj = 0; jj < Allocations[ii]; jj++) {
            CreateBuffer(ii);
        }
    }
}

// uc_orig: VertexBufferPool::CreateBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBufferPool::CreateBuffer(ULONG logsize)
{
    ASSERT(logsize < 16);

    VertexBuffer* p = new VertexBuffer;
    if (!p)
        return;

    HRESULT res = p->Create(m_D3D, m_SysMem, logsize);

    if (FAILED(res)) {
        delete p;
        return;
    }

    if (p->Lock()) {
        p->m_Next = m_FreeList[logsize];
        m_FreeList[logsize] = p;

        m_Count[logsize]++;
    } else {
        ASSERT(0);
    }
}

// uc_orig: VertexBufferPool::CheckBuffers (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBufferPool::CheckBuffers(ULONG logsize, bool time_critical)
{
    VertexBuffer* list = m_BusyListLRU[logsize];

    while (list) {
        VertexBuffer* next = list->m_Next;

        if (list->Lock()) {
            // remove from busy list
            if (list->m_Prev == NULL)
                m_BusyListLRU[logsize] = list->m_Next;
            else
                list->m_Prev->m_Next = list->m_Next;
            if (list->m_Next == NULL)
                m_BusyListMRU[logsize] = list->m_Prev;
            else
                list->m_Next->m_Prev = list->m_Prev;
            // add to free list
            list->m_Prev = NULL;
            list->m_Next = m_FreeList[logsize];
            m_FreeList[logsize] = list;

            if (time_critical)
                return;
        }

        list = next;
    }
}

// Returns a locked buffer of the requested size, creating one if needed.
// uc_orig: VertexBufferPool::GetBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
VertexBuffer* VertexBufferPool::GetBuffer(ULONG logsize)
{
    ASSERT(logsize < 16);

    if (!m_FreeList[logsize]) {
        CheckBuffers(logsize, true);

        if (!m_FreeList[logsize]) {
            CreateBuffer(logsize);

            if (!m_FreeList[logsize]) {
                if (m_BusyListLRU[logsize]) {
                    ASSERT(0);
                    while (!m_FreeList[logsize])
                        CheckBuffers(logsize, true);
                } else {
                    return NULL;
                }
            }
        }
    }

    VertexBuffer* p = m_FreeList[logsize];
    m_FreeList[logsize] = p->m_Next;
    p->m_Next = NULL;

    ASSERT(p->m_LockedPtr);

    return p;
}

// uc_orig: VertexBufferPool::ReleaseBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBufferPool::ReleaseBuffer(VertexBuffer* buffer)
{
    ASSERT(buffer);
    ASSERT(buffer->m_LockedPtr);
    ASSERT(!buffer->m_Next);

    buffer->m_Next = m_FreeList[buffer->m_LogSize];
    m_FreeList[buffer->m_LogSize] = buffer;
}

// Replaces buffer with a larger one (next size class), copying existing vertices.
// uc_orig: VertexBufferPool::ExpandBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
VertexBuffer* VertexBufferPool::ExpandBuffer(VertexBuffer* buffer)
{
    ASSERT(buffer);
    ASSERT(buffer->m_LockedPtr);
    ASSERT(!buffer->m_Next);
    ASSERT(buffer->m_LogSize < 16);

    VertexBuffer* p = GetBuffer(buffer->m_LogSize + 1);

    if (p) {
        memcpy(p->GetPtr(), buffer->GetPtr(), buffer->GetSize() * sizeof(D3DTLVERTEX));
        ReleaseBuffer(buffer);
    } else {
        p = buffer;
    }

    return p;
}

// Unlocks buffer and moves it to the busy list so the GPU can use it.
// Returns the underlying IDirect3DVertexBuffer pointer.
// uc_orig: VertexBufferPool::PrepareBuffer (fallen/DDEngine/Source/vertexbuffer.cpp)
IDirect3DVertexBuffer* VertexBufferPool::PrepareBuffer(VertexBuffer* buffer)
{
    ASSERT(buffer);
    ASSERT(buffer->m_LockedPtr);
    ASSERT(!buffer->m_Next);

    buffer->Unlock();

    buffer->m_Next = NULL;
    buffer->m_Prev = m_BusyListMRU[buffer->m_LogSize];
    if (buffer->m_Prev) {
        buffer->m_Prev->m_Next = buffer;
    } else {
        m_BusyListLRU[buffer->m_LogSize] = buffer;
    }
    m_BusyListMRU[buffer->m_LogSize] = buffer;

    return buffer->m_TheBuffer;
}

// uc_orig: VertexBufferPool::DumpInfo (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBufferPool::DumpInfo(FILE* fd)
{
    ReclaimBuffers();

    for (int ii = 0; ii < 16; ii++) {
        if (m_Count[ii]) {
            fprintf(fd, "Buffer size %d vertices (1 << %d)\n\n", 1 << ii, ii);
            fprintf(fd, "Allocated ever = %d\n", m_Count[ii]);

            VertexBuffer* p;
            int busy = 0;
            int free = 0;

            p = m_FreeList[ii];
            while (p) {
                free++;
                p = p->m_Next;
            }

            p = m_BusyListLRU[ii];
            while (p) {
                busy++;
                p = p->m_Next;
            }

            fprintf(fd, "Free now = %d\n", free);
            fprintf(fd, "Used now = %d\n", m_Count[ii] - free - busy);
            fprintf(fd, "Busy now = %d\n", busy);
        } else {
            fprintf(fd, "Buffer size %d vertices - never allocated\n", 1 << ii);
        }

        fprintf(fd, "\n");
    }
}

// uc_orig: VertexBufferPool::ReclaimBuffers (fallen/DDEngine/Source/vertexbuffer.cpp)
void VertexBufferPool::ReclaimBuffers()
{
    for (int ii = 0; ii < 16; ii++) {
        CheckBuffers(ii, false);
    }
}
