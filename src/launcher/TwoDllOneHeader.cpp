/*
    == References ==========

    https://github.com/MicrosoftDocs/cpp-docs/blob/main/docs/build/walkthrough-creating-and-using-a-dynamic-link-library-cpp.md
*/

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

#include "common_lib.h"
#include "Dll1.h"
#include "dll-tracker.h"

using namespace std;

// test g_test;

int main() {
    auto x  = LocalAlloc(LPTR, 38);
    auto y  = LocalSize(x);
    auto _1 = TlsAlloc();

    const auto cb = [](qcstudio::dll_tracker::dll_event_t _event, const qcstudio::dll_tracker::dll_event_data_t& _data) {
        switch (_event) {
            case qcstudio::dll_tracker::dll_event_t::LOAD:
                wcout << L"Loading ";
                break;
            case qcstudio::dll_tracker::dll_event_t::UNLOAD:
                wcout << L"Unloading ";
                break;
        }
        wcout << "\"" << _data.base_name.c_str() << "\" at \"" << _data.full_path.c_str() << "\"" << endl;
        wcout << "Base addr is 0x" << hex << _data.base_addr << " and size is " << dec << _data.addr_space_size;
        wcout << endl;
    };

    if (qcstudio::dll_tracker::start(cb)) {
        // Sample load 2 dlls

        auto dll1 = LoadLibrary(L"Dll1.dll");
        auto dll2 = LoadLibrary(L"Dll2.dll");

        // Get some dll functions

        auto dll1_foo = (void (*)())GetProcAddress(dll1, "dll1_foo");
        auto dll2_foo = (void (*)())GetProcAddress(dll2, "dll2_foo");

        // Invoke functions

        if (dll1_foo) {
            dll1_foo();
        }

        if (dll2_foo) {
            dll2_foo();
        }

        // Unload dlls

        if (dll1) {
            FreeLibrary(dll1);
        }
        if (dll2) {
            FreeLibrary(dll2);
        }

        // Stop tracking

        qcstudio::dll_tracker::stop();
    }
}