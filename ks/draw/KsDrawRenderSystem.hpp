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

#ifndef KS_DRAW_RENDER_SYSTEM_HPP
#define KS_DRAW_RENDER_SYSTEM_HPP

#include <ks/gl/KsGLImplementation.hpp>
#include <ks/gl/KsGLCommands.hpp>
#include <ks/gl/KsGLUniform.hpp>

#include <ks/ecs/KsEcs.hpp>

#include <ks/draw/KsDrawSystem.hpp>
#include <ks/draw/KsDrawRenderStats.hpp>
#include <ks/draw/KsDrawDebugTextDrawStage.hpp>
#include <ks/draw/KsDrawDrawCallUpdater.hpp>

namespace ks
{
    namespace draw
    {
        // ============================================================= //
        // ============================================================= //

        class MaxShadersReached : public ks::Exception
        {
        public:
            MaxShadersReached(std::string msg) :
                ks::Exception(ks::Exception::ErrorLevel::ERROR,std::move(msg)) {}

            ~MaxShadersReached() = default;
        };

        // ============================================================= //
        // ============================================================= //

        template<typename SceneKeyType,typename DrawKeyType>
        class RenderSystem : public System
        {
            // NOTE: gcc requires scope to be qualified when
            // the alias name matches another class/template name
            using ListUniformListsById =
                std::vector<std::pair<Id,shared_ptr<ListUniformUPtrs>>>;

            using RenderData = draw::RenderData<DrawKeyType>;

            using RenderDataComponentList =
                ecs::ComponentList<SceneKeyType,RenderData>;

            using DrawCall = draw::DrawCall<DrawKeyType>;

            using DrawStage = draw::DrawStage<DrawKeyType>;

            using DebugTextDrawStage = draw::DebugTextDrawStage<DrawKeyType>;

            // TODO: look into using small_vector (folly has an implementation)
            template<typename DataT,typename IndexT=uint>
            struct RecycleIndexListSync
            {
                using RecycleIndexListSyncCallback =
                    std::function<void(DataT& data)>;

                RecycleIndexList<u8> list_async;
                std::vector<std::pair<IndexT,DataT>> list_add;
                std::vector<IndexT> list_rem;
                std::vector<DataT> list_sync;
                bool sync_required;

                RecycleIndexListSyncCallback on_remove;
                RecycleIndexListSyncCallback on_add;

                void Sync()
                {
                    if(sync_required)
                    {
                        if(on_remove)
                        {
                            for(auto rem_id : list_rem)
                            {
                                on_remove(list_sync[rem_id]);
                                list_sync[rem_id] = DataT(); // TODO std::move?
                                list_async.Remove(rem_id);
                            }
                        }
                        else
                        {
                            for(auto rem_id : list_rem)
                            {
                                list_sync[rem_id] = DataT(); // TODO std::move?
                                list_async.Remove(rem_id);
                            }
                        }

                        list_sync.resize(list_async.GetList().size());

                        if(on_add)
                        {
                            for(auto id_data : list_add)
                            {
                                list_sync[id_data.first] = std::move(id_data.second);
                                on_add(list_sync[id_data.first]);
                            }
                        }
                        else
                        {
                            for(auto id_data : list_add)
                            {
                                list_sync[id_data.first] = std::move(id_data.second);
                            }
                        }

                        list_rem.clear();
                        list_add.clear();
                        sync_required = false;
                    }
                }

                // Clear all data without calling any SyncCallbacks
                // (so no on_remove callbacks are invoked)
                void Clear()
                {
                    list_async.Clear();
                    list_add.clear();
                    list_rem.clear();
                    list_sync.clear();
                    sync_required = false;
                }
            };

        public:
            RenderSystem(ecs::Scene<SceneKeyType>* scene) :
                m_scene(scene),
                m_state_set(make_unique<gl::StateSet>()),
                m_waiting_on_sync(false)
            {
                // Create the RenderData component list
                m_scene->template RegisterComponentList<RenderData>(
                            make_unique<RenderDataComponentList>(*m_scene));

                m_cmlist_render_data =
                        static_cast<RenderDataComponentList*>(
                            m_scene->template GetComponentList<RenderData>());

                // Create the sync callbacks for resource lists
                m_list_shaders.on_add =
                        [](shared_ptr<gl::ShaderProgram>& shader) {
                            if(shader) {
                                shader->GLInit();
                            }
                        };

                m_list_shaders.on_remove =
                        [](shared_ptr<gl::ShaderProgram>& shader) {
                            if(shader) {
                                shader->GLCleanUp();
                            }
                        };

                m_list_texture_sets.on_add =
                        [](shared_ptr<TextureSet>& texture_set) {
                            // Init all textures
                            for(auto& desc : texture_set->list_texture_desc) {
                                desc.first->GLInit();
                            }
                        };

                m_list_texture_sets.on_remove =
                        [](shared_ptr<TextureSet>& texture_set) {
                            // Clean up all textures
                            for(auto& desc : texture_set->list_texture_desc) {
                                desc.first->GLCleanUp();
                            }
                        };

                // Setup DrawStages
                m_graph_draw_stages_async.AddNode(nullptr);
                m_debug_text_draw_stage = make_unique<DebugTextDrawStage>();

                // (the debug text draw stage is not part of
                //  the draw stage node graph)
                m_sync_draw_stages = false;

                Reset();
            }

            ~RenderSystem() = default;

            std::string GetDesc() const
            {
                return "RenderSystem";
            }

            std::atomic<bool>& GetWaitingOnSyncFlag()
            {
                return m_waiting_on_sync;
            }

            RenderDataComponentList* GetRenderDataComponentList() const
            {
                return m_cmlist_render_data;
            }

            // ============================================================= //

            Id RegisterDrawStage(shared_ptr<DrawStage> draw_stage)
            {
                // Add to async list
                Id const index =
                        m_graph_draw_stages_async.AddNode(
                            draw_stage);

                m_sync_draw_stages = true;
                return index;
            }

            void RemoveDrawStage(Id index)
            {
                m_graph_draw_stages_async.RemoveNode(index,false);
                m_sync_draw_stages = true;
            }

            void AddDrawStageDependency(Id from,Id to)
            {
                m_graph_draw_stages_async.AddEdge(from,to);
                m_sync_draw_stages = true;
            }

            void RemoveDrawStageDependency(Id from,Id to)
            {
                m_graph_draw_stages_async.RemoveEdge(from,to);
                m_sync_draw_stages = true;
            }

            // ============================================================= //

            Id RegisterShader(std::string shader_desc,
                              std::string shader_source_vsh,
                              std::string shader_source_fsh)
            {
                auto shader =
                        make_shared<gl::ShaderProgram>(
                            shader_source_vsh,
                            shader_source_fsh);

                shader->SetDesc(std::move(shader_desc));

                auto const new_id = m_list_shaders.list_async.Add(0);

                m_list_shaders.list_add.emplace_back(
                            new_id,std::move(shader));

                m_list_shaders.sync_required = true;

                return new_id;
            }

            void RemoveShader(Id shader_id)
            {
                m_list_shaders.list_rem.push_back(shader_id);
                m_list_shaders.sync_required = true;
            }

            // ============================================================= //

            Id RegisterDepthConfig(StateSetCb depth_config)
            {
                auto new_id = m_list_depth_configs.list_async.Add(0);

                m_list_depth_configs.list_add.emplace_back(
                            new_id,std::move(depth_config));

                m_list_depth_configs.sync_required = true;

                return new_id;
            }

            void RemoveDepthConfig(Id depth_config_id)
            {
                m_list_depth_configs.list_rem.push_back(depth_config_id);
                m_list_depth_configs.sync_required = true;
            }

            // ============================================================= //

            Id RegisterBlendConfig(StateSetCb blend_config)
            {
                auto new_id = m_list_blend_configs.list_async.Add(0);

                m_list_blend_configs.list_add.emplace_back(
                            new_id,std::move(blend_config));

                m_list_blend_configs.sync_required = true;

                return new_id;
            }

            void RemoveBlendConfig(Id blend_config_id)
            {
                m_list_blend_configs.list_rem.push_back(blend_config_id);
                m_list_blend_configs.sync_required = true;
            }

            // ============================================================= //

            Id RegisterStencilConfig(StateSetCb stencil_config)
            {
                auto new_id = m_list_stencil_configs.list_async.Add(0);

                m_list_stencil_configs.list_add.emplace_back(
                            new_id,std::move(stencil_config));

                m_list_stencil_configs.sync_required = true;

                return new_id;
            }

            void RemoveStencilConfig(Id stencil_config_id)
            {
                m_list_stencil_configs.list_rem.push_back(stencil_config_id);
                m_list_stencil_configs.sync_required = true;
            }

            // ============================================================= //

            Id RegisterTextureSet(shared_ptr<TextureSet> texture_set)
            {
                auto new_id = m_list_texture_sets.list_async.Add(0);

                m_list_texture_sets.list_add.emplace_back(
                            new_id,std::move(texture_set));

                m_list_texture_sets.sync_required = true;

                return new_id;
            }

            void RemoveTextureSet(Id texture_set_id)
            {
                m_list_texture_sets.list_rem.push_back(texture_set_id);
                m_list_texture_sets.sync_required = true;
            }

            // ============================================================= //

            Id RegisterUniformSet(shared_ptr<UniformSet> uniform_set)
            {
                auto new_id = m_list_uniform_sets.list_async.Add(0);

                m_list_uniform_sets.list_add.emplace_back(
                            new_id,std::move(uniform_set));

                m_list_uniform_sets.sync_required = true;

                return new_id;
            }

            void RemoveUniformSet(Id uniform_set_id)
            {
                m_list_uniform_sets.list_rem.push_back(uniform_set_id);
                m_list_uniform_sets.sync_required = true;
            }

            // ============================================================= //

            Id RegisterSyncCallback(std::function<void()> cb)
            {
                return m_list_sync_cbs.Add(std::move(cb));
            }

            void RemoveSyncCallback(Id cb_id)
            {
                m_list_sync_cbs.Remove(cb_id);
            }

            // ============================================================= //

            void Reset()
            {
                //
                m_init = false;

                // Clear resource lists
                m_list_shaders.Clear();
                m_list_depth_configs.Clear();
                m_list_blend_configs.Clear();
                m_list_stencil_configs.Clear();
                m_list_texture_sets.Clear();
                m_list_uniform_sets.Clear();

                // Reserve index 0 for resource lists
                m_list_shaders.list_async.Add(0);
                m_list_shaders.list_add.emplace_back(0,nullptr);

                StateSetCb state_set_cb_no_op = [](gl::StateSet*){};

                m_list_depth_configs.list_async.Add(0);
                m_list_depth_configs.list_add.emplace_back(
                            0,state_set_cb_no_op);

                m_list_blend_configs.list_async.Add(0);
                m_list_blend_configs.list_add.emplace_back(
                            0,state_set_cb_no_op);

                m_list_stencil_configs.list_async.Add(0);
                m_list_stencil_configs.list_add.emplace_back(
                            0,state_set_cb_no_op);

                m_list_texture_sets.list_async.Add(0);
                m_list_texture_sets.list_add.emplace_back(
                            0,make_shared<TextureSet>());

                m_list_uniform_sets.list_async.Add(0);
                m_list_uniform_sets.list_add.emplace_back(
                            0,make_shared<UniformSet>());

                // Reset DrawStage nodes
                auto& list_draw_stages =
                        m_graph_draw_stages_async.
                            GetSparseNodeList();

                for(uint i=0; i < list_draw_stages.size(); i++)
                {
                    if(list_draw_stages[i].valid && list_draw_stages[i].value)
                    {
                        auto& draw_stage = list_draw_stages[i].value;
                        draw_stage->Reset();
                    }
                }

                // Reset the debug text draw stage
                m_debug_text_draw_stage->Reset();

                //
                m_draw_call_updater.Reset();
                m_list_buffers.clear();
                m_list_draw_calls.clear();
                m_list_opq_draw_calls_by_stage.clear();
                m_list_xpr_draw_calls_by_stage.clear();
            }

            // ============================================================= //

            void Update(time_point const &,
                        time_point const &)
            {
                m_stats.ClearUpdateStats();
                auto timing_start = std::chrono::high_resolution_clock::now();

                std::vector<PairIds> list_ent_rd;

                auto &list_entities = m_scene->GetEntityList();

                // Get the current list of Drawable entities
                auto const drawable_mask =
                        ecs::Scene<SceneKeyType>::template
                            GetComponentMask<RenderData>();

                auto &list_render_data =
                        m_cmlist_render_data->GetSparseList();


                for(uint ent_id=0; ent_id < list_entities.size(); ent_id++)
                {
                    if((list_entities[ent_id].mask & drawable_mask) == drawable_mask)
                    {
                        RenderData& render_data = list_render_data[ent_id];
                        list_ent_rd.emplace_back(ent_id,render_data.GetUniqueId());
                    }
                }

                m_draw_call_updater.Update(list_ent_rd,list_render_data);

                auto timing_end = std::chrono::high_resolution_clock::now();
                m_stats.update_ms = std::chrono::duration_cast<
                        std::chrono::microseconds>(
                            timing_end-timing_start).count()/1000.0;
            }

            // ============================================================= //

            void Sync()
            {
                if(!m_init) {
                    initialize();
                }

                auto timing_start = std::chrono::high_resolution_clock::now();

                syncDrawStages();
                syncShaders();
                syncBuffers(); // must be called after GeometryUpdateTask::Update()
                syncRasterConfigs();
                syncTextures();
                syncUniforms();

                m_draw_call_updater.Sync(m_list_draw_calls);

                auto &list_render_data =
                        m_cmlist_render_data->GetSparseList();

                // Copy over Uniforms to each added DrawCall
                for(auto const ent_id : m_draw_call_updater.GetAddedEntities())
                {
                    auto& render_data = list_render_data[ent_id];
                    auto& draw_call = m_list_draw_calls[ent_id];

                    draw_call.list_uniforms = render_data.GetUniformList();
                }

                // Create the list of current transparent and opaque
                // DrawCalls sorted by stage

                m_list_opq_draw_calls_by_stage.resize(
                            m_list_draw_stages_sync.size());

                m_list_xpr_draw_calls_by_stage.resize(
                            m_list_draw_stages_sync.size());

                for(uint i=0; i < m_list_draw_stages_sync.size(); i++)
                {
                    m_list_opq_draw_calls_by_stage[i].clear();
                    m_list_xpr_draw_calls_by_stage[i].clear();
                }

                for(uint ent_id=0; ent_id < m_list_draw_calls.size(); ent_id++)
                {
                    auto& draw_call = m_list_draw_calls[ent_id];

                    if(draw_call.valid)
                    {
                        auto& render_data = list_render_data[ent_id];
                        draw_call.key = render_data.GetKey();

                        auto& list_draw_calls_by_stage =
                                (render_data.GetTransparency() == Transparency::Opaque) ?
                                    m_list_opq_draw_calls_by_stage :
                                    m_list_xpr_draw_calls_by_stage;

                        for(auto stage : render_data.GetDrawStages()) {
                            list_draw_calls_by_stage[stage].push_back(ent_id);
                        }

                        // We can sync individual uniforms here as well
                        // since all draw_calls are being iterated over
                        if(draw_call.list_uniforms) {
                            auto& list_uniforms = *(draw_call.list_uniforms);
                            for(auto& uniform : list_uniforms) {
                                uniform->Sync();
                            }
                        }
                    }
                }

                // Sync callbacks last
                syncCallbacks();

                auto timing_end = std::chrono::high_resolution_clock::now();
                m_stats.sync_ms = std::chrono::duration_cast<
                        std::chrono::microseconds>(
                            timing_end-timing_start).count()/1000.0;

                m_stats.GenUpdateText();
            }


            // ============================================================= //

            void Render()
            {
                if(!m_init) {
                    return;
                }

                m_stats.ClearRenderStats();
                auto timing_start = std::chrono::high_resolution_clock::now();

                DrawParams<DrawKeyType> stage_params{
                            m_state_set.get(),
                            m_list_shaders.list_sync,
                            m_list_depth_configs.list_sync,
                            m_list_blend_configs.list_sync,
                            m_list_stencil_configs.list_sync,
                            m_list_texture_sets.list_sync,
                            m_list_uniform_sets.list_sync,
                            m_list_draw_calls,
                            nullptr,
                            nullptr
                };

                for(auto stage : m_list_draw_stage_idxs_sync)
                {
                    stage_params.list_opq_draw_calls =
                            &m_list_opq_draw_calls_by_stage[stage];

                    stage_params.list_xpr_draw_calls =
                            &m_list_xpr_draw_calls_by_stage[stage];

                    auto& draw_stage = m_list_draw_stages_sync[stage];
                    draw_stage->Render(stage_params);

                    auto& stage_stats = draw_stage->GetStats();
                    m_stats.shader_switches += stage_stats.shader_switches;
                    m_stats.texture_switches += stage_stats.texture_switches;
                    m_stats.raster_ops += stage_stats.raster_ops;
                    m_stats.draw_calls += stage_stats.draw_calls;
                }

                // Update stats
                auto timing_end = std::chrono::high_resolution_clock::now();
                m_stats.render_ms = std::chrono::duration_cast<
                        std::chrono::microseconds>(
                            timing_end-timing_start).count()/1000.0;

                // Render debug text
                m_stats.GenRenderText();

                glm::vec4 const text_color{1,1,1,1};
                std::string render_debug_text =
                        m_stats.text_update_times+
                        m_stats.text_render_times+
                        m_stats.text_render_data+
                        m_stats.text_update_data;

                m_debug_text_draw_stage->SetText(
                            glm::vec2{-1,-0.5},
                            glm::vec2{0,-1},
                            text_color,
                            render_debug_text);

                m_debug_text_draw_stage->Render(stage_params);
            }

        private:
            void syncDrawStages()
            {
                // DrawStages
                if(m_sync_draw_stages)
                {
                    // Sync the list of DrawStages
                    auto const &list_graph_nodes =
                            m_graph_draw_stages_async.GetSparseNodeList();

                    m_list_draw_stages_sync.resize(list_graph_nodes.size());

                    for(uint i=0; i < m_list_draw_stages_sync.size(); i++)
                    {
                        if(list_graph_nodes[i].valid) {
                            m_list_draw_stages_sync[i] = list_graph_nodes[i].value;
                        }
                        else {
                            m_list_draw_stages_sync[i] = nullptr;
                        }
                    }

                    // Sync the ordered DrawStage indices
                    m_list_draw_stage_idxs_sync =
                            m_graph_draw_stages_async.
                                GetTopologicallySorted();

                    // Remove the invalid stage (0)
                    for(auto it = m_list_draw_stage_idxs_sync.begin();
                        it != m_list_draw_stage_idxs_sync.end();++it)
                    {
                        if(*it == 0) {
                            m_list_draw_stage_idxs_sync.erase(it);
                            break;
                        }
                    }

                    m_sync_draw_stages = false;
                }
            }

            void syncShaders()
            {
                m_list_shaders.Sync();
            }

            void syncRasterConfigs()
            {
                m_list_depth_configs.Sync();
                m_list_blend_configs.Sync();
                m_list_stencil_configs.Sync();
            }

            void syncTextures()
            {
                m_list_texture_sets.Sync();

                for(auto& texture_set : m_list_texture_sets.list_sync)
                {
                    for(auto& desc : texture_set->list_texture_desc)
                    {
                        auto& texture = desc.first;
                        auto tex_unit = desc.second;

                        if((texture->GetUpdateCount() > 0) ||
                            texture->GetParamsUpdated())
                        {
                            texture->GLBind(m_state_set.get(),tex_unit);
                            texture->GLSync();
                            // TODO texture->GLUnbind() ?
                        }
                    }
                }
            }

            void syncUniforms()
            {
                m_list_uniform_sets.Sync();

                for(auto& uniform_set : m_list_uniform_sets.list_sync)
                {
                    for(auto& uniform : uniform_set->list_uniforms)
                    {
                        uniform->Sync();
                    }
                }
            }

            void syncBuffers()
            {
                // Init new buffers
                for(gl::Buffer* buff : m_draw_call_updater.GetBuffersToInit()) {
                    bool ok = buff->GLInit();
                    assert(ok);
                }

                // Sync updated buffers
                for(gl::Buffer* buff : m_draw_call_updater.GetBuffersToSync()) {
                    buff->GLBind();
                    buff->GLSync();
                }

                // Save newly created buffers
                m_list_buffers.insert(
                            m_list_buffers.end(),
                            std::make_move_iterator(
                                m_draw_call_updater.GetNewBuffers().begin()),
                            std::make_move_iterator(
                                m_draw_call_updater.GetNewBuffers().end()));

                // Update stats
                m_stats.buffer_count = m_list_buffers.size();

                for(auto &buff : m_list_buffers) {
                    m_stats.buffer_mem_bytes += buff->GetSizeBytes();
                }
            }

            void syncCallbacks()
            {
                auto& list_callbacks = m_list_sync_cbs.GetList();
                for(auto& callback : list_callbacks) {
                    if(callback) {
                        callback();
                    }
                }
            }


            void initialize()
            {
                assert(!m_init);
                ks::gl::Implementation::GLCapture();
                m_state_set->CaptureState();

                if(m_state_set->GetPixelUnpackAlignment() != 1) {
                    m_state_set->SetPixelUnpackAlignment(1);
                }

                m_init = true;
            }

            // ============================================================= //

            u64 const k_one{1};
            u64 const k_max_shaders{(k_one << DrawKeyType::k_bits_shader)};

            ecs::Scene<SceneKeyType>* const m_scene;
            unique_ptr<gl::StateSet> const m_state_set;

            RenderDataComponentList* m_cmlist_render_data;

            bool m_init;
            std::atomic<bool> m_waiting_on_sync;

            DrawCallUpdater<DrawKeyType> m_draw_call_updater;

            // == DrawStages == //
            bool m_sync_draw_stages;
            std::vector<u8> m_list_draw_stage_idxs_sync; // topo sorted
            std::vector<shared_ptr<DrawStage>> m_list_draw_stages_sync; // sparse
            Graph<shared_ptr<DrawStage>,u8> m_graph_draw_stages_async;

            unique_ptr<DebugTextDrawStage> m_debug_text_draw_stage;

            // == Shaders == //
            RecycleIndexListSync<shared_ptr<gl::ShaderProgram>> m_list_shaders;

            // == Raster Op Configs == //
            RecycleIndexListSync<StateSetCb> m_list_depth_configs;
            RecycleIndexListSync<StateSetCb> m_list_blend_configs;
            RecycleIndexListSync<StateSetCb> m_list_stencil_configs;

            // == TextureSets == //
            RecycleIndexListSync<shared_ptr<TextureSet>> m_list_texture_sets;

            // == UniformSets == //
            RecycleIndexListSync<shared_ptr<UniformSet>> m_list_uniform_sets;

            // == Sync Callbacks == //
            RecycleIndexList<std::function<void()>> m_list_sync_cbs;

            // == DrawCalls == //
            std::vector<DrawCall> m_list_draw_calls;
            std::vector<std::vector<Id>> m_list_opq_draw_calls_by_stage;
            std::vector<std::vector<Id>> m_list_xpr_draw_calls_by_stage;

            // == Uniform Lists == //
            // * This list is used to copy the list of uniforms
            //   shared ptr from RenderData to its corresponding DrawCall
            ListUniformListsById m_list_upd_list_uniforms;

            // == Buffers == //
            std::vector<shared_ptr<gl::Buffer>> m_list_buffers;

            // == debug == //
            std::string const m_log_prefix{"draw::RenderSystem: "};
            RenderStats m_stats;
        };
    }
}

#endif // KS_DRAW_RENDER_SYSTEM_HPP
