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

#include <ks/gui/KsGuiWindow.hpp>
#include <ks/gui/KsGuiApplication.hpp>
#include <ks/platform/KsPlatformMain.hpp>

#include <ks/draw/KsDrawDefaultDrawStage.hpp>
#include <ks/draw/test/KsTestDrawBasicScene.hpp>

namespace test
{
    namespace {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_real_distribution<float> dis(-1,1); // [0,1]
    }

    // ============================================================= //

    shared_ptr<gui::Application> app;

    // ============================================================= //

    // Shader
    std::string const vertex_shader =
                "#ifdef GL_ES\n"
                "    //\n"
                "#else\n"
                "    #define lowp\n"
                "    #define mediump\n"
                "    #define highp\n"
                "#endif\n"
                "\n"
                "attribute vec4 a_v4_position;\n"
                "attribute vec4 a_v4_color;\n"
                "\n"
                "varying lowp vec4 v_v4_color;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_Position = a_v4_position;\n"
                "    v_v4_color = a_v4_color;\n"
                "}\n";

    std::string const frag_shader =
                "#ifdef GL_ES\n"
                "    precision mediump float;\n"
                "#else\n"
                "    #define lowp\n"
                "    #define mediump\n"
                "    #define highp\n"
                "#endif\n"
                "\n"
                "varying lowp vec4 v_v4_color;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = v_v4_color;\n"
                "}\n";

    // ============================================================= //

    // Vertex definition and layout
    struct Vertex {
        glm::vec4 a_v4_position;
        glm::u8vec4 a_v4_color;
    };

    gl::VertexLayout const vx_layout {
        {
            "a_v4_position",
            gl::VertexBuffer::Attribute::Type::Float,
            4,
            false
        }, // 4*4
        {
            "a_v4_color",
            gl::VertexBuffer::Attribute::Type::UByte,
            4,
            true
        } // 1*4
    };

    shared_ptr<draw::VertexBufferAllocator> vx_buff_allocator =
            make_shared<draw::VertexBufferAllocator>(24000*4);

    // Buffer layout
    draw::BufferLayout const buffer_layout(
            gl::Buffer::Usage::Static,
            { vx_layout },
            { vx_buff_allocator });

    // ============================================================= //

    // Receives an 'update' signal from the scene
    class Updater : public ks::Object
    {
    public:
        using base_type = ks::Object;

        Updater(ks::Object::Key const &key,
                shared_ptr<EventLoop> evl,
                shared_ptr<Scene> scene) :
            ks::Object(key,evl),
            m_scene(scene),
            m_setup(false),
            m_draw_stage_id(0),
            m_shader_id(0),
            m_upd_count(0)
        {

        }

        void Init(ks::Object::Key const &,
                  shared_ptr<Updater> const &this_updater)
        {
            m_scene->signal_before_update.Connect(
                        this_updater,
                        &Updater::onUpdate,
                        ks::ConnectionType::Direct);
        }

        ~Updater() = default;


    private:
        void onUpdate()
        {
//            LOG.Trace() << "frame";
//            return;

            if(!m_setup)
            {
                auto render_system = m_scene->GetRenderSystem();

                // Add a draw stage
                m_draw_stage_id =
                        render_system->RegisterDrawStage(
                            make_shared<test::DefaultDrawStage>());

                // Add the shader
                m_shader_id =
                        render_system->RegisterShader(
                            "flat_color_attr",
                            test::vertex_shader,
                            test::frag_shader);

                createTriangle();

                m_setup = true;
                m_last_update = std::chrono::high_resolution_clock::now();
            }

            TimePoint now = std::chrono::high_resolution_clock::now();
            if(CalcDuration<Milliseconds>(m_last_update,now).count() > 500)
            {
                // Remove prev entities
                auto list_ent_ids = m_scene->GetEntityIdList();
                for(Id ent_id : list_ent_ids) {
                    m_scene->RemoveEntity(ent_id);
                }

                // Add new entities
                for(uint i=0; i < 500; i++) {
                    createTriangle();
                }

                m_upd_count++;
                if(m_upd_count == 40) {
                    Signal<> quit;
                    quit.Connect(app,&gui::Application::Quit);
                    quit.Emit();
                }

                m_last_update = now;
            }
        }

        static glm::vec4 getRandomDisplacement()
        {
            return glm::vec4{
                dis(mt),dis(mt),dis(mt),0.0
            };
        }

        void createTriangle()
        {
            // Create the entity
            Id ent_id = m_scene->CreateEntity();

            // Create the Render component

            // Vertex List
            unique_ptr<std::vector<u8>> list_vx =
                    make_unique<std::vector<u8>>();

            list_vx->reserve(3*sizeof(Vertex));

            auto disp = getRandomDisplacement();

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            (glm::vec4(-0.433,-0.25,0,1)*0.5f)+disp,
                            glm::u8vec4(255,0,0,255)
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            (glm::vec4(0.433,-0.25,0,1)*0.5f)+disp,
                            glm::u8vec4(0,255,0,255)
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            (glm::vec4(0,0.5,0,1)*0.5f)+disp,
                            glm::u8vec4(0,0,255,255)
                        });

            draw::DefaultDrawKey draw_key;
            draw_key.SetShader(m_shader_id);
            draw_key.SetPrimitive(gl::Primitive::Triangles);

            std::vector<u8> list_draw_stages = {u8(m_draw_stage_id)};

            auto cmlist_render_data_ptr =
                    static_cast<RenderDataComponentList*>(
                        m_scene->GetComponentList<RenderData>());

            auto &render_data =
                    cmlist_render_data_ptr->Create(
                        ent_id,
                        draw_key,
                        &buffer_layout,
                        nullptr,
                        list_draw_stages,
                        ks::draw::Transparency::Opaque);

            render_data.GetGeometry().
                    GetVertexBuffers().push_back(std::move(list_vx));

            render_data.GetGeometry().
                    SetVertexBufferUpdated(0);
        }

        shared_ptr<Scene> m_scene;
        bool m_setup;
        Id m_draw_stage_id;
        Id m_shader_id;
        uint m_upd_count;

        TimePoint m_last_update;
    };
}

// ============================================================= //
// ============================================================= //

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // Create application
    test::app = make_object<gui::Application>();

    // Create window
    gui::Window::Attributes win_attribs;
    gui::Window::Properties win_props;
    win_props.swap_interval = 1;

    shared_ptr<gui::Window> window =
            test::app->CreateWindow(
                test::app->GetEventLoop(),
                win_attribs,
                win_props);

    shared_ptr<test::Scene> scene =
            make_object<test::Scene>(
                test::app,
                window);

    shared_ptr<test::Updater> test_updater =
            make_object<test::Updater>(
                test::app->GetEventLoop(),
                scene);

    (void)test_updater;

    // Run!
    test::app->Run();

    return 0;
}

// ============================================================= //
// ============================================================= //
