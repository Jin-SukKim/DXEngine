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
    // path는 이미 상대 경로 (예: "Textures/particle.png")
    auto it = m_pathToIndexMap.find(path);
    if (it != m_pathToIndexMap.end())
        return { path, it->second };

    if (m_nextFreeIndex >= MAX_PARTICLE_TEXTURES) {
        std::cout << "[TextureManager] Error: Particle Texture Array is full." << std::endl;
        return { path, -1 };
    }

    // 실제 로드는 presetPath 추가
    std::string fullpath = presetPath + path;
    Image2 img;
    if (!img.Load(fullpath)) {
        std::cout << "[TextureManager] Failed to load: " << fullpath << std::endl;
        return { path, -1 };
    }

    img.Resize(PARTICLE_TEXTURE_WIDTH, PARTICLE_TEXTURE_HEIGHT);
    img.Convert(particleSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

    auto context = GET_SINGLE(RenderBase)->GetContext();

    D3D11Utils::UpdateTextureArraySlice(
        context.Get(),
        m_particleTextureArray->GetTexture(),
        &img,
        static_cast<UINT>(m_nextFreeIndex)
    );

    context->GenerateMips(m_particleTextureArray->GetSRV());

    // 상대 경로로 저장
    m_pathToIndexMap[path] = m_nextFreeIndex;
    return { path, m_nextFreeIndex++ };
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

    // path는 상대 경로로 검색
    auto it = m_texturePathToIdx.find(path);
    if (it != m_texturePathToIdx.end())
        return it->second;

    // 실제 로드는 presetPath 추가
    std::string fullpath = presetPath + path;
    Image2 img;
    if (!img.Load(fullpath)) {
        std::cout << "[TextureManager] Failed to load: " << fullpath << std::endl;
        return -1;
    }

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
    
    // 상대 경로로 저장
    m_texturePathToIdx[path] = index;
    m_indexToPathMap[index] = path;

    return index;
}

int TextureManager::LoadMetallicRoughnessTexture(const std::string& metallicPath, const std::string& roughnessPath)
{
    // 먼저 metallic 경로로 검색
    auto it = m_texturePathToIdx.find(metallicPath);
    if (it != m_texturePathToIdx.end())
        return it->second;

    // roughness 경로로 검색
    it = m_texturePathToIdx.find(roughnessPath);
    if (it != m_texturePathToIdx.end())
        return it->second;

    // GLTF는 metallic과 roughness가 이미 합쳐진 Texture인 경우
    if (!metallicPath.empty() && (metallicPath == roughnessPath)) {
        return LoadTexture(metallicPath, false);
    }
    else {
        auto device = GET_SINGLE(RenderBase)->GetDevice();
        auto context = GET_SINGLE(RenderBase)->GetContext();
        std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();

        // 실제 로드는 presetPath 추가
        std::string fullMetallic = presetPath + metallicPath;
        std::string fullRoughness = presetPath + roughnessPath;
        D3D11Utils::CreateMetallicRoughnessTexture(device.Get(), context.Get(), 
            fullMetallic, fullRoughness, *texture.get());

        if (texture->GetSRV() == nullptr)
            return -1;
        
        int index = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(texture));
        
        // 두 경로 모두 같은 인덱스로 저장
        m_texturePathToIdx[metallicPath] = index;
        m_texturePathToIdx[roughnessPath] = index;
        m_indexToPathMap[index] = metallicPath;  // 대표 경로

        return index;
    }
}

std::pair<int, int> TextureManager::LoadMetallicRoughnessTexture(const std::string& path)
{
    if (path.empty())
        return { -1, -1 };

    auto device = GET_SINGLE(RenderBase)->GetDevice();
    auto context = GET_SINGLE(RenderBase)->GetContext();
    std::unique_ptr<Texture2D> metallic = std::make_unique<Texture2D>();
    std::unique_ptr<Texture2D> roughness = std::make_unique<Texture2D>();
    
    // 실제 로드는 presetPath 추가
    std::string fullpath = presetPath + path;
    D3D11Utils::CreateTexturesFromGLTFCombined(device.Get(), context.Get(), 
        fullpath, *metallic.get(), *roughness.get());

    std::pair<int, int> index = { -1, -1 };
    
    if (metallic->GetSRV() != nullptr) {
        index.first = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(metallic));
        
        // "_metallic" 접미사로 구분
        std::string metallicKey = path + "_metallic";
        m_texturePathToIdx[metallicKey] = index.first;
        m_indexToPathMap[index.first] = metallicKey;
    }

    if (roughness->GetSRV() != nullptr) {
        index.second = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(roughness));
        
        // "_roughness" 접미사로 구분
        std::string roughnessKey = path + "_roughness";
        m_texturePathToIdx[roughnessKey] = index.second;
        m_indexToPathMap[index.second] = roughnessKey;
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
    return "";
}
}
