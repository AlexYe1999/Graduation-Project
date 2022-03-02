#include "Device.hpp"

Device::Device(){ 
	uint32_t dxgiFactoryFlags = 0;

#	if defined(_DEBUG)
{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));
	uint32_t adapterIndex = 1;
	bool adapterFound = false;
	while (m_dxgiFactory->EnumAdapters1(adapterIndex, m_dxgiAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		m_dxgiAdapter->GetDesc1(&desc);
		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
			HRESULT hr = D3D12CreateDevice(m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_d3d12Device));
			if (SUCCEEDED(hr)) {
				adapterFound = true;
				break;
			}
		}
		m_dxgiAdapter = nullptr;
		adapterIndex++;
	}

#if defined(_DEBUG)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		m_dxgiAdapter->GetDesc(&adapterDesc);

		std::wstring text;

		text = L"\n******Selected Adaptor******\n";
		text += adapterDesc.Description;
		text += L"\n\n";

		OutputDebugString(std::string(text.begin(), text.end()).c_str());
	}
#endif
}

Device::~Device(){}