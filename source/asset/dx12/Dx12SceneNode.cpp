#include "Dx12SceneNode.hpp"

void Dx12SceneNode::OnUpdate(){
    SceneNode::OnUpdate();

    if(m_dirtyCount > 0){
        
        auto& currFrameRes = m_renderResource->GetFrameResource();
        
        static ObjectConstBuffer objConst;
        objConst.toWorld = m_toWorld.Transpose();
        objConst.toLocal = m_toWorld.Inverse().Transpose();

        currFrameRes.objectConst->CopyData(
            reinterpret_cast<uint8_t*>(&objConst), sizeof(ObjectConstBuffer), 
            m_nodeIndex * Utils::CalcAlignment<256>(sizeof(ObjectConstBuffer))
        );
        m_dirtyCount--;

    }

}

void Dx12SceneNode::OnRender(){

    auto& currFrameRes = m_renderResource->GetFrameResource();
    auto  cmdList      = m_renderResource->GetCommandList();

    if(m_components.size() > 0){
        cmdList->SetGraphicsRootConstantBufferView(
            0, currFrameRes.objectConst->GetAddress() + m_nodeIndex * Utils::CalcAlignment<256>(sizeof(ObjectConstBuffer))
        );
    }

    for(auto comp : m_components){
        if(dynamic_cast<StaticMesh*>(comp.get()) != nullptr) comp->Execute();
    }

    for(const auto& node : m_childNodes){
        node->OnRender();
    }

}


void Dx12Camera::OnUpdate(){
    CameraNode::OnUpdate();

    if(m_dirtyCount > 0){
        
        auto& framResource = m_renderResource->GetFrameResource();
        
        static MainConstBuffer mainConst;
        mainConst.view = m_toWorld.Inverse().Transpose(); 
        mainConst.proj = m_proj.Transpose();
        mainConst.cameraPos = m_toWorld[3];

        framResource.mainConst->CopyData(reinterpret_cast<uint8_t*>(&mainConst), sizeof(MainConstBuffer));
        m_dirtyCount--;
    }

}
