#pragma once

namespace DE {
	using Microsoft::WRL::ComPtr;

	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			// 디버깅할 때 여기에 breakpoint를 설정
			throw std::exception();
		}
	}
	class D3D11Utils
	{
	};
}