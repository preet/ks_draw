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

            // * Splits a list of single geometries into N merged
            //   geometries
            std::vector<std::vector<Geometry*>>
            CreateSplitSingleGeometryLists(
                    BufferLayout const * buffer_layout,
                    std::vector<Geometry*> const &list_single_gm_all)
            {
                // For each batch group, we have N RenderDatas to represent
                // the merged geometries. A new RenderData is created when
                // either the VertexBuffer or IndexBuffer block size is
                // exceeded. Single geometries are assigned to the RenderData
                // indices they will be merged in accordingly.

                std::vector<std::vector<Geometry*>> list_list_single_gm;

                if(list_single_gm_all.empty())
                {
                    return list_list_single_gm;
                }

                uint render_data_idx=0;
                uint vx_block_used=0;

                uint const vx_block_size =
                        buffer_layout->GetVertexBufferAllocator(0)->
                        GetBlockSize();

                list_list_single_gm.resize(1);

                if(buffer_layout->GetIsIndexed())
                {
                    uint const ix_block_size =
                            buffer_layout->GetIndexBufferAllocator()->
                            GetBlockSize();

                    uint ix_block_used=0;

                    for(auto single_gm : list_single_gm_all)
                    {
                        uint const single_gm_vxbuff_size =
                                single_gm->GetVertexBuffer(0)->size();

                        uint const single_gm_ixbuff_size =
                                single_gm->GetIndexBuffer()->size();

                        vx_block_used += single_gm_vxbuff_size;
                        ix_block_used += single_gm_ixbuff_size;

                        if((vx_block_used > vx_block_size) ||
                           (ix_block_used > ix_block_size))
                        {
                            if((vx_block_size < single_gm_vxbuff_size) ||
                               (ix_block_size < single_gm_ixbuff_size))
                            {
                                throw ks::Exception(
                                            ks::Exception::ErrorLevel::ERROR,
                                            "BatchSystem: Geometry size exceeds "
                                            "BufferAllocator block size");
                            }

                            // Create a new single geometry list
                            list_list_single_gm.emplace_back();

                            render_data_idx++;
                            vx_block_used = single_gm_vxbuff_size;
                            ix_block_used = single_gm_ixbuff_size;
                        }

                        list_list_single_gm[render_data_idx].push_back(single_gm);
                    }
                }
                else
                {
                    for(auto single_gm : list_single_gm_all)
                    {
                        uint const single_gm_vxbuff_size =
                                single_gm->GetVertexBuffer(0)->size();

                        vx_block_used += single_gm_vxbuff_size;

                        if(vx_block_used > vx_block_size)
                        {
                            if(vx_block_size < single_gm_vxbuff_size)
                            {
                                throw ks::Exception(
                                            ks::Exception::ErrorLevel::ERROR,
                                            "BatchSystem: Geometry size exceeds "
                                            "BufferAllocator block size");
                            }

                            // Create a new single geometry list
                            list_list_single_gm.emplace_back();

                            render_data_idx++;
                            vx_block_used = single_gm_vxbuff_size;
                        }

                        list_list_single_gm[render_data_idx].push_back(single_gm);
                    }
                }

                return list_list_single_gm;
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

            std::vector<Geometry>&
            BatchTask::GetListMergedGeometry(uint index)
            {
                return m_list_list_merged_gms[index];
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

                m_list_list_merged_gms.clear();
                m_list_list_merged_gms.resize(list_batch_desc.size());

                for(uint i=0; i < list_batch_desc.size(); i++)
                {
                    auto& batch_desc = list_batch_desc[i];

                    // Create the single geometry list
                    std::vector<Geometry*> list_single_gm_all;
                    list_single_gm_all.reserve(
                                batch_desc.list_all_single_gm_ent_ids.size());

                    // Allow a callback to modify the list of entities
                    // that will be merged together
                    if(m_pre_merge_callback)
                    {
                        auto list_ents_curr =
                                m_pre_merge_callback(
                                    batch_desc.list_all_single_gm_ent_ids);

                        for(auto ent_id : list_ents_curr)
                        {
                            list_single_gm_all.push_back(
                                        &(m_list_batch_geometry[ent_id]));
                        }
                    }
                    else
                    {
                        for(auto ent_id : batch_desc.list_all_single_gm_ent_ids)
                        {
                            list_single_gm_all.push_back(
                                        &(m_list_batch_geometry[ent_id]));
                        }
                    }

                    // Split into lists according to buffer block sizes
                    auto list_list_single_gm =
                            CreateSplitSingleGeometryLists(
                                batch_desc.buffer_layout,
                                list_single_gm_all);

                    auto& list_merged_gms = m_list_list_merged_gms[i];
                    list_merged_gms.resize(list_list_single_gm.size());

                    for(uint j=0; j < list_list_single_gm.size(); j++)
                    {
                        auto const vx_buff_count =
                                batch_desc.buffer_layout->
                                GetVertexBufferCount();

                        auto& merged_gm = list_merged_gms[j];
                        for(uint k=0; k < vx_buff_count; k++)
                        {
                            merged_gm.GetVertexBuffers().push_back(
                                        make_unique<std::vector<u8>>());
                        }
                        if(batch_desc.buffer_layout->GetIsIndexed())
                        {
                            merged_gm.GetIndexBuffer() =
                                    make_unique<std::vector<u8>>();
                        }

                        // Merge single geometries
                        CreateMergedGeometry(
                                    batch_desc.buffer_layout,
                                    list_list_single_gm[j],
                                    &merged_gm);
                    }
                }

                this->onEnded();
                this->onFinished();
            }

            // ============================================================= //
            // ============================================================= //
        }
    }
}
