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

// Header

#include "dll-tracker.h"

// Standard includes

#include <functional>

// Windows includes

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <minwindef.h>
#include <winnt.h>
#include <subauth.h>
#include <libloaderapi.h>

/*
    ==============
    Start Tracking
    ==============

    Receives the function that will receive the
*/

namespace qcstudio::dll_tracker {

    // Windows function signatures and internal data types

    using notification_function_t = void(__stdcall*)(unsigned long, const void*, void*);                            // LdrDllNotification
    using register_function_t     = long(__stdcall*)(unsigned long, notification_function_t, const void*, void**);  // LdrRegisterDllNotification
    using unregister_function_t   = long(__stdcall*)(const void*);                                                  // LdrUnregisterDllNotification
    using user_callback_t         = std::function<void(dll_event_t, const dll_event_data_t&)>;

    // Windows data structures used with notifications (for both load and unload)

    struct notification_data_t {
        uint32_t              reserved_flags;
        const UNICODE_STRING& full_path;
        const UNICODE_STRING& base_name;
        uintptr_t             base_addr;
        uint32_t              size;
    };

    // Storage

    auto register_cookie     = (void*)nullptr;
    auto user_callback       = user_callback_t{};
    auto register_function   = register_function_t{nullptr};
    auto unregister_function = unregister_function_t{nullptr};
    auto debugging           = false;

    // Internal callback

    auto internal_callback = (notification_function_t)[](unsigned long _reason, const void* _data, void* _c) {
        auto& data       = *(notification_data_t*)_data;
        auto  event_data = dll_event_data_t{
            std::wstring(data.full_path.Buffer),
            std::wstring(data.base_name.Buffer),
            data.base_addr,
            data.size};

        if (user_callback) {
            user_callback(_reason == 1 ? dll_event_t::LOAD : dll_event_t::UNLOAD, event_data);
        }
    };

    // The start function

    auto start(std::function<void(dll_event_t, const dll_event_data_t&)>&& _callback, bool _debug) -> bool {
        if (register_cookie) {
            stop();
        }

        auto ntdll = LoadLibraryA("ntdll.dll");
        if (!ntdll) {
            return false;
        }

        register_function   = (register_function_t)GetProcAddress(ntdll, "LdrRegisterDllNotification");
        unregister_function = (unregister_function_t)GetProcAddress(ntdll, "LdrUnregisterDllNotification");

        if (register_function(0, internal_callback, nullptr, &register_cookie) != STATUS_SUCCESS) {
            return false;
        }

        user_callback = std::move(_callback);

        return true;
    }

    // The stop function

    void stop() {
        if (register_cookie && unregister_function) {
            unregister_function(register_cookie);
            register_cookie = nullptr;
        }
    }

}  // namespace qcstudio::dll_tracker