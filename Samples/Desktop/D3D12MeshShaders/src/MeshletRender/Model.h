//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
#pragma once

#include "Span.h"

#include <DirectXCollision.h>

struct Attribute
{
    enum EType : UINT
    {
        Position,
        Normal,
        TexCoord,
        Tangent,
        Bitangent,
        Count
    };

    EType    Type;
    UINT Offset;
};

struct Subset
{
    UINT Offset;
    UINT Count;
};

struct MeshInfo
{
    UINT IndexSize;
    UINT MeshletCount;

    UINT LastMeshletVertCount;
    UINT LastMeshletPrimCount;
};

struct Meshlet
{
    UINT VertCount;
    UINT VertOffset;
    UINT PrimCount;
    UINT PrimOffset;
};

struct PackedTriangle
{
    UINT i0 : 10;
    UINT i1 : 10;
    UINT i2 : 10;
};

struct CullData
{
    DirectX::XMFLOAT4 BoundingSphere; // xyz = center, w = radius
    uint8_t           NormalCone[4];  // xyz = axis, w = -cos(a + 90)
    float             ApexOffset;     // apex = center - axis * offset
};

struct Mesh
{
    D3D12_INPUT_ELEMENT_DESC   LayoutElems[Attribute::Count];
    D3D12_INPUT_LAYOUT_DESC    LayoutDesc;

    std::vector<Span<uint8_t>> Vertices;
    std::vector<UINT>      VertexStrides;
    UINT                   VertexCount;
    DirectX::BoundingSphere    BoundingSphere;

    Span<Subset>               IndexSubsets;
    Span<uint8_t>              Indices;
    UINT                   IndexSize;
    UINT                   IndexCount;

    Span<Subset>               MeshletSubsets;
    Span<Meshlet>              Meshlets;
    Span<uint8_t>              UniqueVertexIndices;
    Span<PackedTriangle>       PrimitiveIndices;
    Span<CullData>             CullingData;

    // D3D resource references
    std::vector<D3D12_VERTEX_BUFFER_VIEW>  VBViews;
    D3D12_INDEX_BUFFER_VIEW                IBView;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> VertexResources;
    Microsoft::WRL::ComPtr<ID3D12Resource>              IndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              MeshletResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              UniqueVertexIndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              PrimitiveIndexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              CullDataResource;
    Microsoft::WRL::ComPtr<ID3D12Resource>              MeshInfoResource;

    // Calculates the number of instances of the last meshlet which can be packed into a single threadgroup.
    UINT GetLastMeshletPackCount(UINT subsetIndex, UINT maxGroupVerts, UINT maxGroupPrims) 
    { 
        if (Meshlets.size() == 0)
            return 0;

        auto& subset = MeshletSubsets[subsetIndex];
        auto& meshlet = Meshlets[subset.Offset + subset.Count - 1];

        return min(maxGroupVerts / meshlet.VertCount, maxGroupPrims / meshlet.PrimCount);
    }

    void GetPrimitive(UINT index, UINT& i0, UINT& i1, UINT& i2) const
    {
        auto prim = PrimitiveIndices[index];
        i0 = prim.i0;
        i1 = prim.i1;
        i2 = prim.i2;
    }

    UINT GetVertexIndex(UINT index) const
    {
        const uint8_t* addr = UniqueVertexIndices.data() + index * IndexSize;
        if (IndexSize == 4)
        {
            return *reinterpret_cast<const UINT*>(addr);
        }
        else 
        {
            return *reinterpret_cast<const uint16_t*>(addr);
        }
    }
};

class Model
{
public:
    HRESULT LoadFromFile(const wchar_t* filename);
    HRESULT UploadGpuResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);

    UINT GetMeshCount() const { return static_cast<UINT>(m_meshes.size()); }
    const Mesh& GetMesh(UINT i) const { return m_meshes[i]; }

    UINT GetPrimitiveCount() const;
    UINT GetVertexCount() const;

    const DirectX::BoundingSphere& GetBoundingSphere() const { return m_boundingSphere; }

    // Iterator interface
    auto begin() { return m_meshes.begin(); }
    auto end() { return m_meshes.end(); }

private:
    std::vector<Mesh>                      m_meshes;
    DirectX::BoundingSphere                m_boundingSphere;

    std::vector<uint8_t>                   m_buffer;
};
