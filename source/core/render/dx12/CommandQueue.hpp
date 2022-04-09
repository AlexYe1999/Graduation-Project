#pragma once
#include "DXSampleHelper.h"
#include <cstdint>
#include <queue>

class CommandQueue{
public:
    CommandQueue(const ComPtr<ID3D12Device8>& device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    ~CommandQueue();

    ComPtr<ID3D12GraphicsCommandList4> GetCommandList();
 
    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList4>& commandList);
 
    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();
 
    const ComPtr<ID3D12CommandQueue>& GetD3D12CommandQueue() const;

protected:
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
    ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(ComPtr<ID3D12CommandAllocator>& allocator);

private:
 
    D3D12_COMMAND_LIST_TYPE     m_commandListType;
    ComPtr<ID3D12Device8>       m_d3d12Device;
    ComPtr<ID3D12CommandQueue>  m_d3d12CommandQueue;
    ComPtr<ID3D12Fence>         m_d3d12Fence;
    HANDLE                      m_fenceEvent;
    uint64_t                    m_fenceValue;
 
    struct CommandAllocatorEntry{
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
    };
    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue      = std::queue<ComPtr<ID3D12GraphicsCommandList4>>;
    CommandAllocatorQueue       m_commandAllocatorQueue;
    CommandListQueue            m_commandListQueue;
};