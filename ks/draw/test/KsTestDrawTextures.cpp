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

#include <ks/draw/test/KsTestDrawBasicScene.hpp>
#include <ks/shared/KsImage.hpp>

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
                "attribute vec2 a_v2_tex0;\n"
                "\n"
                "varying lowp vec2 v_v2_tex0;\n"
                "\n"
                "void main()\n"
                "{\n"
                "   v_v2_tex0 = a_v2_tex0;\n"
                "   gl_Position = a_v4_position;\n"
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
                "varying lowp vec2 v_v2_tex0;\n"
                "uniform lowp sampler2D u_s_tex0;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = texture2D(u_s_tex0,v_v2_tex0);\n"
                "}\n";


    // ============================================================= //

    // VertexLayout
    using AttrType = gl::VertexBuffer::Attribute::Type;

    struct Vertex {
        glm::vec3 a_v3_position; // 12
        glm::vec2 a_v2_tex0; // 8
    }; // sizeof == 20

    gl::VertexLayout const vx_layout {
        { "a_v4_position", AttrType::Float, 3, false },
        { "a_v2_tex0", AttrType::Float, 2, false }
    };

    shared_ptr<draw::VertexBufferAllocator> vx_buff_allocator =
            make_shared<draw::VertexBufferAllocator>(20*6*10);

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
                            "flat_tex",
                            test::vertex_shader,
                            test::frag_shader);

                // Create the texture
                Image<RGBA8> image(16,16);
                auto& list_pixels = image.GetData();
                for(uint i=0; i < 256; i++) {

                    u8 r,g,b,a;
                    r = 0;
                    g = 0;
                    b = 0;
                    a = 255;

                    if(i < 64) {
                        r = 255;
                    }
                    else if(i < 128) {
                        g = 255;
                    }
                    else if(i < 192) {
                        b = 255;
                    }
                    else {
                        r = 255;
                        g = 255;
                    }

                    list_pixels.push_back(RGBA8{r,g,b,a});
                }

                shared_ptr<gl::Texture2D> texture =
                        make_shared<gl::Texture2D>(
                            gl::Texture2D::Format::RGBA8);

                texture->SetFilterModes(
                            gl::Texture::Filter::Nearest,
                            gl::Texture::Filter::Nearest);

                texture->SetWrapModes(
                            gl::Texture::Wrap::ClampToEdge,
                            gl::Texture::Wrap::ClampToEdge);

                texture->UpdateTexture(
                            gl::Texture2D::Update{
                                gl::Texture2D::Update::ReUpload,
                                glm::u16vec2(0,0),
                                shared_ptr<ImageData>(
                                    image.ConvertToImageDataPtr().release())
                            });

                // Add the texture
                shared_ptr<draw::TextureSet> texture_set =
                        make_shared<draw::TextureSet>();

                texture_set->list_texture_desc.emplace_back(
                            std::move(texture),1);

                m_texture_set_id = render_system->RegisterTextureSet(texture_set);

                shared_ptr<draw::UniformSet> uniform_set =
                        make_shared<draw::UniformSet>();

                uniform_set->list_uniforms.push_back(
                            make_shared<gl::Uniform<GLint>>(
                                "u_s_tex0",1));

                m_uniform_set_id = render_system->RegisterUniformSet(uniform_set);

                createRectangle();

                m_setup = true;
            }
        }

        void createRectangle()
        {
            // Create the entity
            Id ent_id = m_scene->CreateEntity();

            // Create the Render component

            // Vertex List
            unique_ptr<std::vector<u8>> list_vx =
                    make_unique<std::vector<u8>>();

            list_vx->reserve(6*sizeof(Vertex));

            auto disp = glm::vec4(0,0,0,0);

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec3{-0.4,-0.4,0},
                            glm::vec2{0,1}
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec3{0.4,0.4,0},
                            glm::vec2{1,0}
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec3{-0.4,0.4,0},
                            glm::vec2{0,0}
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec3{-0.4,-0.4,0},
                            glm::vec2{0,1}
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec3{0.4,-0.4,0},
                            glm::vec2{1,1}
                        });

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{
                            glm::vec3{0.4,0.4,0},
                            glm::vec2{1,0}
                        });

            draw::DefaultDrawKey draw_key;
            draw_key.SetShader(m_shader_id);
            draw_key.SetPrimitive(gl::Primitive::Triangles);
            draw_key.SetTextureSet(m_texture_set_id);
            draw_key.SetUniformSet(m_uniform_set_id);

            std::vector<u8> list_draw_stages = {u8(m_draw_stage_id)};

            auto cmlist_render_data_ptr =
                    static_cast<RenderDataComponentList*>(
                        m_scene->GetComponentList<RenderData>());

            auto& render_data =
                    cmlist_render_data_ptr->Create(
                        ent_id,
                        draw_key,
                        &buffer_layout,
                        nullptr,
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
        Id m_texture_set_id;
        Id m_uniform_set_id;

        RenderData* m_render_data;
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
            make_object<test::Scene>(
                app,
                window);

    shared_ptr<test::Updater> test_updater =
            make_object<test::Updater>(
                app->GetEventLoop(),
                scene);

    (void)test_updater;

    // Run!
    app->Run();

    return 0;
}

// ============================================================= //
// ============================================================= //
