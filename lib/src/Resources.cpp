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

#include "Resources.h"
#include "Node.h"
#include "Application.h"
#include "io/File.h"
#include "io/MemoryStream.h"
#include "graphics/MeshRenderer.h"
#include "graphics/SkinnedMeshRenderer.h"
#include "graphics/Mesh.h"
#include "graphics/Material.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "animation/Animation.h"

namespace Viry3D
{
    static Map<String, Ref<Object>> g_loading_cache;

    static String ReadString(MemoryStream& ms)
    {
        int size = ms.Read<int>();
        return ms.ReadString(size);
    }

    static Ref<Texture> ReadTexture(const String& path)
    {
        if (g_loading_cache.Contains(path))
        {
            return RefCast<Texture>(g_loading_cache[path]);
        }

        Ref<Texture> texture;

        String full_path = Application::Instance()->GetDataPath() + "/" + path;
        if (File::Exist(full_path))
        {
            MemoryStream ms(File::ReadAllBytes(full_path));

            String texture_name = ReadString(ms);
            int width = ms.Read<int>();
            int height = ms.Read<int>();
            SamplerAddressMode wrap_mode = (SamplerAddressMode) ms.Read<int>();
            FilterMode filter_mode = (FilterMode) ms.Read<int>();
            String texture_type = ReadString(ms);

            if (texture_type == "Texture2D")
            {
                int mipmap_count = ms.Read<int>();
                String png_path = ReadString(ms);

                texture = Texture::LoadTexture2DFromFile(Application::Instance()->GetDataPath() + "/" + png_path, filter_mode, wrap_mode, mipmap_count > 1);
                texture->SetName(texture_name);
            }
        }

        g_loading_cache.Add(path, texture);

        return texture;
    }

    static Ref<Material> ReadMaterial(const String& path)
    {
        if (g_loading_cache.Contains(path))
        {
            return RefCast<Material>(g_loading_cache[path]);
        }

        Ref<Material> material;

        String full_path = Application::Instance()->GetDataPath() + "/" + path;
        if (File::Exist(full_path))
        {
            MemoryStream ms(File::ReadAllBytes(full_path));

            String material_name = ReadString(ms);
            String shader_name = ReadString(ms);
            int property_count = ms.Read<int>();

            Ref<Shader> shader = Shader::Find(shader_name);
            if (shader)
            {
                material = RefMake<Material>(shader);
                material->SetName(material_name);
            }

            for (int i = 0; i < property_count; ++i)
            {
                String property_name = ReadString(ms);
                MaterialProperty::Type property_type = (MaterialProperty::Type) ms.Read<int>();

                switch (property_type)
                {
                    case MaterialProperty::Type::Color:
                    {
                        Color value = ms.Read<Color>();
                        if (material)
                        {
                            material->SetColor(property_name, value);
                        }
                        break;
                    }
                    case MaterialProperty::Type::Vector:
                    {
                        Vector4 value = ms.Read<Vector4>();
                        if (material)
                        {
                            material->SetVector(property_name, value);
                        }
                        break;
                    }
                    case MaterialProperty::Type::Float:
                    case MaterialProperty::Type::Range:
                    {
                        float value = ms.Read<float>();
                        if (material)
                        {
                            material->SetFloat(property_name, value);
                        }
                        break;
                    }
                    case MaterialProperty::Type::Texture:
                    {
                        Vector4 uv_scale_offset = ms.Read<Vector4>();
                        String texture_path = ReadString(ms);
                        if (texture_path.Size() > 0)
                        {
                            Ref<Texture> texture = ReadTexture(texture_path);
                            if (material && texture)
                            {
                                material->SetTexture(property_name, texture);
                            }
                        }
                        break;
                    }
                }
            }
        }

        g_loading_cache.Add(path, material);

        return material;
    }

    static void ReadRenderer(MemoryStream& ms, const Ref<Renderer>& renderer)
    {
        int lightmap_index = ms.Read<int>();
        Vector4 lightmapScaleOffset = ms.Read<Vector4>();
        bool cast_shadow = ms.Read<byte>() == 1;
        bool receive_shadow = ms.Read<byte>() == 1;

        (void) lightmap_index;
        (void) lightmapScaleOffset;
        (void) cast_shadow;
        (void) receive_shadow;
        
        int material_count = ms.Read<int>();
        for (int i = 0; i < material_count; ++i)
        {
            String material_path = ReadString(ms);
            if (material_path.Size() > 0)
            {
                Ref<Material> material = ReadMaterial(material_path);
                if (material)
                {
                    renderer->SetMaterial(material);
                }
            }
        }
    }

    static void ReadMeshRenderer(MemoryStream& ms, const Ref<MeshRenderer>& renderer)
    {
        ReadRenderer(ms, renderer);

        String mesh_path = ReadString(ms);
        auto mesh = Mesh::LoadFromFile(Application::Instance()->GetDataPath() + "/" + mesh_path);
        renderer->SetMesh(mesh);
    }

    static void ReadSkinnedMeshRenderer(MemoryStream& ms, const Ref<SkinnedMeshRenderer>& renderer)
    {
        ReadMeshRenderer(ms, renderer);

        int bone_count = ms.Read<int>();

        Vector<String> bones(bone_count);
        for (int i = 0; i < bone_count; ++i)
        {
            bones[i] = ReadString(ms);
        }
        renderer->SetBonePaths(bones);
    }

    static void ReadAnimation(MemoryStream& ms, const Ref<Animation>& animation)
    {
        int clip_count = ms.Read<int>();

        Vector<AnimationClip> clips(clip_count);

        for (int i = 0; i < clip_count; ++i)
        {
            String clip_name = ReadString(ms);
            float clip_length = ms.Read<float>();
            float clip_fps = ms.Read<float>();
            int clip_wrap_mode = ms.Read<int>();
            int curve_count = ms.Read<int>();

            AnimationClip& clip = clips[i];
            clip.name = clip_name;
            clip.length = clip_length;
            clip.fps = clip_fps;
            clip.wrap_mode = (AnimationWrapMode) clip_wrap_mode;

            for (int j = 0; j < curve_count; ++j)
            {
                String curve_path = ReadString(ms);
                int property_type = ms.Read<int>();
                int key_count = ms.Read<int>();

                AnimationCurveWrapper* curve = nullptr;
                for (int k = 0; k < clip.curves.Size(); ++k)
                {
                    if (clip.curves[k].path == curve_path)
                    {
                        curve = &clip.curves[k];
                        break;
                    }
                }
                if (curve == nullptr)
                {
                    AnimationCurveWrapper new_path_curve;
                    new_path_curve.path = curve_path;
                    clip.curves.Add(new_path_curve);
                    curve = &clip.curves[clip.curves.Size() - 1];
                }
                
                curve->property_types.Add((CurvePropertyType) property_type);
                curve->curves.Add(AnimationCurve());

                AnimationCurve* anim_curve = &curve->curves[curve->curves.Size() - 1];

                for (int k = 0; k < key_count; ++k)
                {
                    float time = ms.Read<float>();
                    float value = ms.Read<float>();
                    float in_tangent = ms.Read<float>();
                    float out_tangent = ms.Read<float>();

                    anim_curve->AddKey(time, value, in_tangent, out_tangent);
                }
            }
        }

        animation->SetClips(std::move(clips));
    }

    static Ref<Node> ReadNode(MemoryStream& ms, const Ref<Node>& parent)
    {
        Ref<Node> node;

        String name = ReadString(ms);
        int layer = ms.Read<int>();
        bool active = ms.Read<byte>() == 1;

        (void) layer;
        (void) active;

        Vector3 local_pos = ms.Read<Vector3>();
        Quaternion local_rot = ms.Read<Quaternion>();
        Vector3 local_scale = ms.Read<Vector3>();

        int com_count = ms.Read<int>();
        for (int i = 0; i < com_count; ++i)
        {
            String com_name = ReadString(ms);

            if (com_name == "MeshRenderer")
            {
                assert(!node);

                auto com = RefMake<MeshRenderer>();
                ReadMeshRenderer(ms, com);
                node = com;
            }
            else if (com_name == "SkinnedMeshRenderer")
            {
                assert(!node);

                auto com = RefMake<SkinnedMeshRenderer>();
                ReadSkinnedMeshRenderer(ms, com);
                node = com;

                if (parent)
                {
                    com->SetBonesRoot(Node::GetRoot(parent));
                }
                else
                {
                    com->SetBonesRoot(com);
                }
            }
            else if (com_name == "Animation")
            {
                assert(!node);

                auto com = RefMake<Animation>();
                ReadAnimation(ms, com);
                node = com;
            }
        }

        if (!node)
        {
            node = RefMake<Node>();
        }

        if (parent)
        {
            Node::SetParent(node, parent);
        }

        node->SetName(name);
        node->SetLocalPosition(local_pos);
        node->SetLocalRotation(local_rot);
        node->SetLocalScale(local_scale);

        int child_count = ms.Read<int>();
        for (int i = 0; i < child_count; ++i)
        {
            ReadNode(ms, node);
        }

        return node;
    }

    Ref<Node> Resources::Load(const String& path)
    {
        Ref<Node> node;

        String full_path = Application::Instance()->GetDataPath() + "/" + path;
        if (File::Exist(full_path))
        {
            MemoryStream ms(File::ReadAllBytes(full_path));

            node = ReadNode(ms, Ref<Node>());

            g_loading_cache.Clear();
        }

        return node;
    }
}
