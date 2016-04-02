/*
   Copyright (C) 2015-2016 Preet Desai (preet.desai@gmail.com)

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

#include <ks/gui/KsGuiWindow.hpp>
#include <ks/gui/KsGuiApplication.hpp>
#include <ks/platform/KsPlatformMain.hpp>

#include <ks/draw/test/KsTestDrawBasicScene.hpp>
#include <ks/draw/KsDrawSystem.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace test
{
    // ============================================================= //

    // Shader
    std::string const vertex_shader =
                "#ifdef GL_ES\n"
                "    //\n"
                "#else\n"
                "    #define lowp\n"
                "    #define mediump\n"
                "    #define highp\n"
                "#endif\n"
                "\n"
                "attribute vec4 a_v4_position;\n"
                "attribute vec4 a_v4_color;\n"
                "\n"
                "uniform mat4 u_m4_model;\n"
                "\n"
                "varying lowp vec4 v_v4_color;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_Position = u_m4_model * a_v4_position;\n"
                "    v_v4_color = a_v4_color;\n"
                "}\n";


    std::string const frag_shader =
                "#ifdef GL_ES\n"
                "    precision mediump float;\n"
                "#else\n"
                "    #define lowp\n"
                "    #define mediump\n"
                "    #define highp\n"
                "#endif\n"
                "\n"
                "varying lowp vec4 v_v4_color;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = v_v4_color;\n"
                "}\n";

    // ============================================================= //

    // Vertex definition and layout
    struct Vertex {
        glm::vec4 a_v4_position;
        glm::u8vec4 a_v4_color;
    };

    gl::VertexLayout const vx_layout {
        {
            "a_v4_position",
            gl::VertexBuffer::Attribute::Type::Float,
            4,
            false
        },
        {
            "a_v4_color",
            gl::VertexBuffer::Attribute::Type::UByte,
            4,
            true
        }
    };

    shared_ptr<draw::VertexBufferAllocator> vx_buff_allocator =
            make_shared<draw::VertexBufferAllocator>(24000*4);


    // Buffer layout
    draw::BufferLayout const buffer_layout(
            gl::Buffer::Usage::Static,
            { vx_layout },
            { vx_buff_allocator });

    // ============================================================= //

    struct BodyData
    {
        glm::vec2 position;
        glm::vec2 velocity;
    };

    // ============================================================= //

    class BodySystem : public ks::draw::System
    {
    public:
        BodySystem(Scene* scene) :
            m_scene(scene)
        {

        }

        ~BodySystem()
        {

        }

        std::string GetDesc() const
        {
            return "BodySystem";
        }

        void Update(TimePoint const &,
                    TimePoint const &)
        {
            auto& list_body_data =
                    static_cast<ecs::ComponentList<SceneKey,BodyData>*>(
                        m_scene->GetComponentList<BodyData>())->GetSparseList();

            auto& list_render_data =
                    m_scene->GetRenderSystem()->
                    GetRenderDataComponentList()->GetSparseList();

            auto& list_entities = m_scene->GetEntityList();

            auto const mask = m_scene->GetComponentMask<BodyData,RenderData>();

            for(uint ent_id=0; ent_id < list_entities.size(); ent_id++)
            {
                if((list_entities[ent_id].mask & mask) == mask)
                {
                    auto& body_data = list_body_data[ent_id];
                    auto& render_data = list_render_data[ent_id];

                    body_data.position += body_data.velocity;

                    auto u_m4_model_base = render_data.GetUniformList()->at(0).get();
                    auto u_m4_model = static_cast<gl::Uniform<glm::mat4>*>(u_m4_model_base);
                    u_m4_model->Update(
                                glm::translate(
                                    glm::vec3(
                                        body_data.position.x,
                                        body_data.position.y,
                                        0.0)));
                }
            }
        }

    private:
        Scene* const m_scene;
    };

    // ============================================================= //

    struct CollisionData
    {
        glm::vec4 bbox;
        bool movable;
        bool xsec_overlap;
    };

    // ============================================================= //

    class CollisionSystem : public ks::draw::System
    {
    public:
        CollisionSystem(Scene* scene) :
            m_scene(scene)
        {

        }

        ~CollisionSystem()
        {

        }

        std::string GetDesc() const
        {
            return "CollisionSystem";
        }

        void Update(TimePoint const &,
                    TimePoint const &)
        {
            auto& list_collision_data =
                    static_cast<ecs::ComponentList<SceneKey,CollisionData>*>(
                        m_scene->GetComponentList<CollisionData>())->GetSparseList();

            auto& list_body_data =
                    static_cast<ecs::ComponentList<SceneKey,BodyData>*>(
                        m_scene->GetComponentList<BodyData>())->GetSparseList();

            auto& list_entities = m_scene->GetEntityList();

            auto const collideable_mask =
                    m_scene->GetComponentMask<CollisionData,BodyData>();

            // Get the list of collideable entities
            std::vector<Id> list_coll_movable_ents;
            std::vector<Id> list_coll_immovable_ents;
            for(uint ent_id=0; ent_id < list_entities.size(); ent_id++)
            {
                if((list_entities[ent_id].mask & collideable_mask)==collideable_mask)
                {
                    auto& collision_data = list_collision_data[ent_id];
                    if(collision_data.movable)
                    {
                        list_coll_movable_ents.push_back(ent_id);
                    }
                    else
                    {
                        list_coll_immovable_ents.push_back(ent_id);
                    }
                }
            }

            // Resolve collisions
            // We check all movable collideable types against
            // immovable collideable types

            for(auto mv_ent_id : list_coll_movable_ents)
            {
                auto& collision_data_a =
                        list_collision_data[mv_ent_id];

                auto& body_data_a = list_body_data[mv_ent_id];

                for(auto imv_ent_id : list_coll_immovable_ents)
                {
                    auto const &collision_data_b =
                            list_collision_data[imv_ent_id];

                    auto& body_data_b = list_body_data[imv_ent_id];


                    glm::vec2 a_bl{
                        collision_data_a.bbox.x + body_data_a.position.x,
                        collision_data_a.bbox.y + body_data_a.position.y
                    };

                    glm::vec2 a_tr{
                        collision_data_a.bbox.z + body_data_a.position.x,
                        collision_data_a.bbox.w + body_data_a.position.y
                    };

                    glm::vec2 b_bl{
                        collision_data_b.bbox.x + body_data_b.position.x,
                        collision_data_b.bbox.y + body_data_b.position.y
                    };

                    glm::vec2 b_tr{
                        collision_data_b.bbox.z + body_data_b.position.x,
                        collision_data_b.bbox.w + body_data_b.position.y
                    };

                    bool xsec = CalcRectOverlap(a_bl,a_tr,b_bl,b_tr);

                    if(xsec)
                    {
                        glm::vec2 b_tl{
                            b_bl.x,b_tr.y
                        };

                        glm::vec2 b_br{
                            b_tr.x,b_bl.y
                        };

                        // Left or right
                        if(CalcRectVEdgeOverlap(a_bl,a_tr,b_bl,b_tl) ||
                           CalcRectVEdgeOverlap(a_bl,a_tr,b_br,b_tr))
                        {
                            // Reflect velocity along x
                            body_data_a.velocity.x *= -1.0;

                            if(body_data_a.velocity.x > 0)
                            {
                                float delta_x = b_tr.x-a_bl.x;
                                body_data_a.position.x += (delta_x*1.1);
                            }
                            else
                            {
                                float delta_x = a_tr.x-b_bl.x;
                                body_data_a.position.x -= (delta_x*1.1);
                            }
                        }
                        else
                        {
                            // Reflect velocity along y
                            body_data_a.velocity.y *= -1.0;

                            if(body_data_a.velocity.y > 0)
                            {
                                float delta_y = b_tr.y-a_bl.y;
                                body_data_a.position.y += (delta_y*1.1);
                            }
                            else
                            {
                                float delta_y = a_tr.y-b_bl.y;
                                body_data_a.position.y -= (delta_y*1.1);
                            }
                        }
                    }
                }
            }
        }

        static bool CalcRectOverlap(glm::vec2 const &r1_bl,
                                    glm::vec2 const &r1_tr,
                                    glm::vec2 const &r2_bl,
                                    glm::vec2 const &r2_tr)
        {
//            LOG.Trace() << "r1_bl " << glm::to_string(r1_bl);
//            LOG.Trace() << "r1_tr " << glm::to_string(r1_tr);
//            LOG.Trace() << "r2_bl " << glm::to_string(r2_bl);
//            LOG.Trace() << "r1_tr " << glm::to_string(r2_tr);

            // check if rectangles intersect
            if((r1_tr.x < r2_bl.x) || (r1_bl.x > r2_tr.x) ||
               (r1_tr.y < r2_bl.y) || (r1_bl.y > r2_tr.y))
            {
                return false;
            }

            return true;
        }

        static bool CalcRectHEdgeOverlap(glm::vec2 const &r_bl,
                                         glm::vec2 const &r_tr,
                                         glm::vec2 const &e_l,
                                         glm::vec2 const &e_r)
        {
            // Horizontal edge
            bool not_xsec =
                    (r_bl.x > e_r.x) ||
                    (r_tr.x < e_l.x) ||
                    (r_bl.y > e_l.y) ||
                    (r_tr.y < e_l.y);

            return (!not_xsec);
        }

        static bool CalcRectVEdgeOverlap(glm::vec2 const &r_bl,
                                         glm::vec2 const &r_tr,
                                         glm::vec2 const &e_b,
                                         glm::vec2 const &e_t)
        {
            // Horizontal edge
            bool not_xsec =
                    (r_bl.y > e_t.y) ||
                    (r_tr.y < e_b.y) ||
                    (r_bl.x > e_b.x) ||
                    (r_tr.x < e_b.x);

            return (!not_xsec);
        }

    private:
        Scene* const m_scene;
    };

    // ============================================================= //

    class Block
    {
    public:
        Block(Scene* scene,
              Id draw_stage_id,
              Id shader_id,
              float height,
              float width,
              glm::u8vec4 color,
              glm::vec2 position)
        {
            // Create Entity
            m_entity_id = scene->CreateEntity();

            // Add components
            createRenderData(scene,draw_stage_id,shader_id,height,width,color);
            createBodyData(scene,position);
            createCollisionData(scene,height,width);
        }

        ~Block()
        {

        }

    private:
        void createRenderData(Scene* scene,
                              Id draw_stage_id,
                              Id shader_id,
                              float height,
                              float width,
                              glm::u8vec4 color)
        {
            // Vertex List
            glm::vec4 v0(-0.5*width,-0.5*height,0,1); // bl
            glm::vec4 v1(0.5*width,0.5*height,0,1); // tr
            glm::vec4 v2(-0.5*width,0.5*height,0,1); // tl
            glm::vec4 v3(0.5*width,-0.5*height,0,1); // br

            unique_ptr<std::vector<u8>> list_vx =
                    make_unique<std::vector<u8>>();

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{v0,color});

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{v1,color});

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{v2,color});

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{v0,color});

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{v3,color});

            gl::Buffer::PushElement<Vertex>(
                        *list_vx,
                        Vertex{v1,color});


            draw::DefaultDrawKey draw_key;
            draw_key.SetShader(shader_id);
            draw_key.SetPrimitive(gl::Primitive::Triangles);

            std::vector<u8> list_draw_stages = {u8(draw_stage_id)};

            auto list_uniforms =
                    make_shared<draw::ListUniformUPtrs>();

            list_uniforms->push_back(
                        make_unique<gl::Uniform<glm::mat4>>(
                            "u_m4_model",glm::mat4(1.0)));

            auto cmlist_render_data_ptr =
                    static_cast<RenderDataComponentList*>(
                        scene->GetComponentList<RenderData>());

            auto& render_data =
                    cmlist_render_data_ptr->Create(
                        m_entity_id,
                        draw_key,
                        &buffer_layout,
                        list_uniforms,
                        list_draw_stages,
                        ks::draw::Transparency::Opaque);

            auto& geometry = render_data.GetGeometry();
            geometry.GetVertexBuffers().push_back(std::move(list_vx));
            geometry.SetVertexBufferUpdated(0);
        }

        void createBodyData(Scene* scene,
                            glm::vec2 position)
        {
            auto cmlist_body_data =
                    static_cast<ecs::ComponentList<SceneKey,BodyData>*>(
                        scene->GetComponentList<BodyData>());

            auto& body_data =
                    cmlist_body_data->Create(m_entity_id);

            body_data.position = position;
            body_data.velocity = glm::vec2(0,0);
        }

        void createCollisionData(Scene* scene,
                                 float height,
                                 float width)
        {
            auto cmlist_collision_data =
                    static_cast<ecs::ComponentList<SceneKey,CollisionData>*>(
                        scene->GetComponentList<CollisionData>());

            auto& collision_data =
                    cmlist_collision_data->Create(m_entity_id);

            collision_data.bbox =
                    glm::vec4(width*-0.5,
                              height*-0.5,
                              width*0.5,
                              height*0.5);

            collision_data.movable = false;
            collision_data.xsec_overlap = false;
        }

        Id m_entity_id;
    };

    // ============================================================= //

    class Ball
    {
    public:
        Ball(Scene* scene,
             Id draw_stage_id,
             Id shader_id,
             float radius,
             glm::vec2 initial_position,
             glm::vec2 initial_velocity,
             glm::u8vec4 color0,
             glm::u8vec4 color1)
        {
            m_radius = radius;

            // Create Entity
            m_entity_id = scene->CreateEntity();

            createRenderData(scene,draw_stage_id,shader_id,color0,color1);
            createCollisionData(scene);
            createBody(scene,initial_position,initial_velocity);
        }

    private:
        void createCollisionData(Scene* scene)
        {
            auto cmlist_collision_data =
                    static_cast<ecs::ComponentList<SceneKey,CollisionData>*>(
                        scene->GetComponentList<CollisionData>());

            auto& collision_data =
                    cmlist_collision_data->Create(m_entity_id);

            collision_data.bbox =
                    glm::vec4(m_radius*-1,
                              m_radius*-1,
                              m_radius,
                              m_radius);

            collision_data.movable = true;
            collision_data.xsec_overlap = false;
        }

        void createBody(Scene* scene,
                        glm::vec2 initial_position,
                        glm::vec2 initial_velocity)
        {
            auto cmlist_body_data =
                    static_cast<ecs::ComponentList<SceneKey,BodyData>*>(
                        scene->GetComponentList<BodyData>());

            auto& body_data =
                    cmlist_body_data->Create(m_entity_id);

            body_data.position = initial_position;
            body_data.velocity = initial_velocity;
        }

        void createRenderData(Scene* scene,
                              Id draw_stage_id,
                              Id shader_id,
                              glm::u8vec4 color0,
                              glm::u8vec4 color1)
        {
            // RenderData
            // Vertex List
            unique_ptr<std::vector<u8>> list_vx =
                    make_unique<std::vector<u8>>();

            float const radius = m_radius;
            uint const sides = 16;
            float const slice_rads = 6.28318530718/sides;

            for(uint i=1; i < sides+1; i++)
            {
                float start_rads = i*slice_rads;
                float end_rads = (i-1)*slice_rads;

                gl::Buffer::PushElement<Vertex>(
                            *list_vx,
                            Vertex{
                                glm::vec4(0,0,0,1),
                                color0
                            });

                gl::Buffer::PushElement<Vertex>(
                            *list_vx,
                            Vertex{
                                glm::vec4(cos(start_rads)*radius,sin(start_rads)*radius,0,1),
                                color1
                            });

                gl::Buffer::PushElement<Vertex>(
                            *list_vx,
                            Vertex{
                                glm::vec4(cos(end_rads)*radius,sin(end_rads)*radius,0,1),
                                color1
                            });
            }



            draw::DefaultDrawKey draw_key;
            draw_key.SetShader(shader_id);
            draw_key.SetPrimitive(gl::Primitive::Triangles);

            std::vector<u8> list_draw_stages = {u8(draw_stage_id)};

            auto list_uniforms =
                    make_shared<draw::ListUniformUPtrs>();

            list_uniforms->push_back(
                        make_unique<gl::Uniform<glm::mat4>>(
                            "u_m4_model",glm::mat4(1.0)));

            auto cmlist_render_data_ptr =
                    static_cast<RenderDataComponentList*>(
                        scene->GetComponentList<RenderData>());

            auto& render_data =
                    cmlist_render_data_ptr->Create(
                        m_entity_id,
                        draw_key,
                        &buffer_layout,
                        list_uniforms,
                        list_draw_stages,
                        ks::draw::Transparency::Opaque);

            auto& geometry = render_data.GetGeometry();
            geometry.GetVertexBuffers().push_back(std::move(list_vx));
            geometry.SetVertexBufferUpdated(0);
        }

        Id m_entity_id;
        float m_radius;
    };

    // ============================================================= //

    // Receives an 'update' signal from the scene
    class Updater : public ks::Object
    {
    public:
        using base_type = ks::Object;

        Updater(ks::Object::Key const &key,
                shared_ptr<EventLoop> evl,
                shared_ptr<Scene> scene) :
            ks::Object(key,evl),
            m_scene(scene),
            m_setup(false),
            m_draw_stage_id(0),
            m_shader_id(0)
        {

        }

        void Init(ks::Object::Key const &,
                  shared_ptr<Updater> const &this_updater)
        {
            m_scene->signal_before_update.Connect(
                        this_updater,
                        &Updater::onUpdate,
                        ks::ConnectionType::Direct);

            m_body_system = make_unique<BodySystem>(m_scene.get());
            m_collision_system = make_unique<CollisionSystem>(m_scene.get());
        }

        ~Updater() = default;


    private:
        // This gets called before test::Scene::onUpdate
        void onUpdate()
        {
            if(!m_setup)
            {
                m_scene->template RegisterComponentList<BodyData>(
                            make_unique<ecs::ComponentList<SceneKey,BodyData>>(*m_scene));

                m_scene->template RegisterComponentList<CollisionData>(
                            make_unique<ecs::ComponentList<SceneKey,CollisionData>>(*m_scene));

                auto render_system = m_scene->GetRenderSystem();

                // Add a draw stage
                m_draw_stage_id =
                        render_system->RegisterDrawStage(
                            make_shared<test::DefaultDrawStage>());

                // Add the shader
                m_shader_id =
                        render_system->RegisterShader(
                            "flat_color_attr",
                            test::vertex_shader,
                            test::frag_shader);

                // Create a ball
                auto to_rads =
                        [](float degs) -> float {
                            return (degs*3.14159265359/180.0);
                        };

                for(uint i=0; i < 36; i++)
                {
                    uint angle_degs = (10*i)+10;
                    float angle_rads = to_rads(angle_degs);
                    if((angle_degs)%90==0) {
                        continue;
                    }

                    bool even = (i%2==0);
                    float kplier = (even) ? 0.01 : 0.02;

                    glm::u8vec4 c0 = (even) ?
                                glm::u8vec4(0,0,255,255) :
                                glm::u8vec4(255,0,0,255);

                    glm::u8vec4 c1 = (even) ?
                                glm::u8vec4(0,255,255,255) :
                                glm::u8vec4(255,255,0,255);

                    glm::vec2 init_v(cos(angle_rads),sin(angle_rads));

                    auto ball =
                            make_unique<Ball>(
                                m_scene.get(),
                                m_draw_stage_id,
                                m_shader_id,
                                0.1,
                                glm::vec2(0,0),
                                init_v*kplier,
                                c1,
                                c0);
                }

                // Create the border blocks
                m_block_left =
                        make_unique<Block>(
                            m_scene.get(),
                            m_draw_stage_id,
                            m_shader_id,
                            2.0,
                            0.1,
                            glm::u8vec4(70,70,70,255),
                            glm::vec2(-1.0+0.1*0.5,0));

                m_block_right =
                        make_unique<Block>(
                            m_scene.get(),
                            m_draw_stage_id,
                            m_shader_id,
                            2.0,
                            0.1,
                            glm::u8vec4(70,70,70,255),
                            glm::vec2(1.0-0.1*0.5,0));

                m_block_top =
                        make_unique<Block>(
                            m_scene.get(),
                            m_draw_stage_id,
                            m_shader_id,
                            0.1,
                            2.0,
                            glm::u8vec4(70,70,70,255),
                            glm::vec2(0,1.0-0.5*0.1));

                m_block_bottom =
                        make_unique<Block>(
                            m_scene.get(),
                            m_draw_stage_id,
                            m_shader_id,
                            0.1,
                            2.0,
                            glm::u8vec4(70,70,70,255),
                            glm::vec2(0,-1.0+0.5*0.1));

                m_setup = true;
            }

            TimePoint a,b;
            m_body_system->Update(a,b);
            m_collision_system->Update(a,b);
        }

        shared_ptr<Scene> m_scene;
        bool m_setup;
        Id m_draw_stage_id;
        Id m_shader_id;

        unique_ptr<BodySystem> m_body_system;
        unique_ptr<CollisionSystem> m_collision_system;
        unique_ptr<Ball> m_ball;
        unique_ptr<Block> m_block_left;
        unique_ptr<Block> m_block_right;
        unique_ptr<Block> m_block_top;
        unique_ptr<Block> m_block_bottom;
    };
}

// ============================================================= //
// ============================================================= //

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // Create application
    shared_ptr<gui::Application> app =
            MakeObject<gui::Application>();

    // Create window
    gui::Window::Attributes win_attribs;
    gui::Window::Properties win_props;
    win_props.width = 480;
    win_props.height = 480;
    win_props.swap_interval = 1;

    shared_ptr<gui::Window> window =
            app->CreateWindow(
                app->GetEventLoop(),
                win_attribs,
                win_props);

    shared_ptr<test::Scene> scene =
            MakeObject<test::Scene>(
                app,
                window);

    shared_ptr<test::Updater> test_updater =
            MakeObject<test::Updater>(
                app->GetEventLoop(),
                scene);

    (void)test_updater;

    // Run!
    app->Run();

    return 0;
}

// ============================================================= //
// ============================================================= //
