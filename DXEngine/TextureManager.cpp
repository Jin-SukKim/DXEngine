#include "pch.h"
#include "TextureManager.h"
#include "Image2.h"

namespace DE {
    namespace fs = std::filesystem;

void TextureManager::Initialize()
{
    auto device = GET_SINGLE(RenderBase)->GetDevice();
    auto context = GET_SINGLE(RenderBase)->GetContext();

    m_particleTextureArray = std::make_unique<Texture2D>();
    
    ComPtr<ID3D11Texture2D> tex;
    ComPtr<ID3D11ShaderResourceView> srv;

    D3D11Utils::CreateTexture2DArray(
        device.Get(),
        PARTICLE_TEXTURE_WIDTH,
        PARTICLE_TEXTURE_HEIGHT,
        MAX_PARTICLE_TEXTURES,
        particleSRGB,
        tex,
        srv);
    
    m_particleTextureArray->SetResource(tex, srv);
    m_pathToIndexMap.clear();
    m_nextFreeIndex = 0;
}

TextureManager::TextureEntity TextureManager::LoadParticleTexture(const std::string& path)
{
    // TODO: 기본 Texture를 만들어서 없다면 기본 texture의 path와 idx를 반환
    std::string fullpath = presetPath + path;
    auto it = m_pathToIndexMap.find(fullpath);
    if (it != m_pathToIndexMap.end())
        return { fullpath, it->second };

    if (m_nextFreeIndex >= MAX_PARTICLE_TEXTURES) {
        std::cout << "[TextureManager] Error: Particle Texture Arry is full." << std::endl;
        return { fullpath, -1 };
    }

    Image2 img;
    if (!img.Load(fullpath))
        return { fullpath, -1 };

    img.Resize(PARTICLE_TEXTURE_WIDTH, PARTICLE_TEXTURE_HEIGHT);
    img.Convert(particleSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

    auto context = GET_SINGLE(RenderBase)->GetContext();

    D3D11Utils::UpdateTextureArraySlice(
        context.Get(),
        m_particleTextureArray->GetTexture(),
        &img,
        static_cast<UINT>(m_nextFreeIndex)
    );

    // 전체 Array에 대해 수행되므로 비용이 많이 소모될 수 있음
    context->GenerateMips(m_particleTextureArray->GetSRV());

    m_pathToIndexMap[fullpath] = m_nextFreeIndex;
    return { fullpath, m_nextFreeIndex++ };
}

void TextureManager::BindParticleTextures()
{
    if (m_particleTextureArray == nullptr)
        return;
    auto context = GET_SINGLE(RenderBase)->GetContext();
    context->PSSetShaderResources(14, 1, m_particleTextureArray->GetAddressOfSRV());
}
}
