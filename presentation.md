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


AlphaBlend Particle 개수
128 + 10 * 폭발 개수 + 1024