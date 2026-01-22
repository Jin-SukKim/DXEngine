#pragma once
#include "Gui.h"

namespace DE {
    class Outliner : public Gui
    {
    public:
        void Initialize() override;
        void Update() override;
    
        void SetActorLists(const std::vector<std::vector<Actor*>>& actorLists);
    
    private:
        std::vector<Actor*> m_actors;
    };
}
