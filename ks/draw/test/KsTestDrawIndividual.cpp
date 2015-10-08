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

#include <ks/gui/KsGuiWindow.hpp>
#include <ks/gui/KsGuiApplication.hpp>
#include <ks/platform/KsPlatformMain.hpp>

#include <ks/draw/test/KsTestDrawBasicScene.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
                "attribute vec4 a_v4_color;\n"
                "\n"
                "uniform mat4 u_m4_model;\n"
                "\n"
                "varying lowp vec4 v_v4_color;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_Position = u_m4_model * a_v4_position;\n"
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
        },
        {
            "a_v4_color",
            gl::VertexBuffer::Attribute::Type::UByte,
            4,
            true
        }
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
            m_shader_id(0)
        {

        }

        void Init(ks::Object::Key const &,
                  shared_ptr<Updater> const &this_updater)
        {
            m_scene->signal_before_update.Connect(
                        this_updater,
                        &Updater::onUpdate);
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
            }

            m_angle_rads += (3.14/180.0);
            m_xf = glm::rotate(m_angle_rads,glm::vec3(0,0,1));

            auto u_m4_model_base_ptr =
                    m_render_data->GetUniformList()->at(0).get();

            auto u_m4_model_ptr =
                    static_cast<gl::Uniform<glm::mat4>*>(
                        u_m4_model_base_ptr);

            u_m4_model_ptr->Update(m_xf);
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

            auto disp = glm::vec4(0,0,0,0);

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            (glm::vec4(-0.433,-0.25,0,1)*0.5f)+disp,
                            glm::u8vec4(0,255,255,255)
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            (glm::vec4(0.433,-0.25,0,1)*0.5f)+disp,
                            glm::u8vec4(255,0,255,255)
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            (glm::vec4(0,0.5,0,1)*0.5f)+disp,
                            glm::u8vec4(255,255,0,255)
                        });

            draw::DefaultDrawKey draw_key;
            draw_key.SetShader(m_shader_id);
            draw_key.SetPrimitive(gl::Primitive::Triangles);

            std::vector<u8> list_draw_stages = {u8(m_draw_stage_id)};

            auto list_uniforms =
                    make_shared<draw::ListUniformUPtrs>();

            list_uniforms->push_back(
                        make_unique<gl::Uniform<glm::mat4>>(
                            "u_m4_model",m_xf));

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

            auto& geometry = render_data.GetGeometry();
            geometry.GetVertexBuffers().push_back(std::move(list_vx));
            geometry.SetVertexBufferUpdated(0);

            m_render_data = &render_data;
        }

        shared_ptr<Scene> m_scene;
        bool m_setup;
        Id m_draw_stage_id;
        Id m_shader_id;

        RenderData* m_render_data;

        glm::mat4 m_xf;
        float m_angle_rads{0};
    };
}

// ============================================================= //
// ============================================================= //

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // Create application
    shared_ptr<gui::Application> app =
            make_object<gui::Application>();

    // Create the render thread
    shared_ptr<EventLoop> render_evl = make_shared<EventLoop>();

    std::thread render_thread =
            EventLoop::LaunchInThread(render_evl);

    // Create the scene thread
    shared_ptr<EventLoop> scene_evl = make_shared<EventLoop>();

    std::thread scene_thread =
            EventLoop::LaunchInThread(scene_evl);


    // Create window
    gui::Window::Attributes win_attribs;
    gui::Window::Properties win_props;

    shared_ptr<gui::Window> window =
            app->CreateWindow(
                render_evl,
                win_attribs,
                win_props);

    shared_ptr<test::Scene> scene =
            make_object<test::Scene>(
                scene_evl,
                window,
                std::chrono::milliseconds(17));

    shared_ptr<test::Updater> test_updater =
            make_object<test::Updater>(
                scene_evl,
                scene);

    (void)test_updater;

    // Run!
    app->Run();

    // Stop threads
    EventLoop::RemoveFromThread(scene_evl,scene_thread,true);
    EventLoop::RemoveFromThread(render_evl,render_thread,true);

    return 0;
}

// ============================================================= //
// ============================================================= //
