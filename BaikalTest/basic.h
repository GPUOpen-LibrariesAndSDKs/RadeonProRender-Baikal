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
#pragma once

#include "gtest/gtest.h"

#include "CLW.h"
#include "Renderers/ptrenderer.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <cstdlib>

extern int g_argc;
extern char** g_argv;

class BasicTest : public ::testing::Test
{
public:
    static int constexpr kMaxPlatforms = 5;

    virtual void SetUp()
    {
        std::vector<CLWPlatform> platforms;

        ASSERT_NO_THROW(CLWPlatform::CreateAllPlatforms(platforms));
        ASSERT_GT(platforms.size(), 0);

        char* device_index_option = GetCmdOption(g_argv, g_argv + g_argc, "-device");
        char* platform_index_option = GetCmdOption(g_argv, g_argv + g_argc, "-platform");

        auto platform_index = platform_index_option ? (int)atoi(platform_index_option) : 0;
        auto device_index = device_index_option ? (int)atoi(device_index_option) : 0;

        ASSERT_LT(platform_index, platforms.size());
        ASSERT_LT(device_index, platforms[platform_index].GetDeviceCount());

        auto platform = platforms[platform_index];
        auto device = platform.GetDevice(device_index);
        auto context = CLWContext::Create(device);

        ASSERT_NO_THROW(m_renderer.reset(new Baikal::PtRenderer(context, 0, 5)));
    }

    virtual void TearDown()
    {
    }

    static char* GetCmdOption(char ** begin, char ** end, const std::string & option)
    {
        char ** itr = std::find(begin, end, option);
        if (itr != end && ++itr != end)
        {
            return *itr;
        }
        return 0;
    }

    static bool CmdOptionExists(char** begin, char** end, const std::string& option)
    {
        return std::find(begin, end, option) != end;
    }

    std::unique_ptr<Baikal::PtRenderer> m_renderer;
};


// Test renderer instance creation
TEST_F(BasicTest, Init)
{
    ASSERT_TRUE(true);
}
