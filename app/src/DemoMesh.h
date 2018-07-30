/*
* Viry3D
* Copyright 2014-2018 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Demo.h"
#include "Application.h"
#include "graphics/Display.h"
#include "graphics/Camera.h"
#include "graphics/Shader.h"
#include "graphics/Material.h"
#include "graphics/MeshRenderer.h"
#include "graphics/Mesh.h"
#include "graphics/Texture.h"
#include "math/Quaternion.h"
#include "time/Time.h"
#include "ui/CanvasRenderer.h"
#include "ui/Label.h"
#include "ui/Font.h"

namespace Viry3D
{
    class DemoMesh : public Demo
    {
    public:
        struct CameraParam
        {
            Vector3 pos;
            Quaternion rot;
            float fov;
            float near_clip;
            float far_clip;
        };
        CameraParam m_camera_param = {
            Vector3(0, 0, -5),
            Quaternion::Identity(),
            45,
            1,
            1000
        };

        Camera* m_camera;
        MeshRenderer* m_renderer_cube;
        Label* m_label;
        float m_deg = 0;

        void InitMesh()
        {
            String vs = R"(
UniformBuffer(0, 0) uniform UniformBuffer00
{
	mat4 u_view_matrix;
	mat4 u_projection_matrix;
} buf_0_0;

UniformBuffer(1, 0) uniform UniformBuffer10
{
	mat4 u_model_matrix;
} buf_1_0;

Input(0) vec4 a_pos;
Input(2) vec2 a_uv;

Output(0) vec2 v_uv;

void main()
{
	gl_Position = a_pos * buf_1_0.u_model_matrix * buf_0_0.u_view_matrix * buf_0_0.u_projection_matrix;
	v_uv = a_uv;

	vulkan_convert();
}
)";
            String fs = R"(
precision highp float;

UniformTexture(0, 1) uniform sampler2D u_texture;

Input(0) vec2 v_uv;

Output(0) vec4 o_frag;

void main()
{
    o_frag = texture(u_texture, v_uv);
}
)";
            RenderState render_state;

            auto shader = RefMake<Shader>(
                    vs,
                    Vector<String>(),
                    fs,
                    Vector<String>(),
                    render_state);
            auto material = RefMake<Material>(shader);

            Vector<Vertex> vertices(8);
            Memory::Zero(&vertices[0], vertices.SizeInBytes());
            vertices[0].vertex = Vector3(-0.5f, 0.5f, -0.5f);
            vertices[1].vertex = Vector3(-0.5f, -0.5f, -0.5f);
            vertices[2].vertex = Vector3(0.5f, -0.5f, -0.5f);
            vertices[3].vertex = Vector3(0.5f, 0.5f, -0.5f);
            vertices[4].vertex = Vector3(-0.5f, 0.5f, 0.5f);
            vertices[5].vertex = Vector3(-0.5f, -0.5f, 0.5f);
            vertices[6].vertex = Vector3(0.5f, -0.5f, 0.5f);
            vertices[7].vertex = Vector3(0.5f, 0.5f, 0.5f);
            vertices[0].uv = Vector2(0, 0);
            vertices[1].uv = Vector2(0, 1);
            vertices[2].uv = Vector2(1, 1);
            vertices[3].uv = Vector2(1, 0);
            vertices[4].uv = Vector2(1, 0);
            vertices[5].uv = Vector2(1, 1);
            vertices[6].uv = Vector2(0, 1);
            vertices[7].uv = Vector2(0, 0);

            Vector<unsigned short> indices({
                                                   0, 1, 2, 0, 2, 3,
                                                   3, 2, 6, 3, 6, 7,
                                                   7, 6, 5, 7, 5, 4,
                                                   4, 5, 1, 4, 1, 0,
                                                   4, 0, 3, 4, 3, 7,
                                                   1, 5, 6, 1, 6, 2
                                           });
            auto cube = RefMake<Mesh>(vertices, indices);

            auto renderer = RefMake<MeshRenderer>();
            renderer->SetMaterial(material);
            renderer->SetMesh(cube);
            m_camera->AddRenderer(renderer);
            m_renderer_cube = renderer.get();

            auto sphere = Mesh::LoadFromFile(Application::Instance()->GetDataPath() + "/Library/unity default resources.Sphere.mesh");

            renderer = RefMake<MeshRenderer>();
            renderer->SetMaterial(material);
            renderer->SetMesh(sphere);
            m_camera->AddRenderer(renderer);

            Matrix4x4 model = Matrix4x4::Translation(Vector3(1.5f, 0, 0));
            renderer->SetInstanceMatrix("u_model_matrix", model);

            auto texture = Texture::LoadTexture2DFromFile(Application::Instance()->GetDataPath() + "/texture/logo.jpg", VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, true);
            material->SetTexture("u_texture", texture);

            Vector3 camera_forward = m_camera_param.rot * Vector3(0, 0, 1);
            Vector3 camera_up = m_camera_param.rot * Vector3(0, 1, 0);
            Matrix4x4 view = Matrix4x4::LookTo(m_camera_param.pos, camera_forward, camera_up);
            Matrix4x4 projection = Matrix4x4::Perspective(m_camera_param.fov, m_camera->GetTargetWidth() / (float) m_camera->GetTargetHeight(), m_camera_param.near_clip, m_camera_param.far_clip);
            material->SetMatrix("u_view_matrix", view);
            material->SetMatrix("u_projection_matrix", projection);
        }

        void InitUI()
        {
            auto canvas = RefMake<CanvasRenderer>();
            m_camera->AddRenderer(canvas);

            auto label = RefMake<Label>();
            canvas->AddView(label);

            label->SetAlignment(ViewAlignment::Left | ViewAlignment::Top);
            label->SetPivot(Vector2(0, 0));
            label->SetSize(Vector2i(100, 30));
            label->SetOffset(Vector2i(40, 40));
            label->SetFont(Font::GetFont(FontType::PingFangSC));
            label->SetFontSize(28);
            label->SetTextAlignment(ViewAlignment::Left | ViewAlignment::Top);

            m_label = label.get();
        }

        virtual void Init()
        {
            m_camera = Display::Instance()->CreateCamera();

            this->InitMesh();
            this->InitUI();
        }

        virtual void Done()
        {
            Display::Instance()->DestroyCamera(m_camera);
            m_camera = nullptr;
        }

        virtual void Update()
        {
            m_deg += 0.1f;

            Matrix4x4 model = Matrix4x4::Rotation(Quaternion::Euler(Vector3(0, m_deg, 0)));
            m_renderer_cube->SetInstanceMatrix("u_model_matrix", model);

            m_label->SetText(String::Format("FPS:%d", Time::GetFPS()));
        }

        virtual void OnResize(int width, int height)
        {
            Matrix4x4 projection = Matrix4x4::Perspective(m_camera_param.fov, m_camera->GetTargetWidth() / (float) m_camera->GetTargetHeight(), m_camera_param.near_clip, m_camera_param.far_clip);
            m_renderer_cube->GetMaterial()->SetMatrix("u_projection_matrix", projection);
        }
    };
}