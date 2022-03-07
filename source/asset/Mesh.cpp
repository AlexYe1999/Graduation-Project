#include "Mesh.hpp"

void StaticMesh::Execute(){
    OnRender();
}

void StaticMesh::OnUpdate(){

}

void StaticMesh::OnRender(){
    for(auto& mesh : m_meshes){
        mesh->OnRender();
    }
}