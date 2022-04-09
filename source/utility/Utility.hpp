#pragma once
#include <cstdint>
#include <cassert>
#include <codecvt>

#include <string>
#include <memory>
#include <array>
#include <tuple>
#include <unordered_map>

#include <algorithm>

#include <fstream>
#include <filesystem>

namespace Utility{
	
	namespace PreDef{
		const float PI = 3.1415926538f;
	}

    template<size_t Alignment = 4, typename T>
    constexpr T CalcAlignment(T dataSize){
        return (dataSize+Alignment-1) & ~(Alignment-1);
    }

	// convert string to wstring
	static std::wstring ToWString(const std::string& input){
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(input);
	}

	// convert wstring to string 
	static std::string ToString(const std::wstring& input){
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(input);
	}


}