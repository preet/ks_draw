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

#include <ks/gl/KsGLCommands.hpp>
#include <ks/gl/KsGLUniform.hpp>
#include <ks/shared/KsImage.hpp>

#include <ks/draw/KsDrawDebugTextDrawStage.hpp>

namespace ks
{
    namespace draw
    {
        namespace detail_debug_text_draw_stage
        {
            namespace {
                // Shaders
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
                            "varying vec2 v_v2_tex0;\n"
                            "\n"
                            "void main()\n"
                            "{\n"
                            "    gl_Position = a_v4_position;\n"
                            "    v_v2_tex0 = a_v2_tex0;\n"
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
                            "varying vec2 v_v2_tex0;\n"
                            "uniform lowp sampler2D u_s_tex0;\n"
                            "uniform lowp vec4 u_v4_text_color;\n"
                            "\n"
                            "void main()\n"
                            "{\n"
                            "    vec4 color0 = vec4(1,1,1,1.0-texture2D(u_s_tex0,v_v2_tex0).r);\n"
                            "    gl_FragColor = color0*u_v4_text_color;\n"
                            "}\n";

                struct Vertex {
                    glm::vec4 a_v4_position;
                    glm::vec2 a_v2_tex0;
                };

                gl::VertexLayout const vx_layout {
                    {
                        "a_v4_position",
                        gl::VertexBuffer::Attribute::Type::Float,
                        4,
                        false
                    },
                    {
                        "a_v2_tex0",
                        gl::VertexBuffer::Attribute::Type::Float,
                        2,
                        false
                    }
                };

                // Glyph atlas image
                // * generated using GIMP
                // * Each byte represents a single pixel (1728 total pixels)
                // * A value of 1 is a white pixel
                // * A value of 0 is a black pixel

                uint const g_glyph_atlas_width{36};

                uint const g_glyph_atlas_height{48};

                std::vector<u8> const g_glyph_atlas_image_data {
                    0,0,0,0,0,1,1,0,0,1,1,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,0,1,1,1,0,1,0,0,
                    0,0,0,1,
                    0,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,
                    0,1,1,1,1,1,0,1,0,1,1,1,0,1,0,1,
                    1,1,1,1,
                    0,1,1,1,0,1,1,1,0,1,1,1,0,0,0,0,
                    0,1,1,0,0,0,0,1,0,0,0,0,0,1,0,0,
                    0,0,0,1,
                    0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,
                    1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,
                    1,1,0,1,
                    0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,1,1,1,1,0,1,0,0,
                    0,0,0,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,1,1,1,0,1,1,1,0,
                    1,1,1,1,
                    0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,
                    0,1,0,1,1,1,0,1,1,1,0,1,1,1,1,1,
                    0,1,1,1,
                    0,0,0,0,0,1,1,1,1,1,0,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,1,1,0,1,1,1,1,1,
                    0,1,1,1,
                    0,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,
                    0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,
                    0,1,1,1,
                    0,0,0,0,0,1,1,1,1,1,0,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,1,1,1,0,1,1,1,0,
                    1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    1,0,0,0,1,1,0,0,0,0,0,1,0,0,0,0,
                    0,1,0,0,0,0,1,1,0,0,0,0,0,1,0,0,
                    0,0,0,1,
                    0,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,
                    1,1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,
                    1,1,1,1,
                    0,0,0,0,0,1,0,0,0,0,1,1,0,1,1,1,
                    1,1,0,1,1,1,0,1,0,0,0,0,1,1,0,0,
                    0,0,1,1,
                    0,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,
                    1,1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,
                    1,1,1,1,
                    0,1,1,1,0,1,0,0,0,0,0,1,0,0,0,0,
                    0,1,0,0,0,0,1,1,0,0,0,0,0,1,0,1,
                    1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    0,0,0,0,0,1,0,1,1,1,0,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,0,1,1,1,0,1,0,1,
                    1,1,1,1,
                    0,1,1,1,1,1,0,1,1,1,0,1,1,1,0,1,
                    1,1,1,1,1,1,0,1,0,1,1,0,1,1,0,1,
                    1,1,1,1,
                    0,1,0,0,0,1,0,0,0,0,0,1,1,1,0,1,
                    1,1,1,1,1,1,0,1,0,0,0,1,1,1,0,1,
                    1,1,1,1,
                    0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,
                    1,1,0,1,1,1,0,1,0,1,1,0,1,1,0,1,
                    1,1,1,1,
                    0,0,0,0,1,1,0,1,1,1,0,1,0,0,0,0,
                    0,1,0,0,0,0,0,1,0,1,1,1,0,1,0,0,
                    0,0,0,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    0,1,1,1,0,1,0,1,1,1,0,1,1,0,0,0,
                    1,1,0,0,0,0,0,1,1,0,0,0,1,1,0,0,
                    0,0,1,1,
                    0,0,1,0,0,1,0,0,1,1,0,1,0,1,1,1,
                    0,1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,
                    1,1,0,1,
                    0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,
                    0,1,0,0,0,0,0,1,0,1,0,1,0,1,0,0,
                    0,0,0,1,
                    0,1,1,1,0,1,0,1,1,0,0,1,0,1,1,1,
                    0,1,0,1,1,1,1,1,0,1,1,0,0,1,0,1,
                    1,0,1,1,
                    0,1,1,1,0,1,0,1,1,1,0,1,1,0,0,0,
                    1,1,0,1,1,1,1,1,1,0,0,0,0,1,0,1,
                    1,1,0,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    1,0,0,0,0,1,0,0,0,0,0,1,0,1,1,1,
                    0,1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,
                    1,1,0,1,
                    0,1,1,1,1,1,1,1,0,1,1,1,0,1,1,1,
                    0,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,
                    1,0,1,1,
                    1,0,0,0,1,1,1,1,0,1,1,1,0,1,1,1,
                    0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,1,
                    0,1,1,1,
                    1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,
                    0,1,1,0,1,0,1,1,0,0,1,0,0,1,1,0,
                    1,0,1,1,
                    0,0,0,0,1,1,1,1,0,1,1,1,0,0,0,0,
                    0,1,1,1,0,1,1,1,0,1,1,1,0,1,0,1,
                    1,1,0,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    0,1,1,1,0,1,0,0,0,0,0,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    1,0,1,0,1,1,1,1,1,0,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,
                    0,1,1,1,
                    1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,
                    1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,
                    0,1,1,1,
                    1,1,0,1,1,1,0,0,0,0,0,1,1,1,0,1,
                    1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,
                    1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,
                    1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,
                    0,0,1,1,
                    1,1,1,1,1,1,1,1,0,1,1,1,1,0,1,0,
                    1,1,1,1,1,0,1,1,1,0,0,0,1,1,1,0,
                    1,0,1,1,
                    0,0,0,0,0,1,0,0,0,0,0,1,1,1,0,1,
                    1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,
                    1,0,1,1,
                    1,1,1,1,1,1,1,1,0,1,1,1,1,0,1,0,
                    1,1,1,0,1,1,1,1,1,0,0,0,1,1,1,0,
                    1,0,1,1,
                    1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
                    1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
                    0,0,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    1,1,1,1
                };
            }


            Impl::Impl() :
                m_init(false)
            {
                m_shader =
                        make_unique<gl::ShaderProgram>(
                            vertex_shader,
                            frag_shader);

                m_vertex_buffer =
                        make_unique<gl::VertexBuffer>(
                            vx_layout,
                            gl::Buffer::Usage::Dynamic);

                m_glyph_atlas =
                        make_unique<gl::Texture2D>(
                            g_glyph_atlas_width,
                            g_glyph_atlas_height,
                            gl::Texture2D::Format::LUMINANCE8,
                            gl::Texture::Filter::Linear,
                            gl::Texture::Filter::Linear,
                            gl::Texture::Wrap::ClampToEdge,
                            gl::Texture::Wrap::ClampToEdge);

                // Set the glyph atlas texture
                uint const pixel_count = g_glyph_atlas_image_data.size();

                Image<R8> glyph_atlas_image(
                            g_glyph_atlas_width,
                            g_glyph_atlas_height);

                auto &list_pixels = glyph_atlas_image.GetData();
                list_pixels.reserve(pixel_count);

                for(uint i=0; i < pixel_count ; i++) {
                    list_pixels.push_back(
                        {static_cast<u8>(g_glyph_atlas_image_data[i]*255)});
                }

                m_glyph_atlas->UpdateTexture(
                            gl::Texture2D::Update{
                                gl::Texture2D::Update::ReUpload,
                                glm::u16vec2{0,0},
                                glyph_atlas_image.ConvertToImageDataPtr().release()
                            });

                // Populate the glyph lookups
                m_str_glyph_chars =
                        " 0123456789()ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:;-+*/=";

                uint const glyph_width=5;
                uint const glyph_height=5;

                uint const glyph_rows = 8;
                uint const glyph_cols = 6;

                float const pixel_width = 1.0/g_glyph_atlas_width;
                float const pixel_height = 1.0/g_glyph_atlas_height;

                m_list_glyph_texcoords.reserve((glyph_rows*glyph_cols)+1);


                // Add a set of texcoords for space ' '
                m_list_glyph_texcoords.push_back(
                            glm::vec4{1.0-pixel_width,
                                      1.0-pixel_height,
                                      1.0,
                                      1.0});

                // Add the rest of the glyphs
                for(uint row=0; row < glyph_rows; row++)
                {
                    for(uint col=0; col < glyph_cols; col++)
                    {
                        float s0 = (col*(glyph_width+1)) / float(g_glyph_atlas_width);
                        float s1 = s0 + (glyph_width/float(g_glyph_atlas_width));
                        float t0 = (row*(glyph_height+1)) / float(g_glyph_atlas_height);
                        float t1 = t0 + (glyph_height/float(g_glyph_atlas_height));

                        m_list_glyph_texcoords.push_back(
                                    glm::vec4{s0,t0,s1,t1});
                    }
                }
            }

            void Impl::Render(gl::StateSet* state_set)
            {
                if(!m_init) {
                    this->init(state_set);
                }

                // uniforms
                gl::Uniform<GLint> u_sampler("u_s_tex0",0);
                gl::Uniform<glm::vec4> u_text_color("u_v4_text_color",{});

                // vertex buffer
                unique_ptr<std::vector<u8>> list_vx_ptr;

                {
                    std::lock_guard<std::mutex> lock(m_mutex);

                    if(m_list_strings.empty()) {
                        return;
                    }

                    // setup uniforms
                    u_sampler.Sync();

                    u_text_color.Update(m_text_color);
                    u_text_color.Sync();

                    // setup vertex buffer
                    list_vx_ptr = make_unique<std::vector<u8>>();

                    uint char_count=0;
                    uint col_count=0;
                    for(auto &str : m_list_strings) {
                        if(str.size() > col_count) {
                            col_count = str.size();
                        }
                        char_count += str.size();
                    }

                    uint row_count = m_list_strings.size();

                    float const glyph_width =
                            ((m_textbox_br.x-m_textbox_tl.x)/
                                col_count)*5.0/6.0;

                    float const glyph_height =
                            ((m_textbox_br.y-m_textbox_tl.y)/
                                row_count)*2.0/3.0;

                    float const glyph_hspace = glyph_width/5;
                    float const glyph_vspace = glyph_height/2;

                    // top-left corner
                    glm::vec2 pen{
                        m_textbox_tl.x,
                        m_textbox_tl.y
                    };

                    auto &list_vx = *list_vx_ptr;
                    list_vx.reserve(sizeof(Vertex)*4*char_count);

                    for(auto &text_line : m_list_strings)
                    {
                        for(char c : text_line) {
                            // Get tex coords for this glyph
                            glm::vec4 texcoords = m_list_glyph_texcoords.back();
                            for(uint i=0; i < m_str_glyph_chars.size(); i++) {
                                if(m_str_glyph_chars[i] == c) {
                                    texcoords = m_list_glyph_texcoords[i];
                                }
                            }

                            // Add the vertices for this glyph (bl,bl,tl,br,tr,tr)
                            gl::Buffer::PushElement<Vertex>(
                                        list_vx,
                                        Vertex{
                                            glm::vec4(pen.x,pen.y+glyph_height,0,1),
                                            glm::vec2(texcoords.x,texcoords.w)
                                        });

                            gl::Buffer::PushElement<Vertex>(
                                        list_vx,
                                        Vertex{
                                            glm::vec4(pen.x,pen.y+glyph_height,0,1),
                                            glm::vec2(texcoords.x,texcoords.w)
                                        });

                            gl::Buffer::PushElement<Vertex>(
                                        list_vx,
                                        Vertex{
                                            glm::vec4(pen.x,pen.y,0,1),
                                            glm::vec2(texcoords.x,texcoords.y)
                                        });

                            gl::Buffer::PushElement<Vertex>(
                                        list_vx,
                                        Vertex{
                                            glm::vec4(pen.x+glyph_width,pen.y+glyph_height,0,1),
                                            glm::vec2(texcoords.z,texcoords.w)
                                        });

                            gl::Buffer::PushElement<Vertex>(
                                        list_vx,
                                        Vertex{
                                            glm::vec4(pen.x+glyph_width,pen.y,0,1),
                                            glm::vec2(texcoords.z,texcoords.y)
                                        });

                            gl::Buffer::PushElement<Vertex>(
                                        list_vx,
                                        Vertex{
                                            glm::vec4(pen.x+glyph_width,pen.y,0,1),
                                            glm::vec2(texcoords.z,texcoords.y)
                                        });


                            pen.x += glyph_width + glyph_hspace;
                        }
                        pen.x = m_textbox_tl.x;
                        pen.y += glyph_height + glyph_vspace;
                    }
                }

                m_vertex_buffer->UpdateBuffer(
                            gl::Buffer::Update{
                                gl::Buffer::Update::ReUpload |
                                gl::Buffer::Update::KeepSrcData,
                                0,0,
                                list_vx_ptr->size(),
                                list_vx_ptr.get()
                            });

                m_vertex_buffer->GLBind();
                m_vertex_buffer->GLSync();

                // Render
                m_shader->GLEnable(state_set);
                state_set->SetBlend(true);
                state_set->SetBlendFunction(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
                                            GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
                m_glyph_atlas->GLBind(state_set,0);

                u_sampler.GLSetUniform(m_shader.get());
                u_text_color.GLSetUniform(m_shader.get());

                m_vertex_buffer->GLBindVxBuff(m_shader.get(),0);

                gl::DrawArrays(gl::Primitive::TriangleStrip,
                               m_vertex_buffer->GetVertexSizeBytes(),
                               0,list_vx_ptr->size());

                m_vertex_buffer->GLUnbind();
            }

            void Impl::SetText(glm::vec2 textbox_tl,
                               glm::vec2 textbox_br,
                               glm::vec4 text_color,
                               std::string text)
            {
                // Convert lower case letters to upper case
                for(char& c : text) {
                    if((c > 96) && (c < 123)) {
                        c -= 32;
                    }
                }

                std::vector<std::string> list_strings;

                // Split text into a list of strings using '\n'
                std::stringstream ss(text);
                std::string line="";
                while(std::getline(ss,line,'\n')) {
                    if(!line.empty()) {
                        list_strings.push_back(line);
                    }
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                m_textbox_tl = textbox_tl;
                m_textbox_br = textbox_br;
                m_text_color = text_color;
                m_list_strings = list_strings;
            }

            void Impl::init(gl::StateSet* state_set)
            {
                bool ok = m_shader->GLInit();
                assert(ok);
                m_vertex_buffer->GLInit();
                m_glyph_atlas->GLInit();
                m_glyph_atlas->GLBind(state_set,0);
                m_glyph_atlas->GLSync();

                m_init = true;
            }
        }
    }
}
