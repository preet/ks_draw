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

using namespace ks;

namespace test
{   
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
                "\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_Position = a_v4_position;\n"
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
                "uniform float u_f_dark;\n"
                "uniform float u_fa_color[3];\n"
                "\n"
                "void main()\n"
                "{\n"
                "   vec4 color;"
                "   color.r = mix(u_f_dark,u_fa_color[0],0.5);"
                "   color.g = mix(u_f_dark,u_fa_color[1],0.5);"
                "   color.b = mix(u_f_dark,u_fa_color[2],0.5);"
                "   color.a = 1.0;"
                "   gl_FragColor = color;\n"
                "}\n";

    // ============================================================= //

    // Vertex definition and layout
    struct Vertex {
        glm::vec4 a_v4_position;
    };

    gl::VertexLayout const vx_layout {
        {
            "a_v4_position",
            gl::VertexBuffer::Attribute::Type::Float,
            4,
            false
        }
    };

    shared_ptr<draw::VertexBufferAllocator> vx_buff_allocator =
            make_shared<draw::VertexBufferAllocator>(128);

    // Buffer layout
    draw::BufferLayout const buffer_layout(
            gl::Buffer::Usage::Static,
            { vx_layout },
            { vx_buff_allocator });

    // ============================================================= //

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
            m_shader_id(0)
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
                            "test_uniforms",
                            test::vertex_shader,
                            test::frag_shader);

                createTriangle();

                m_setup = true;
            }
        }

        void createTriangle()
        {
            // Create the entity
            Id ent_id = m_scene->CreateEntity();

            // Create the render component

            // Vertex List
            unique_ptr<std::vector<u8>> list_vx =
                    make_unique<std::vector<u8>>();

            list_vx->reserve(3*sizeof(Vertex));


            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec4(-0.433,-0.25,0,1)
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec4(0.433,-0.25,0,1)
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec4(0,0.5,0,1)
                        });

            // geometry desc
            draw::DefaultDrawKey draw_key;
            draw_key.SetShader(m_shader_id);
            draw_key.SetPrimitive(gl::Primitive::Triangles);

            std::vector<u8> list_draw_stages = {u8(m_draw_stage_id)};

            // uniforms
            auto list_uniforms = make_shared<draw::ListUniformUPtrs>();

            list_uniforms->emplace_back(
                        make_unique<gl::UniformArray<float>>(
                            "u_fa_color",
                            std::vector<float>{
                                0,
                                1,
                                1}));

            list_uniforms->emplace_back(
                        make_unique<gl::Uniform<float>>(
                            "u_f_dark",1));


            auto cmlist_render_data_ptr =
                    static_cast<RenderDataComponentList*>(
                        m_scene->GetComponentList<RenderData>());

            auto& render_data =
                    cmlist_render_data_ptr->Create(
                        ent_id,
                        draw_key,
                        &buffer_layout,
                        list_uniforms,
                        list_draw_stages,
                        ks::draw::Transparency::Opaque);

            render_data.GetGeometry().
                    GetVertexBuffers().
                    push_back(std::move(list_vx));

            render_data.GetGeometry().
                    SetVertexBufferUpdated(0);


            // Create the update timer
            m_uf_update_timer =
                    MakeObject<CallbackTimer>(
                        m_scene->GetEventLoop(),
                        Milliseconds(1000),
                        [list_uniforms]()
                        {
                            gl::UniformArray<float>* u_fa_color =
                                    static_cast<gl::UniformArray<float>*>(
                                        list_uniforms->at(0).get());

                            std::vector<float> list_channels =
                                    u_fa_color->GetData();

                            std::swap(list_channels[0],list_channels[1]);
                            std::swap(list_channels[2],list_channels[0]);
                            u_fa_color->Update(list_channels);


                            gl::Uniform<float>* u_f_dark =
                                    static_cast<gl::Uniform<float>*>(
                                        list_uniforms->at(1).get());

                            float darken = u_f_dark->GetData();
                            if(darken == 1.0) {
                                u_f_dark->Update(0.0);
                            }
                            else {
                                u_f_dark->Update(1.0);
                            }
                        });

            m_uf_update_timer->Start();
        }

        shared_ptr<Scene> m_scene;
        bool m_setup;
        Id m_draw_stage_id;
        Id m_shader_id;
        shared_ptr<CallbackTimer> m_uf_update_timer;
    };

    // ============================================================= //

}

// ============================================================= //
// ============================================================= //

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // Create application
    shared_ptr<gui::Application> app =
            MakeObject<gui::Application>();

    // Create window
    gui::Window::Attributes win_attribs;
    gui::Window::Properties win_props;
    win_props.swap_interval = 1;

    shared_ptr<gui::Window> window =
            app->CreateWindow(
                app->GetEventLoop(),
                win_attribs,
                win_props);

    shared_ptr<test::Scene> scene =
            MakeObject<test::Scene>(
                app,
                window);

    shared_ptr<test::Updater> test_updater =
            MakeObject<test::Updater>(
                app->GetEventLoop(),
                scene);

    (void)test_updater;

    // Run!
    app->Run();

    return 0;
}

// ============================================================= //
// ============================================================= //
