#include "SceneNode.hpp"

SceneNode::SceneNode(uint32_t nodeIndex, SceneNode* pParentNode)
    : m_isVisible(true)
    , m_isDirty(true)
    , m_nodeIndex(nodeIndex)
    , m_dirtyCount(DirtyCount)
    , m_parentNode(pParentNode)
{}

void SceneNode::OnUpdate(){

    if(m_isVisible){
        for(const auto& comp : m_components){
            comp->Execute();
        }
    }

    if(m_isDirty == true){
        m_toWorld = s * r * t * m_parentNode->GetToWorld();
        m_isDirty = false;
        m_dirtyCount = DirtyCount;
    }

    for(const auto& node : m_childNodes){
        node->OnUpdate();
    }

}

void SceneNode::AddChild(std::unique_ptr<SceneNode>&& childNode){
    m_childNodes.emplace_back(std::move(childNode));
}

void SceneNode::AddComponent(std::shared_ptr<IComponent>& component){
    m_components.emplace_back(component);
}

void SceneNode::SetMatrix(
    const GeoMath::Matrix4f* S, 
    const GeoMath::Matrix4f* R,
    const GeoMath::Matrix4f* T
){

    if(S != nullptr) this->s = *S;
    if(R != nullptr) this->r = *R;
    if(T != nullptr) this->t = *T;
    Pollute();
}

void SceneNode::Pollute(){
    if(m_isDirty == false){

        for(auto& node : m_childNodes){
            node->Pollute();
        }

        m_isDirty = true;
    }
}


void Scene::OnUpdate(){

    if(m_isVisible){
        for(const auto& comp : m_components){
            comp->Execute();
        }
    }

    for(const auto& node : m_childNodes){
        node->OnUpdate();
    }
}

CameraNode::CameraNode(uint32_t nodeIndex, SceneNode* pParentNode)
    : SceneNode(nodeIndex, pParentNode)
    , m_nearZ(1.0f)
    , m_farZ(8000.0f)
    , m_aspect(1.8f)
    , m_fov(0.20f * 3.1415926535f)
    , m_pitch(0.0f)
    , m_yaw(0.0f)
{
    m_proj = GeoMath::Matrix4f::Perspective(m_fov, m_aspect, m_nearZ, m_farZ);
}

void CameraNode::OnUpdate(){

    if(m_isVisible){
        for(const auto& comp : m_components){
            comp->Execute();
        }
    }

    if(m_isDirty == true){
        m_toWorld = s * r * t * m_parentNode->GetTransform();
        m_isDirty = false;
        m_dirtyCount = DirtyCount;
    }

    for(const auto& node : m_childNodes){
        node->OnUpdate();
    }

}

void CameraNode::Input(const uint16_t input, const int16_t x, const int16_t y){

    switch(input){
        case 'W':
        case 'w':
        {
            t[3] += (GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * r)[2];
            break;
        }
        case 'S':
        case 's':
        {
            t[3] -= (GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * r)[2];
            break;
        }
        case 'D':
        case 'd':
        {
            t[3] += (GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * r)[0];
            break;
        }
        case 'A':
        case 'a':
        {
            t[3] -= (GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * r)[0];
            break;
        }
        case WM_MOUSEMOVE:
        {
            m_pitch = y * 0.001f;
            m_yaw   = x * 0.001f;
            s = GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * s;
            break;
        }
        default:
        {
            break;
        }
    }

    Pollute();
}

void CameraNode::SetMatrix(
    const GeoMath::Matrix4f* S,
    const GeoMath::Matrix4f* R,
    const GeoMath::Matrix4f* T
){
    if(R != nullptr) this->r = *R;
    if(T != nullptr) this->t = *T;
    Pollute();
}

void CameraNode::SetLens(
    const float zNear, const float zFar,
    const float aspect, const float fovY
){
    m_nearZ  = zNear;
    m_farZ   = zFar;
    m_fov    = fovY;
    m_aspect = aspect;
    m_proj   = GeoMath::Matrix4f::Perspective(m_fov, m_aspect, m_nearZ, m_farZ);
}

GeoMath::Matrix4f CameraNode::GetView() const{
    return m_toWorld.Inverse();
}

const GeoMath::Matrix4f& CameraNode::GetProj() const{
    return m_proj;
}

FirstPersonCamera::FirstPersonCamera(uint32_t nodeIndex, SceneNode* pParentNode)
    : CameraNode(nodeIndex, pParentNode)
{}

void FirstPersonCamera::Input(const uint16_t input, const int16_t x, const int16_t y){
    switch(input){
        case WM_MOUSEMOVE:
        {
            m_pitch = y * 0.001f;
            m_yaw   = x * 0.001f;
            s = GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * s;
            break;
        }
        default:
        {
            break;
        }
    }

    Pollute();
}

void FirstPersonCamera::SetParentVisibility(const bool isVisible){
    m_parentNode->SetVisibility(isVisible);
}

ThirdPersonCamera::ThirdPersonCamera(uint32_t nodeIndex,SceneNode* pParentNode)
    : CameraNode(nodeIndex, pParentNode)
    , m_distance(20)
    , m_minDistance(20)
    , m_maxDistance(1000)
{}

void ThirdPersonCamera::OnUpdate(){
    if(m_isVisible){
        for(const auto& comp : m_components){
            comp->Execute();
        }
    }

    if(m_isDirty == true){
        t = GeoMath::Matrix4f::Translation(0.0f, 0.0f, -m_distance);
        m_toWorld = t * r * m_parentNode->GetTransform();
        m_isDirty = false;
        m_dirtyCount = DirtyCount;
    }

    for(const auto& node : m_childNodes){
        node->OnUpdate();
    }
}

void ThirdPersonCamera::Input(const uint16_t input, const int16_t x, const int16_t y){
    switch(input){
        case WM_MOUSEMOVE:
        {
            m_pitch = y * 0.001f;
            m_yaw   = x * 0.001f;
            r = GeoMath::Matrix4f::Rotation(m_pitch, m_yaw, 0.0f) * r;
            break;
        }
        case WM_MOUSEWHEEL:
        {
            m_distance -= x * 0.05f;
            m_distance = m_minDistance < m_distance    ? m_distance : m_minDistance;
            m_distance = m_distance    < m_maxDistance ? m_distance : m_maxDistance;
            break;
        }
        default:
        {
            break;
        }
    }
    Pollute();
}