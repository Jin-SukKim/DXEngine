#pragma once
#include "ParticleModule.h"

namespace DE {
class ParticleModuleFactory
{
public:
	using Creator = std::function<std::unique_ptr<ParticleModule>()>;

	static void Register(const std::string& type, Creator creator);
	template<typename T>
	static void Register(const std::string& type);
	static std::unique_ptr<ParticleModule> Create(const std::string& type);

private:
	static std::map<std::string, Creator>& GetRegistry();
};

template<typename T>
inline void ParticleModuleFactory::Register(const std::string& type)
{
	Register(type, []() -> std::unique_ptr<ParticleModule> {
		return std::make_unique<T>();
	});
}
}

