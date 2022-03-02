#include "ReflectableStruct.hpp"

namespace rtti{

    VarTypeBase::VarTypeBase(VarTypeData&& varTypeData)
        : m_structType(ReflectedStruct::GetCurrentPtr()->GetStructType())
        , m_offset(ReflectedStruct::GetCurrentPtr()->SetTypeData(std::move(varTypeData)))
    {}

    size_t ReflectedStruct::SetTypeData(VarTypeData&& varTypeData){
        size_t offset = m_structSize;
        m_variables.emplace_back(varTypeData);
        m_structSize += varTypeData.GetSize();
        return offset;
    }

    StructType AOS::GetStructType(){ return StructType::AOS; }
    StructType SOA::GetStructType(){ return StructType::SOA; }
}