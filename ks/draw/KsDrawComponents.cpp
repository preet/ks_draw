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

#include <ks/draw/KsDrawComponents.hpp>

namespace ks
{
    namespace draw
    {
        // ============================================================= //
        // ============================================================= //

        BufferLayout::BufferLayout(
                gl::Buffer::Usage usage,
                std::vector<gl::VertexLayout> list_vx_layout,
                std::vector<shared_ptr<VertexBufferAllocator>> list_vx_allocators,
                shared_ptr<IndexBufferAllocator> ix_allocator) :
            m_usage(usage),
            m_list_vx_layout(list_vx_layout),
            m_list_vx_allocators(list_vx_allocators),
            m_ix_allocator(ix_allocator),
            m_is_indexed(ix_allocator!=nullptr),
            m_vx_buffer_count(list_vx_layout.size()),
            m_list_vx_size_bytes(genListVertexLayoutSizes(m_list_vx_layout))
        {

        }

        BufferLayout::~BufferLayout()
        {

        }

        gl::Buffer::Usage BufferLayout::GetBufferUsage() const
        {
            return m_usage;
        }

        gl::VertexLayout const &
        BufferLayout::GetVertexLayout(uint index) const
        {
            return m_list_vx_layout[index];
        }

        shared_ptr<VertexBufferAllocator> const &
        BufferLayout::GetVertexBufferAllocator(uint index) const
        {
            return m_list_vx_allocators[index];
        }

        shared_ptr<IndexBufferAllocator> const &
        BufferLayout::GetIndexBufferAllocator() const
        {
            return m_ix_allocator;
        }

        bool BufferLayout::GetIsIndexed() const
        {
            return m_is_indexed;
        }

        uint BufferLayout::GetVertexBufferCount() const
        {
            return m_vx_buffer_count;
        }

        uint BufferLayout::GetVertexSizeBytes(uint index) const
        {
            return m_list_vx_size_bytes[index];
        }

        std::vector<uint> BufferLayout::genListVertexLayoutSizes(
                std::vector<gl::VertexLayout> const &list_vx_layout)
        {
            std::vector<uint> list_vx_size_bytes;

            for(auto const& vx_layout : list_vx_layout) {
                list_vx_size_bytes.push_back(0);
                for(auto& attr : vx_layout) {
                    list_vx_size_bytes.back() += attr.GetSizeBytes();
                }
            }

            return list_vx_size_bytes;
        }

        // ============================================================= //
        // ============================================================= //

        bool Geometry::GetUpdatedGeometry() const
        {
            return m_upd_geometry;
        }

        std::vector<u8> const & Geometry::GetUpdatedVertexBuffers() const
        {
            return m_list_upd_vx;
        }

        bool Geometry::GetUpdatedIndexBuffer() const
        {
            return m_upd_ix;
        }

        std::vector<UPtrBuffer> const & Geometry::GetVertexBuffers() const
        {
            return m_list_vx_buffs;
        }

        UPtrBuffer const & Geometry::GetVertexBuffer(uint index) const
        {
            return m_list_vx_buffs[index];
        }

        UPtrBuffer const & Geometry::GetIndexBuffer() const
        {
            return m_ix_buff;
        }

        std::vector<UPtrBuffer>& Geometry::GetVertexBuffers()
        {
            return m_list_vx_buffs;
        }

        UPtrBuffer& Geometry::GetVertexBuffer(uint index)
        {
            return m_list_vx_buffs[index];
        }

        UPtrBuffer& Geometry::GetIndexBuffer()
        {
            return m_ix_buff;
        }

        bool Geometry::GetRetainGeometry() const
        {
            return m_retain_geometry;
        }


        void Geometry::SetRetainGeometry(bool retain)
        {
            m_retain_geometry = retain;
        }

        void Geometry::SetVertexBufferUpdated(uint index)
        {
            for(auto const idx : m_list_upd_vx) {
                if(idx == index) {
                    return;
                }
            }
            m_list_upd_vx.push_back(index);
            m_upd_geometry = true;
        }

        void Geometry::SetIndexBufferUpdated()
        {
            if(m_ix_buff) {
                m_upd_ix = true;
                m_upd_geometry = true;
            }
        }

        void Geometry::SetAllUpdated()
        {
            m_list_upd_vx.clear();
            for(uint i=0; i < m_list_vx_buffs.size(); i++) {
                m_list_upd_vx.push_back(i);
            }

            if(m_ix_buff) {
                m_upd_ix = true;
            }
            m_upd_geometry = true;
        }

        void Geometry::ClearGeometryUpdates() // rn ClearUpdates
        {
            m_upd_geometry = false;
            m_upd_ix = false;
            m_list_upd_vx.clear();
        }

        // ============================================================= //
        // ============================================================= //

        BatchData::BatchData(Id group_id) :
            m_group_id(group_id)
        {}

        Id BatchData::GetGroupId() const
        {
            return m_group_id;
        }

        Geometry& BatchData::GetGeometry()
        {
            return m_geometry;
        }

        Geometry const & BatchData::GetGeometry() const
        {
            return m_geometry;
        }

        bool BatchData::GetRebuild() const
        {
            return m_rebuild;
        }

        void BatchData::SetGroupId(Id group_id)
        {
            m_group_id = group_id;
        }

        void BatchData::SetRebuild(bool rebuild)
        {
            m_rebuild = rebuild;
        }

        // ============================================================= //
        // ============================================================= //
    }
}
