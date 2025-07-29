#pragma once
#include "PostProcess.h"

namespace DE {
	class Texture2D;
	class BloomEffect : public PostProcess
	{
		using Super = PostProcess;
	public:
		void Initialize(const std::vector<ComPtr<ID3D11ShaderResourceView>>& resources, const std::vector<ComPtr<ID3D11RenderTargetView>>& targets, int width, int height) override;
		void Update() override;
		void Render() override;
		void SetFilterLevel(int bloomLevels) { m_bloomLevels = bloomLevels; }
	private:
		ImageFilter m_combineFilter;
		std::vector<ImageFilter> m_bloomDownFilters;
		std::vector<ImageFilter> m_bloomUpFilters;
		// ShaderResourceView & RenderTargetView
		std::vector<Texture2D> m_bloomTextures;

		int m_bloomLevels = 1;
	};
}
