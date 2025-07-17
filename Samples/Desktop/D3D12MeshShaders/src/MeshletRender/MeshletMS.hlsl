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

#include "Shared.h"

#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=2), \
                  RootConstants(b2, num32bitconstants=3), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3), \
                  SRV(t4)"

struct FConstants
{
	float4x4 View;
	float4x4 ViewProj;
    uint     DrawMeshlets;
};

struct FDrawParams
{
	uint InstanceCount;
	uint InstanceOffset;
};

struct FMeshInfo
{
    uint IndexBytes;
	uint MeshletCount;
    uint MeshletOffset;
};

struct FMeshlet
{
	uint VertCount;
	uint VertOffset;
	uint PrimCount;
	uint PrimOffset;
};

struct FInstanceData
{
	float4x4 World;
	float4x4 WorldInvTranspose;
};

struct FVertexInput
{
    float3 Position;
    float3 Normal;
};

struct FVertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    uint   MeshletIndex : COLOR0;
};

ConstantBuffer<FConstants>  Globals             : register(b0);
ConstantBuffer<FDrawParams> DrawParams          : register(b1);
ConstantBuffer<FMeshInfo>   MeshInfo            : register(b2);

StructuredBuffer<FVertexInput>  Vertices            : register(t0);
StructuredBuffer<FMeshlet>      Meshlets            : register(t1);
ByteAddressBuffer               UniqueVertexIndices : register(t2);
StructuredBuffer<uint>          PrimitiveIndices    : register(t3);
StructuredBuffer<FInstanceData> InstanceDatas       : register(t4);

/////
// Data Loaders

uint3 UnpackPrimitiveIndex(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitiveIndex(FMeshlet m, uint index)
{
    return UnpackPrimitiveIndex(PrimitiveIndices[m.PrimOffset + index]);
}

uint GetVertexIndex(FMeshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;

    if (MeshInfo.IndexBytes == 4) // 32-bit Vertex Indices
    {
        return UniqueVertexIndices.Load(localIndex * 4);
    }
    else // 16-bit Vertex Indices
    {
        // Byte address must be 4-byte aligned.
        uint wordOffset = (localIndex & 0x1);
        uint byteOffset = (localIndex / 2) * 4;

        // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
        uint indexPair = UniqueVertexIndices.Load(byteOffset);
        uint index = (indexPair >> (wordOffset * 16)) & 0xffff;

        return index;
    }
}

FVertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex, uint instanceIndex)
{
	FInstanceData n = InstanceDatas[DrawParams.InstanceOffset + instanceIndex];
    FVertexInput v = Vertices[vertexIndex];

	float4 positionWS = mul(float4(v.Position, 1), n.World);
    
    FVertexOut vout;
	vout.PositionVS = mul(positionWS, Globals.View).xyz;
	vout.PositionHS = mul(positionWS, Globals.ViewProj);
    vout.Normal = mul(float4(v.Normal, 0), n.WorldInvTranspose).xyz;
    vout.MeshletIndex = meshletIndex;

    return vout;
}

[RootSignature(ROOT_SIG)]
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out vertices FVertexOut verts[MAX_VERTS],
    out indices uint3 tris[MAX_PRIMS]
)
{
	uint meshletIndex = gid / DrawParams.InstanceCount;
    FMeshlet m = Meshlets[meshletIndex];

	uint startInstance = gid % DrawParams.InstanceCount;
	uint instanceCount = 1;
    
    if(meshletIndex == MeshInfo.MeshletCount - 1)
	{
		const uint instancesPerGroup = min(MAX_VERTS / m.VertCount, MAX_PRIMS / m.PrimCount);
        
		uint unpackedGroupCount = (MeshInfo.MeshletCount - 1) * DrawParams.InstanceCount;
		uint packedIndex = gid - unpackedGroupCount;

		startInstance = packedIndex * instancesPerGroup;
		instanceCount = min(DrawParams.InstanceCount - startInstance, instancesPerGroup);
	}
    
	const uint vertCount = m.VertCount * instanceCount;
	const uint primCount = m.PrimCount * instanceCount;
    
	SetMeshOutputCounts(vertCount, primCount);

    if (gtid < vertCount)
    {
		uint readIndex = gtid % m.VertCount;
		uint instanceId = gtid / m.VertCount;
        
		uint vertexIndex = GetVertexIndex(m, readIndex);
		uint instanceIndex = startInstance + instanceId;
        
		verts[gtid] = GetVertexAttributes(meshletIndex, vertexIndex, instanceIndex);

	}

	if (gtid < primCount)
    {
		uint readIndex = gtid % m.PrimCount;
		uint instanceId = gtid / m.PrimCount;
        
		tris[gtid] = GetPrimitiveIndex(m, readIndex) + (m.VertCount * instanceId);
	}
}
