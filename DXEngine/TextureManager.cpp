#include "pch.h"
#include "TextureManager.h"
#include "Image2.h"
#include "SimplexNoise.h"
#include <DirectXPackedVector.h>

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

    GenerateCurlNoiseTexture(64, 4.0f);
}

TextureManager::TextureEntity TextureManager::LoadParticleTexture(const std::string& path)
{
    // path�� �̹� ��� ��� (��: "Textures/particle.png")
    auto it = m_pathToIndexMap.find(path);
    if (it != m_pathToIndexMap.end())
        return { path, it->second };

    if (m_nextFreeIndex >= MAX_PARTICLE_TEXTURES) {
        std::cout << "[TextureManager] Error: Particle Texture Array is full." << std::endl;
        return { path, -1 };
    }

    // ���� �ε�� presetPath �߰�
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

    // ��� ��η� ����
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

    // path�� ��� ��η� �˻�
    auto it = m_texturePathToIdx.find(path);
    if (it != m_texturePathToIdx.end())
        return it->second;

    // ���� �ε�� presetPath �߰�
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
    
    // ��� ��η� ����
    m_texturePathToIdx[path] = index;
    m_indexToPathMap[index] = path;

    return index;
}

int TextureManager::LoadMetallicRoughnessTexture(const std::string& metallicPath, const std::string& roughnessPath)
{
    // ���� metallic ��η� �˻�
    auto it = m_texturePathToIdx.find(metallicPath);
    if (it != m_texturePathToIdx.end())
        return it->second;

    // roughness ��η� �˻�
    it = m_texturePathToIdx.find(roughnessPath);
    if (it != m_texturePathToIdx.end())
        return it->second;

    // GLTF�� metallic�� roughness�� �̹� ������ Texture�� ���
    if (!metallicPath.empty() && (metallicPath == roughnessPath)) {
        return LoadTexture(metallicPath, false);
    }
    else {
        auto device = GET_SINGLE(RenderBase)->GetDevice();
        auto context = GET_SINGLE(RenderBase)->GetContext();
        std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();

        // ���� �ε�� presetPath �߰�
        std::string fullMetallic = presetPath + metallicPath;
        std::string fullRoughness = presetPath + roughnessPath;
        D3D11Utils::CreateMetallicRoughnessTexture(device.Get(), context.Get(), 
            fullMetallic, fullRoughness, *texture.get());

        if (texture->GetSRV() == nullptr)
            return -1;
        
        int index = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(texture));
        
        // �� ��� ��� ���� �ε����� ����
        m_texturePathToIdx[metallicPath] = index;
        m_texturePathToIdx[roughnessPath] = index;
        m_indexToPathMap[index] = metallicPath;  // ��ǥ ���

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
    
    // ���� �ε�� presetPath �߰�
    std::string fullpath = presetPath + path;
    D3D11Utils::CreateTexturesFromGLTFCombined(device.Get(), context.Get(), 
        fullpath, *metallic.get(), *roughness.get());

    std::pair<int, int> index = { -1, -1 };
    
    if (metallic->GetSRV() != nullptr) {
        index.first = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(metallic));
        
        std::string metallicKey = path;
        m_texturePathToIdx[metallicKey] = index.first;
        m_indexToPathMap[index.first] = metallicKey;
    }

    if (roughness->GetSRV() != nullptr) {
        index.second = static_cast<int>(m_textures.size());
        m_textures.emplace_back(std::move(roughness));
        
        std::string roughnessKey = path;
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

void TextureManager::GenerateCurlNoiseTexture(UINT resolution, float frequency)
{
    using DirectX::PackedVector::XMConvertFloatToHalf;

    const UINT res = resolution;
    const float invRes = 1.0f / static_cast<float>(res);
    const float eps = 0.01f;

    // Seed offsets for 3 independent noise fields (Fx, Fy, Fz)
    const float ox1 = 31.4f, oy1 = 27.1f, oz1 = 19.7f;
    const float ox2 = 57.3f, oy2 = 43.2f, oz2 = 61.8f;

    // RGBA16F: 4 half per texel = 8 bytes
    std::vector<uint16_t> pixelData(res * res * res * 4);

    auto sampleNoise = [&](float x, float y, float z, float seedX, float seedY, float seedZ) {
        return SimplexNoise::Noise3D(
            (x + seedX) * frequency,
            (y + seedY) * frequency,
            (z + seedZ) * frequency);
    };

    for (UINT z = 0; z < res; z++) {
        for (UINT y = 0; y < res; y++) {
            for (UINT x = 0; x < res; x++) {
                float fx = static_cast<float>(x) * invRes;
                float fy = static_cast<float>(y) * invRes;
                float fz = static_cast<float>(z) * invRes;

                // Partial derivatives via central differences
                // dFz/dy
                float dFz_dy = (sampleNoise(fx, fy + eps, fz, ox2, oy2, oz2)
                              - sampleNoise(fx, fy - eps, fz, ox2, oy2, oz2)) / (2.0f * eps);
                // dFy/dz
                float dFy_dz = (sampleNoise(fx, fy, fz + eps, ox1, oy1, oz1)
                              - sampleNoise(fx, fy, fz - eps, ox1, oy1, oz1)) / (2.0f * eps);
                // dFx/dz
                float dFx_dz = (sampleNoise(fx, fy, fz + eps, 0, 0, 0)
                              - sampleNoise(fx, fy, fz - eps, 0, 0, 0)) / (2.0f * eps);
                // dFz/dx
                float dFz_dx = (sampleNoise(fx + eps, fy, fz, ox2, oy2, oz2)
                              - sampleNoise(fx - eps, fy, fz, ox2, oy2, oz2)) / (2.0f * eps);
                // dFy/dx
                float dFy_dx = (sampleNoise(fx + eps, fy, fz, ox1, oy1, oz1)
                              - sampleNoise(fx - eps, fy, fz, ox1, oy1, oz1)) / (2.0f * eps);
                // dFx/dy
                float dFx_dy = (sampleNoise(fx, fy + eps, fz, 0, 0, 0)
                              - sampleNoise(fx, fy - eps, fz, 0, 0, 0)) / (2.0f * eps);

                // curl = nabla x F
                float curlX = dFz_dy - dFy_dz;
                float curlY = dFx_dz - dFz_dx;
                float curlZ = dFy_dx - dFx_dy;

                // Store magnitude in alpha, normalize direction in RGB
                float mag = std::sqrt(curlX * curlX + curlY * curlY + curlZ * curlZ);
                if (mag > 0.0001f) {
                    float invMag = 1.0f / mag;
                    curlX *= invMag;
                    curlY *= invMag;
                    curlZ *= invMag;
                }

                UINT idx = (z * res * res + y * res + x) * 4;
                pixelData[idx + 0] = XMConvertFloatToHalf(curlX);
                pixelData[idx + 1] = XMConvertFloatToHalf(curlY);
                pixelData[idx + 2] = XMConvertFloatToHalf(curlZ);
                pixelData[idx + 3] = XMConvertFloatToHalf(mag);
            }
        }
    }

    auto device = GET_SINGLE(RenderBase)->GetDevice();
    D3D11Utils::CreateTexture3D(
        device.Get(),
        res, res, res,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        pixelData.data(),
        m_curlNoiseTexture,
        m_curlNoiseSRV);

    std::cout << "[TextureManager] Curl Noise 3D Texture generated ("
              << res << "^3, " << (res * res * res * 8 / 1024) << " KB)" << std::endl;
}

void TextureManager::BindCurlNoiseTexture(UINT slot)
{
    if (!m_curlNoiseSRV)
        return;
    auto context = GET_SINGLE(RenderBase)->GetContext();
    ID3D11ShaderResourceView* srv = m_curlNoiseSRV.Get();
    context->CSSetShaderResources(slot, 1, &srv);
}

}
