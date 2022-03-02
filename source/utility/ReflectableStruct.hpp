#pragma once
#include "GeoMath.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace rtti{

	struct VarTypeData{
		enum class ScaleType : uint8_t{
			Float,
			Int,
			UInt
		};

		ScaleType scale;
		uint8_t dimension;
		uint8_t semanticIndex;
		const char* semantic;

		size_t GetSize() const { return static_cast<size_t>(dimension) * 4; }
	};

	enum class StructType : uint8_t{
		AOS = 0,
		SOA = 1
	};

	class VarTypeBase{
	protected:
		VarTypeBase(VarTypeData&& varTypeData);
		StructType m_structType;
		size_t     m_offset;
	};

	template<typename T>
	class VarType : public VarTypeBase{
	public:
		void CopyToBuffer(uint8_t* dstBuffer, uint8_t* data, size_t dataCount) const{
			switch(m_structType){
				case StructType::AOS:


					break;
				case StructType::SOA:
					memcpy(dstBuffer + (m_offset * dataCount), data, dataCount * sizeof(T));
					break;
				default:
					break;
			}
		}

	protected:
		VarType(VarTypeData&& varTypeData) 
			: VarTypeBase(std::move(varTypeData))
		{}

	};

	template<typename T>
	struct Var{
		Var(char const* semantic, uint8_t semanticIndex){}
	};


	class ReflectedStruct{
	public:
		static ReflectedStruct* GetCurrentPtr() { return ReflectedStructPtr; }

		const std::vector<VarTypeData>& GetVarTypeData() const { return m_variables; }

		size_t GetStructSize() const { return m_structSize; }
		size_t SetTypeData(VarTypeData&& varTypeData);

		virtual StructType GetStructType() = 0;

	protected:
		ReflectedStruct() : m_structSize(0)
		{ 
			ReflectedStructPtr = this; 
		}

		size_t m_structSize;
		std::vector<VarTypeData> m_variables;
	
	private:
		inline static ReflectedStruct* ReflectedStructPtr = nullptr;
	};

	class AOS : public ReflectedStruct{
	public:
		AOS() = default;
		virtual StructType GetStructType() override;
    };

    class SOA : public ReflectedStruct{
	public:
		SOA() = default;
		virtual StructType GetStructType() override;
	};


}


