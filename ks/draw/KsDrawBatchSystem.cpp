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

#include <ks/draw/KsDrawBatchSystem.hpp>

namespace ks
{
    namespace draw
    {
        namespace detail
        {
            // ============================================================= //
            // ============================================================= //

            void CreateMergedGeometry(BufferLayout const * buffer_layout,
                                      std::vector<Geometry*> &list_single_gm,
                                      Geometry* merged_gm)
            {
                std::vector<u16> list_single_gm_vx_counts(
                            list_single_gm.size());

                auto const vx_buff_count =
                        buffer_layout->GetVertexBufferCount();

                // VertexBuffers
                for(uint index=0; index < vx_buff_count; index++)
                {
                    auto const vx_size_bytes =
                            buffer_layout->GetVertexSizeBytes(index);

                    auto& merged_vx_data = merged_gm->GetVertexBuffer(index);
                    merged_vx_data->clear();

                    for(uint i=0; i < list_single_gm.size(); i++)
                    {
                        auto single_gm = list_single_gm[i];
                        auto const & single_vx_data = single_gm->GetVertexBuffer(index);

                        list_single_gm_vx_counts[i] =
                                single_vx_data->size()/
                                vx_size_bytes;

                        merged_vx_data->insert(
                                    merged_vx_data->end(),
                                    single_vx_data->begin(),
                                    single_vx_data->end());
                    }
                }

                // IndexBuffer
                if(buffer_layout->GetIsIndexed())
                {
                    auto& merged_ix_data = merged_gm->GetIndexBuffer();
                    merged_ix_data->clear();

                    // Merge single geometry ix data
                    u16 vx_count=0;
                    for(uint i=0; i < list_single_gm.size(); i++)
                    {
                        auto single_gm = list_single_gm[i];
                        auto const & single_ix_data = single_gm->GetIndexBuffer();

                        auto ix_data_beg =
                                merged_ix_data->insert(
                                    merged_ix_data->end(),
                                    single_ix_data->begin(),
                                    single_ix_data->end());

                        u16* ix_ptr = reinterpret_cast<u16*>(&(*ix_data_beg));

                        // Increment indices by the vertex count
                        IncrementListIx(ix_ptr,single_ix_data->size()/2,vx_count);
                        vx_count += list_single_gm_vx_counts[i];
                    }
                }
            }

            // ============================================================= //
            // ============================================================= //

            BatchTask::BatchTask(unique_ptr<std::vector<BatchDesc>> list_batch_desc,
                                 std::vector<Geometry>& list_batch_geometry,
                                 BatchPreMergeCallback pre_merge_callback) :
                m_list_batch_desc(std::move(list_batch_desc)),
                m_list_batch_geometry(list_batch_geometry),
                m_pre_merge_callback(std::move(pre_merge_callback))
            {

            }

            BatchTask::~BatchTask()
            {

            }

            std::vector<BatchTask::BatchDesc> const &
            BatchTask::GetListBatchDesc() const
            {
                return *m_list_batch_desc;
            }

            void BatchTask::Cancel()
            {
                // TODO Cancel not supported, maybe
                // throw an exception here?
            }

            void BatchTask::Process()
            {
                this->process();
            }

            void BatchTask::process()
            {
                if(this->IsStarted()) {
                    return;
                }
                this->onStarted();

                auto& list_batch_desc = *m_list_batch_desc;
                for(auto& batch_desc : list_batch_desc)
                {
                    auto const merged_ent = batch_desc.merged_ent;
                    auto merged_gm = &(m_list_batch_geometry[merged_ent]);

                    // The merged geometry buffers need to be created
                    // the first time they are used
                    if(merged_gm->GetVertexBuffers().empty())
                    {
                        auto const vx_buff_count =
                                batch_desc.buffer_layout->
                                    GetVertexBufferCount();

                        for(uint i=0; i < vx_buff_count; i++)
                        {
                            merged_gm->GetVertexBuffers().push_back(
                                        make_unique<std::vector<u8>>());
                        }

                        if(batch_desc.buffer_layout->GetIsIndexed())
                        {
                            merged_gm->GetIndexBuffer() =
                                    make_unique<std::vector<u8>>();
                        }
                    }

                    // Create the single geometry list
                    std::vector<Geometry*> list_geometry;
                    list_geometry.reserve(batch_desc.list_ents.size());

                    // Allow a callback to modify the list of entities
                    // that will be merged together
                    if(m_pre_merge_callback)
                    {
                        auto list_ents_curr =
                                m_pre_merge_callback(batch_desc.list_ents);

                        for(auto ent_id : list_ents_curr)
                        {
                            list_geometry.push_back(
                                        &(m_list_batch_geometry[ent_id]));
                        }
                    }
                    else
                    {
                        for(auto ent_id : batch_desc.list_ents)
                        {
                            list_geometry.push_back(
                                        &(m_list_batch_geometry[ent_id]));
                        }
                    }

                    // Merge
                    CreateMergedGeometry(
                                batch_desc.buffer_layout,
                                list_geometry,
                                merged_gm);
                }

                this->onEnded();
                this->onFinished();
            }

            // ============================================================= //
            // ============================================================= //
        }
    }
}
