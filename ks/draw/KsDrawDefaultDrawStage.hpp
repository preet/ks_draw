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

#ifndef KS_DRAW_DEFAULT_DRAW_STAGE_HPP
#define KS_DRAW_DEFAULT_DRAW_STAGE_HPP

#include <ks/draw/KsDrawDrawStage.hpp>
#include <ks/draw/KsDrawRenderSystem.hpp>

namespace ks
{
    namespace draw
    {
        template<typename DrawKeyType>
        class DefaultDrawStage : public DrawStage<DrawKeyType>
        {
        public:
            DefaultDrawStage() = default;
            ~DefaultDrawStage() = default;

            void Render(DrawParams<DrawKeyType>& p)
            {
                // reset stats
                this->m_stats.reset();

                auto& list_shaders = *(p.list_shaders);
                auto& list_draw_calls = *(p.list_draw_calls);
                auto& list_opq_draw_calls = *(p.list_opq_draw_calls);
                auto& list_xpr_draw_calls = *(p.list_xpr_draw_calls);

                // clear the framebuffer
                // TODO setup
                p.state_set->SetClearColor(0.15,0.15,0.15,1.0);
                gl::Clear(gl::ColorBufferBit);

                // For the default draw stage (which is just an example
                // more than anything else), we only sort by key
                auto draw_key_sort = [&list_draw_calls](Id a, Id b) -> bool
                {
                    return (list_draw_calls[a].key < list_draw_calls[b].key);
                };

                std::stable_sort(list_opq_draw_calls.begin(),
                                 list_opq_draw_calls.end(),
                                 draw_key_sort);

                std::stable_sort(list_xpr_draw_calls.begin(),
                                 list_xpr_draw_calls.end(),
                                 draw_key_sort);

                DrawKeyType prev_key; // key value should be 0

                // Transparent draw calls
                for(auto const dc_id : list_xpr_draw_calls)
                {
                    auto& draw_call = list_draw_calls[dc_id];
                    setupState(p,prev_key,draw_call.key);

                    gl::ShaderProgram* shader =
                            list_shaders[draw_call.key.GetShader()].get();

                    // Set the individual uniforms
                    if(draw_call.list_uniforms)
                    {
                        auto &list_uniforms = *(draw_call.list_uniforms);
                        for(auto &uniform : list_uniforms) {
                            uniform->GLSetUniform(shader);
                        }
                    }

                    // Draw!
                    issueDrawCall(shader,draw_call);
                }

                // Opaque draw calls
                for(auto const dc_id : list_opq_draw_calls)
                {
                    auto& draw_call = list_draw_calls[dc_id];
                    setupState(p,prev_key,draw_call.key);

                    gl::ShaderProgram* shader =
                            list_shaders[draw_call.key.GetShader()].get();

                    // Set the individual uniforms
                    if(draw_call.list_uniforms)
                    {
                        auto &list_uniforms = *(draw_call.list_uniforms);
                        for(auto &uniform : list_uniforms) {
                            uniform->GLSetUniform(shader);
                        }
                    }

                    // Draw!
                    issueDrawCall(shader,draw_call);
                }
            }

        protected:
            void issueDrawCall(gl::ShaderProgram* shader,
                               DrawCall<DrawKeyType> &draw_call)
            {
                // Draw
                // TODO: Since we are drawing all buffers in
                // a tight loop, we might be able to check
                // when we really need to bind/unbind buffers
                // to avoid redundant calls

                auto primitive = draw_call.key.GetPrimitive();

                if(draw_call.draw_ix.buffer) {
                    // bind vertex buffers
                    bool ok = true;
                    for(auto& range : draw_call.list_draw_vx) {
                        ok = ok && range.buffer->GLBindVxBuff(
                                    shader,range.start_byte);
                    }

                    // bind index buffer
                    ok = ok && draw_call.draw_ix.buffer->GLBind();

                    assert(ok);

                    ks::gl::DrawElements(
                                primitive,
                                draw_call.draw_ix.start_byte,
                                draw_call.draw_ix.size_bytes);

                    for(auto& range : draw_call.list_draw_vx) {
                        range.buffer->GLUnbind();
                    }
                    draw_call.draw_ix.buffer->GLUnbind();
                }
                else {
                    // bind vertex buffers
                    bool ok = true;
                    for(auto& range : draw_call.list_draw_vx) {
                        ok = ok && range.buffer->GLBindVxBuff(
                                    shader,range.start_byte);
                    }

                    auto& first_range = draw_call.list_draw_vx[0];

                    ks::gl::DrawArrays(
                                primitive,
                                first_range.buffer->GetVertexSizeBytes(),
                                0,first_range.size_bytes);

                    for(auto& range : draw_call.list_draw_vx) {
                        range.buffer->GLUnbind();
                    }
                }

                this->m_stats.draw_calls++;
            }

        private:
            void setupState(DrawParams<DrawKeyType>& p,
                            DrawKeyType& prev_key,
                            DrawKeyType const curr_key)
            {
                if(!(prev_key == curr_key))
                {
                    if(prev_key.GetShader() != curr_key.GetShader())
                    {
                        auto& list_shaders = *(p.list_shaders);

                        gl::ShaderProgram* shader =
                                list_shaders[curr_key.GetShader()].get();

                        shader->GLEnable(p.state_set);
                        this->m_stats.shader_switches++;
                    }

                    auto const depth_config = curr_key.GetDepthConfig();
                    if((prev_key.GetDepthConfig() != depth_config) && (depth_config > 0))
                    {
                        auto& list_depth_configs = *(p.list_depth_configs);
                        list_depth_configs[depth_config](p.state_set);
                    }

                    auto const blend_config = curr_key.GetBlendConfig();
                    if((prev_key.GetBlendConfig() != blend_config) && (blend_config > 0))
                    {
                        auto& list_blend_configs = *(p.list_blend_configs);
                        list_blend_configs[blend_config](p.state_set);
                    }

                    auto const stencil_config = curr_key.GetStencilConfig();
                    if((prev_key.GetStencilConfig() != stencil_config) && (stencil_config > 0))
                    {
                        auto& list_stencil_configs = *(p.list_stencil_configs);
                        list_stencil_configs[curr_key.GetStencilConfig()](p.state_set);
                    }

//                    if(prev_key.GetTextureSet() != curr_key.GetTextureSet())
//                    {
//                        //
//                    }

//                    if(prev_key.GetUniformSet() != curr_key.GetUniformSet())
//                    {
//                        //
//                    }

                    prev_key = curr_key;
                }
            }
        };
    }
}

#endif // KS_DRAW_DEFAULT_DRAW_STAGE_HPP
