#include "pch.h"
#include "AssetManager.h"

namespace DE {
    namespace fs = std::filesystem;
UINT AssetManager::Load(std::wstring& path)
{
    if (m_textures.size() >= AssetManager::MaxTextureCount) {
        std::cout << "[AssetManager]: Texture Max Count: 30. Texture load failed." << std::endl;
        return 0;
    }
    
    std::string absPath = fs::absolute(path).string();
    auto it = m_texturesIdx.find(absPath);
    if (it == m_texturesIdx.end()) {
        ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
        ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

        std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
        D3D11Utils::CreateTexture(device, context, absPath, true, *texture.get());
        //m_textures.emplace_back(L"Temp");

        m_texturesIdx[absPath] = m_textures.size() - 1;
    }

    return m_texturesIdx[absPath];
}

void AssetManager::BindGPU()
{
    // TODO: 일반적으로 Pixel Shader에만 Bind (Constant Buffer로 Index만 갱신해서 사용)
}
}
