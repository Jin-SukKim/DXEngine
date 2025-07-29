#include "pch.h"
#include "BloomEffect.h"
#include "RenderBase.h"
#include "Texture2D.h"

void DE::BloomEffect::Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height)
{
	Super::Initialize(resources, targets, width, height);

	// Bloom Down/Up
	m_bloomTextures.resize(m_bloomLevels);
	for (int i = 0; i < m_bloomLevels; ++i) {
		// 원본 크기부터 시작해서 2배씩 최대 2^bloomLevels까지 Down/Up Sampling하기 위해 div로 나누기
		int div = int(pow(2, i));
		this->CreateBuffer(width / div, height / div, m_bloomTextures[i]);
	}

	// Down-Sampling - 해상도를 낮춰서(Down Sampling) Blur 효과 적용할 때 훨씬 부드러운 결과를 낼 수 있게 함
	m_bloomDownFilters.resize(m_bloomLevels - 1);
	for (int i = 0; i < m_bloomLevels - 1; ++i) {
		int div = int(pow(2, i + 1));
		m_bloomDownFilters[i].Initialize(RenderBase::graphicsCommon.bloomDownPS, width / div, height / div);

		// i 번째 Texture의 데이터를 가지고 i + 1번째 Texture에 Rendering (즉, 0번 Index에 가까울수록 해상도가 높은 Texture이기에 점점 낮은 해상도로 Down-Sampling)
		if (i == 0)
			m_bloomDownFilters[i].SetShaderResources({ resources[0] });
		else
			m_bloomDownFilters[i].SetShaderResources({ m_bloomTextures[i].GetSRV() });

		m_bloomDownFilters[i].SetRenderTargets({ m_bloomTextures[i + 1].GetRTV() });

		m_bloomDownFilters[i].UpdateConstantBuffer();
	}

	// Up-Sampling - Gaussian Blur를 2배씩 Up Sampling해 원래 해상도까지 순차적으로 적용
	m_bloomUpFilters.resize(m_bloomLevels - 1);
	for (int i = 0; i < m_bloomLevels - 1; ++i) {
		int level = m_bloomLevels - 2 - i;
		int div = int(pow(2, level));
		m_bloomUpFilters[i].Initialize(RenderBase::graphicsCommon.bloomUpPS, width / div, height / div);

		// i + 1번째 Texture를 읽어서 i 번째 Texture에 Rendering (0번 Index에서 멀수록 해상도가 낮은 Texture이기에 낮은 Texture에서 높은 Texture로 Up-Sampling)
		m_bloomUpFilters[i].SetShaderResources({ m_bloomTextures[level + 1].GetSRV() });
		m_bloomUpFilters[i].SetRenderTargets({ m_bloomTextures[level].GetRTV() });

		m_bloomUpFilters[i].UpdateConstantBuffer();
	}

	// Combine + ToneMapping
	m_combineFilter.Initialize(RenderBase::graphicsCommon.combinePS, width, height);
	// resource[1]은 모션 블러를 위한 이전 프레임 결과
	m_combineFilter.SetShaderResources({ resources[0], m_bloomTextures[0].GetSRV(), resources[1] });
	m_combineFilter.SetRenderTargets(targets);
	m_combineFilter.GetConstData().strength = 0.3f; // Bloom Strength;
	m_combineFilter.GetConstData().option1 = 1.0f;  // Exposure로 사용;
	m_combineFilter.GetConstData().option2 = 2.2f; // Gamma로 사용;
	m_combineFilter.GetConstData().option3 = 0.0f; // Motion Blur 계수로 사용;

	// 주의: float render target에서는 Gamma correction 하지 않음 (gamma = 1.0)
	m_combineFilter.UpdateConstantBuffer();
}

void DE::BloomEffect::Render()
{
	Super::Render();

	// 블룸이 필요한 경우에만 계산
	if (m_combineFilter.GetConstData().strength > 0.f) {
		for (int i = 0; i < m_bloomDownFilters.size(); ++i)
			RenderImageFilter(m_bloomDownFilters[i]);
		for (int i = 0; i < m_bloomUpFilters.size(); ++i)
			RenderImageFilter(m_bloomUpFilters[i]);
	}

	// 화면 렌더링
	RenderImageFilter(m_combineFilter);
}

void DE::BloomEffect::Update()
{
	for (ImageFilter& f : m_bloomDownFilters)
		f.UpdateConstantBuffer();
	for (ImageFilter& f : m_bloomUpFilters)
		f.UpdateConstantBuffer();
	
	m_combineFilter.UpdateConstantBuffer();
}
