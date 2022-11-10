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

// refs: https://github.com/MicrosoftDocs/cpp-docs/blob/main/docs/build/walkthrough-creating-and-using-a-dynamic-link-library-cpp.md

// C++ includes

#include <iostream>
#include <string>
#include <type_traits>

// Windows includes

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <minwindef.h>
#include <winnt.h>
#include <subauth.h>
#include <libloaderapi.h>

// Our includes

#include "dll-tracker.h"

using namespace std;

int main() {
    // Callback called by our library

    const auto cb = [](qcstudio::dll_tracker::dll_event_t _event, const qcstudio::dll_tracker::dll_event_data_t& _data) {
        switch (_event) {
            case qcstudio::dll_tracker::dll_event_t::LOAD:
                wcout << L"Loading ";
                break;
            case qcstudio::dll_tracker::dll_event_t::UNLOAD:
                wcout << L"Unloading ";
                break;
        }
        wcout << "\"" << _data.base_name.c_str() << "\" at \"" << _data.full_path.c_str() << "\" with";
        wcout << "base addr 0x" << hex << _data.base_addr << " and size " << dec << _data.addr_space_size;
        wcout << endl;
    };

    // Test: start/load dlls/get functions/call functions/stop

    if (qcstudio::dll_tracker::start(cb)) {
        auto foo_module = LoadLibrary(L"foo.dll");
        auto bar_module = LoadLibrary(L"bar.dll");

        using signature_t = void (*)();

        if (auto foo_function = (signature_t)GetProcAddress(foo_module, "foo")) {
            foo_function();
        }
        if (auto bar_function = (signature_t)GetProcAddress(bar_module, "bar")) {
            bar_function();
        }

        if (foo_module) {
            FreeLibrary(foo_module);
        }
        if (bar_module) {
            FreeLibrary(bar_module);
        }

        qcstudio::dll_tracker::stop();
    }
}