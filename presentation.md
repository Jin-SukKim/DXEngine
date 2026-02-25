2주차
- ParticleSystem과 ParticleEmitter를 구현
	- WorldMove, Stop, Speed, SpawnPer, Slow, Result, pause 등 기본 기능
	- 여러 Emitter를 하나의 System이 가질 수 있도록 구현
- Json Data를 Load해오는 Data Driven 방식으로 구현

3주차
- 3D Mesh, Texturing, Spawn (Vertex, Surface, Texture), Local/World Space 구현
	- 기능 확장

4주차
- Spawner와 ClickEffect를 구현해 이펙트를 생성
- SubEmitter 시스템 추가
- 다양한 Effect 구현
- 메모리 풀을 위해 Particle System, Particle Emitter Refactoring

5~6주차
- 대규모 파티클 시스템을 위한 메모리 풀 구현
	- Defragmentation, GPU Compacting, 
	- Particle Double Buffering -> IndexBuffer Double Buffering
- Priority Based Remove로 오래된 파티클 제거 (메모리가 가득 차서 더이상 파티클 생성 못할때)
- 파티클 시스템 초기화 병목 개선, Update/Rendering 최적화
- overdraw 방지를 위한 Off-Screen Particle 구현

7주차
- AlphaBlend 모드 추가
- Soft Particle
- Sprite Animation 개선
- Velocity Stretch Billboard 구현
- Noise, Curve Data 구현

8주차
- 기존 기능 버그 수정
- 이펙트 생성 및 씬 구성

프로젝트 시작 날짜 (Jan 6) - 1월 6일
be223c99d22aecc1676431f13edf4cf975f7898e - 이 Git Commit부터 ParticleSystem 프로젝트 시작


이펙트 당 Particle 개수

Firework
	- UpFirework : Additive, 최대 512, spawnrate 10, perSpawn 5
	- Burst : Additive, 최대 2048, burst 2048

Rain
	- Drop : Additive, 최대 2048, spawnRate 40, perSpawn 50
	- Mist : AlphaBlend, 최대 512, spawnRate 15, perSpawn 15
	- Splash : AlphaBlend, 최대 128, spawnRate 10, perSpawn 15

CrystalShatter
	- Flash : Additive, 최대 32, burst 32
	- Dust : AlphaBlend, 최대 256, spawnRate 10, perSpawn 15
	- Fragment : Additive, 최대 128, burst 128

GalaxySwirl
	- Core : Additive, 최대 32, spawnRate 5, perSpawn 1
	- Star : Additive, 최대 1024, spawnRate 20, perSpawn 5

PortalGateway
	- Flash : Additive, 최대 18, spawnRate 5, perSpawn 1
	- Energy : AlphaBlend, 최대 512, spawnRate 20, perSpawn 4
	- Spark : Additive, 최대 256, spawnRate 10, perSpawn 5
	- Ring : Additive, 최대 512, spawnRate 40, perSpawn 3

Explosion
	- Explosion : Additive, 최대 10, burst 10
	- Smoke : AlphaBlend, 최대 10, burst 10
	- Spark : Additive, 최대 50, burst 50
	- Debrie : Opaque, 최대 15, burst 15 (Mesh)

SwordClash 
	- Circle : Additive, 최대 1, burst 1
	- Flash : Additive, 최대 10, burst 10
	- Spark : Additive, 최대 30, burst 30
	- Ember : Opaque, 최대 32, burst 32

Magic
	- Charge : Additive, 최대 1024, spawnRate 100, perSpawn 10
	- Core : Additive, 최대 256, spawnRate 60, perSpawn 5
	- RuneCircle : Additive, 최대 500, spawnRate 16, perSpawn 16
	- Release : Additive, 최대 250, burst 250
	- Ice : Opaque, 최대 10, burst 10 (Mesh)

FireFly
	- FireFly : Additive, 최대 512, spawnRate 15, perSpawn 5

Fire
	- Fire : Additive, 최대 10000, spawnRate 30, perSpawn 100
	- Smoke : AlphaBlend, 최대 1024, spawnRate 15, perSpawn 50
	- Ember(VortexAura) : Additive, 최대 8000, spawnRate 15, perSpawn 80

SparkBurst
	- SparkBurst : Additive, 최대 10000, spawnRate 30, perSpawn 150

Fog
	- Fog : AlphaBlend, 최대 512, spawnRate 10, perSpawn 15 (SoftParticle)

Custom
	- slow : Additive, 최대 20000, spawnRate 30, perSpawn 1000

BoxMesh
	- mesh : Additive, 최대 5120, spawnRate 10, perSpawn 512