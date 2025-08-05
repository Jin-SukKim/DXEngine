#pragma once
#include "Gui.h"

namespace DE {
    class Outliner : public Gui
    {
    public:
        void Initialize() override;
        void Update() override;
    
        void SetActorLists(const std::vector<std::vector<std::shared_ptr<Actor>>>& actorLists);
    
    private:
        std::vector<std::shared_ptr<Actor>> m_actors;
    };
}
