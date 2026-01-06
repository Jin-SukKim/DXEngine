#pragma once

namespace DE {
	class Gui
	{
	public:
		Gui() {};
		virtual ~Gui() {}

		virtual void Initialize() {}
		virtual void Update() {}

		void SetSelectedActor(Actor* actor) { m_actor = actor; }
		Actor* GetSelectedActor() { return m_actor; }
	
	protected:
		Actor* m_actor; // 정보를 보여주기 위한 actor
		bool m_show = true;
	};
}
