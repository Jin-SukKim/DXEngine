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
    Generate2DCurlNoiseTexture(64, 4.0f);
    CreateDefaultCurveLUT();
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

void TextureManager::CreateCurveLUTArray()
{
    auto device = GET_SINGLE(RenderBase)->GetDevice();
    const UINT width = CURVE_LUT_RESOLUTION;
    const UINT height = static_cast<UINT>(ParticleCurveType::COUNT);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = CURVE_LUT_MAX_SLICES;
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, m_curveLUTArray.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = CURVE_LUT_MAX_SLICES;
    ThrowIfFailed(device->CreateShaderResourceView(m_curveLUTArray.Get(), &srvDesc, m_curveLUTArraySRV.GetAddressOf()));

    m_nextCurveLUTSlice = 0;

    std::cout << "[TextureManager] Curve LUT Array created: "
        << width << "x" << height << "x" << CURVE_LUT_MAX_SLICES << std::endl;
}

int TextureManager::LoadCurveLUT(const std::string& key, std::unordered_map<ParticleCurveType, CurveData>& curveData)
{
    // Cache check
    auto it = m_curveLUTCache.find(key);
    if (it != m_curveLUTCache.end())
        return it->second;

    if (m_nextCurveLUTSlice >= CURVE_LUT_MAX_SLICES) {
        std::cout << "[TextureManager] Error: Curve LUT Array is full." << std::endl;
        return 0; // fallback to default slice
    }

    const UINT width = CURVE_LUT_RESOLUTION;
    const UINT height = static_cast<UINT>(ParticleCurveType::COUNT);

    std::vector<float> pixels(width * height, 0.f);

    // Fill each row
    for (UINT row = 0; row < height; ++row) {
        ParticleCurveType curveType = static_cast<ParticleCurveType>(row);
        auto curveIt = curveData.find(curveType);

        if (curveIt != curveData.end()) {
            // Use provided curve data
            const auto& baked = curveIt->second.CreateCurveData();
            UINT srcSize = static_cast<UINT>(baked.size());
            for (UINT x = 0; x < width; ++x) {
                // Resample to fixed resolution
                float t = static_cast<float>(x) / static_cast<float>(width - 1);
                UINT srcIdx = static_cast<UINT>(t * (srcSize - 1));
                srcIdx = std::min(srcIdx, srcSize - 1);
                pixels[row * width + x] = baked[srcIdx];
            }
        }
        else {
            // Default values
            // COLOR(0), ALPHA(1), SIZE(2): linear 0->1 (identity remap)
            // VELOCITY(3), DRAG(4), GRAVITY(5), NOISE_STRENGTH(6), VORTEX(7), ORBIT(8): constant 1.0
            for (UINT x = 0; x < width; ++x) {
                if (row <= 2) {
                    pixels[row * width + x] = static_cast<float>(x) / static_cast<float>(width - 1);
                }
                else {
                    pixels[row * width + x] = 1.0f;
                }
            }
        }
    }

    // Write into array slice via UpdateSubresource
    auto context = GET_SINGLE(RenderBase)->GetContext();
    UINT sliceIndex = m_nextCurveLUTSlice;
    UINT subresource = D3D11CalcSubresource(0, sliceIndex, 1);
    context->UpdateSubresource(
        m_curveLUTArray.Get(),
        subresource,
        nullptr,
        pixels.data(),
        width * sizeof(float),          // row pitch
        width * height * sizeof(float)   // slice pitch
    );

    m_curveLUTCache[key] = static_cast<int>(sliceIndex);
    ++m_nextCurveLUTSlice;

    std::cout << "[TextureManager] Curve LUT created: " << key
        << " (slice " << sliceIndex << ", " << width << "x" << height << ")" << std::endl;

    return static_cast<int>(sliceIndex);
}

void TextureManager::CreateDefaultCurveLUT()
{
    CreateCurveLUTArray();
    std::unordered_map<ParticleCurveType, CurveData> empty;
    LoadCurveLUT("__default_curve_lut__", empty);
}

void TextureManager::BindCurveLUTArray(UINT slot)
{
    if (!m_curveLUTArraySRV)
        return;
    auto context = GET_SINGLE(RenderBase)->GetContext();
    context->CSSetShaderResources(slot, 1, m_curveLUTArraySRV.GetAddressOf());
}

void TextureManager::GenerateCurlNoiseTexture(UINT resolution, float frequency)
{
    using DirectX::PackedVector::XMConvertFloatToHalf;
    const UINT  res = resolution;
    const float invRes = 1.0f / static_cast<float>(res);
    const float eps = 0.01f;
    const float inv2eps = 1.0f / (2.0f * eps);

    // 3개의 독립 노이즈 — seed만 다르면 됨
    SimplexNoise noiseX(0), noiseY(1), noiseZ(2);

    // Noise3D 하나는 스칼라값(숫자 1개) 만 반환
    auto sampleX = [&](float x, float y, float z) { return noiseX.Noise3D(x * frequency, y * frequency, z * frequency); };
    auto sampleY = [&](float x, float y, float z) { return noiseY.Noise3D(x * frequency, y * frequency, z * frequency); };
    auto sampleZ = [&](float x, float y, float z) { return noiseZ.Noise3D(x * frequency, y * frequency, z * frequency); };

    std::vector<uint16_t> pixelData(res * res * res * 4);

    for (UINT z = 0; z < res; z++)
        for (UINT y = 0; y < res; y++)
            for (UINT x = 0; x < res; x++)
            {
                float fx = x * invRes, fy = y * invRes, fz = z * invRes;

                // 편미분 (중앙 차분)
                // curlX = dFz/dy - dFy/dz   → 편미분 2개
                // dFz/dy ≈ (Fz(y+ε) - Fz(y-ε)) / 2ε  → 2번 샘플
                float dFx_dy = (sampleX(fx, fy + eps, fz) - sampleX(fx, fy - eps, fz)) * inv2eps;
                float dFx_dz = (sampleX(fx, fy, fz + eps) - sampleX(fx, fy, fz - eps)) * inv2eps;
                float dFy_dx = (sampleY(fx + eps, fy, fz) - sampleY(fx - eps, fy, fz)) * inv2eps;
                float dFy_dz = (sampleY(fx, fy, fz + eps) - sampleY(fx, fy, fz - eps)) * inv2eps;
                float dFz_dx = (sampleZ(fx + eps, fy, fz) - sampleZ(fx - eps, fy, fz)) * inv2eps;
                float dFz_dy = (sampleZ(fx, fy + eps, fz) - sampleZ(fx, fy - eps, fz)) * inv2eps;

                // curl = ∇ × F (Curl은 벡터장의 회전을 구하는 연산)
                float curlX = dFz_dy - dFy_dz;
                float curlY = dFz_dx - dFx_dz;
                float curlZ = dFy_dx - dFx_dy;

                float mag = std::sqrt(curlX * curlX + curlY * curlY + curlZ * curlZ);
                float invMag = (mag > 0.0001f) ? 1.0f / mag : 0.0f;

                UINT idx = (z * res * res + y * res + x) * 4;
                pixelData[idx + 0] = XMConvertFloatToHalf(curlX * invMag);
                pixelData[idx + 1] = XMConvertFloatToHalf(curlY * invMag);
                pixelData[idx + 2] = XMConvertFloatToHalf(curlZ * invMag);
                pixelData[idx + 3] = XMConvertFloatToHalf(mag);
            }

    auto device = GET_SINGLE(RenderBase)->GetDevice();
    D3D11Utils::CreateTexture3D(device.Get(), res, res, res,
        DXGI_FORMAT_R16G16B16A16_FLOAT, pixelData.data(),
        m_curlNoiseTexture, m_curlNoiseSRV);

    std::cout << "[TextureManager] Curl Noise Texture: "
        << res << "^3, " << (res * res * res * 8 / 1024) << " KB\n";
}

void TextureManager::BindCurlNoiseTexture(UINT slot)
{
    if (!m_curlNoiseSRV)
        return;
    auto context = GET_SINGLE(RenderBase)->GetContext();
    ID3D11ShaderResourceView* srv = m_curlNoiseSRV.Get();
    context->CSSetShaderResources(slot, 1, &srv);
}

void TextureManager::Generate2DCurlNoiseTexture(UINT resolution, float frequency)
{
    using DirectX::PackedVector::XMConvertFloatToHalf;
    const UINT  res = resolution;
    const float invRes = 1.0f / static_cast<float>(res);
    const float eps = 0.01f;
    const float inv2eps = 1.0f / (2.0f * eps);

    // 2D Curl은 스칼라 노이즈 1개로 충분
    SimplexNoise noise(0);
    auto sample = [&](float x, float y) {
        return noise.Noise2D(x * frequency, y * frequency);
    };

    // 2D 텍스처: res * res
    std::vector<uint16_t> pixelData(res * res * 4);

    for (UINT y = 0; y < res; y++)
        for (UINT x = 0; x < res; x++)
        {
            float fx = x * invRes, fy = y * invRes;

            // 2D Curl: 스칼라 포텐셜 ψ의 편미분
            // curlX =  dψ/dy
            // curlY = -dψ/dx
            float dPsi_dx = (sample(fx + eps, fy) - sample(fx - eps, fy)) * inv2eps;
            float dPsi_dy = (sample(fx, fy + eps) - sample(fx, fy - eps)) * inv2eps;

            float curlX = dPsi_dy;
            float curlY = -dPsi_dx;

            float mag = std::sqrt(curlX * curlX + curlY * curlY);
            float invMag = (mag > 0.0001f) ? 1.0f / mag : 0.0f;

            UINT idx = (y * res + x) * 4;
            pixelData[idx + 0] = XMConvertFloatToHalf(curlX * invMag);  // R: X 방향
            pixelData[idx + 1] = XMConvertFloatToHalf(curlY * invMag);  // G: Y 방향
            pixelData[idx + 2] = XMConvertFloatToHalf(0.0f);            // B: 미사용
            pixelData[idx + 3] = XMConvertFloatToHalf(mag);             // A: 크기
        }

    auto device = GET_SINGLE(RenderBase)->GetDevice();
    D3D11Utils::CreateTexture(
        device.Get(), res, res,
        DXGI_FORMAT_R16G16B16A16_FLOAT, pixelData.data(),
        m_curlNoise2D);

    std::cout << "[TextureManager] 2D Curl Noise Texture: "
        << res << "^2, " << (res * res * 8 / 1024) << " KB\n";
}

void TextureManager::Bind2DCurlNoiseTexture(UINT slot)
{
    auto context = GET_SINGLE(RenderBase)->GetContext();
    context->PSSetShaderResources(slot, 1, m_curlNoise2D.GetAddressOfSRV());
}
}
