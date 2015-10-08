/*
   Copyright (C) 2015 Preet Desai (preet.desai@gmail.com)

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

#include <ks/shared/KsCallbackTimer.hpp>
#include <ks/gui/KsGuiWindow.hpp>
#include <ks/draw/KsDrawScene.hpp>
#include <ks/draw/KsDrawRenderSystem.hpp>
#include <ks/draw/KsDrawBatchSystem.hpp>
#include <ks/draw/KsDrawDefaultDrawKey.hpp>
#include <ks/draw/KsDrawDefaultDrawStage.hpp>

using namespace ks;

namespace test
{
    // ============================================================= //

    struct SceneKey {
        static uint const max_component_types{8};
    };

    // ============================================================= //

    using DefaultDrawStage = draw::DefaultDrawStage<draw::DefaultDrawKey>;

    using RenderData = draw::RenderData<draw::DefaultDrawKey>;

    using RenderDataComponentList = ecs::ComponentList<SceneKey,RenderData>;

    using BatchData = draw::BatchData;

    using BatchDataComponentList = ecs::ComponentList<SceneKey,BatchData>;

    using Batch = draw::Batch<draw::DefaultDrawKey>;

    // ============================================================= //

    class Scene : public ks::draw::Scene<SceneKey>
    {
        using RenderSystem =
            draw::RenderSystem<SceneKey,draw::DefaultDrawKey>;

        using BatchSystem =
            draw::BatchSystem<SceneKey,draw::DefaultDrawKey>;

    public:
        using base_type = ks::draw::Scene<SceneKey>;

        Scene(ks::Object::Key const &key,
              shared_ptr<EventLoop> event_loop,
              shared_ptr<gui::Window> window,
              std::chrono::milliseconds update_dt_ms) :
            ks::draw::Scene<SceneKey>(key,event_loop),
            m_window(window),
            m_update_dt_ms(update_dt_ms)
        {
            // Create a window layer
            m_win_layer = window->CreateLayer(0);

            // Create Systems
            m_render_system =
                    make_unique<RenderSystem>(
                        this);

            m_batch_system =
                    make_unique<BatchSystem>(
                        this,
                        m_render_system->GetRenderDataComponentList());
        }

        void Init(ks::Object::Key const &,
                  shared_ptr<test::Scene> const &)
        {
            // Post the init task to this scene's EventLoop
            auto init_task =
                    make_shared<ks::Task>(
                        [this](){
                            this->onInit();
                        });

            this->GetEventLoop()->PostTask(init_task);
        }

        ~Scene() {}

        RenderSystem* GetRenderSystem()
        {
            return m_render_system.get();
        }

        BatchSystem* GetBatchSystem()
        {
            return m_batch_system.get();
        }

        Signal<> signal_before_update;

    private:
        // Should be called from the context
        // of this Scene's event loop
        void onInit()
        {
            shared_ptr<test::Scene> this_scene =
                    std::static_pointer_cast<test::Scene>(
                        shared_from_this());

            // Create update timer
            Scene* scene_ptr = this_scene.get();

            this_scene->m_update_timer =
                    make_object<CallbackTimer>(
                        this_scene->GetEventLoop(),
                        this_scene->m_update_dt_ms,
                        [=](){ scene_ptr->onUpdate(); });


            // Window Layer ---> Scene
            m_win_layer->signal_render.Connect(
                        this_scene,
                        &Scene::onRender,
                        ks::ConnectionType::Direct);

            m_win_layer->signal_sync.Connect(
                        this_scene,
                        &Scene::onSync,
                        ks::ConnectionType::Direct);


            // Do the first Sync (is this necessary?)
            shared_ptr<gui::Window> window = m_window.lock();
            gui::Window* window_ptr = window.get();
            auto sync_task =
                    make_shared<ks::Task>(
                        [window_ptr,this](){
                            window_ptr->SyncLayer(m_win_layer);
                        });

            window->GetEventLoop()->PostTask(sync_task);
            sync_task->Wait();


            // Start updates
            m_update_timer->Start();
        }

        void onUpdate()
        {
//            LOG.Trace() << "U";

            signal_before_update.Emit();

            draw::time_point a;
            draw::time_point b;

            m_batch_system->Update(a,b);
            m_render_system->Update(a,b);

            // Sync
            auto window = m_window.lock();
            gui::Window* window_ptr = window.get();

            auto sync_task =
                    make_shared<ks::Task>(
                        [window_ptr,this](){
                            window_ptr->SyncLayer(m_win_layer);
                        });

            window->GetEventLoop()->PostTask(sync_task);
            sync_task->Wait();
        }

        void onSync()
        {
//            LOG.Trace() << "S";
            m_render_system->Sync();
        }

        void onRender()
        {
//            LOG.Trace() << "R";
            m_render_system->Render();
        }

        void onWindowCreated()
        {
            auto window = m_window.lock();
            window->SyncLayer(m_win_layer);

            m_update_timer->Start();
        }

        void onWindowResized(uint, uint)
        {}


        weak_ptr<gui::Window> m_window;
        shared_ptr<gui::Layer> m_win_layer;
        std::chrono::milliseconds m_update_dt_ms;
        shared_ptr<CallbackTimer> m_update_timer;
        unique_ptr<RenderSystem> m_render_system;
        unique_ptr<BatchSystem> m_batch_system;
    };

    // ============================================================= //
}
