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

#ifndef KS_DRAW_DRAW_CALL_UPDATER_HPP
#define KS_DRAW_DRAW_CALL_UPDATER_HPP

#include <set>
#include <ks/draw/KsDrawComponents.hpp>
#include <ks/draw/KsDrawDrawStage.hpp>

namespace ks
{
    namespace draw
    {
        // ============================================================= //
        // ============================================================= //

        class GeometryBufferAllocFailed : public ks::Exception
        {
        public:
            GeometryBufferAllocFailed(std::string msg) :
                ks::Exception(ks::Exception::ErrorLevel::FATAL,std::move(msg),true)
            {}

            ~GeometryBufferAllocFailed() = default;
        };

        // ============================================================= //

        using PairIds = std::pair<Id,Id>;

        // ============================================================= //
        // ============================================================= //

        // Creates DrawCalls from RenderData
        template<typename DrawKeyType>
        class DrawCallUpdater final
        {
            using RenderData = draw::RenderData<DrawKeyType>;
            using DrawCall = draw::DrawCall<DrawKeyType>;

        public:
            struct GeometryRanges
            {
                bool valid{false};
                bool vx_ranges_valid{false};
                bool ix_range_valid{false};

                std::vector<VertexBufferAllocator::Range> list_vx_ranges;
                IndexBufferAllocator::Range ix_range;

                BufferLayout const * buffer_layout{nullptr};
            };

            void Update(std::vector<PairIds> const &list_ent_rd_curr,
                        std::vector<RenderData>& list_render_data)
            {
                m_list_ent_rd_prev = m_list_ent_rd_curr;
                m_list_ent_rd_curr = list_ent_rd_curr;

                m_list_ents_rem.clear();
                m_list_ents_add.clear();
                m_list_ents_upd.clear();

                m_list_buffers_to_init.clear();
                m_list_buffers_to_sync.clear();
                m_list_new_buffers.clear();

                // We need to resize the RenderGeometry list if
                // its smaller than the entity count
                m_entity_count = list_render_data.size();
                if(m_list_geometry_ranges.size() < m_entity_count) {
                    m_list_geometry_ranges.resize(m_entity_count);
                }

                // Create lists for added and removed Entities by
                // diffing against unique RenderData ids
                createDiffs(m_list_ents_rem,m_list_ents_add);

                // Process removed RenderData/Geometry
                for(auto const ent_id : m_list_ents_rem)
                {
                    auto& geometry_ranges = m_list_geometry_ranges[ent_id];
                    removeGeometryRanges(geometry_ranges);
                }

                // Process added RenderData/Geometry
                for(auto const ent_id : m_list_ents_add)
                {
                    auto& render_data = list_render_data[ent_id];
                    auto& geometry = render_data.GetGeometry();
                    auto& geometry_ranges = m_list_geometry_ranges[ent_id];

                    auto const vx_buff_count =
                            render_data.GetBufferLayout()->
                                GetVertexBufferCount();

                    geometry_ranges.buffer_layout = render_data.GetBufferLayout();
                    geometry_ranges.list_vx_ranges.resize(vx_buff_count);
                    geometry_ranges.valid = true;

                    // Ensure this is uploaded in the case where RenderData
                    // has been removed/added but GeometryData is the same
                    // TODO Shoudl this be removed?
                    for(uint i=0; i < vx_buff_count; i++) {
                        geometry.SetVertexBufferUpdated(i);
                    }

                    if(geometry_ranges.buffer_layout->GetIsIndexed()) {
                        geometry.SetIndexBufferUpdated();
                    }
                }

                // Process all current RenderData/Geometry
                for(auto const ent_rd : m_list_ent_rd_curr)
                {
                    auto& render_data = list_render_data[ent_rd.first];
                    auto& geometry = render_data.GetGeometry();

                    if(geometry.GetUpdatedGeometry() &&
                       checkUpdatedGeometry(geometry))
                    {
                        createGeometryRanges(
                                    m_list_geometry_ranges[ent_rd.first],
                                    geometry);

                        geometry.ClearGeometryUpdates();

                        m_list_ents_upd.push_back(ent_rd.first);
                    }
                }
            }

            void Sync(std::vector<DrawCall>& list_draw_calls)
            {
                // Resize draw_calls if necessary
                if(list_draw_calls.size() < m_list_geometry_ranges.size()) {
                    list_draw_calls.resize(m_list_geometry_ranges.size());
                }

                // Clear removed draw calls
                for(auto const ent_id : m_list_ents_rem)
                {
                    auto& draw_call = list_draw_calls[ent_id];
                    draw_call.list_draw_vx.clear();
                    draw_call.draw_ix.buffer = nullptr;
                    draw_call.list_uniforms = nullptr;
                    draw_call.valid = false;
                }

                // Setup new draw calls
                for(auto const ent_id : m_list_ents_upd)
                {
                    auto& geometry = m_list_geometry_ranges[ent_id];

                    bool const geometry_valid =
                            (geometry.buffer_layout->GetIsIndexed()) ?
                                (geometry.vx_ranges_valid && geometry.ix_range_valid) :
                                (geometry.vx_ranges_valid);

                    if(!geometry_valid) {
                        continue;
                    }

                    auto& draw_call = list_draw_calls[ent_id];

                    draw_call.list_draw_vx.resize(
                                geometry.list_vx_ranges.size());

                    for(uint i=0; i < geometry.list_vx_ranges.size(); i++) {
                        auto& vx_alloc_range = geometry.list_vx_ranges[i];
                        auto& draw_range = draw_call.list_draw_vx[i];
                        draw_range.buffer = vx_alloc_range.block->data;
                        draw_range.start_byte = vx_alloc_range.start;
                        draw_range.size_bytes = vx_alloc_range.size;
                    }

                    if(geometry.buffer_layout->GetIsIndexed()) {
                        draw_call.draw_ix.buffer = geometry.ix_range.block->data;
                        draw_call.draw_ix.start_byte = geometry.ix_range.start;
                        draw_call.draw_ix.size_bytes = geometry.ix_range.size;
                    }
                    draw_call.valid = true;
                }
            }

            void Reset()
            {
                m_entity_count = 0;
                m_list_ents_rem.clear();
                m_list_ents_add.clear();
                m_list_ents_upd.clear();
                m_list_ent_rd_curr.clear();
                m_list_ent_rd_prev.clear();
                m_list_geometry_ranges.clear();
                m_list_buffers_to_init.clear();
                m_list_buffers_to_sync.clear();
                m_list_new_buffers.clear();
            }

            std::set<gl::Buffer*>& GetBuffersToInit()
            {
                return m_list_buffers_to_init;
            }

            std::set<gl::Buffer*>& GetBuffersToSync()
            {
                return m_list_buffers_to_sync;
            }

            std::vector<shared_ptr<gl::Buffer>>& GetNewBuffers()
            {
                return m_list_new_buffers;
            }

            std::vector<Id>& GetRemovedEntities()
            {
                return m_list_ents_rem;
            }

            std::vector<Id>& GetAddedEntities()
            {
                return m_list_ents_add;
            }

            std::vector<Id>& GetUpdatedEntities()
            {
                return m_list_ents_upd;
            }

            // ============================================================= //

#ifndef KS_ENV_AUTO_TEST
        private:
#endif
            void createDiffs(std::vector<Id>& list_ents_rem,
                             std::vector<Id>& list_ents_add)
            {
                auto compare_less_rd =
                        [this](PairIds const &a, PairIds const &b) -> bool
                {
                    return (a.second < b.second);
                };

                // TODO opt if either new or old calls are empty
                // (no need to do a sort or set_diff in those cases)
                std::vector<PairIds> list_ents_rd_rem;
                list_ents_rd_rem.reserve(m_list_ent_rd_prev.size());

                std::vector<PairIds> list_ents_rd_add;
                list_ents_rd_add.reserve(m_list_ent_rd_curr.size());


                // Sort curr RenderData entities by uid
                std::sort(m_list_ent_rd_curr.begin(),
                          m_list_ent_rd_curr.end(),
                          compare_less_rd);

                // Split into added calls and removed calls

                // Get elements in the previous set of render
                // data but not in this set
                std::set_difference(
                    m_list_ent_rd_prev.begin(),
                    m_list_ent_rd_prev.end(),
                    m_list_ent_rd_curr.begin(),
                    m_list_ent_rd_curr.end(),
                    std::back_inserter(list_ents_rd_rem),
                    compare_less_rd);


                // Get elements in this set of render data
                // but not in the previous set
                std::set_difference(
                    m_list_ent_rd_curr.begin(),
                    m_list_ent_rd_curr.end(),
                    m_list_ent_rd_prev.begin(),
                    m_list_ent_rd_prev.end(),
                    std::back_inserter(list_ents_rd_add),
                    compare_less_rd);


                // Create EntityId lists
                list_ents_rem.reserve(list_ents_rd_rem.size());
                for(auto& ent_rd : list_ents_rd_rem) {
                    list_ents_rem.push_back(ent_rd.first);
                }

                list_ents_add.reserve(list_ents_rd_add.size());
                for(auto& ent_rd : list_ents_rd_add) {
                    list_ents_add.push_back(ent_rd.first);
                }
            }

            bool checkUpdatedGeometry(Geometry& geometry)
            {
                for(auto vx_buff_idx : geometry.GetUpdatedVertexBuffers())
                {
                    if(geometry.GetVertexBuffer(vx_buff_idx)->empty())
                    {
                        return false;
                    }
                }

                if(geometry.GetUpdatedIndexBuffer())
                {
                    if(geometry.GetIndexBuffer()->empty())
                    {
                        return false;
                    }
                }

                return true;
            }

            void removeGeometryRanges(GeometryRanges& geometry)
            {
                bool empty;

                // release buffer ranges
                if(geometry.vx_ranges_valid)
                {
                    for(uint i=0; i < geometry.list_vx_ranges.size(); i++)
                    {
                        geometry.buffer_layout->
                                GetVertexBufferAllocator(i)->
                                ReleaseRange(geometry.list_vx_ranges[i],empty);
                    }
//                    geometry.vx_ranges_valid = false;
                }

                if(geometry.ix_range_valid)
                {
                    geometry.buffer_layout->
                            GetIndexBufferAllocator()->
                            ReleaseRange(geometry.ix_range,empty);

//                    geometry.ix_range_valid = false;
                }

                // reset all
                geometry = GeometryRanges();
            }

            void createGeometryRanges(GeometryRanges& geometry, // rn geometry_ranges
                                      Geometry& geometry_data) // rn geometry
            {
                auto const keep_buff_data =
                        geometry_data.GetRetainGeometry();


                // Vertex Buffers
                for(auto const index : geometry_data.GetUpdatedVertexBuffers())
                {
                    auto& vx_range = geometry.list_vx_ranges[index];
                    auto& vx_data = geometry_data.GetVertexBuffer(index);

                    // Release the previous ranges
                    if(geometry.vx_ranges_valid) {
                        bool empty;
                        geometry.buffer_layout->
                                GetVertexBufferAllocator(index)->
                                    ReleaseRange(vx_range,empty);
                    }

                    // Acquire new range
                    bool created_buffer;
                    acquireVxBuffRange(
                                geometry,
                                index,
                                vx_data->size(),
                                created_buffer);

                    shared_ptr<gl::Buffer> buffer = vx_range.block->data;

                    if(created_buffer) {
                        m_list_buffers_to_init.insert(buffer.get());
                        m_list_new_buffers.push_back(buffer);
                    }

                    // Update the vertex buffer
                    if(keep_buff_data) {
                        buffer->UpdateBuffer(
                                    gl::Buffer::Update{
                                        gl::Buffer::Update::KeepSrcData,
                                        vx_range.start,
                                        0,vx_range.size,
                                        vx_data.get()
                                    });
                    }
                    else {
                        buffer->UpdateBuffer(
                                    gl::Buffer::Update{
                                        gl::Buffer::Update::Defaults,
                                        vx_range.start,
                                        0,vx_range.size,
                                        vx_data.release()
                                    });
                    }

                    m_list_buffers_to_sync.insert(buffer.get());

                    geometry.vx_ranges_valid = true;
                }

                // Index Buffer
                if(geometry_data.GetUpdatedIndexBuffer())
                {
                    auto& ix_data = geometry_data.GetIndexBuffer();

                    // Release the previous range
                    if(geometry.ix_range_valid) {
                        bool empty;
                        geometry.buffer_layout->
                                GetIndexBufferAllocator()->
                                    ReleaseRange(geometry.ix_range,empty);
                    }

                    // Acquire new range
                    bool created_buffer;
                    acquireIxBuffRange(
                                geometry,
                                ix_data->size(),
                                created_buffer);

                    shared_ptr<gl::Buffer> buffer =
                            geometry.ix_range.block->data;

                    if(created_buffer) {
                        m_list_buffers_to_init.insert(buffer.get());
                        m_list_new_buffers.push_back(buffer);
                    }

                    // Update the index buffer
                    if(keep_buff_data) {
                        buffer->UpdateBuffer(
                                    gl::Buffer::Update{
                                        gl::Buffer::Update::KeepSrcData,
                                        geometry.ix_range.start,
                                        0,geometry.ix_range.size,
                                        ix_data.get()
                                    });
                    }
                    else {
                        buffer->UpdateBuffer(
                                    gl::Buffer::Update{
                                        gl::Buffer::Update::Defaults,
                                        geometry.ix_range.start,
                                        0,geometry.ix_range.size,
                                        ix_data.release()
                                    });
                    }

                    m_list_buffers_to_sync.insert(buffer.get());
                }
            }

            void acquireVxBuffRange(GeometryRanges& geometry,
                                    uint const vx_buff_index,
                                    uint const list_vx_sz,
                                    bool& created_buffer)
            {
                auto& vx_allocator = geometry.buffer_layout->
                        GetVertexBufferAllocator(vx_buff_index);

                VertexBufferAllocator::Range& vx_range =
                        geometry.list_vx_ranges[vx_buff_index];

                uint const block_sz = vx_allocator->GetBlockSize();

                if(block_sz < list_vx_sz) {
                    throw GeometryBufferAllocFailed(
                                "draw: GeometryUpdateTask: "
                                "list_vx size exceeds capacity: "
                                "block_sz: "+ks::to_string(block_sz)+", "
                                "list_vx_sz: "+ks::to_string(list_vx_sz));
                }

                created_buffer = false;

                // Find space to store the data
                vx_range = vx_allocator->AcquireRange(list_vx_sz);

                if(vx_range.size==0) {
                    // Allocate a new block
                    // TODO maybe pass back the buffer?
                    vx_allocator->CreateBlock(
                                make_shared<gl::VertexBuffer>(
                                    geometry.buffer_layout->GetVertexLayout(vx_buff_index),
                                    geometry.buffer_layout->GetBufferUsage()));
                    created_buffer = true;

                    vx_range = vx_allocator->AcquireRange(list_vx_sz);

                    assert(vx_range.size == list_vx_sz);

                    // Reserve space for an entire block in the gl buffer
                    vx_range.block->data->UpdateBuffer(
                                gl::Buffer::Update{
                                    gl::Buffer::Update::ReUpload |
                                    gl::Buffer::Update::KeepSrcData,
                                    0,0,block_sz,nullptr
                                });
                }
            }

            void acquireIxBuffRange(GeometryRanges& geometry,
                                    uint const list_ix_sz,
                                    bool& created_buffer)
            {
                auto buff_usage = geometry.buffer_layout->
                        GetBufferUsage();

                auto& ix_allocator = geometry.buffer_layout->
                        GetIndexBufferAllocator();

                uint const block_sz = ix_allocator->GetBlockSize();

                if(block_sz < list_ix_sz) {
                    throw GeometryBufferAllocFailed(
                                "draw: GeometryUpdateTask: "
                                "list_ix size exceeds capacity");
                }

                created_buffer = false;
                geometry.ix_range = ix_allocator->AcquireRange(list_ix_sz);

                if(geometry.ix_range.size == 0) {
                    ix_allocator->CreateBlock(
                                make_shared<gl::IndexBuffer>(buff_usage));

                    created_buffer = true;

                    geometry.ix_range = ix_allocator->AcquireRange(list_ix_sz);
                    assert(geometry.ix_range.size == list_ix_sz);

                    geometry.ix_range.block->data->UpdateBuffer(
                                gl::Buffer::Update{
                                    gl::Buffer::Update::ReUpload |
                                    gl::Buffer::Update::KeepSrcData,
                                    0,0,block_sz,nullptr
                                });
                }

                geometry.ix_range_valid = true;
            }


            uint m_entity_count;

            // <Entity Id, RenderData Id>
            std::vector<Id> m_list_ents_rem;
            std::vector<Id> m_list_ents_add;
            std::vector<Id> m_list_ents_upd;

            std::vector<PairIds> m_list_ent_rd_curr;
            std::vector<PairIds> m_list_ent_rd_prev;
            std::vector<GeometryRanges> m_list_geometry_ranges;
            std::set<gl::Buffer*> m_list_buffers_to_init;
            std::set<gl::Buffer*> m_list_buffers_to_sync;
            std::vector<shared_ptr<gl::Buffer>> m_list_new_buffers;
        };

        // ============================================================= //
        // ============================================================= //
    }
}

#endif // KS_DRAW_DRAW_CALL_UPDATER_HPP
