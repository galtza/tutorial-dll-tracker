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
#include <winternl.h>

/*
    Windows data structures and signatures
*/

// clang-format off

// Windows data received as 'load' event is received (+ associated aliases)

using LDR_DLL_LOADED_NOTIFICATION_DATA = struct {
    ULONG            Flags;         // Reserved.
    PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
    PVOID            DllBase;       // A pointer to the base address for the DLL in memory.
    ULONG            SizeOfImage;   // The size of the DLL image, in bytes.
};
using PLDR_DLL_LOADED_NOTIFICATION_DATA = LDR_DLL_LOADED_NOTIFICATION_DATA*;

// Windows data received as 'unload' event is received (+ associated aliases)

using LDR_DLL_UNLOADED_NOTIFICATION_DATA = struct {
    ULONG            Flags;         // Reserved.
    PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
    PVOID            DllBase;       // A pointer to the base address for the DLL in memory.
    ULONG            SizeOfImage;   // The size of the DLL image, in bytes.
};
using PLDR_DLL_UNLOADED_NOTIFICATION_DATA = LDR_DLL_UNLOADED_NOTIFICATION_DATA*;

// Generic notification data structure for any event (notice that 'load' and 'unload' are actually the same data) + the aliases

using LDR_DLL_NOTIFICATION_DATA = union {
    LDR_DLL_LOADED_NOTIFICATION_DATA   Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
};
using PLDR_DLL_NOTIFICATION_DATA = LDR_DLL_NOTIFICATION_DATA*;
using PCLDR_DLL_NOTIFICATION_DATA = const LDR_DLL_NOTIFICATION_DATA*;

// Signature for the notification callback

using LDR_DLL_NOTIFICATION_FUNCTION = VOID NTAPI (
  _In_     ULONG                       NotificationReason,
  _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
  _In_opt_ PVOID                       Context
);
using PLDR_DLL_NOTIFICATION_FUNCTION = LDR_DLL_NOTIFICATION_FUNCTION*;

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

using LdrRegisterDllNotification = NTSTATUS (NTAPI*)(
  _In_     ULONG                          Flags,
  _In_     PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
  _In_opt_ PVOID                          Context,
  _Out_    PVOID                          *Cookie
);

using LdrUnregisterDllNotification = NTSTATUS (NTAPI*) (
  _In_ PVOID Cookie
);

using LdrDllNotification = VOID (CALLBACK*)(
  _In_     ULONG                       NotificationReason,
  _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
  _In_opt_ PVOID                       Context
);

// clang-format on

/*
    ==============
    Start Tracking
    ==============

    Receives the function that will receive the
*/

namespace qcstudio::dll_tracker {

    using namespace std;

    // Storage

    auto cookie   = (void*)nullptr;
    auto callback = callback_t{};
    auto reg      = LdrRegisterDllNotification{nullptr};
    auto unreg    = LdrUnregisterDllNotification{nullptr};

    // Internal callback

    auto internal_callback = (LdrDllNotification)[](ULONG _reason, PCLDR_DLL_NOTIFICATION_DATA _notification_data, PVOID _ctx) {
        if (_notification_data && callback) {
            switch (_reason) {
                case LDR_DLL_NOTIFICATION_REASON_LOADED: {
                    callback(
                        true,
                        wstring(_notification_data->Loaded.FullDllName->Buffer),
                        wstring(_notification_data->Loaded.BaseDllName->Buffer),
                        (uintptr_t)_notification_data->Loaded.DllBase,
                        _notification_data->Loaded.SizeOfImage);
                    break;
                }
                case LDR_DLL_NOTIFICATION_REASON_UNLOADED: {
                    callback(
                        true,
                        wstring(_notification_data->Unloaded.FullDllName->Buffer),
                        wstring(_notification_data->Unloaded.BaseDllName->Buffer),
                        (uintptr_t)_notification_data->Unloaded.DllBase,
                        _notification_data->Unloaded.SizeOfImage);
                    break;
                }
            }
        }
    };

    // The start function

    bool start(callback_t&& _callback) {
        if (cookie) {
            stop();
        }

        auto ntdll = LoadLibraryA("ntdll.dll");
        if (!ntdll) {
            return false;
        }

        reg   = (LdrRegisterDllNotification)GetProcAddress(ntdll, "LdrRegisterDllNotification");
        unreg = (LdrUnregisterDllNotification)GetProcAddress(ntdll, "LdrUnregisterDllNotification");
        if (!reg || !unreg) {
            return false;
        }

        if (reg(0, internal_callback, nullptr, &cookie)) {
            return false;
        }

        callback = move(_callback);

        return true;
    }

    // The stop function

    void stop() {
        if (cookie && unreg) {
            unreg(cookie);
        }
        cookie = nullptr;
    }

}  // namespace qcstudio::dll_tracker