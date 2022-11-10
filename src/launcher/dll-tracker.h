/*
    MIT License

    Copyright (c) 2022 Raúl Ramos

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once

// C++ includes

#include <functional>

// The tracker declarations

namespace qcstudio::dll_tracker {

    enum class dll_event_t {
        LOAD,
        UNLOAD
    };

    struct dll_event_data_t {
        std::wstring full_path, base_name;
        uintptr_t    base_addr;
        size_t       addr_space_size;
    };

    auto start(std::function<void(dll_event_t, const dll_event_data_t&)>&& _callback, bool _debug = false) -> bool;
    void stop();

}  // namespace qcstudio::dll_tracker