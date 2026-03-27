#ifndef ENGINE_GRAPHICS_GRAPHICS_API_VERTEX_BUFFER_H
#define ENGINE_GRAPHICS_GRAPHICS_API_VERTEX_BUFFER_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "engine/platform/uc_common.h"
#include <stdio.h>

// Initialize and tear down the global vertex buffer pool.
// uc_orig: VB_Init (fallen/DDEngine/Source/vertexbuffer.cpp)
extern void VB_Init();
// uc_orig: VB_Term (fallen/DDEngine/Source/vertexbuffer.cpp)
extern void VB_Term();

// A single D3D hardware vertex buffer.
// Managed by VertexBufferPool; do not create directly.
// uc_orig: VertexBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
class VertexBuffer {
public:
    // uc_orig: VertexBuffer::~VertexBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    ~VertexBuffer();

    // uc_orig: VertexBuffer::GetPtr (fallen/DDEngine/Headers/vertexbuffer.h)
    D3DTLVERTEX* GetPtr()
    {
        ASSERT(m_LockedPtr);
        return m_LockedPtr;
    }
    // uc_orig: VertexBuffer::GetSize (fallen/DDEngine/Headers/vertexbuffer.h)
    ULONG GetSize() { return 1 << m_LogSize; }
    // uc_orig: VertexBuffer::GetLogSize (fallen/DDEngine/Headers/vertexbuffer.h)
    ULONG GetLogSize() { return m_LogSize; }

private:
    friend class VertexBufferPool;

    // uc_orig: VertexBuffer::VertexBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    VertexBuffer();

    // uc_orig: VertexBuffer::Create (fallen/DDEngine/Headers/vertexbuffer.h)
    HRESULT Create(IDirect3D3* d3d, bool force_system, ULONG logsize = 6);

    // uc_orig: VertexBuffer::Lock (fallen/DDEngine/Headers/vertexbuffer.h)
    D3DTLVERTEX* Lock();
    // uc_orig: VertexBuffer::Unlock (fallen/DDEngine/Headers/vertexbuffer.h)
    void Unlock();

    ULONG m_LogSize;
    D3DTLVERTEX* m_LockedPtr;
    VertexBuffer* m_Prev;
    VertexBuffer* m_Next;

    IDirect3DVertexBuffer* m_TheBuffer;
};

// Pool of D3D vertex buffers — allocates, recycles, and expands them.
// Uses a free list and a busy list per size class (log2 of vertex count).
// uc_orig: VertexBufferPool (fallen/DDEngine/Headers/vertexbuffer.h)
class VertexBufferPool {
public:
    // uc_orig: VertexBufferPool::VertexBufferPool (fallen/DDEngine/Headers/vertexbuffer.h)
    VertexBufferPool();
    // uc_orig: VertexBufferPool::~VertexBufferPool (fallen/DDEngine/Headers/vertexbuffer.h)
    ~VertexBufferPool();

    // uc_orig: VertexBufferPool::Create (fallen/DDEngine/Headers/vertexbuffer.h)
    void Create(IDirect3D3* d3d, bool force_system);

    // uc_orig: VertexBufferPool::GetBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    VertexBuffer* GetBuffer(ULONG logsize = 6);
    // uc_orig: VertexBufferPool::ReleaseBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    void ReleaseBuffer(VertexBuffer* buffer);

    // uc_orig: VertexBufferPool::ExpandBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    VertexBuffer* ExpandBuffer(VertexBuffer* buffer);
    // uc_orig: VertexBufferPool::PrepareBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    IDirect3DVertexBuffer* PrepareBuffer(VertexBuffer* buffer);

    // uc_orig: VertexBufferPool::DumpInfo (fallen/DDEngine/Headers/vertexbuffer.h)
    void DumpInfo(FILE* fd);

    // uc_orig: VertexBufferPool::ReclaimBuffers (fallen/DDEngine/Headers/vertexbuffer.h)
    void ReclaimBuffers();

private:
    IDirect3D3* m_D3D;
    bool m_SysMem;
    VertexBuffer* m_FreeList[16];
    VertexBuffer* m_BusyListLRU[16];
    VertexBuffer* m_BusyListMRU[16];
    ULONG m_Count[16];

    // uc_orig: VertexBufferPool::CreateBuffer (fallen/DDEngine/Headers/vertexbuffer.h)
    void CreateBuffer(ULONG logsize);
    // uc_orig: VertexBufferPool::CheckBuffers (fallen/DDEngine/Headers/vertexbuffer.h)
    void CheckBuffers(ULONG logsize, bool time_critical);
};

// uc_orig: TheVPool (fallen/DDEngine/Source/vertexbuffer.cpp)
extern VertexBufferPool* TheVPool;

#endif // ENGINE_GRAPHICS_GRAPHICS_API_VERTEX_BUFFER_H
