#include "pch.h"
#include "ParticleModuleFactory.h"

namespace DE {


void ParticleModuleFactory::Register(const std::string& type, Creator creator)
{
    GetRegistry()[type] = creator;
}

std::unique_ptr<ParticleModule> ParticleModuleFactory::Create(const std::string& type)
{
    auto& reg = GetRegistry();
    if (reg.find(type) != reg.end())
        return reg[type]();
    return nullptr;
}

std::map<std::string, ParticleModuleFactory::Creator>& ParticleModuleFactory::GetRegistry()
{
    static std::map<std::string, Creator> registry;
    return registry;
}

}
