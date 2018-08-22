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
#include <filesystem>
#include <string>

// Visual Studio 2015 work-around ... 
// std::filesystem was incorporated into C++-17 (which is obviously after VS
// 2015 was released). However, Microsoft implemented the draft standard in
// the std::exerimental namespace. To avoid nasty ripple effects when the
// compiler is updated, make it look like the standard here
namespace std
{
    namespace filesystem = experimental::filesystem::v1;
}

struct DGenConfig
{
    std::filesystem::path scene_file;
    std::filesystem::path light_file;
    std::filesystem::path camera_file;
    std::filesystem::path spp_file;
    std::filesystem::path output_dir;
    size_t width, height;
    size_t split_num = 1;
    size_t split_idx = 0;
    size_t offset_idx = 0;
    std::uint32_t num_bounces = 5;
    bool gamma_correction;
};

#define THROW_EX(text) throw std::runtime_error(std::string(__func__) + ": " + text);