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

#include <ks/shared/KsCallbackTimer.hpp>
#include <ks/gui/KsGuiApplication.hpp>
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
              weak_ptr<gui::Application> app,
              weak_ptr<gui::Window> win) :
            ks::draw::Scene<SceneKey>(
                key,app.lock()->GetEventLoop()),
            m_app(app),
            m_win(win),
            m_running(false)
        {
            // Create Systems
            m_render_system =
                    make_unique<RenderSystem>(
                        this);

            m_batch_system =
                    make_unique<BatchSystem>(
                        this);
        }

        void Init(ks::Object::Key const &,
                  shared_ptr<test::Scene> const &scene)
        {
            auto app = m_app.lock();

            // Application ---> Scene
            app->signal_init.Connect(
                        scene,
                        &Scene::onAppInit);

            app->signal_pause.Connect(
                        scene,
                        &Scene::onAppPause,
                        ks::ConnectionType::Direct);

            app->signal_resume.Connect(
                        scene,
                        &Scene::onAppResume);

            app->signal_quit.Connect(
                        scene,
                        &Scene::onAppQuit,
                        ks::ConnectionType::Direct);

            app->signal_processed_events->Connect(
                        scene,
                        &Scene::onAppProcEvents);

            // Scene ---> Application
            signal_app_process_events.Connect(
                        app,
                        &gui::Application::ProcessEvents);
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

        shared_ptr<gui::Window> GetWindow()
        {
            return m_win.lock();
        }

        Signal<> signal_before_update;
        Signal<> signal_before_render;


    private:
        void onAppInit()
        {
            m_running = true;
            signal_app_process_events.Emit();
        }

        void onAppPause()
        {
            m_running = false;
        }

        void onAppResume()
        {
            m_running = true;
            signal_app_process_events.Emit();
        }

        void onAppQuit()
        {
            m_running = false;
        }

        void onAppProcEvents(bool)
        {
            if(m_running)
            {
                // Update
                signal_before_update.Emit();

                draw::time_point a;
                draw::time_point b;

                m_batch_system->Update(a,b);
                m_render_system->Update(a,b);

                // Sync + Render
                auto win = m_win.lock();
                if(win->SetContextCurrent())
                {
                    m_render_system->Sync();
                    m_render_system->Render();
                    win->SwapBuffers();
                }

                // Schedule next update, vsync will eventually
                // block this until the next vblank
                signal_app_process_events.Emit();
            }
        }

        weak_ptr<gui::Application> m_app;
        weak_ptr<gui::Window> m_win;
        std::atomic<bool> m_running;

        unique_ptr<RenderSystem> m_render_system;
        unique_ptr<BatchSystem> m_batch_system;

        Signal<> signal_app_process_events;
    };

    // ============================================================= //
}
