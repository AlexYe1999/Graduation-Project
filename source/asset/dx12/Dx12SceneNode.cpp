#include "Dx12SceneNode.hpp"
#include <cstddef>

void Dx12SceneNode::OnUpdate(){
    SceneNode::OnUpdate();
    if(m_dirtyCount > 0){
        
        auto& currFrameRes = m_graphicsMgr->GetFrameResource();
        
        static ObjectConstBuffer objConst;
        objConst.toWorld = m_toWorld.Transpose();
        objConst.toLocal = m_toWorld.Inverse().Transpose();
        objConst.objectIndex = m_nodeIndex;

        currFrameRes.objectConst->CopyData(
            reinterpret_cast<uint8_t*>(&objConst), sizeof(ObjectConstBuffer), 
            m_nodeIndex * Utility::CalcAlignment<256>(sizeof(ObjectConstBuffer))
        );

        for(auto comp : m_components){
            if(dynamic_cast<StaticMesh*>(comp.get()) != nullptr){
                for(auto index : dynamic_cast<StaticMesh*>(comp.get())->GetIndices()){
                    currFrameRes.rayTraceInstanceDesc->CopyData(
                        reinterpret_cast<uint8_t*>(&m_toWorld.Transpose()), sizeof(GeoMath::Vector4f) * 3,
                        index * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)
                    );
                }
            }
        }

        currFrameRes.isAccelerationStructureDitry = true;
        m_dirtyCount--;
    }

}

void Dx12SceneNode::OnRender(){

    auto& currFrameRes = m_graphicsMgr->GetFrameResource();
    auto  cmdList      = m_graphicsMgr->GetCommandList();

    if(m_components.size() > 0){
        cmdList->SetGraphicsRootConstantBufferView(
            0, currFrameRes.objectConst->GetGpuVirtualAddress(m_nodeIndex)
        );
    }

    for(auto comp : m_components){
        if(dynamic_cast<StaticMesh*>(comp.get()) != nullptr) comp->Execute();
    }

    for(const auto& node : m_childNodes){
        node->OnRender();
    }

}

void Dx12SceneNode::OnTraceRay(){
    for(const auto& node : m_childNodes){
        node->OnTraceRay();
    }
}

void Dx12Camera::OnUpdate(){
    CameraNode::OnUpdate();

    if(m_dirtyCount > 0){
        auto& framResource = m_graphicsMgr->GetFrameResource();
        
        static MainConstBuffer mainConst;
        mainConst.model = m_toWorld.Transpose();
        mainConst.view  = m_toWorld.Inverse().Transpose(); 
        mainConst.proj  = m_proj.Transpose();
        mainConst.cameraPosition = m_toWorld[3];
        mainConst.fov = m_fov;

        framResource.mainConst->CopyData(reinterpret_cast<uint8_t*>(&mainConst), sizeof(MainConstBuffer));
        m_dirtyCount--;
    }

}
