#include "Mesh.hpp"

void StaticMesh::Execute(){
    OnRender();
}

void StaticMesh::OnRender(){
    for(auto& mesh : m_meshes){
        mesh->OnRender();
    }
}