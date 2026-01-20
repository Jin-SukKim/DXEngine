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

int TextureManager::LoadTexture(const std::string& path, bool isSRGB)
{
    if (path.empty())
        return -1;
    std::string fullpath = presetPath + path;
    auto it = m_pathToIndexMap.find(fullpath);
    if (it != m_pathToIndexMap.end())
        return it->second;

    Image2 img;
    if (!img.Load(fullpath))
        return -1;

    DXGI_FORMAT format = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    img.Convert(format);

    auto device = GET_SINGLE(RenderBase)->GetDevice();
    auto context = GET_SINGLE(RenderBase)->GetContext();
    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
    D3D11Utils::CreateTexture(device.Get(), context.Get(), &img, format, *texture.get());

    if (texture->GetSRV() == nullptr)
        return -1;

    int index = static_cast<int>(m_textures.size());
    m_textures.emplace_back(std::move(texture));
    m_texturePathToIdx[fullpath] = index;
    m_indexToPathMap[index] = path;

    return index;
}

int TextureManager::LoadMetallicRoughnessTexture(const std::string& metallicPath, const std::string& roughnessPath)
{
    std::string fullpath1 = presetPath + metallicPath;
    std::string fullpath2 = presetPath + roughnessPath;
    auto it = m_pathToIndexMap.find(fullpath1);
    if (it != m_pathToIndexMap.end())
        return it->second;

    it = m_pathToIndexMap.find(fullpath2);
    if (it != m_pathToIndexMap.end())
        return it->second;

    int index = -1;
    // GLTF는 metallic과 roughness가 이미 합쳐진 Texture를 사용
    if (!metallicPath.empty() && (metallicPath == roughnessPath)) {
         index = LoadTexture(metallicPath, false);

    }
    else {
        auto device = GET_SINGLE(RenderBase)->GetDevice();
        auto context = GET_SINGLE(RenderBase)->GetContext();
        std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();

        D3D11Utils::CreateMetallicRoughnessTexture(device.Get(), context.Get(), metallicPath, roughnessPath, *texture.get());

        if (texture->GetSRV() == nullptr)
            return -1;
        
        index = index < -1 ? static_cast<int>(m_textures.size()) : index;
        m_textures.emplace_back(std::move(texture));
        m_texturePathToIdx[fullpath1] = index;
        m_texturePathToIdx[fullpath2] = index;
    }

    return index;
}

std::pair<int, int>  TextureManager::LoadMetallicRoughnessTexture(const std::string& path)
{
    if (path.empty())
        return { -1, -1 };

    std::string fullpath = presetPath + path;

    auto device = GET_SINGLE(RenderBase)->GetDevice();
    auto context = GET_SINGLE(RenderBase)->GetContext();
    std::unique_ptr<Texture2D> metallic = std::make_unique<Texture2D>();
    std::unique_ptr<Texture2D> roughness = std::make_unique<Texture2D>();
    D3D11Utils::CreateTexturesFromGLTFCombined(device.Get(), context.Get(), path, *metallic.get(), *roughness.get());

    std::pair<int, int> index = { 0, 0 };
    if (metallic->GetSRV() == nullptr)
        index.first = -1;
    if (roughness->GetSRV() == nullptr)
        index.second = -1;

    if (index.first > -1) {
        index.first = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(metallic));
        m_texturePathToIdx[fullpath + "metallic"] = index.first;
    }

    if (index.second > -1) {
        index.second = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(roughness));
        m_texturePathToIdx[fullpath + "roughness"] = index.second;
    }

    return index;
}

ID3D11ShaderResourceView* TextureManager::GetTextureSRV(int index)
{
    if (index < 0 || index >= m_textures.size())
        return nullptr;

    return m_textures[index]->GetSRV();
}
std::string TextureManager::GetTexturePath(int index)
{
    if (m_indexToPathMap.find(index) != m_indexToPathMap.end())
        return m_indexToPathMap[index];
    return ""; // 찾지 못함
}
}
