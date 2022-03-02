#pragma once
#include<cstdint>
#include<cassert>

#include<codecvt>
#include<string>

#include<fstream>
#include<filesystem>

#include<wrl.h>
#include<windows.h>

namespace Utils{
    
    template<typename T, size_t Alignment = 4>
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