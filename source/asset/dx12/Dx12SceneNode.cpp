#include "Dx12SceneNode.hpp"

void Dx12SceneNode::OnUpdate(){
    SceneNode::OnUpdate();

    if(m_dirtyCount > 0){

        m_dirtyCount--;
    }

}