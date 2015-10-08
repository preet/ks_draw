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

#include <catch/catch.hpp>

#include <ks/draw/KsDrawDrawCallUpdater.hpp>
#include <ks/draw/KsDrawDefaultDrawKey.hpp>

namespace {

    using namespace ks;

    using DrawCallUpdater = draw::DrawCallUpdater<draw::DefaultDrawKey>;
    using GeometryRanges = DrawCallUpdater::GeometryRanges;
    using RenderData = draw::RenderData<draw::DefaultDrawKey>;
    using DrawCall = draw::DrawCall<draw::DefaultDrawKey>;
    using UPtrBuffer = draw::UPtrBuffer;

    // Vertex definition
    struct Vertex {
        glm::vec4 a_v4_position;
        glm::u8vec4 a_v4_color;
    };

    // Vertex layout
    gl::VertexLayout const vx_layout {
        {
            "a_v4_position",
            gl::VertexBuffer::Attribute::Type::Float,
            4,
            false
        }, // 4*4
        {
            "a_v4_color",
            gl::VertexBuffer::Attribute::Type::UByte,
            4,
            true
        } // 1*4
    };

    // VertexBuffer allocator
    shared_ptr<draw::VertexBufferAllocator> vx_buff_alloc =
            make_shared<draw::VertexBufferAllocator>(1024);

    // IndexBuffer allocator
    shared_ptr<draw::IndexBufferAllocator> ix_buff_alloc =
            make_shared<draw::IndexBufferAllocator>(1024);

    // Buffer layout
    draw::BufferLayout buffer_layout(
            gl::Buffer::Usage::Static,
            { vx_layout },
            { vx_buff_alloc },
            ix_buff_alloc);


    UPtrBuffer GenVertexData(uint vx_count)
    {
        UPtrBuffer data =
                make_unique<std::vector<u8>>(
                    sizeof(Vertex)*vx_count,9);

        return data;
    }

    UPtrBuffer GenIndexData(uint ix_count)
    {
        UPtrBuffer data =
                make_unique<std::vector<u8>>(
                    sizeof(u16)*ix_count,9);

        return data;
    }

    RenderData GenRenderData(uint element_count=0)
    {
        RenderData render_data{
                    draw::DefaultDrawKey{},
                    &buffer_layout,
                    nullptr,
                    std::vector<u8>{},
                    draw::Transparency::Opaque
                    };

        auto& geometry = render_data.GetGeometry();

        geometry.GetVertexBuffers().resize(1);

        geometry.GetVertexBuffer(0) =
                (GenVertexData(element_count));

        geometry.SetVertexBufferUpdated(0);

        geometry.GetIndexBuffer() =
                (GenIndexData(element_count));

        geometry.SetIndexBufferUpdated();

        return render_data;
    }

//    bool VerifyVxBuff(UPtrBuffer& vx_data, uint vx_count)
//    {
//        auto check_data = GenVertexData(vx_count);
//        return ((*check_data)==(*vx_data));
//    }

//    bool VerifyIxBuff(UPtrBuffer& ix_data, uint ix_count)
//    {
//        auto check_data = GenIndexData(ix_count);
//        return ((*check_data)==(*ix_data));
//    }
}

//int main()
TEST_CASE("ks::draw::DrawCallUpdater","[draw_draw_call_updater]")
{
    DrawCallUpdater task;
    std::vector<ks::draw::PairIds> list_ent_rd_curr;
    std::vector<RenderData> list_render_data(100);

    SECTION("Verify Remove/Add/Update Diffs")
    {
        std::vector<Id> list_ents_rem;
        std::vector<Id> list_ents_add;

        // Add some entities: 1,2
        list_render_data[1] = GenRenderData(3);
        list_ent_rd_curr.emplace_back(1,list_render_data[1].GetUniqueId());

        list_render_data[2] = GenRenderData(3);
        list_ent_rd_curr.emplace_back(2,list_render_data[2].GetUniqueId());

        task.Update(list_ent_rd_curr,
                    list_render_data);

        list_ents_add = {1,2};
        list_ents_rem = {};
        REQUIRE(task.m_list_ents_add == list_ents_add);
        REQUIRE(task.m_list_ents_rem == list_ents_rem);


        // Add 3
        list_render_data[3] = GenRenderData(3);
        list_ent_rd_curr.emplace_back(3,list_render_data[3].GetUniqueId());

        task.Update(list_ent_rd_curr,
                    list_render_data);

        list_ents_add = {3};
        list_ents_rem = {};
        REQUIRE(task.m_list_ents_add == list_ents_add);
        REQUIRE(task.m_list_ents_rem == list_ents_rem);


        // Remove 1
        list_render_data[1] = RenderData();
        list_ent_rd_curr.erase(list_ent_rd_curr.begin());

        task.Update(list_ent_rd_curr,
                    list_render_data);

        list_ents_add = {};
        list_ents_rem = {1};
        REQUIRE(task.m_list_ents_add == list_ents_add);
        REQUIRE(task.m_list_ents_rem == list_ents_rem);


        // Remove 2 and then add it multiple times without
        // calling Setup in between
        list_render_data[2] = RenderData();
        list_ent_rd_curr.erase(list_ent_rd_curr.begin());

        list_render_data[2] = GenRenderData(3);
        list_ent_rd_curr.emplace_back(2,list_render_data[2].GetUniqueId());

        list_render_data[2] = RenderData();
        list_ent_rd_curr.pop_back();

        list_render_data[2] = GenRenderData(3);
        list_ent_rd_curr.emplace_back(2,list_render_data[2].GetUniqueId());

        list_render_data[2] = RenderData();
        list_ent_rd_curr.pop_back();

        list_render_data[2] = GenRenderData();
        list_ent_rd_curr.emplace_back(2,list_render_data[2].GetUniqueId());

        task.Update(list_ent_rd_curr,
                    list_render_data);

        list_ents_add = {2};
        list_ents_rem = {2};
        REQUIRE(task.m_list_ents_add == list_ents_add);
        REQUIRE(task.m_list_ents_rem == list_ents_rem);
    }

    SECTION("Verify Remove/Add/Update RenderData --> GeometryRanges")
    {
        std::vector<DrawCall> list_draw_calls;
        auto size_bytes_3_vx = GenVertexData(3)->size();
        auto size_bytes_3_ix = GenIndexData(3)->size();
        auto size_bytes_5_vx = GenVertexData(5)->size();
        auto size_bytes_10_ix = GenIndexData(10)->size();


        // Add Entity 1
        list_render_data[1] = GenRenderData(3);
        list_ent_rd_curr.emplace_back(1,list_render_data[1].GetUniqueId());

        task.Update(list_ent_rd_curr,list_render_data);
        GeometryRanges* geometry = &(task.m_list_geometry_ranges[1]);
        REQUIRE(list_render_data[1].GetGeometry().GetUpdatedGeometry()==false);
        REQUIRE(geometry->valid == true);
        REQUIRE(geometry->list_vx_ranges[0].size == size_bytes_3_vx);
        REQUIRE(geometry->ix_range.size == size_bytes_3_ix);

        task.Sync(list_draw_calls);
        DrawCall* draw_call = &(list_draw_calls[1]);
        REQUIRE(draw_call->valid == true);
        REQUIRE(draw_call->list_draw_vx[0].size_bytes == size_bytes_3_vx);
        REQUIRE(draw_call->draw_ix.size_bytes == size_bytes_3_ix);


        // Update Entity 1 Vertex Data
        list_render_data[1].GetGeometry().GetVertexBuffer(0) = GenVertexData(5);
        list_render_data[1].GetGeometry().SetVertexBufferUpdated(0);
        REQUIRE(list_render_data[1].GetGeometry().GetUpdatedGeometry());

        task.Update(list_ent_rd_curr,list_render_data);
        REQUIRE(geometry->list_vx_ranges[0].size == size_bytes_5_vx);
        REQUIRE(geometry->ix_range.size == size_bytes_3_ix);

        task.Sync(list_draw_calls);
        REQUIRE(draw_call->valid == true);
        REQUIRE(draw_call->list_draw_vx[0].size_bytes == size_bytes_5_vx);
        REQUIRE(draw_call->draw_ix.size_bytes == size_bytes_3_ix);


        // Update Entity 1 Index Data
        list_render_data[1].GetGeometry().GetIndexBuffer() = GenIndexData(10);
        list_render_data[1].GetGeometry().SetIndexBufferUpdated();
        REQUIRE(list_render_data[1].GetGeometry().GetUpdatedGeometry());

        task.Update(list_ent_rd_curr,list_render_data);
        REQUIRE(geometry->list_vx_ranges[0].size == size_bytes_5_vx);
        REQUIRE(geometry->ix_range.size == size_bytes_10_ix);

        task.Sync(list_draw_calls);
        REQUIRE(draw_call->valid == true);
        REQUIRE(draw_call->list_draw_vx[0].size_bytes == size_bytes_5_vx);
        REQUIRE(draw_call->draw_ix.size_bytes == size_bytes_10_ix);


        // Remove Entity 1
        list_ent_rd_curr.clear();
        task.Update(list_ent_rd_curr,list_render_data);
        REQUIRE(geometry->valid == false);
        REQUIRE(geometry->list_vx_ranges.empty());

        task.Sync(list_draw_calls);
        REQUIRE(draw_call->valid == false);
        REQUIRE(draw_call->list_draw_vx.empty());
    }
}
