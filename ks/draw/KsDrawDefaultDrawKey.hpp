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

#ifndef KS_DRAW_DEFAULT_DRAW_KEY_HPP
#define KS_DRAW_DEFAULT_DRAW_KEY_HPP

#include <ks/gl/KsGLVertexBuffer.hpp>

namespace ks
{
    namespace draw
    {
        class DefaultDrawKey final
        {
        public:
            // [bits from MSBit to LSBit]
            // [5]: Shader
            // [4]: DepthConfig
            // [6]: BlendConfig
            // [4]: StencilConfig
            // [9]: TextureSet
            // [6]: UniformSet
            // [3]: Primitive
            // [2]: BatchType
            // [14]: BatchGroup
            // [1]: UpdatePriority

            // number of bits
            static const u8 k_bits_primitive        = 3;
            static const u8 k_bits_uniform_set      = 6;
            static const u8 k_bits_texture_set      = 9;
            static const u8 k_bits_stencil_config   = 4;
            static const u8 k_bits_blend_config     = 6;
            static const u8 k_bits_depth_config     = 4;
            static const u8 k_bits_shader           = 5;

            // starting bit position
            static const u8 k_sbit_primitive        = 0;
            static const u8 k_sbit_uniform_set      = k_sbit_primitive+k_bits_primitive;
            static const u8 k_sbit_texture_set      = k_sbit_uniform_set+k_bits_uniform_set;
            static const u8 k_sbit_stencil_config   = k_sbit_texture_set+k_bits_texture_set;
            static const u8 k_sbit_blend_config     = k_sbit_stencil_config+k_bits_stencil_config;
            static const u8 k_sbit_depth_config     = k_sbit_blend_config+k_bits_blend_config;
            static const u8 k_sbit_shader           = k_sbit_depth_config+k_bits_depth_config;

            // mask
            template<u8 sbit,u8 bits>
            struct mask {
                static const u64 value;
            };

            // map int to primitive type
            static const std::vector<gl::Primitive> k_list_prim_to_int;

        public:
            Id GetShader() const;
            Id GetDepthConfig() const;
            Id GetBlendConfig() const;
            Id GetStencilConfig() const;
            Id GetTextureSet() const;
            Id GetUniformSet() const;
            gl::Primitive GetPrimitive() const;

            void SetShader(Id shader);
            void SetDepthConfig(Id depth_config);
            void SetBlendConfig(Id blend_config);
            void SetStencilConfig(Id stencil_config);
            void SetTextureSet(Id texture_set);
            void SetUniformSet(Id uniform_set);
            void SetPrimitive(gl::Primitive primitive);

            bool operator < (DefaultDrawKey const &right) const
            {
                return (this->m_key < right.m_key);
            }

            bool operator == (DefaultDrawKey const &other) const
            {
                return (this->m_key == other.m_key);
            }

        private:
            void setData(u64 mask, u8 sbit, Id data)
            {
                m_key &= (~mask);
                m_key |= ((data << sbit) & mask);
            }

            Id m_key{0};
        };

        template<u8 sbit,u8 bits>
        u64 const DefaultDrawKey::mask<sbit,bits>::value =
                ((u64(1) << bits)-1) << sbit;
    }
}

#endif // KS_DRAW_DEFAULT_DRAW_KEY
