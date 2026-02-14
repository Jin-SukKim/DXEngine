#pragma once

namespace DE {
	class Texture2D
	{
	public:
		const auto GetTexture() const { return m_texture.Get(); }
		const auto GetRTV() const { return m_rtv.Get(); }
		const auto GetSRV() const { return m_srv.Get(); }
		const auto GetUAV() const { return m_uav.Get(); }
		const auto GetAddressOfTexture() { return m_texture.GetAddressOf(); }
		const auto GetAddressOfRTV() { return m_rtv.GetAddressOf(); }
		const auto GetAddressOfSRV() { return m_srv.GetAddressOf(); }
		const auto GetAddressOfUAV() { return m_uav.GetAddressOf(); }

		void SetResource(const ComPtr<ID3D11Texture2D>& texture, const ComPtr<ID3D11ShaderResourceView>& srv);
	private:
		ComPtr<ID3D11Texture2D> m_texture;
		ComPtr<ID3D11ShaderResourceView> m_srv;
		ComPtr<ID3D11RenderTargetView> m_rtv;
		ComPtr<ID3D11UnorderedAccessView> m_uav;
	};
}