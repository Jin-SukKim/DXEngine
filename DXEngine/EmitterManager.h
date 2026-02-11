#pragma once

namespace DE {

	class ParticleEmitter;

class EmitterManager
{
private:
	std::map<int, ParticleEmitter*> m_activeEmitters;

};

}

