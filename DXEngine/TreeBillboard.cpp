#include "pch.h"
#include "TreeBillboard.h"
#include "GraphicsCommon.h"
#include "RenderBase.h"
namespace DE {
	TreeBillboard::TreeBillboard(ComPtr<ID3D11Device>& device, ComPtr<ID3D11DeviceContext>& context, const std::wstring& name) : Super(device, context, name)
	{
		std::vector<std::string> filenames = {
			"../Assets/Textures/TreeBillboards/1.png",
			"../Assets/Textures/TreeBillboards/2.png",
			"../Assets/Textures/TreeBillboards/3.png",
			"../Assets/Textures/TreeBillboards/4.png",
			"../Assets/Textures/TreeBillboards/5.png" };
		Vector3 point = { 0.f, 0.f, 0.f };
		this->SetBillboard(device, context, { point }, 1.0f, filenames, RenderBase::graphicsCommon.TreeBillboardPS);
	}

	void TreeBillboard::Initialize()
	{
		Super::Initialize();
	}

}