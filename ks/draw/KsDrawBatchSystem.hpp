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

#ifndef KS_BATCH_SYSTEM_HPP
#define KS_BATCH_SYSTEM_HPP

#include <ks/ecs/KsEcs.hpp>
#include <ks/draw/KsDrawSystem.hpp>
#include <ks/draw/KsDrawComponents.hpp>
#include <ks/shared/KsThreadPool.hpp>

namespace ks
{
    namespace draw
    {

        // ============================================================= //
        // ============================================================= //

        using BatchPreMergeCallback =
            std::function<std::vector<Id>(std::vector<Id> const &)>;

        using BatchPreTaskCallback = std::function<void()>;

        namespace detail
        {
            inline void IncrementListIx(u16* ix_ptr, u16 ix_count, u16 inc)
            {
                for(u16 i=0; i < ix_count; i++) {
                    *ix_ptr += inc;
                    ix_ptr++;
                }
            }

            void CreateMergedGeometry(BufferLayout const * buffer_layout,
                                      std::vector<Geometry*> &list_single_gm,
                                      Geometry* merged_gm);

            class BatchTask final : public ks::ThreadPool::Task
            {
            public:
                struct BatchDesc
                {
                    Id batch_id;
                    Id merged_ent;
                    std::vector<Id> list_ents;
                    BufferLayout const * buffer_layout;
                };

                BatchTask(unique_ptr<std::vector<BatchDesc>> list_batch_desc,
                          std::vector<Geometry>& list_batch_geometry,
                          BatchPreMergeCallback pre_merge_callback);

                ~BatchTask();

                std::vector<BatchDesc> const & GetListBatchDesc() const;

                void Cancel() override;

                void Process();

            private:
                void process() override;

                unique_ptr<std::vector<BatchDesc>> m_list_batch_desc;
                std::vector<Geometry>& m_list_batch_geometry;
                BatchPreMergeCallback m_pre_merge_callback;
            };
        }

        // ============================================================= //
        // ============================================================= //

        template<typename SceneKeyType,
                 typename DrawKeyType>
        class BatchSystem final : public draw::System
        {
            struct BatchGroup
            {
                shared_ptr<Batch<DrawKeyType>> batch;

                Id merged_ent{0};
                Geometry* merged_gm{nullptr};
                bool rebuild{false};

                std::vector<Id> list_ents_prev;
                std::vector<Id> list_ents_curr;
                std::vector<Id> list_ents_rem;
                std::vector<Id> list_ents_upd;
            };

        public:
            using RenderData = draw::RenderData<DrawKeyType>;

            using BatchDataComponentList =
                ecs::ComponentList<SceneKeyType,BatchData>;

            using RenderDataComponentList =
                ecs::ComponentList<SceneKeyType,RenderData>;

            BatchSystem(ecs::Scene<SceneKeyType>* scene) :
                m_scene(scene),
                m_cmlist_render_data(
                    static_cast<RenderDataComponentList*>(
                        scene->template GetComponentList<RenderData>())),
                m_thread_pool(1)
            {
                // Create the BatchData component list
                m_scene->template RegisterComponentList<BatchData>(
                            make_unique<BatchDataComponentList>(*m_scene));

                m_cmlist_batch_data =
                        static_cast<BatchDataComponentList*>(
                            m_scene->template GetComponentList<BatchData>());

                // Reserve batch id 0 as invalid
                m_list_batch_groups.Add(nullptr);

                // Create and process an empty batch task
                // to setup initial valid state
                m_batch_task =
                        make_shared<detail::BatchTask>(
                            make_unique<std::vector<detail::BatchTask::BatchDesc>>(),
                            m_list_batch_geometry,
                            m_pre_merge_callback_mf);

                // (doesn't do anything except set the Task
                //  state to finished)
                m_batch_task->Process();
            }

            ~BatchSystem()
            {

            }

            std::string GetDesc() const override
            {
                return "BatchSystem";
            }

            BatchDataComponentList* GetBatchDataComponentList() const
            {
                return m_cmlist_batch_data;
            }

            shared_ptr<Batch<DrawKeyType>> const & GetBatch(Id batch_id) const
            {
                return m_list_batch_groups[batch_id]->batch;
            }

            Id GetBatchEntity(Id batch_id)
            {
                return m_list_batch_groups[batch_id]->merged_ent;
            }

            Id RegisterBatch(shared_ptr<Batch<DrawKeyType>> batch)
            {
                auto const batch_id =
                        m_list_batch_groups.Add(
                            make_unique<BatchGroup>());

                auto& batch_group = m_list_batch_groups.Get(batch_id);

                auto merged_ent_id = m_scene->CreateEntity();

                // Create the RenderData
                auto& render_data =
                        m_cmlist_render_data->Create(
                            merged_ent_id,
                            batch->GetKey(),
                            batch->GetBufferLayout(),
                            batch->GetUniformList(),
                            batch->GetDrawStages(),
                            batch->GetTransparency());

                // TODO set geometry's RetainGeometry
                auto& geometry = render_data.GetGeometry();

                auto const vx_buff_count =
                        batch->GetBufferLayout()->
                            GetVertexBufferCount();

                for(uint i=0; i < vx_buff_count; i++)
                {
                    geometry.GetVertexBuffers().
                            push_back(make_unique<std::vector<u8>>());
                }

                if(batch->GetBufferLayout()->GetIsIndexed())
                {
                    geometry.GetIndexBuffer() =
                            make_unique<std::vector<u8>>();
                }

                batch_group->batch = batch;
                batch_group->merged_ent = merged_ent_id;
                batch_group->merged_gm = &geometry;

                return batch_id;
            }

            void RemoveBatch(Id batch_id)
            {
                auto& batch_group = m_list_batch_groups.Get(batch_id);
                m_scene->RemoveEntity(batch_group->merged_ent);
                m_list_batch_groups.Remove(batch_id);
            }

            void SetSFPreMergeCallback(BatchPreMergeCallback callback)
            {
                m_pre_merge_callback_sf = std::move(callback);
            }

            void ClearSFPreMergeCallback()
            {
                m_pre_merge_callback_sf = nullptr;
            }

            void SetMFPreTaskCallback(BatchPreTaskCallback callback)
            {
                m_pre_task_callback = std::move(callback);
            }

            void ClearMFPreTaskCallback()
            {
                m_pre_task_callback = nullptr;
            }

            void SetMFPreMergeCallback(BatchPreMergeCallback callback)
            {
                m_pre_merge_callback_mf = std::move(callback);
            }

            void ClearMFPreMergeCallback()
            {
                m_pre_merge_callback_mf = nullptr;
            }

            void Update(time_point const &,time_point const &) override
            {
                // Clear previous list of Batchable entities
                auto const num_batch_groups =
                        m_list_batch_groups.GetList().size();

                for(auto& batch_group : m_list_batch_groups.GetList())
                {
                    if(batch_group)
                    {
                        batch_group->rebuild = false;
                        batch_group->list_ents_curr.clear();
                        batch_group->list_ents_rem.clear();
                        batch_group->list_ents_upd.clear();
                    }
                }

                // Get the current list of Batchable entities
                auto const batchable_mask =
                        ecs::Scene<SceneKeyType>::template
                            GetComponentMask<BatchData>();

                auto& list_entities = m_scene->GetEntityList();
                auto& list_batch_data = m_cmlist_batch_data->GetSparseList();
                auto& list_render_data = m_cmlist_render_data->GetSparseList();

                for(uint ent_id=0; ent_id < list_entities.size(); ent_id++)
                {
                    if((list_entities[ent_id].mask & batchable_mask) == batchable_mask)
                    {
                        auto& batch_data = list_batch_data[ent_id];

                        if(batch_data.GetGroupId() > 0)
                        {
                            auto& batch_group = m_list_batch_groups[batch_data.GetGroupId()];

                            // Note that we automatically get ordered
                            // insert for Entity Ids
                            batch_group->list_ents_curr.push_back(ent_id);

                            // Check for updated
                            if(batch_data.GetRebuild())
                            {
                                batch_group->rebuild = true;
                                batch_group->list_ents_upd.push_back(ent_id);
                            }
                        }
                    }
                }

                std::vector<uint> list_sf_batch_groups;
                std::vector<uint> list_mf_batch_groups;

                for(uint group = 0; group < num_batch_groups; group++)
                {
                    auto& batch_group = m_list_batch_groups[group];
                    if(batch_group)
                    {
                        auto& batch = batch_group->batch;
                        if(batch->GetUpdatePriority()==UpdatePriority::SingleFrame) {
                            list_sf_batch_groups.push_back(group);
                        }
                        else {
                            list_mf_batch_groups.push_back(group);
                        }
                    }
                }

                updateBatchGroupsSF(list_sf_batch_groups,
                                    list_batch_data);

                updateBatchTask(list_mf_batch_groups,
                                list_batch_data,
                                list_render_data);
            }

            void WaitOnMultiFrameBatch()
            {
                m_batch_task->Wait();
            }

            static void CopyGeometryBuffers(Geometry const & from, Geometry& to)
            {
                to.GetVertexBuffers().clear();

                for(auto const &vx_data : from.GetVertexBuffers())
                {
                    to.GetVertexBuffers().push_back(
                                make_unique<std::vector<u8>>(*vx_data));
                }

                if(from.GetIndexBuffer())
                {
                    to.GetIndexBuffer() =
                            make_unique<std::vector<u8>>(
                                *(from.GetIndexBuffer()));
                }
            }

            template<typename T>
            static void OrderedUniqueInsert(std::vector<T>& list_data,T ins_data)
            {
                // lower bound == first value thats greater than or equal
                auto it = std::lower_bound(list_data.begin(),
                                           list_data.end(),
                                           ins_data);

                // insert when: it==end, *it != ins_data (ins_data is greater)
                if((it==list_data.end()) || (*it != ins_data) ) {
                    list_data.insert(it,ins_data);
                }
            }

        private:
            void updateBatchGroupsSF(std::vector<uint> const &list_sf_batch_groups,
                                     std::vector<BatchData>& list_batch_data)
            {
                for(auto const group : list_sf_batch_groups)
                {
                    auto& batch_group = m_list_batch_groups[group];
                    auto& batch = batch_group->batch;

                    // If there were no updated GeometryData check to
                    // see if any have been added or removed
                    if(!batch_group->rebuild)
                    {
                        // Find out which entities were removed (ie.
                        // in previous but not in current)
                        std::set_difference(
                                    batch_group->list_ents_prev.begin(),
                                    batch_group->list_ents_prev.end(),
                                    batch_group->list_ents_curr.begin(),
                                    batch_group->list_ents_curr.end(),
                                    std::back_inserter(batch_group->list_ents_rem));

                        // We assume that any entities with BatchData that
                        // were added have updated Geometry so there's no
                        // explicit search for 'added entities'
                        batch_group->rebuild =
                                (!batch_group->list_ents_rem.empty());
                    }

                    if(batch_group->rebuild)
                    {
                        std::vector<Geometry*> list_geometry;
                        list_geometry.reserve(batch_group->list_ents_curr.size());

                        // Allow a callback to modify the list of entities
                        // that will be merged together. Note we don't save
                        // the modified list; the original batch_group->
                        // list_ents_curr must be kept to do proper diffs
                        if(m_pre_merge_callback_sf)
                        {
                            auto list_ents_curr =
                                    m_pre_merge_callback_sf(
                                        batch_group->list_ents_curr);

                            for(auto ent_id : list_ents_curr)
                            {
                                list_geometry.push_back(
                                            &(list_batch_data[ent_id].GetGeometry()));
                            }
                        }
                        else
                        {
                            for(auto ent_id : batch_group->list_ents_curr)
                            {
                                list_geometry.push_back(
                                            &(list_batch_data[ent_id].GetGeometry()));
                            }
                        }

                        detail::CreateMergedGeometry(
                                    batch->GetBufferLayout(),
                                    list_geometry,
                                    batch_group->merged_gm);

                        // Clear updates
                        for(auto ent_id : batch_group->list_ents_upd)
                        {
                            auto& batch_data = list_batch_data[ent_id];
                            batch_data.SetRebuild(false);
                            batch_data.GetGeometry().ClearGeometryUpdates();
                        }

                        batch_group->merged_gm->SetAllUpdated();
                    }

                    batch_group->list_ents_prev =
                            batch_group->list_ents_curr;
                }
            }

            void updateBatchGroupsMF(std::vector<uint> const &list_mf_batch_groups,
                                     std::vector<BatchData>& list_batch_data)
            {
                // Resize BatchGeometry list if necessary
                if(m_list_batch_geometry.size() < m_scene->GetEntityList().size()) {
                    m_list_batch_geometry.resize(m_scene->GetEntityList().size());
                }

                for(auto const group : list_mf_batch_groups)
                {
                    auto& batch_group = m_list_batch_groups[group];

                    // For a MultiFramed batch we need to know which
                    // Geometry must be removed or copied for processing
                    // in a separate thread

                    std::set_difference(
                                batch_group->list_ents_prev.begin(),
                                batch_group->list_ents_prev.end(),
                                batch_group->list_ents_curr.begin(),
                                batch_group->list_ents_curr.end(),
                                std::back_inserter(batch_group->list_ents_rem));

                    batch_group->rebuild =
                            batch_group->rebuild ||
                            (!batch_group->list_ents_rem.empty());

                    if(batch_group->rebuild)
                    {
                        // Remove old geometry
                        for(auto const ent_id : batch_group->list_ents_rem)
                        {
                            m_list_batch_geometry[ent_id].GetVertexBuffers().clear();
                            m_list_batch_geometry[ent_id].GetIndexBuffer().reset();
                        }

                        // The updated geometry must be copied over *after*
                        // all old geometry has been removed from every batch
                    }
                }

                // Copy updated merged geometry
                for(auto const group : list_mf_batch_groups)
                {
                    auto& batch_group = m_list_batch_groups[group];

                    for(auto const ent_id : batch_group->list_ents_upd)
                    {
                        CopyGeometryBuffers(
                                    list_batch_data[ent_id].GetGeometry(),
                                    m_list_batch_geometry[ent_id]);

                        // Clear updates
                        auto& batch_data = list_batch_data[ent_id];
                        batch_data.SetRebuild(false);
                        batch_data.GetGeometry().ClearGeometryUpdates();
                    }
                }
            }

            void updateBatchTask(std::vector<uint> const &list_mf_batch_groups,
                                 std::vector<BatchData>& list_batch_data,
                                 std::vector<RenderData>& list_render_data)
            {
                if(!m_batch_task->IsFinished())
                {
                    return;
                }

                updateBatchGroupsMF(list_mf_batch_groups,
                                    list_batch_data);

                // Sync back the merged geometry from m_list_batch_geometry
                // to its corresponding RenderData geometry
                for(auto const & batch_desc : m_batch_task->GetListBatchDesc())
                {
                    // Ensure the batch group is still valid
                    auto const batch_id = batch_desc.batch_id;
                    if(batch_id < m_list_batch_groups.GetList().size() &&
                       m_list_batch_groups[batch_id])
                    {
                        auto const merged_ent = batch_desc.merged_ent;

                        // TODO move instead of copy
                        CopyGeometryBuffers(
                                    m_list_batch_geometry[merged_ent],
                                    list_render_data[merged_ent].GetGeometry());

                        list_render_data[merged_ent].GetGeometry().SetAllUpdated();

                        auto& batch_group = m_list_batch_groups[batch_id];
                        batch_group->list_ents_prev = batch_group->list_ents_curr;
                    }
                }

                // Create a desc of the current MultiFrame batch group state
                auto list_batch_desc =
                        make_unique<std::vector<detail::BatchTask::BatchDesc>>();

                for(auto const group : list_mf_batch_groups)
                {
                    auto& batch_group = m_list_batch_groups[group];

                    if(batch_group->rebuild)
                    {
                        list_batch_desc->push_back(
                                    detail::BatchTask::BatchDesc{
                                        group,
                                        batch_group->merged_ent,
                                        batch_group->list_ents_curr,
                                        batch_group->batch->GetBufferLayout()
                                    });
                    }
                }

                // Invoke the PreTaskCallback if its valid
                if(m_pre_task_callback)
                {
                    m_pre_task_callback();
                }

                // Launch a new task
                m_batch_task =
                        make_shared<detail::BatchTask>(
                            std::move(list_batch_desc),
                            m_list_batch_geometry,
                            m_pre_merge_callback_mf);

                m_thread_pool.PushBack(m_batch_task);
            }



            ecs::Scene<SceneKeyType>* const m_scene;
            RenderDataComponentList* const m_cmlist_render_data;

            BatchDataComponentList* m_cmlist_batch_data;
            RecycleIndexList<unique_ptr<BatchGroup>> m_list_batch_groups;
            std::vector<Geometry> m_list_batch_geometry;
            shared_ptr<detail::BatchTask> m_batch_task;

            BatchPreMergeCallback m_pre_merge_callback_sf;
            BatchPreMergeCallback m_pre_merge_callback_mf;
            BatchPreTaskCallback m_pre_task_callback;

            // The thread pool must be destroyed before any resources
            // used by it or the task (like list_batch_geometry) so keep
            // it last (destruction happens in rev order so this will
            // be destroyed first)
            ThreadPool m_thread_pool;
        };

        // ============================================================= //
        // ============================================================= //
    }
}

#endif // KS_BATCH_SYSTEM_HPP
