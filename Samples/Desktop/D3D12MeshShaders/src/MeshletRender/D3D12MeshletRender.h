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

#include "DXSample.h"
#include "Model.h"
#include "StepTimer.h"
#include "SimpleCamera.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12MeshletRender : public DXSample
{
public:
    D3D12MeshletRender(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnKeyDown(UINT8 key);
    virtual void OnKeyUp(UINT8 key);

private:

    static const UINT FrameCount = 2;

    _declspec(align(256u)) struct SceneConstantBuffer
    {
        XMFLOAT4X4 View;
        XMFLOAT4X4 ViewProj;
        UINT       DrawMeshlets;
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT    m_viewport;
    CD3DX12_RECT        m_scissorRect;

    ComPtr<ID3D12Device2>        m_device;
    ComPtr<IDXGISwapChain3>      m_swapChain;

    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pipelineState;

    ComPtr<ID3D12Resource>       m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource>       m_depthStencil;
    
    ComPtr<ID3D12CommandAllocator>      m_commandAllocators[FrameCount];
    ComPtr<ID3D12CommandQueue>          m_commandQueue;
    ComPtr<ID3D12GraphicsCommandList6>  m_commandList;

    UINT               m_srvDescriptorSize;
    UINT               m_rtvDescriptorSize;
    UINT               m_dsvDescriptorSize;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    ComPtr<ID3D12Resource> m_constantBuffer;
    SceneConstantBuffer*   m_constantData;

    StepTimer              m_timer;
    SimpleCamera           m_camera;
    std::vector<Model>     m_modelLODs;
    
    // Synchronization objects.
    UINT                m_frameIndex;
    UINT                m_frameCounter;
    HANDLE              m_fenceEvent;
    UINT64              m_fenceValues[FrameCount];
    ComPtr<ID3D12Fence> m_fence;

    /*-------- V1.Instancing --------*/
    
    struct InstanceData
    {
        XMFLOAT4X4 World;
        XMFLOAT4X4 WorldInvTranspose;
    };

    ComPtr<ID3D12Resource> m_instanceBuffer;
    ComPtr<ID3D12Resource> m_instanceUpload;

    InstanceData* m_instanceData;

    UINT m_instanceLevel;
    UINT m_instanceCount;

    bool m_updateInstances;
    bool m_drawMeshlets;

    void RegenerateInstances();

private:

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void MoveToNextFrame();
    void WaitForGpu();

    //void ToggleFullscreen() override;

private:

    static const wchar_t* c_lodFilenames[];
    
    static const wchar_t* c_ampShaderFilename;
    static const wchar_t* c_meshShaderFilename;
    static const wchar_t* c_pixelShaderFilename;
};
