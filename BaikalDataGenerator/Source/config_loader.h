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

#include <vector>
#include "utils.h"
#include "input_info.h"

using CameraIterator = std::vector<CameraInfo>::const_iterator;
using LightsIterator = std::vector<LightInfo>::const_iterator;
using SppIterator = std::vector<int>::const_iterator;

class ConfigLoader
{
public:
    explicit ConfigLoader(const DGenConfig& config);

    CameraIterator CamStatesBegin() const;
    CameraIterator CamStatesEnd() const;

    LightsIterator LightsBegin() const;
    LightsIterator LightsEnd() const;

    SppIterator SppBegin() const;
    SppIterator SppEnd() const;

private:

    void ValidateConfig(const DGenConfig& config) const;

    void LoadCameraConfig(const std::filesystem::path& file_name);
    void LoadLightConfig(const std::filesystem::path& file_name);
    void LoadSppConfig(const std::filesystem::path& file_name);

    std::vector<CameraInfo> m_camera_states;
    std::vector<LightInfo> m_light_settings;
    std::vector<int> m_spp;
};