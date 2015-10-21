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

#ifndef KS_DRAW_DEBUG_TEXT_DRAW_STAGE_HPP
#define KS_DRAW_DEBUG_TEXT_DRAW_STAGE_HPP

#include <ks/draw/KsDrawDrawStage.hpp>
#include <ks/gl/KsGLTexture2D.hpp>

namespace ks
{
    namespace draw
    {
        namespace detail_debug_text_draw_stage
        {
            // Hide the actual implementation since its
            // not a class template

            class Impl final
            {
            public:
                Impl();

                // * textbox coords are in screen space
                //   x: [-1,1], y:[-1,1]
                // * text must be in ASCII
                void SetText(glm::vec2 textbox_tl,
                             glm::vec2 textbox_br,
                             glm::vec4 text_color,
                             std::string text);

                void Reset();
                void Render(gl::StateSet* state_set);

            private:
                void initialize(gl::StateSet* state_set);

                bool m_init;
                unique_ptr<gl::ShaderProgram> m_shader;
                unique_ptr<gl::VertexBuffer> m_vertex_buffer;
                unique_ptr<gl::Texture2D> m_glyph_atlas;

                std::string m_str_glyph_chars;
                std::vector<glm::vec4> m_list_glyph_texcoords;

                mutable std::mutex m_mutex;
                glm::vec2  m_textbox_tl;
                glm::vec2  m_textbox_br;
                glm::vec4  m_text_color;
                std::vector<std::string> m_list_strings;
            };
        }

        template<typename DrawKeyType>
        class DebugTextDrawStage final : public DrawStage<DrawKeyType>
        {
        public:
            DebugTextDrawStage()
            {
                m_impl = make_unique<detail_debug_text_draw_stage::Impl>();
            }

            ~DebugTextDrawStage() = default;

            void SetText(glm::vec2 textbox_tl,
                         glm::vec2 textbox_br,
                         glm::vec4 text_color,
                         std::string text)
            {
                m_impl->SetText(textbox_tl,
                                textbox_br,
                                text_color,
                                text);
            }

            void Reset() override
            {
                m_impl->Reset();
            }

            void Render(DrawParams<DrawKeyType>& p) override
            {
                m_impl->Render(p.state_set);
            }

        private:
            unique_ptr<detail_debug_text_draw_stage::Impl> m_impl;
        };
    }
}



#endif // KS_DRAW_DEBUG_TEXT_DRAW_STAGE_HPP
