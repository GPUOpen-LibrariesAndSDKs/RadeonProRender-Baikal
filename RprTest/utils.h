/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

namespace
{
    using namespace RadeonRays;

    //control how many frames to render in tests for resulting image
    int kRenderIterations = 256;
    //default structure for Vertex data
    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    //multi texture Vertex
    struct VertexMT
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex0[2];
        rpr_float tex1[2];
    };
    rpr_shape CreateSphere(rpr_context context, std::uint32_t lat, std::uint32_t lon, float r, RadeonRays::float3 const& c)
    {
        auto num_verts = (lat - 2) * lon + 2;
        auto num_tris = (lat - 2) * (lon - 1) * 2;

        std::vector<RadeonRays::float3> vertices(num_verts);
        std::vector<RadeonRays::float3> normals(num_verts);
        std::vector<RadeonRays::float2> uvs(num_verts);
        std::vector<std::uint32_t> indices(num_tris * 3);


        auto t = 0U;
        for (auto j = 1U; j < lat - 1; j++)
            for (auto i = 0U; i < lon; i++)
            {
                float theta = float(j) / (lat - 1) * (float)M_PI;
                float phi = float(i) / (lon - 1) * (float)M_PI * 2;
                vertices[t].x = r * sinf(theta) * cosf(phi) + c.x;
                vertices[t].y = r * cosf(theta) + c.y;
                vertices[t].z = r * -sinf(theta) * sinf(phi) + c.z;
                normals[t].x = sinf(theta) * cosf(phi);
                normals[t].y = cosf(theta);
                normals[t].z = -sinf(theta) * sinf(phi);
                uvs[t].x = phi / (2 * (float)M_PI);
                uvs[t].y = theta / ((float)M_PI);
                ++t;
            }

        vertices[t].x = c.x; vertices[t].y = c.y + r; vertices[t].z = c.z;
        normals[t].x = 0; normals[t].y = 1; normals[t].z = 0;
        uvs[t].x = 0; uvs[t].y = 0;
        ++t;
        vertices[t].x = c.x; vertices[t].y = c.y - r; vertices[t].z = c.z;
        normals[t].x = 0; normals[t].y = -1; normals[t].z = 0;
        uvs[t].x = 1; uvs[t].y = 1;
        ++t;

        t = 0U;
        for (auto j = 0U; j < lat - 3; j++)
            for (auto i = 0U; i < lon - 1; i++)
            {
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
                indices[t++] = j * lon + i + 1;
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
            }

        for (auto i = 0U; i < lon - 1; i++)
        {
            indices[t++] = (lat - 2) * lon;
            indices[t++] = i;
            indices[t++] = i + 1;
            indices[t++] = (lat - 2) * lon + 1;
            indices[t++] = (lat - 3) * lon + i + 1;
            indices[t++] = (lat - 3) * lon + i;
        }

        std::vector<int> faces(indices.size() / 3, 3);
        rpr_int status = RPR_SUCCESS;
        rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
            (rpr_float const*)vertices.data(), vertices.size(), sizeof(float3),
            (rpr_float const*)normals.data(), normals.size(), sizeof(float3),
            (rpr_float const*)uvs.data(), uvs.size(), sizeof(float2),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            faces.data(), faces.size(), &mesh);
        assert(status == RPR_SUCCESS);

        return mesh;
    }
}

#define FR_MACRO_CLEAN_SHAPE_RELEASE(mesh1__, scene1__)\
    status = rprShapeSetMaterial(mesh1__, NULL);\
    assert(status == RPR_SUCCESS);\
    status = rprSceneDetachShape(scene1__, mesh1__);\
    assert(status == RPR_SUCCESS);\
    status = rprObjectDelete(mesh1__); mesh1__ = NULL;\
    assert(status == RPR_SUCCESS);

#define FR_MACRO_SAFE_FRDELETE(a)    { status = rprObjectDelete(a); a = NULL;   assert(status == RPR_SUCCESS); }

