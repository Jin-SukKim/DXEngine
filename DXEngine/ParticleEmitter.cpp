#include "pch.h"
#include "ParticleEmitter.h"
#include "RenderModule.h"
#include "ParticleModuleFactory.h"

#include "SpawnModule.h"
#include "VisualModule.h"
#include "ForceModule.h"
#include "VortexModule.h"
#include "RenderModule.h"	
#include "MaterialModule.h"	
#include "ParticleContext.h"
namespace DE {

	ParticleEmitter::ParticleEmitter(const std::wstring& name) : m_name(name)
	{
		ParticleModuleFactory::Register<SpawnModule>("Spawn");
		ParticleModuleFactory::Register<VisualModule>("Visual");
		ParticleModuleFactory::Register<ForceModule>("Force");
		ParticleModuleFactory::Register<VortexModule>("Vortex");
		ParticleModuleFactory::Register<BillboardRenderModule>("BillboardRender");
		ParticleModuleFactory::Register<MaterialModule>("Material");
		ParticleModuleFactory::Register<MeshRenderModule>("MeshRender");
	}

	ParticleEmitter::~ParticleEmitter()
	{
		if (m_watcherID != 0 && !m_jsonPath.empty()) {
			try {
				FileWatcher::Get().Unregister(m_jsonPath, m_watcherID);
			}
			catch (...) {
				// 프로그램 종료 시 무시
			}
		}
	}

	ParticleEmitter::ParticleEmitter(const ParticleEmitter& other)
		: m_jsonPath(other.m_jsonPath)
		, m_watcherID(0)  // Hot-Reload는 복사 안 함
	{
		// Factory 등록 (필요시)
		ParticleModuleFactory::Register<SpawnModule>("Spawn");
		ParticleModuleFactory::Register<VisualModule>("Visual");
		ParticleModuleFactory::Register<ForceModule>("Force");
		ParticleModuleFactory::Register<VortexModule>("Vortex");
		ParticleModuleFactory::Register<BillboardRenderModule>("BillboardRender");
		ParticleModuleFactory::Register<MaterialModule>("Material");
		ParticleModuleFactory::Register<MeshRenderModule>("MeshRender");

		//  Module 복제
		for (const auto& mod : other.m_modules) {
			if (mod) {
				auto clonedModule = mod->Clone();
				if (clonedModule) {
					m_modules.push_back(std::move(clonedModule));
				}
			}
		}

		// Constant Buffer 데이터 복사
		m_consts = other.m_consts;
		m_frameConsts = other.m_frameConsts;

		// GPU 버퍼는 Initialize()에서 재생성
	}

	void ParticleEmitter::Initialize()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();
		
		m_consts.Initialize();
		m_frameConsts.Initialize();

		ParticleInitContext initCtx = { 
			device.Get(), 
			m_consts.GetCpu(),
			m_frameConsts.GetCpu() 
		};

		for (auto& mod : m_modules)
			mod->Initialize(initCtx);

		InitializeShaders(device.Get());
		InitializeBuffers(device);
	}

	void ParticleEmitter::OnSpawn()
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		SimulationContext simCtx = {
			context.Get(),
			m_consts,
			m_frameConsts,
			0.f,
			0.f,
			m_consume,
			m_append,
			m_countSRV.Get(),
			m_dispatchArgs.GetBuffer(),
			device.Get(),
			this->GetModule<RenderModule>()
		};

		for (auto& mod : m_modules)
			mod->OnSpawn(simCtx);

		m_consts.Upload();
		context->CSSetConstantBuffers(5, 1, m_consts.GetAddressOf());
	}

	void ParticleEmitter::Reset()
	{
		Initialize();
	}

	void ParticleEmitter::SetParticleConfig(const ParticleConsts& config)
	{
		m_consts.SetCpuData(config);
	}

	void ParticleEmitter::SetHotReloadInfo(const std::wstring& path, FileWatcher::CallbackID id)
	{
		m_jsonPath = path;
		m_watcherID = id;
	}

	void ParticleEmitter::InitializeShaders(ID3D11Device* device)
	{
		// 셰이더 로드
		m_argsUpdateCS.Initialize(device, L"ParticleArgsUpdateCS.hlsl");
	}

	void ParticleEmitter::InitializeBuffers(ComPtr<ID3D11Device>& device)
	{
		// 핑퐁 업데이트를 위한 이중 버퍼 파티클 저장소
		m_consume.Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);
		m_append.Initialize(device.Get(), m_frameConsts.GetCpu().maxParticles);

		// 간접 디스패치 및 드로우 인수
		m_dispatchArgs.Initialize(device.Get(), { 0, 1, 1 }, 4);

		// 활성 파티클 개수를 추적하는 카운터 버퍼
		D3D11Utils::CreateBuffer(device.Get(), sizeof(UINT), nullptr,
			DXGI_FORMAT_R32_UINT, m_countBuffer, m_countSRV);
	}

	void ParticleEmitter::Update(const float& dt, const float& time)
	{
		ComPtr<ID3D11Device>& device = GET_SINGLE(RenderBase)->GetDevice();
		ComPtr<ID3D11DeviceContext>& context = GET_SINGLE(RenderBase)->GetContext();

		m_frameConsts.GetCpu().dt = dt;
		m_frameConsts.GetCpu().time = time;
		
		SimulationContext simCtx = {
			context.Get(),
			m_consts,
			m_frameConsts,
			dt,
			time,
			m_consume,
			m_append,
			m_countSRV.Get(),
			m_dispatchArgs.GetBuffer(),
			device.Get(),
		};

		for (auto& mod : m_modules)
			mod->OnUpdateCPU(simCtx);

		m_frameConsts.Upload();
		context->CSSetConstantBuffers(4, 1, m_frameConsts.GetAddressOf());
		context->CSSetConstantBuffers(5, 1, m_consts.GetAddressOf());

		for (auto& mod : m_modules)
			mod->PreUpdate(simCtx);

		UpdateArgsBuffers(context.Get());

		for (auto& mod : m_modules)
			mod->UpdateArgs(simCtx);

		for (auto& mod : m_modules)
			mod->OnUpdate(simCtx);

		// 다음 프레임을 위한 버퍼 교환 
		swap(m_consume, m_append);
	}

	void ParticleEmitter::UpdateArgsBuffers(ID3D11DeviceContext* context)
	{
		// Append 버퍼에서 현재 파티클 개수를 카운트 버퍼로 복사
		context->CopyStructureCount(m_countBuffer.Get(), 0, m_consume.GetUAV());

		ID3D11UnorderedAccessView* argUAVs[] = {
			m_dispatchArgs.GetUAV()
		};

		context->CSSetShaderResources(0, 1, m_countSRV.GetAddressOf());
		context->CSSetUnorderedAccessViews(0, 1, argUAVs, nullptr);

		// Indirect Args Update
		m_argsUpdateCS.Dispatch(context, 1, 1, 1);
	}

	void ParticleEmitter::Render()
	{
		ID3D11DeviceContext* context = GET_SINGLE(RenderBase)->GetContext().Get();
		
		RenderContext renderCtx = {
			context,
			m_consts,
			m_frameConsts,
			m_consume.GetSRV(),
			this->GetModule<MaterialModule>()
		};

		context->PSSetConstantBuffers(5, 1, m_consts.GetAddressOf());
		for (auto& mod : m_modules)
			mod->OnRender(renderCtx);
	}

	void ParticleEmitter::AddModule(std::unique_ptr<ParticleModule>&& module) {
		for (auto& mod : m_modules) {
			if (typeid(*mod) == typeid(*module)) {
				mod = std::move(module);
				break;
			}
		}

		if (module)
			m_modules.emplace_back(std::move(module));

		std::sort(m_modules.begin(), m_modules.end(),
			[](const std::unique_ptr<ParticleModule>& a, const std::unique_ptr<ParticleModule>& b) {
				return a->GetPriority() < b->GetPriority();
			});
	}
}