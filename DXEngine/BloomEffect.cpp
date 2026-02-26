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
		// Bloom Level에 따라 2배나 1/2배로 Down/Up Sampling
		int div = int(pow(2, i));
		this->CreateBuffer(width / div, height / div, m_bloomTextures[i]);
	}

	// Down-Sampling - Blur 1/2 배로 DownSampling
	m_bloomDownFilters.resize(m_bloomLevels - 1);
	for (int i = 0; i < m_bloomLevels - 1; ++i) {
		int div = int(pow(2, i + 1));
		m_bloomDownFilters[i].Initialize(RenderBase::graphicsCommon.bloomDownPS, width / div, height / div);

		if (i == 0)
			m_bloomDownFilters[i].SetShaderResources({ resources[0] });
		else
			m_bloomDownFilters[i].SetShaderResources({ m_bloomTextures[i].GetSRV() });

		m_bloomDownFilters[i].SetRenderTargets({ m_bloomTextures[i + 1].GetRTV() });

		m_bloomDownFilters[i].UpdateConstantBuffer();
	}

	// Brightness threshold: HDR > 1.0 only blooms
	m_bloomDownFilters[0].GetConstData().threshold = 0.8f;
	m_bloomDownFilters[0].UpdateConstantBuffer();

	// Up-Sampling - Gaussian Blur 2배로 UpSampling
	m_bloomUpFilters.resize(m_bloomLevels - 1);
	for (int i = 0; i < m_bloomLevels - 1; ++i) {
		int level = m_bloomLevels - 2 - i;
		int div = int(pow(2, level));
		m_bloomUpFilters[i].Initialize(RenderBase::graphicsCommon.bloomUpPS, width / div, height / div);

		m_bloomUpFilters[i].SetShaderResources({ m_bloomTextures[level + 1].GetSRV() });
		m_bloomUpFilters[i].SetRenderTargets({ m_bloomTextures[level].GetRTV() });

		m_bloomUpFilters[i].UpdateConstantBuffer();
	}

	// Combine + ToneMapping
	m_combineFilter.Initialize(RenderBase::graphicsCommon.combinePS, width, height);
	// resource[1]      
	m_combineFilter.SetShaderResources({ resources[0], m_bloomTextures[0].GetSRV(), resources[1] });
	m_combineFilter.SetRenderTargets(targets);
	m_combineFilter.GetConstData().strength = 0.5f; // Bloom Strength
	m_combineFilter.GetConstData().option1 = 1.0f;  // Exposure
	m_combineFilter.GetConstData().option2 = 2.2f; // Gamma
	m_combineFilter.GetConstData().option3 = 0.0f; // Motion Blur 

	// float render target Gamma correction (gamma = 1.0)
	m_combineFilter.UpdateConstantBuffer();
}

void DE::BloomEffect::Render()
{
	Super::Render();

	if (m_combineFilter.GetConstData().strength > 0.f) {
		for (int i = 0; i < m_bloomDownFilters.size(); ++i)
			RenderImageFilter(m_bloomDownFilters[i]);
		for (int i = 0; i < m_bloomUpFilters.size(); ++i)
			RenderImageFilter(m_bloomUpFilters[i]);
	}

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
