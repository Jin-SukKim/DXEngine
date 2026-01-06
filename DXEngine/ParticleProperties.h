#pragma once

namespace DE {
	struct ParticleProperties {
		Vector3 color;
		float size;
		Vector2 scale;
		Vector3 position;
		float speed;
		Vector3 direction;
		Vector3 gravity;
		float lifeTime;
	};

	struct EmitterProperties {
		Vector2 minMaxSize;
		Vector2 minMaxScaleX;
		Vector2 minMaxScaleY;
		Vector2 minMaxSpeed;
		Vector2 minMaxLifeTime;
	};
}