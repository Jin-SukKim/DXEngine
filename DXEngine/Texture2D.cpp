#include "pch.h"
#include "Texture2D.h"

void DE::Texture2D::SetResource(const ComPtr<ID3D11Texture2D>& texture, const ComPtr<ID3D11ShaderResourceView>& srv)
{
	m_texture = texture;
	m_srv = srv;
}
