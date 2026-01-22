#pragma once
#include <Windows.h>

// TODO: Engine, Renderer, GUI, Manager class 등 1개만 필요한건 Singleton으로 사용
/* ex)
class CollisionManager
{
	GENERATE_SINGLE(CollisionManager);
public:
}
*/
#define GENERATE_SINGLE(className)		\
private:								\
	className() {}						\
public:									\
	static className* GetInstance()		\
	{									\
		static className s_instance;	\
		return &s_instance;				\
	}									

/*	ex)
GET_SINGLE(AssetManager)->LoadTexture(...)
*/
#define GET_SINGLE(className) className::GetInstance()
// #define AssetManager GET_SINGLE(AssetManager)

/* ex)
class Enemy : public GameActor
{
	GENERATE_BODY(Enemy, GameActor)
public:
}
*/
#define GENERATE_BODY(className, parent)	\
	using Super = parent;					\
public:										\
	className();							\
	virtual ~className() override;

// #define DEVICE ....->GetDevice()
// #define CONTEXT ....->GetDeviceContext()
// #define SET_PSO ..
// TODO: 필요한걸 Define

namespace DE {


	struct WindowInfo {
		WindowInfo(HWND hwnd, int width, int height) : hwnd(hwnd), width(width), height(height) {}
		HWND hwnd;
		int width;
		int height;
		//Microsoft::WRL::ComPtr<ID3D11Device> device;
		//Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	};

	// 각 오브젝트가 가지고 있는 Component의 Update 순서를 지정하기 위해 사용
	enum class ComponentType {
		Transform,
		Model,
		BoundingVolume,
		ParticleSystem,
		MaxComponentType
	};
}