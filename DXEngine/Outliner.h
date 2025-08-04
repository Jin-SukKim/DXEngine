#pragma once

namespace DE {
    class Outliner
    {
    public:
        void Initialize(const std::vector<std::vector<std::shared_ptr<Actor>>>& actorLists);
        void Update();
    
        std::vector<std::shared_ptr<Actor>> m_actors;
        bool m_show = true;
    };
}
