#include "Dx12Material.hpp"

Dx12Material::Dx12Material(uint32_t matIndex, bool isDoubleSide)
    : Material(matIndex)
    , m_matFlag(PipelineStateFlag::PIPELINE_STATE_INITIAL_FLAG)
    , m_graphicsMgr(Dx12GraphicsManager::GetInstance())
{
    m_matFlag = isDoubleSide ? PipelineStateFlag::PIPELINE_STATE_CULL_MODE_NONE : PipelineStateFlag::PIPELINE_STATE_CULL_MODE_FRONT;
    m_matFlag = m_matFlag | PipelineStateFlag::PIPELINE_STATE_FRONT_COUNTER_CLOCKWISE;
}

void Dx12Material::OnRender(){
    m_graphicsMgr->SetPipelineStateFlag(m_matFlag, 0x38, false);
}