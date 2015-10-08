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

#include <ks/draw/KsDrawComponents.hpp>
#include <ks/draw/KsDrawDefaultDrawKey.hpp>
#include <ks/draw/KsDrawScene.hpp>
#include <ks/draw/KsDrawRenderSystem.hpp>
#include <ks/draw/KsDrawBatchSystem.hpp>

namespace test_draw_batch_system
{
    using Geometry = ks::draw::Geometry;
    using RenderData = ks::draw::RenderData<ks::draw::DefaultDrawKey>;
    using BatchData = ks::draw::BatchData;
    using UPtrBuffer = ks::draw::UPtrBuffer;


    // Vertex definition
    struct Vertex {
        glm::vec4 a_v4_position;
        glm::u8vec4 a_v4_color;
    };

    // Vertex layout
    ks::gl::VertexLayout const vx_layout {
        {
            "a_v4_position",
            ks::gl::VertexBuffer::Attribute::Type::Float,
            4,
            false
        }, // 4*4
        {
            "a_v4_color",
            ks::gl::VertexBuffer::Attribute::Type::UByte,
            4,
            true
        } // 1*4
    };

    // VertexBuffer allocator
    ks::shared_ptr<ks::draw::VertexBufferAllocator> vx_buff_alloc =
            ks::make_shared<ks::draw::VertexBufferAllocator>(1024);

    // IndexBuffer allocator
    ks::shared_ptr<ks::draw::IndexBufferAllocator> ix_buff_alloc =
            ks::make_shared<ks::draw::IndexBufferAllocator>(1024);

    // Buffer layout
    ks::draw::BufferLayout buffer_layout(
            ks::gl::Buffer::Usage::Static,
            { vx_layout },
            { vx_buff_alloc },
            ix_buff_alloc);


    struct SceneKey {
        static uint const max_component_types{8};
    };


    class Scene : public ks::draw::Scene<SceneKey>
    {
    public:
        using RenderSystem =
            ks::draw::RenderSystem<
                SceneKey,
                ks::draw::DefaultDrawKey
            >;

        using BatchSystem =
            ks::draw::BatchSystem<
                SceneKey,
                ks::draw::DefaultDrawKey
            >;

        Scene(ks::Object::Key const &key,
              ks::shared_ptr<ks::EventLoop> const &evl) :
            ks::draw::Scene<SceneKey>(
                key,
                evl)
        {}

        void Init(ks::Object::Key const &,
                  ks::shared_ptr<Scene> const &)
        {
            m_render_system =
                    ks::make_unique<RenderSystem>(
                        this);

            m_batch_system =
                    ks::make_unique<BatchSystem>(
                        this,
                        m_render_system->GetRenderDataComponentList());
        }

        ~Scene() = default;

        void onUpdate()
        {

        }

        void onSync()
        {

        }

        void onRender()
        {

        }

        ks::unique_ptr<RenderSystem> m_render_system;
        ks::unique_ptr<BatchSystem> m_batch_system;
    };


    UPtrBuffer GenVertexData(uint vx_count)
    {
        UPtrBuffer data =
                ks::make_unique<std::vector<ks::u8>>(
                    sizeof(Vertex)*vx_count,9);

        return data;
    }

    UPtrBuffer GenIndexData(uint ix_count)
    {
        UPtrBuffer data =
                ks::make_unique<std::vector<ks::u8>>(
                    sizeof(ks::u16)*ix_count,9);

        return data;
    }

    RenderData* CreateRenderData(Scene* scene,
                                 ks::Id entity_id)
    {
        auto cmlist = scene->m_render_system->
                GetRenderDataComponentList();

        auto& render_data =
                cmlist->Create(
                    entity_id,
                    ks::draw::DefaultDrawKey{},
                    &buffer_layout,
                    nullptr,
                    std::vector<ks::u8>{},
                    ks::draw::Transparency::Opaque);

        return &render_data;
    }

    BatchData* CreateBatchData(Scene* scene,
                               ks::Id entity_id,
                               ks::Id batch_id)
    {
        auto cmlist = scene->m_batch_system->
                GetBatchDataComponentList();

        auto& batch_data =
                cmlist->Create(
                    entity_id,
                    batch_id);

        return &batch_data;
    }

    template<typename T> // T==RenderData or BatchData!
    Geometry* FillGeometry(T* component,ks::uint element_count)
    {
        auto& geometry_data = component->GetGeometry();
        geometry_data.ClearGeometryUpdates();

        geometry_data.GetVertexBuffers().resize(1);

        geometry_data.GetVertexBuffer(0) =
                std::move(GenVertexData(element_count));

        geometry_data.SetVertexBufferUpdated(0);

        geometry_data.GetIndexBuffer() =
                std::move(GenIndexData(element_count));

        geometry_data.SetIndexBufferUpdated();

        return &geometry_data;
    }

    uint GetVertexSizeBytes(uint count)
    {
        return (sizeof(Vertex)*count);
    }

    uint GetIndexSizeBytes(uint count)
    {
        return (sizeof(ks::u16)*count);
    }
}

// int main()
TEST_CASE("ks::draw::BatchSystem")
{
    using namespace test_draw_batch_system;

    ks::draw::time_point tp0;
    ks::draw::time_point tp1;

    ks::shared_ptr<Scene> scene =
            ks::make_object<Scene>(
                ks::make_shared<ks::EventLoop>());

    auto& render_system = scene->m_render_system;
    auto& batch_system = scene->m_batch_system;

    auto& list_render_data =
            scene->m_render_system->
                GetRenderDataComponentList()->
                GetSparseList();

    auto& list_batch_data =
            scene->m_batch_system->
                GetBatchDataComponentList()->
                GetSparseList();

    // Create a SingleFrame batch group
    ks::shared_ptr<ks::draw::Batch<ks::draw::DefaultDrawKey>> batch0 =
            ks::make_shared<ks::draw::Batch<ks::draw::DefaultDrawKey>>(
                ks::draw::DefaultDrawKey{},
                &buffer_layout,
                nullptr,
                std::vector<ks::u8>{},
                ks::draw::Transparency::Opaque,
                ks::draw::UpdatePriority::SingleFrame);

    auto const batch0_id = batch_system->RegisterBatch(batch0);
    auto const batch0_ent = batch_system->GetBatchEntity(batch0_id);


    // Create a MultiFrame batch group
    ks::shared_ptr<ks::draw::Batch<ks::draw::DefaultDrawKey>> batch1 =
            ks::make_shared<ks::draw::Batch<ks::draw::DefaultDrawKey>>(
                ks::draw::DefaultDrawKey{},
                &buffer_layout,
                nullptr,
                std::vector<ks::u8>{},
                ks::draw::Transparency::Opaque,
                ks::draw::UpdatePriority::MultiFrame);

    auto const batch1_id = batch_system->RegisterBatch(batch1);
    auto const batch1_ent = batch_system->GetBatchEntity(batch1_id);

    SECTION("Add/Remove/Update Entities [SingleFrame]")
    {
        auto& geometry_batch0 = list_render_data[batch0_ent].GetGeometry();
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->empty());
        REQUIRE(geometry_batch0.GetIndexBuffer()->empty());

        // Add Entity 1
        // NOTE 'Entity 1 doesn't correspond to Id==1', the
        // Ids aren't obvious because Batches have their own
        // Id as well
        auto const ent1 = scene->CreateEntity();
        auto batch_data1 = CreateBatchData(scene.get(),ent1,batch0_id);
        auto geometry_data1 = FillGeometry(batch_data1,2);

        batch_system->Update(tp0,tp1);
        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(2));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(2));

        // Add Entity 2
        auto const ent2 = scene->CreateEntity();
        auto batch_data2 = CreateBatchData(scene.get(),ent2,batch0_id);
        auto geometry_data2 = FillGeometry(batch_data2,5);

        batch_system->Update(tp0,tp1);
        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(7));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(7));

        // Update Entity 1's geometry
        geometry_data1->GetVertexBuffer(0) = std::move(GenVertexData(10));
        geometry_data1->GetIndexBuffer() = std::move(GenIndexData(10));
        geometry_data1->SetVertexBufferUpdated(0);
        geometry_data1->SetIndexBufferUpdated();

        batch_system->Update(tp0,tp1);
        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(15));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(15));

        // Remove Entity 1
        scene->RemoveEntity(ent1);
        batch_system->Update(tp0,tp1);
        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(5));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(5));

        // Remove Entity 2
        scene->RemoveEntity(ent2);
        batch_system->Update(tp0,tp1);
        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->empty());
        REQUIRE(geometry_batch0.GetIndexBuffer()->empty());
    }

    SECTION("Add/Remove/Update Entities [MultiFrame]")
    {
        auto& geometry_batch0 = list_render_data[batch1_ent].GetGeometry();
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->empty());
        REQUIRE(geometry_batch0.GetIndexBuffer()->empty());

        // Add Entity 1
        // NOTE 'Entity 1 doesn't correspond to Id==1', the
        // Ids aren't obvious because Batches have their own
        // Id as well
        auto const ent1 = scene->CreateEntity();
        auto batch_data1 = CreateBatchData(scene.get(),ent1,batch1_id);
        auto geometry_data1 = FillGeometry(batch_data1,2);

        // For MultiFrame batches, BatchSystem takes at least
        // one extra Update to set the RenderData
        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(2));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(2));


        // Add Entity 2
        auto const ent2 = scene->CreateEntity();
        auto batch_data2 = CreateBatchData(scene.get(),ent2,batch1_id);
        auto geometry_data2 = FillGeometry(batch_data2,5);

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(7));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(7));

        // Update Entity 1's geometry
        geometry_data1->GetVertexBuffer(0) = std::move(GenVertexData(10));
        geometry_data1->GetIndexBuffer() = std::move(GenIndexData(10));
        geometry_data1->SetVertexBufferUpdated(0);
        geometry_data1->SetIndexBufferUpdated();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(15));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(15));

        // Remove Entity 1
        scene->RemoveEntity(ent1);

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(5));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(5));

        // Remove Entity 2
        scene->RemoveEntity(ent2);

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->empty());
        REQUIRE(geometry_batch0.GetIndexBuffer()->empty());
    }
}
