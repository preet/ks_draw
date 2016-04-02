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
                        this);
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


    UPtrBuffer GenVertexData(uint vx_count,uint val=9)
    {
        UPtrBuffer data =
                ks::make_unique<std::vector<ks::u8>>();

        auto& list_vx = *data;
        for(uint i=0; i < vx_count; i++) {
            ks::gl::Buffer::PushElement<Vertex>(
                        list_vx,
                        Vertex{
                            glm::vec4{},
                            glm::u8vec4{val,val,val,val}
                        });
        }

        return data;
    }

    UPtrBuffer GenIndexData(uint ix_count,uint val=9)
    {
        UPtrBuffer data =
                ks::make_unique<std::vector<ks::u8>>();

        auto& list_ix = *data;
        for(uint i=0; i < ix_count; i++) {
            ks::gl::Buffer::PushElement<ks::u16>(list_ix,val);
        }


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
    Geometry* FillGeometry(T* component,ks::uint element_count,uint val=9)
    {
        auto& geometry_data = component->GetGeometry();
        geometry_data.ClearGeometryUpdates();

        geometry_data.GetVertexBuffers().resize(1);

        geometry_data.GetVertexBuffer(0) =
                std::move(GenVertexData(element_count,val));

        geometry_data.SetVertexBufferUpdated(0);

        geometry_data.GetIndexBuffer() =
                std::move(GenIndexData(element_count,val));

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

    ks::TimePoint tp0;
    ks::TimePoint tp1;

    ks::shared_ptr<Scene> scene =
            ks::MakeObject<Scene>(
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

    SECTION("[MultiFrame] unintentionally reused Batch groups")
    {
        // If a MultiFrame batch has its BatchGroup deleted and
        // then recreated before the batch returns, the merged gm
        // will be tied to the 'wrong' group

        // We should be using uids to ensure this doesn't happen

        ks::shared_ptr<ks::draw::Batch<ks::draw::DefaultDrawKey>> batch_a =
                ks::make_shared<ks::draw::Batch<ks::draw::DefaultDrawKey>>(
                    ks::draw::DefaultDrawKey{},
                    &buffer_layout,
                    nullptr,
                    std::vector<ks::u8>{},
                    ks::draw::Transparency::Opaque,
                    ks::draw::UpdatePriority::MultiFrame);

        auto const batch_a_id = batch_system->RegisterBatch(batch_a);

        // Add a couple of entities
        auto const ent1 = scene->CreateEntity();
        auto batch_data1 = CreateBatchData(scene.get(),ent1,batch_a_id);
        auto geometry_data1 = FillGeometry(batch_data1,2);
        batch_data1->SetRebuild(true);

        auto const ent2 = scene->CreateEntity();
        auto batch_data2 = CreateBatchData(scene.get(),ent2,batch_a_id);
        auto geometry_data2 = FillGeometry(batch_data2,5);
        batch_data2->SetRebuild(true);

        batch_system->Update(tp0,tp1);

        // Remove and add another batch group
        scene->RemoveEntity(ent1);
        scene->RemoveEntity(ent2);
        batch_system->RemoveBatch(batch_a_id);

        ks::shared_ptr<ks::draw::Batch<ks::draw::DefaultDrawKey>> batch_b =
                ks::make_shared<ks::draw::Batch<ks::draw::DefaultDrawKey>>(
                    ks::draw::DefaultDrawKey{},
                    &buffer_layout,
                    nullptr,
                    std::vector<ks::u8>{},
                    ks::draw::Transparency::Opaque,
                    ks::draw::UpdatePriority::MultiFrame);

        auto const batch_b_id = batch_system->RegisterBatch(batch_b);

        REQUIRE(batch_a_id == batch_b_id);

        batch_system->WaitOnMultiFrameBatch();

        // The MultiFrame groups have a single update delay
        batch_system->Update(tp0,tp1);

        // If the new batch's merged entity has a non-empty
        // RenderData, the wrong data was assigned. It must be empty.
        auto const list_batch_b_ents = batch_system->GetBatchEntities(batch_b_id);
        REQUIRE(list_batch_b_ents.size()==0);
    }

    SECTION("Add/Remove/Update Entities [SingleFrame]")
    {
        // Add Entity 1
        // NOTE 'Entity 1 doesn't correspond to Id==1', the
        // Ids aren't obvious because Batches have their own
        // Id as well
        auto const ent1 = scene->CreateEntity();
        auto batch_data1 = CreateBatchData(scene.get(),ent1,batch0_id);
        auto geometry_data1 = FillGeometry(batch_data1,2);
        batch_data1->SetRebuild(true);

        batch_system->Update(tp0,tp1);

        auto list_batch0_ents = batch_system->GetBatchEntities(batch0_id);
        REQUIRE(list_batch0_ents.size()==1);
        auto& geometry_batch0 =
                list_render_data[list_batch0_ents[0]].GetGeometry();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(2));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(2));

        // Add Entity 2
        auto const ent2 = scene->CreateEntity();
        auto batch_data2 = CreateBatchData(scene.get(),ent2,batch0_id);
        auto geometry_data2 = FillGeometry(batch_data2,5);
        batch_data2->SetRebuild(true);

        batch_system->Update(tp0,tp1);
        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(7));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(7));

        // Update Entity 1's geometry
        geometry_data1->GetVertexBuffer(0) = GenVertexData(10);
        geometry_data1->GetIndexBuffer() = GenIndexData(10);
        geometry_data1->SetVertexBufferUpdated(0);
        geometry_data1->SetIndexBufferUpdated();
        batch_data1->SetRebuild(true);

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

        // Shouldn't be any more merged geometries
        list_batch0_ents = batch_system->GetBatchEntities(batch0_id);
        REQUIRE(list_batch0_ents.size()==0);
    }

    SECTION("PreMerge, PostMerge, and PreTask Callbacks")
    {
        SECTION("SingleFrame")
        {
            uint pre_callback_count=0;
            uint post_callback_count=0;

            // Pre merge callback
            batch_system->SetSFPreMergeCallback(
                        [&]
                        (ks::Id batch_id,
                         std::vector<ks::Id> const &list_ents_ref)
                        {
                            (void)batch_id;
                            // Sort in descending order by Id
                            std::vector<ks::Id> list_ents = list_ents_ref;
                            std::sort(list_ents.begin(),list_ents.end());
                            std::reverse(list_ents.begin(),list_ents.end());

                            pre_callback_count++;
                            return list_ents;
                        });

            // Post merge callback
            batch_system->SetSFPostMergeCallback(
                        [&]
                        (ks::Id,
                         std::vector<ks::Id> const &,
                         std::vector<std::vector<ks::Id>> const &)
                        {
                            post_callback_count++;
                        });

            // Add Entity 1,2,3
            auto const ent1 = scene->CreateEntity();
            auto batch_data1 = CreateBatchData(scene.get(),ent1,batch0_id);
            auto geometry_data1 = FillGeometry(batch_data1,3,1);
            batch_data1->SetRebuild(true);

            auto const ent2 = scene->CreateEntity();
            auto batch_data2 = CreateBatchData(scene.get(),ent2,batch0_id);
            auto geometry_data2 = FillGeometry(batch_data2,3,2);
            batch_data2->SetRebuild(true);

            auto const ent3 = scene->CreateEntity();
            auto batch_data3 = CreateBatchData(scene.get(),ent3,batch0_id);
            auto geometry_data3 = FillGeometry(batch_data3,3,3);
            batch_data3->SetRebuild(true);

            // Update
            batch_system->Update(tp0,tp1);

            // (verify pre merge callback)
            REQUIRE(pre_callback_count==1);

            // Verify expected vertices
            auto list_vx1 = GenVertexData(3,1);
            auto list_vx2 = GenVertexData(3,2);
            auto list_vx3 = GenVertexData(3,3);
            std::vector<ks::u8> list_vx;

            // insert in reverse order
            list_vx.insert(list_vx.end(),list_vx3->begin(),list_vx3->end());
            list_vx.insert(list_vx.end(),list_vx2->begin(),list_vx2->end());
            list_vx.insert(list_vx.end(),list_vx1->begin(),list_vx1->end());

            auto const list_batch0_ents = batch_system->GetBatchEntities(batch0_id);
            REQUIRE(list_batch0_ents.size()==1);
            auto& geometry_batch0 =
                    list_render_data[list_batch0_ents[0]].GetGeometry();

            auto& vertex_buffer = *(geometry_batch0.GetVertexBuffer(0));
            REQUIRE(list_vx == vertex_buffer);

            // (verify post merge callback)
            REQUIRE(post_callback_count==1);
        }

        SECTION("MultiFrame")
        {
            uint pre_task_callback_count=0;
            uint pre_merge_callback_count=0;
            uint post_merge_callback_count=0;

            // pre task callback
            batch_system->SetMFPreTaskCallback(
                        [&]
                        ()
                        {
                            pre_task_callback_count++;
                        });

            // Pre merge callback
            batch_system->SetMFPreMergeCallback(
                        [&]
                        (ks::Id batch_id,
                         std::vector<ks::Id> const &list_ents_ref)
                        {
                            (void)batch_id;

                            // Sort in descending order by Id
                            std::vector<ks::Id> list_ents = list_ents_ref;
                            std::sort(list_ents.begin(),list_ents.end());
                            std::reverse(list_ents.begin(),list_ents.end());

                            pre_merge_callback_count++;
                            return list_ents;
                        });

            // Post merge callback
            batch_system->SetMFPostMergeCallback(
                        [&]
                        (ks::Id,
                         std::vector<ks::Id> const &,
                         std::vector<std::vector<ks::Id>> const &)
                        {
                            post_merge_callback_count++;
                        });


            // Add Entity 1,2,3
            auto const ent1 = scene->CreateEntity();
            auto batch_data1 = CreateBatchData(scene.get(),ent1,batch1_id);
            auto geometry_data1 = FillGeometry(batch_data1,3,1);
            batch_data1->SetRebuild(true);

            auto const ent2 = scene->CreateEntity();
            auto batch_data2 = CreateBatchData(scene.get(),ent2,batch1_id);
            auto geometry_data2 = FillGeometry(batch_data2,3,2);
            batch_data2->SetRebuild(true);

            auto const ent3 = scene->CreateEntity();
            auto batch_data3 = CreateBatchData(scene.get(),ent3,batch1_id);
            auto geometry_data3 = FillGeometry(batch_data3,3,3);
            batch_data3->SetRebuild(true);

            // Update

            // For MultiFrame batches, BatchSystem takes at least
            // one extra Update to set the RenderData

            batch_system->Update(tp0,tp1);
            batch_system->WaitOnMultiFrameBatch();
            REQUIRE(pre_task_callback_count==1);
            REQUIRE(pre_merge_callback_count==1);

            // post merge occurs on the second update
            REQUIRE(post_merge_callback_count==0);

            batch_system->Update(tp0,tp1);
            batch_system->WaitOnMultiFrameBatch();
            REQUIRE(post_merge_callback_count==1);
            REQUIRE(pre_task_callback_count==2);

            // nothing to rebuild, so the merge callback shouldn't
            // be called
            REQUIRE(pre_merge_callback_count==1);
            REQUIRE(post_merge_callback_count==1);

            batch_system->Update(tp0,tp1);
            batch_system->WaitOnMultiFrameBatch();
            REQUIRE(post_merge_callback_count==1);

            // Verify expected vertices
            auto list_vx1 = GenVertexData(3,1);
            auto list_vx2 = GenVertexData(3,2);
            auto list_vx3 = GenVertexData(3,3);
            std::vector<ks::u8> list_vx;

            // insert in reverse order
            list_vx.insert(list_vx.end(),list_vx3->begin(),list_vx3->end());
            list_vx.insert(list_vx.end(),list_vx2->begin(),list_vx2->end());
            list_vx.insert(list_vx.end(),list_vx1->begin(),list_vx1->end());


            auto const list_batch1_ents =
                    batch_system->GetBatchEntities(batch1_id);

            REQUIRE(list_batch1_ents.size()==1);

            auto& geometry_batch1 =
                    list_render_data[list_batch1_ents[0]].GetGeometry();

            auto& vertex_buffer = *(geometry_batch1.GetVertexBuffer(0));
            REQUIRE(list_vx == vertex_buffer);
        }
    }

    SECTION("Add/Remove/Update Entities [MultiFrame]")
    {
        auto list_batch1_ents = batch_system->GetBatchEntities(batch1_id);
        REQUIRE(list_batch1_ents.size()==0);

        // Add Entity 1
        // NOTE 'Entity 1 doesn't correspond to Id==1', the
        // Ids aren't obvious because Batches have their own
        // Id as well
        auto const ent1 = scene->CreateEntity();
        auto batch_data1 = CreateBatchData(scene.get(),ent1,batch1_id);
        auto geometry_data1 = FillGeometry(batch_data1,2);
        batch_data1->SetRebuild(true);

        // For MultiFrame batches, BatchSystem takes at least
        // one extra Update to set the RenderData
        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        list_batch1_ents = batch_system->GetBatchEntities(batch1_id);
        REQUIRE(list_batch1_ents.size()==1);
        auto& geometry_batch0 =
                list_render_data[list_batch1_ents[0]].GetGeometry();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(2));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(2));


        // Add Entity 2
        auto const ent2 = scene->CreateEntity();
        auto batch_data2 = CreateBatchData(scene.get(),ent2,batch1_id);
        auto geometry_data2 = FillGeometry(batch_data2,5);
        batch_data2->SetRebuild(true);

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        batch_system->Update(tp0,tp1);
        batch_system->WaitOnMultiFrameBatch();

        REQUIRE(geometry_batch0.GetUpdatedGeometry());
        REQUIRE(geometry_batch0.GetVertexBuffer(0)->size() == GetVertexSizeBytes(7));
        REQUIRE(geometry_batch0.GetIndexBuffer()->size() == GetIndexSizeBytes(7));

        // Update Entity 1's geometry
        geometry_data1->GetVertexBuffer(0) = GenVertexData(10);
        geometry_data1->GetIndexBuffer() = GenIndexData(10);
        geometry_data1->SetVertexBufferUpdated(0);
        geometry_data1->SetIndexBufferUpdated();
        batch_data1->SetRebuild(true);

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

        list_batch1_ents = batch_system->GetBatchEntities(batch1_id);
        REQUIRE(list_batch1_ents.size()==0);
    }
}
