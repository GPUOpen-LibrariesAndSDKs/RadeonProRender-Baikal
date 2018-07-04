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

#include <sstream>
#include <string>
#include <vector>

class CmdParser
{
public:
    CmdParser(int argc, char* argv[]);

    bool OptionExists(const std::string& option) const;

    template <class T = std::string>
    T GetOption(const std::string& option) const
    {
        auto value = GetOptionValue(option);

        if (!value)
        {
            std::stringstream ss;

            ss << std::string(__func__) << ": "
               << option << " :option or its value is missed";

            throw std::logic_error(ss.str());
        }

        return LexicalCast<T>(*value);
    }


    template <class T = std::string>
    T GetOption(const std::string& option, const T default_value) const
    {
        auto value = GetOptionValue(option);

        if (!value)
        {
            return default_value;
        }
        else
        {
            return LexicalCast<T>(*value);
        }
    }

private:
    const std::string* GetOptionValue(const std::string& option) const;

    template <typename T>
    static T LexicalCast(const std::string& str)
    {
        T var;
        std::istringstream iss(str);

        if (iss.fail())
        {
            throw std::logic_error(std::string(__func__) + "cmd stream failed");
        }

        iss >> var;
        return var;
    }

    std::vector<std::string> m_cmd_line;
};