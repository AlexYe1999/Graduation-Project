#include "SceneNode.hpp"

SceneNode::SceneNode(uint32_t nodeIndex, SceneNode* pParentNode)
    : m_isVisible(true)
    , m_isDirty(true)
    , m_isUseSingleMatrix(false)
    , m_dirtyCount(FrameCount)
    , m_nodeIndex(nodeIndex)
    , m_parentNode(pParentNode)
{}

void SceneNode::OnUpdate(){

    if(m_isDirty == true){
        m_toWorld = (m_isUseSingleMatrix ? r : s * r * t) * m_parentNode->GetToWorld();
        m_isDirty = false;
        m_dirtyCount = FrameCount;
    }

    if(m_isVisible){
        for(const auto comp : m_components){
            if(dynamic_cast<StaticMesh*>(comp.get()) == nullptr) comp->Execute();
        }
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
    m_isUseSingleMatrix = false;
    if(S != nullptr) s = *S;
    if(R != nullptr) r = *R;
    if(T != nullptr) t = *T;
    Pollute();
}

void SceneNode::SetMatrix(const GeoMath::Matrix4f& toWorld){
    m_isUseSingleMatrix = true;
    r = toWorld;
    Pollute();
}

GeoMath::Matrix4f SceneNode::GetTransform() const {
    return (m_isUseSingleMatrix ? r : r * t) * m_parentNode->GetTransform(); 
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

    if(m_isDirty == true){
        m_toWorld = m_isUseSingleMatrix ? r : s * r * t;
        m_isDirty = false;
        m_dirtyCount = FrameCount;
    }

    if(m_isVisible){
        for(const auto& comp : m_components){
            comp->Execute();
        }
    }

    for(const auto& node : m_childNodes){
        node->OnUpdate();
    }
}

void Scene::OnRender(){
    for(const auto& node : m_childNodes){
        node->OnRender();
    }
}

CameraNode::CameraNode(uint32_t nodeIndex, SceneNode* pParentNode)
    : SceneNode(nodeIndex, pParentNode)
    , m_nearZ(1.0f)
    , m_farZ(200.0f)
    , m_aspect(1.8f)
    , m_fov(0.30f * 3.1415926535f)
{
    m_proj = GeoMath::Matrix4f::Perspective(m_fov, m_aspect, m_nearZ, m_farZ);
}

void CameraNode::OnUpdate(){

    if(m_isDirty == true){
        m_toWorld = r;
        m_isDirty = false;
        m_dirtyCount = FrameCount;
    }

    if(m_isVisible){
        for(const auto& comp : m_components){
            comp->Execute();
        }
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
            if(m_isUseSingleMatrix){
                r[3] += r[2];
            }
            else{
                t[3] += r[2];
            }
            break;
        }
        case 'S':
        case 's':
        {
            if(m_isUseSingleMatrix){
                r[3] -= r[2];
            }
            else{
                t[3] -= r[2];
            }
            break;
        }
        case 'D':
        case 'd':
        {
            if(m_isUseSingleMatrix){
                r[3] += r[0];
            }
            else{
                t[3] += r[0];
            }
            break;
        }
        case 'A':
        case 'a':
        {
            if(m_isUseSingleMatrix){
                r[3] -= r[0];
            }
            else{
                t[3] -= r[0];
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            r = GeoMath::Matrix4f::Rotation(y * 0.001f, x * 0.001f, 0.0f) * r;
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
    m_isUseSingleMatrix = true;
    if(R != nullptr) r = *R;
    if(T != nullptr) t = *T;
    r *= t;
    Pollute();
}

void CameraNode::SetLens(
    const float* zNear, const float* zFar,
    const float* aspect, const float* fovY
){
    if(zNear  != nullptr) m_nearZ  = *zNear;
    if(zFar   != nullptr) m_farZ   = *zFar;
    if(fovY   != nullptr) m_fov    = *fovY;
    if(aspect != nullptr) m_aspect = *aspect;

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
            r = GeoMath::Matrix4f::Rotation(y * 0.001f, x * 0.001f, 0.0f) * r;
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
        m_dirtyCount = FrameCount;
    }

    for(const auto& node : m_childNodes){
        node->OnUpdate();
    }
}

void ThirdPersonCamera::Input(const uint16_t input, const int16_t x, const int16_t y){
    switch(input){
        case WM_MOUSEMOVE:
        {
            r = GeoMath::Matrix4f::Rotation(y * 0.001f, x * 0.001f, 0.0f) * r;
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