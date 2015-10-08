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

#ifndef KS_DRAW_DRAW_STAGE_HPP
#define KS_DRAW_DRAW_STAGE_HPP

#include <ks/gl/KsGLVertexBuffer.hpp>
#include <ks/gl/KsGLIndexBuffer.hpp>
#include <ks/gl/KsGLUniform.hpp>

namespace ks
{
    namespace draw
    {
        using ListUniformUPtrs =
            std::vector<
                unique_ptr<gl::UniformBase>
            >;

        // T = gl::IndexBuffer or gl::VertexBuffer
        template<typename T>
        struct DrawRange {
            shared_ptr<T> buffer;
            uint start_byte;
            uint size_bytes;
        };

        template<typename DrawKeyType>
        struct DrawCall
        {
            DrawKeyType key;
            std::vector<DrawRange<gl::VertexBuffer>> list_draw_vx;
            DrawRange<gl::IndexBuffer> draw_ix;
            shared_ptr<ListUniformUPtrs> list_uniforms;
            bool valid;
        };

        // * Params required to issue a set of DrawCalls for a given stage
        template<typename DrawKeyType>
        struct DrawParams
        {
            gl::StateSet* state_set;
            std::vector<shared_ptr<gl::ShaderProgram>>* list_shaders;
            std::vector<DrawCall<DrawKeyType>>* list_draw_calls;
            std::vector<Id>* list_opq_draw_calls;
            std::vector<Id>* list_xpr_draw_calls;
        };

        template<typename DrawKeyType>
        class DrawStage
        {
        public:
            struct Stats
            {
                uint shader_switches;
                uint texture_switches;
                uint raster_ops;
                uint draw_calls;

                Stats()
                {
                    reset();
                }

                void reset()
                {
                    shader_switches = 0;
                    texture_switches = 0;
                    raster_ops = 0;
                    draw_calls = 0;
                }
            };

            virtual ~DrawStage() = default;

            // Called by the render thread
            Stats const & GetStats() const
            {
                return m_stats;
            }

            // Called by the render thread
            virtual void Render(DrawParams<DrawKeyType>& params) = 0;

        protected:
            Stats m_stats;
        };
    }
}

#endif // KS_DRAW_DRAW_STAGE_HPP
