#pragma once

namespace DE {
	class Texture2D
	{
	public:
		const auto GetTexture() { return m_texture.Get(); }
		const auto GetRTV() { return m_rtv.Get(); }
		const auto GetSRV() { return m_srv.Get(); }
		const auto GetAddressOfTexture() { return m_texture.GetAddressOf(); }
		const auto GetAddressOfRTV() { return m_rtv.GetAddressOf(); }
		const auto GetAddressOfSRV() { return m_srv.GetAddressOf(); }
	private:
		ComPtr<ID3D11Texture2D> m_texture;
		ComPtr<ID3D11ShaderResourceView> m_srv;
		ComPtr<ID3D11RenderTargetView> m_rtv;
	};
}