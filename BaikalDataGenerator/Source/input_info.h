/**********************************************************************
Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.

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

#pragma once

#include <radeon_rays.h>
#include <string>

struct CameraInfo
{
    std::size_t index;
    std::string type;
    RadeonRays::float3 pos;
    RadeonRays::float3 at;
    RadeonRays::float3 up;
    RadeonRays::float2 sensor_size;
    RadeonRays::float2 zcap;
    float aperture;
    float focus_distance;
    float focal_length;
};

struct LightInfo
{
    std::string type;
    RadeonRays::float3 pos;
    RadeonRays::float3 dir;
    RadeonRays::float3 rad;

    // cone shape, this option available only for spot light
    RadeonRays::float2 cs;

    // this options available only for ibl
    // path to texture image
    std::string texture;
    float mul;
};