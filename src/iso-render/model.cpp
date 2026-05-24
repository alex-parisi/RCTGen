#include "model.h"
#include "palette.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <png.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

void texture_init(texture_t* texture, std::uint16_t width, std::uint16_t height)
{
    texture->width = width;
    texture->height = height;
    texture->pixels = static_cast<vector3_t*>(std::malloc(static_cast<std::size_t>(width) * height * sizeof(color_t)));
}

int texture_load_png(texture_t* texture, const char* filename)
{
    std::FILE* fp = std::fopen(filename, "rb");
    if (!fp) return 1;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { std::fclose(fp); return 2; }

    png_infop info = png_create_info_struct(png);
    if (!info) { std::fclose(fp); return 3; }

    if (setjmp(png_jmpbuf(png))) std::abort(); // TODO Not sure what this does

    png_init_io(png, fp);
    png_read_info(png, info);

    texture->width = static_cast<std::uint16_t>(png_get_image_width(png, info));
    texture->height = static_cast<std::uint16_t>(png_get_image_height(png, info));

    const png_byte color_type = png_get_color_type(png, info);
    const png_byte bit_depth = png_get_bit_depth(png, info);
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    std::vector<std::vector<png_byte>> row_storage(texture->height);
    std::vector<png_bytep> row_pointers(texture->height);
    const std::size_t rowbytes = png_get_rowbytes(png, info);
    for (int y = 0; y < texture->height; ++y)
    {
        row_storage[y].resize(rowbytes);
        row_pointers[y] = row_storage[y].data();
    }

    png_read_image(png, row_pointers.data());

    texture->pixels = static_cast<vector3_t*>(std::malloc(static_cast<std::size_t>(texture->width) * texture->height * sizeof(vector3_t)));
    for (int y = 0; y < texture->height; ++y)
        for (int x = 0; x < texture->width; ++x)
        {
            const color_t c{
                static_cast<std::uint8_t>(row_pointers[y][4 * x]),
                static_cast<std::uint8_t>(row_pointers[y][4 * x + 1]),
                static_cast<std::uint8_t>(row_pointers[y][4 * x + 2])};
            texture->pixels[x + y * texture->width] = vector_from_color(c);
        }
    std::fclose(fp);
    return 0;
}

namespace
{
inline float wrap_coord(float coord) noexcept
{
    return std::clamp(coord - std::floor(coord), 0.0f, 1.0f);
}
}

vector3_t texture_sample(texture_t* texture, vector2_t coord)
{
    std::uint16_t tex_x = static_cast<std::uint16_t>(static_cast<std::uint32_t>(texture->width * wrap_coord(coord.x)));
    std::uint16_t tex_y = static_cast<std::uint16_t>(static_cast<std::uint32_t>(texture->height * wrap_coord(coord.y)));
    if (tex_x == texture->width) tex_x = 0;
    if (tex_y == texture->height) tex_y = 0;
    if (tex_x >= texture->width) std::printf("X: %d Width %d\n", tex_x, texture->width);
    if (tex_y >= texture->height) std::printf("Y: %d Height %d\n", tex_y, texture->height);
    assert(tex_x < texture->width && tex_y < texture->height);
    return texture->pixels[tex_y * texture->width + tex_x];
}

void texture_destroy(texture_t* texture)
{
    std::free(texture->pixels);
}

material_t material_color(vector3_t color, vector3_t specular_color, float specular_exponent, std::uint8_t flags)
{
    material_t material{};
    material.flags = static_cast<std::uint16_t>(flags & ~MATERIAL_HAS_TEXTURE);
    material.color = color;
    material.specular_color = specular_color;
    material.specular_exponent = specular_exponent;
    material.ambient_color = vector3(0.0f, 0.0f, 0.0f);
    return material;
}

material_t material_texture(const char* filename, vector3_t specular_color, float specular_exponent, std::uint8_t flags)
{
    material_t material{};
    if (texture_load_png(&material.texture, filename))
    {
        std::printf("Failed to load %s\n", filename);
        material.flags = static_cast<std::uint16_t>(flags & ~MATERIAL_HAS_TEXTURE);
        material.color = vector3(0.25f, 0.25f, 0.25f);
    }
    else
    {
        material.flags = static_cast<std::uint16_t>(flags | MATERIAL_HAS_TEXTURE);
    }
    material.specular_color = specular_color;
    material.specular_exponent = specular_exponent;
    material.ambient_color = vector3(0.0f, 0.0f, 0.0f);
    return material;
}

int mesh_load_transform(mesh_t* output, const char* filename, matrix_t mat)
{
    int import_flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals;

    const double determinant = mat.determinant();
    if (std::fabs(std::fabs(determinant) - 1.0) > 0.001) std::printf("Warning: transformation matrix is not orthonormal\n");
    if (determinant < 0) import_flags |= aiProcess_FlipWindingOrder;

    const aiScene* scene = aiImportFile(filename, import_flags);
    if (!scene)
    {
        std::printf("Importing file \"%s\" failed with error: %s\n", filename, aiGetErrorString());
        return 1;
    }

    output->num_materials = scene->mNumMaterials;
    output->materials = static_cast<material_t*>(std::malloc(scene->mNumMaterials * sizeof(material_t)));

    for (std::size_t i = 0; i < scene->mNumMaterials; ++i)
    {
        output->materials[i].flags = 0;
        output->materials[i].region = 0;
        output->materials[i].color = vector3(0.5f, 0.5f, 0.5f);
        output->materials[i].specular_color = vector3(0.5f, 0.5f, 0.5f);
        output->materials[i].specular_exponent = 50;
        output->materials[i].ambient_color = vector3(0.0f, 0.0f, 0.0f);

        const aiMaterial* mat_in = scene->mMaterials[i];

        aiString name;
        if (aiGetMaterialString(mat_in, AI_MATKEY_NAME, &name) == AI_SUCCESS)
        {
            if (std::strstr(name.data, "Remap1") != nullptr)
            {
                output->materials[i].flags |= MATERIAL_IS_REMAPPABLE;
                output->materials[i].region = 1;
            }
            else if (std::strstr(name.data, "Remap2") != nullptr)
            {
                output->materials[i].flags |= MATERIAL_IS_REMAPPABLE;
                output->materials[i].region = 2;
            }
            else if (std::strstr(name.data, "Remap3") != nullptr)
            {
                output->materials[i].flags |= MATERIAL_IS_REMAPPABLE;
                output->materials[i].region = 3;
            }
            else if (std::strstr(name.data, "Greyscale") != nullptr)
            {
                output->materials[i].region = 4;
            }
            else if (std::strstr(name.data, "Peep") != nullptr) output->materials[i].region = 5;
            else if (std::strstr(name.data, "Chain") != nullptr) output->materials[i].region = 6;

            if (std::strstr(name.data, "VisibleMask") != nullptr) output->materials[i].flags |= MATERIAL_IS_VISIBLE_MASK;
            else if (std::strstr(name.data, "Mask") != nullptr)  output->materials[i].flags |= MATERIAL_IS_MASK;
            if (std::strstr(name.data, "NoAO") != nullptr)       output->materials[i].flags |= MATERIAL_NO_AO;
            if (std::strstr(name.data, "Edge") != nullptr)       output->materials[i].flags |= MATERIAL_BACKGROUND_AA;
            if (std::strstr(name.data, "DarkEdge") != nullptr)   output->materials[i].flags |= MATERIAL_BACKGROUND_AA_DARK;
            if (std::strstr(name.data, "NoBleed") != nullptr)    output->materials[i].flags |= MATERIAL_NO_BLEED;
            if (std::strstr(name.data, "FlatShaded") != nullptr) output->materials[i].flags |= MATERIAL_IS_FLAT_SHADED;
        }

        aiColor4D diffuse;
        aiString texture_path;
        if (aiGetMaterialString(mat_in, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), &texture_path) == AI_SUCCESS)
        {
            output->materials[i].flags |= MATERIAL_HAS_TEXTURE;
            if (texture_load_png(&output->materials[i].texture, texture_path.data))
            {
                std::printf("Failed to load texture \"%s\"\r\n", texture_path.data);
                std::free(output->vertices);
                std::free(output->normals);
                std::free(output->faces);
                std::free(output->materials);
                aiReleaseImport(scene);
                return 1;
            }
        }
        else if (aiGetMaterialColor(mat_in, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS)
        {
            output->materials[i].color = vector3(diffuse.r, diffuse.g, diffuse.b);
        }

        aiColor4D specular;
        if (aiGetMaterialColor(mat_in, AI_MATKEY_COLOR_SPECULAR, &specular) == AI_SUCCESS)
        {
            output->materials[i].specular_color = vector3(specular.r, specular.g, specular.b);
            float specular_strength;
            if (aiGetMaterialFloatArray(mat_in, AI_MATKEY_SHININESS_STRENGTH, &specular_strength, nullptr) == AI_SUCCESS)
            {
                output->materials[i].specular_color = output->materials[i].specular_color * specular_strength;
            }
        }

        float specular_exponent;
        if (aiGetMaterialFloatArray(mat_in, AI_MATKEY_SHININESS, &specular_exponent, nullptr) == AI_SUCCESS)
        {
            output->materials[i].specular_exponent = specular_exponent;
        }

        aiColor4D ambient_color;
        if (aiGetMaterialColor(mat_in, AI_MATKEY_COLOR_AMBIENT, &ambient_color) == AI_SUCCESS)
        {
            output->materials[i].ambient_color = vector3(ambient_color.r, ambient_color.g, ambient_color.b);
        }
    }

    output->num_vertices = 0;
    output->num_faces = 0;
    for (std::size_t j = 0; j < scene->mNumMeshes; ++j)
    {
        const aiMesh* m = scene->mMeshes[j];
        assert(m->mNormals);
        for (std::size_t i = 0; i < m->mNumFaces; ++i)
        {
            if (m->mFaces[i].mNumIndices < 3) continue;
            assert(m->mFaces[i].mNumIndices == 3);
            output->num_faces++;
        }
        output->num_vertices += m->mNumVertices;
    }

    std::printf("Loading model with %zu vertices and %zu faces\n", output->num_vertices, output->num_faces);

    output->vertices = static_cast<vector3_t*>(std::malloc(output->num_vertices * sizeof(vector3_t)));
    output->normals  = static_cast<vector3_t*>(std::malloc(output->num_vertices * sizeof(vector3_t)));
    output->uvs      = static_cast<vector2_t*>(std::malloc(output->num_vertices * sizeof(vector2_t)));
    output->faces    = static_cast<face_t*>   (std::malloc(output->num_faces    * sizeof(face_t)));

    std::size_t mesh_start_vertex = 0;
    std::size_t mesh_start_face = 0;

    for (std::size_t j = 0; j < scene->mNumMeshes; ++j)
    {
        const aiMesh* m = scene->mMeshes[j];
        assert(m->mNormals);
        for (std::size_t i = 0; i < m->mNumVertices; ++i)
        {
            output->vertices[mesh_start_vertex + i] = matrix_vector(mat, vector3(m->mVertices[i].x, m->mVertices[i].y, m->mVertices[i].z));
            output->normals [mesh_start_vertex + i] = matrix_vector(mat, vector3(m->mNormals[i].x,  m->mNormals[i].y,  m->mNormals[i].z));
            if (m->mTextureCoords[0])
                output->uvs[mesh_start_vertex + i] = vector2(m->mTextureCoords[0][i].x, m->mTextureCoords[0][i].y);
            else
                output->uvs[mesh_start_vertex + i] = vector2(0.0f, 0.0f);
        }

        for (std::size_t i = 0; i < m->mNumFaces; ++i)
        {
            if (m->mFaces[i].mNumIndices < 3) continue;
            assert(m->mFaces[i].mNumIndices == 3);
            output->faces[mesh_start_face].material = m->mMaterialIndex;
            for (std::uint32_t k = 0; k < 3; ++k)
                output->faces[mesh_start_face].indices[k] = mesh_start_vertex + m->mFaces[i].mIndices[k];
            mesh_start_face++;
        }
        mesh_start_vertex += m->mNumVertices;
    }
    aiReleaseImport(scene);
    return 0;
}

void mesh_destroy(mesh_t* mesh)
{
    for (std::uint32_t i = 0; i < mesh->num_materials; ++i)
    {
        if (mesh->materials[i].flags & MATERIAL_HAS_TEXTURE)
            texture_destroy(&mesh->materials[i].texture);
    }

    std::free(mesh->vertices);
    std::free(mesh->normals);
    std::free(mesh->faces);
    std::free(mesh->materials);
}

int mesh_load(mesh_t* output, const char* filename)
{
    return mesh_load_transform(output, filename, matrix_identity());
}
