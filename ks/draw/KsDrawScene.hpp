/*
   Copyright (C) 2015-2016 Preet Desai (preet.desai@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef KS_DRAW_SCENE_HPP
#define KS_DRAW_SCENE_HPP

#include <ks/ecs/KsEcs.hpp>
#include <ks/shared/KsGraph.hpp>
#include <ks/draw/KsDrawSystem.hpp>

namespace ks
{
    namespace draw
    {
        template<typename SceneKey>
        class Scene : public ks::ecs::Scene<SceneKey>
        {
        public:
            using base_type = ks::ecs::Scene<SceneKey>;

            Scene(ks::Object::Key const &key,
                  shared_ptr<EventLoop> event_loop) :
                ks::ecs::Scene<SceneKey>(key,event_loop)
            {}

            void Init(ks::Object::Key const &,
                      shared_ptr<Scene> const &)
            {}

            ~Scene() = default;

            // Its expected that this function is only called
            // with the update thread
            Graph<shared_ptr<System>> GetSystemGraph() const
            {
                return m_system_graph;
            }

            // Its expected that this function is only called
            // with the update thread
            void SetSystemGraph(Graph<shared_ptr<System>> system_graph)
            {
                m_system_graph = std::move(system_graph);
            }

        protected:
            Graph<shared_ptr<System>> m_system_graph;
        };
    }
}

#endif // KS_DRAW_SCENE_HPP
