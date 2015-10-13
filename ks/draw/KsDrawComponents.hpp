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

#ifndef KS_DRAW_COMPONENTS_HPP
#define KS_DRAW_COMPONENTS_HPP

#include <ks/shared/KsRangeAllocator.hpp>
#include <ks/shared/KsThreadPool.hpp>
#include <ks/shared/KsRecycleIndexList.hpp>

#include <ks/gl/KsGLVertexBuffer.hpp>
#include <ks/gl/KsGLIndexBuffer.hpp>
#include <ks/gl/KsGLUniform.hpp>

#include <ks/shared/KsGraph.hpp>

namespace ks
{
    namespace draw
    {
        // ============================================================= //
        // ============================================================= //

        using UPtrBuffer =
            unique_ptr<
                std::vector<u8>
            >;

        using VertexBufferAllocator =
            RangeAllocator<
                shared_ptr<gl::VertexBuffer>
            >;

        using IndexBufferAllocator =
            RangeAllocator <
                shared_ptr<gl::IndexBuffer>
            >;

        using ListUniformUPtrs =
            std::vector<
                unique_ptr<gl::UniformBase>
            >;

        enum class Transparency : u8
        {
            Opaque,
            Transparent
        };

        enum class UpdatePriority : u8
        {
            SingleFrame,
            MultiFrame
        };

        // ============================================================= //
        // ============================================================= //

        class BufferLayout final
        {
        public:
            BufferLayout(gl::Buffer::Usage usage,
                         std::vector<gl::VertexLayout> list_vx_layout,
                         std::vector<shared_ptr<VertexBufferAllocator>> list_vx_allocators,
                         shared_ptr<IndexBufferAllocator> ix_allocator=nullptr);

            ~BufferLayout();

            gl::Buffer::Usage GetBufferUsage() const;

            gl::VertexLayout const &
            GetVertexLayout(uint index) const;

            shared_ptr<VertexBufferAllocator> const &
            GetVertexBufferAllocator(uint index) const;

            shared_ptr<IndexBufferAllocator> const &
            GetIndexBufferAllocator() const;

            bool GetIsIndexed() const;

            uint GetVertexBufferCount() const;

            uint GetVertexSizeBytes(uint index) const;

            static std::vector<uint> genListVertexLayoutSizes(
                    std::vector<gl::VertexLayout> const &list_vx_layout);

        private:
            gl::Buffer::Usage m_usage;
            std::vector<gl::VertexLayout> m_list_vx_layout;
            std::vector<shared_ptr<VertexBufferAllocator>> m_list_vx_allocators;
            shared_ptr<IndexBufferAllocator> m_ix_allocator;

            bool m_is_indexed;
            uint m_vx_buffer_count;
            std::vector<uint> m_list_vx_size_bytes;
        };

        // ============================================================= //
        // ============================================================= //

        class Geometry final
        {
        public:
            Geometry() = default;
            ~Geometry() = default;

            Geometry(Geometry&&) = default;
            Geometry& operator = (Geometry&&) = default;

            bool GetUpdatedGeometry() const;
            std::vector<u8> const & GetUpdatedVertexBuffers() const;
            bool GetUpdatedIndexBuffer() const;
            std::vector<UPtrBuffer> const & GetVertexBuffers() const;
            UPtrBuffer const & GetVertexBuffer(uint index) const;
            UPtrBuffer const & GetIndexBuffer() const;
            std::vector<UPtrBuffer>& GetVertexBuffers();
            UPtrBuffer& GetVertexBuffer(uint index);
            UPtrBuffer& GetIndexBuffer();
            bool GetRetainGeometry() const;

            void SetRetainGeometry(bool retain);
            void SetVertexBufferUpdated(uint index);
            void SetIndexBufferUpdated();
            void SetAllUpdated();
            void ClearGeometryUpdates(); // rn ClearUpdates

        private:
            bool m_retain_geometry{true};
            bool m_upd_geometry{false};
            bool m_upd_ix{false};
            std::vector<u8> m_list_upd_vx;

            std::vector<UPtrBuffer> m_list_vx_buffs;
            UPtrBuffer m_ix_buff;
        };

        // ============================================================= //
        // ============================================================= //

        template<typename DrawKeyType>
        class Batch final
        {
        public:
            Batch(DrawKeyType key,
                  BufferLayout const * buffer_layout,
                  shared_ptr<ListUniformUPtrs> list_uniforms,
                  std::vector<u8> list_draw_stages,
                  Transparency transparency,
                  UpdatePriority priority) :
                m_key(key),
                m_buffer_layout(buffer_layout),
                m_list_uniforms(list_uniforms),
                m_list_draw_stages(list_draw_stages),
                m_transparency(transparency),
                m_priority(priority),
                m_upd(true)
            {}

            ~Batch() = default;

            DrawKeyType GetKey() const
            {
                return m_key;
            }

            BufferLayout const * GetBufferLayout() const
            {
                return m_buffer_layout;
            }

            shared_ptr<ListUniformUPtrs>& GetUniformList()
            {
                return m_list_uniforms;
            }

            std::vector<u8> const & GetDrawStages() const
            {
                return m_list_draw_stages;
            }

            Transparency GetTransparency() const
            {
                return m_transparency;
            }

            UpdatePriority GetUpdatePriority() const
            {
                return m_priority;
            }

            void SetKey(DrawKeyType key)
            {
                m_key = key;
                m_upd = true;
            }

            void SetDrawStages(std::vector<u8> list_draw_stages)
            {
                m_list_draw_stages = list_draw_stages;
                m_upd = true;
            }

            void SetTransparency(Transparency transparency)
            {
                m_transparency = transparency;
                m_upd = true;
            }

            void ClearUpdated()
            {
                m_upd = false;
            }

        private:
            DrawKeyType m_key;
            BufferLayout const * m_buffer_layout;
            shared_ptr<ListUniformUPtrs> m_list_uniforms;
            std::vector<u8> m_list_draw_stages;
            Transparency m_transparency;
            UpdatePriority m_priority;

            bool m_upd;
        };

        // ============================================================= //
        // ============================================================= //

        template<typename DrawKeyType>
        class RenderData final
        {
        public:
            // This constructor should be used to construct
            // RenderData objects by the user
            RenderData(DrawKeyType key,
                       BufferLayout const * buffer_layout,
                       shared_ptr<ListUniformUPtrs> list_uniforms,
                       std::vector<u8> list_draw_stages,
                       Transparency transparency) :
                m_uid(genId()),
                m_key(key),
                m_buffer_layout(buffer_layout),
                m_list_uniforms(list_uniforms),
                m_list_draw_stages(list_draw_stages),
                m_transparency(transparency),
                m_enabled(true)
            {}

            // This constructor is for internal use by the
            // RenderDataComponentList and creates a default
            // invalid RenderData object
            RenderData() :
                m_uid(0)
            {}
            // invalid RenderData object
            ~RenderData() = default;
            RenderData(RenderData&&) = default;
            RenderData& operator = (RenderData&&) = default;

            Id GetUniqueId() const
            {
                return m_uid;
            }

            DrawKeyType GetKey() const
            {
                return m_key;
            }

            BufferLayout const * GetBufferLayout() const
            {
                return m_buffer_layout;
            }

            shared_ptr<ListUniformUPtrs>& GetUniformList()
            {
                return m_list_uniforms;
            }

            std::vector<u8> const & GetDrawStages() const
            {
                return m_list_draw_stages;
            }

            Transparency GetTransparency() const
            {
                return m_transparency;
            }

            Geometry& GetGeometry()
            {
                return m_geometry;
            }

            bool GetEnabled() const
            {
                return m_enabled;
            }

            void SetKey(DrawKeyType key)
            {
                m_key = key;
            }

            void SetDrawStages(std::vector<u8> list_draw_stages)
            {
                m_list_draw_stages = list_draw_stages;
            }

            void SetTransparency(Transparency transparency)
            {
                m_transparency = transparency;
            }

            void SetEnabled(bool enabled)
            {
                m_enabled = enabled;
            }

        private:
            Id m_uid;
            DrawKeyType m_key;
            BufferLayout const * m_buffer_layout;
            shared_ptr<ListUniformUPtrs> m_list_uniforms;
            std::vector<u8> m_list_draw_stages;
            Transparency m_transparency;

            // update/behaviour flags
            bool m_enabled{false};

            // geometry
            Geometry m_geometry;

            // unique id gen
            // TODO no need for this mutex?
            static std::mutex s_id_mutex;
            static Id s_id_counter;

            static Id genId()
            {
                std::lock_guard<std::mutex> lock(s_id_mutex);
                Id id = s_id_counter;
                s_id_counter++;
                return id;
            }
        };

        template<typename DrawKeyType>
        std::mutex RenderData<DrawKeyType>::s_id_mutex;

        // Starts at 1 because 0 is reserved to mean 'invalid'
        template<typename DrawKeyType>
        Id RenderData<DrawKeyType>::s_id_counter = 1;

        // ============================================================= //
        // ============================================================= //

        class BatchData final
        {
        public:
            BatchData(Id group_id);
            BatchData() = default;
            ~BatchData() = default;
            BatchData(BatchData&&) = default;
            BatchData& operator = (BatchData&&) = default;

            Id GetGroupId() const;
            Geometry& GetGeometry();
            Geometry const & GetGeometry() const;
            bool GetRebuild() const;

            void SetGroupId(Id group_id);
            void SetRebuild(bool rebuild);

        private:
            Id m_group_id{0};
            Geometry m_geometry;
            bool m_rebuild;
        };

        // ============================================================= //
        // ============================================================= //
    }
}

#endif // KS_DRAW_COMPONENTS_HPP
