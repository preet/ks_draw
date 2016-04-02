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

#include <ks/draw/KsDrawDefaultDrawKey.hpp>

namespace ks
{
    namespace draw
    {
        const std::vector<gl::Primitive> DefaultDrawKey::k_list_prim_to_int = {
            gl::Primitive::Triangles,       // 0
            gl::Primitive::TriangleFan,     // 1
            gl::Primitive::TriangleStrip,   // 2
            gl::Primitive::Lines,           // 3
            gl::Primitive::LineLoop,        // 4
            gl::Primitive::LineStrip,       // 5
            gl::Primitive::Points           // 6
        };

        Id DefaultDrawKey::GetShader() const
        {
            return ((m_key & mask<k_sbit_shader,k_bits_shader>::value) >> k_sbit_shader);
        }

        Id DefaultDrawKey::GetDepthConfig() const
        {
            return ((m_key & mask<k_sbit_depth_config,k_bits_depth_config>::value) >> k_sbit_depth_config);
        }

        Id DefaultDrawKey::GetBlendConfig() const
        {
            return ((m_key & mask<k_sbit_blend_config,k_bits_blend_config>::value) >> k_sbit_blend_config);
        }

        Id DefaultDrawKey::GetStencilConfig() const
        {
            return ((m_key & mask<k_sbit_stencil_config,k_bits_stencil_config>::value) >> k_sbit_stencil_config);
        }

        Id DefaultDrawKey::GetTextureSet() const
        {
            return ((m_key & mask<k_sbit_texture_set,k_bits_texture_set>::value) >> k_sbit_texture_set);
        }

        Id DefaultDrawKey::GetUniformSet() const
        {
            return ((m_key & mask<k_sbit_uniform_set,k_bits_uniform_set>::value) >> k_sbit_uniform_set);
        }

        gl::Primitive DefaultDrawKey::GetPrimitive() const
        {
            Id primitive = ((m_key & mask<k_sbit_primitive,k_bits_primitive>::value) >> k_sbit_primitive);
            return k_list_prim_to_int[primitive];
        }

        // ============================================================= //

        void DefaultDrawKey::SetShader(Id shader)
        {
            setData(mask<k_sbit_shader,k_bits_shader>::value, k_sbit_shader, shader);
        }

        void DefaultDrawKey::SetDepthConfig(Id depth_config)
        {
            setData(mask<k_sbit_depth_config,k_bits_depth_config>::value, k_sbit_depth_config, depth_config);
        }

        void DefaultDrawKey::SetBlendConfig(Id blend_config)
        {
            setData(mask<k_sbit_blend_config,k_bits_blend_config>::value, k_sbit_blend_config, blend_config);
        }

        void DefaultDrawKey::SetStencilConfig(Id stencil_config)
        {
            setData(mask<k_sbit_stencil_config,k_bits_stencil_config>::value, k_sbit_stencil_config, stencil_config);
        }

        void DefaultDrawKey::SetTextureSet(Id texture_set)
        {
            setData(mask<k_sbit_texture_set,k_bits_texture_set>::value, k_sbit_texture_set, texture_set);
        }

        void DefaultDrawKey::SetUniformSet(Id uniform_set)
        {
            setData(mask<k_sbit_uniform_set,k_bits_uniform_set>::value, k_sbit_uniform_set, uniform_set);
        }

        void DefaultDrawKey::SetPrimitive(ks::gl::Primitive primitive)
        {
            Id primitive_int=0;

            if(primitive == ks::gl::Primitive::Triangles) {
                primitive_int = 0;
            }
            else if(primitive == ks::gl::Primitive::TriangleFan) {
                primitive_int = 1;
            }
            else if(primitive == ks::gl::Primitive::TriangleStrip) {
                primitive_int = 2;
            }
            else if(primitive == ks::gl::Primitive::Lines) {
                primitive_int = 3;
            }
            else if(primitive == ks::gl::Primitive::LineLoop) {
                primitive_int = 4;
            }
            else if(primitive == ks::gl::Primitive::LineStrip) {
                primitive_int = 5;
            }
            else if(primitive == ks::gl::Primitive::Points) {
                primitive_int = 6;
            }

            setData(mask<k_sbit_primitive,k_bits_primitive>::value, k_sbit_primitive, primitive_int);
        }
    }
}
