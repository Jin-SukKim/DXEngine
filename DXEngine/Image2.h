#pragma once
#include "Resource.h"
#include <DirectXTex.h> 

namespace DE {
    class Image2 : public Resource
    {
        using Super = Resource;
    public:
        Image2(const std::wstring& name = L"") : Super(name) {}
        ~Image2() override = default;

        // 통합 Load 함수 (확장자 자동 판별)
        bool Load(const std::string& filename) override;

        // Resize 기능
        bool Resize(size_t targetWidth, size_t targetHeight);

        // 포맷 변환 (Array 생성을 위해 포맷 통일 시 필요)
        bool Convert(DXGI_FORMAT targetFormat);

        // Getter
        const DirectX::ScratchImage& GetBuffer() const { return m_scratchImage; }
        uint32_t GetWidth() const { return static_cast<uint32_t>(m_scratchImage.GetMetadata().width); }
        uint32_t GetHeight() const { return static_cast<uint32_t>(m_scratchImage.GetMetadata().height); }
        DXGI_FORMAT GetFormat() const { return m_scratchImage.GetMetadata().format; }

    private:
        DirectX::ScratchImage m_scratchImage;
    };
}