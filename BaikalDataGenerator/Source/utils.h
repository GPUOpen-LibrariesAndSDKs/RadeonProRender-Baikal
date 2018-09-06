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

#include "filesystem.h"
#include <cstdint>
#include <cstddef>
#include <sstream>
#include <vector>


struct DGenConfig
{
    unsigned device_idx = 0;
    std::filesystem::path scene_file;
    std::filesystem::path light_file;
    std::filesystem::path camera_file;
    std::filesystem::path spp_file;
    std::filesystem::path output_dir;
    size_t width = 0;
    size_t height = 0;
    size_t split_num = 1;
    size_t split_idx = 0;
    std::int32_t offset_idx = 0;
    std::uint32_t num_bounces = 5;
    bool gamma_correction = false;
};

template<typename T>
std::vector<std::vector<T>> SplitVector(const std::vector<T>& vec, size_t n)
{
    std::vector<std::vector<T>> splits;

    size_t length = vec.size() / n;
    size_t remain = vec.size() % n;

    size_t begin = 0;
    size_t end = 0;

    for (size_t i = 0; i < std::min(n, vec.size()); ++i)
    {
        end += (remain > 0) ? (length + ((remain--) != 0)) : length;
        splits.push_back(std::vector<T>(vec.begin() + begin, vec.begin() + end));
        begin = end;
    }

    return splits;
}

template<typename T>
std::vector<T> GetSplitByIdx(const std::vector<T>& vec, size_t n, size_t idx)
{
    size_t length = vec.size() / n;
    size_t remain = vec.size() % n;

    size_t begin = 0;
    size_t end = 0;

    if (idx < remain)
    {
        begin = idx * (length + 1);
        end = begin + length + 1;
    }
    else
    {
        begin = remain * (length + 1) + (idx - remain) * length;
        end = begin + length;
    }
    return std::vector<T>(vec.begin() + begin, vec.begin() + end);
}

#define THROW_EX(insertions) \
    { \
        std::ostringstream stream; \
        stream << std::string(__func__) << ": " << insertions; \
        throw std::runtime_error(stream.str()); \
    }
