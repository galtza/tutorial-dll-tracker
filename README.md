:information_source: Medium level

# Tutorial: tracking down *dll* events

This is a tutorial about how to track down *Load* and *Unload* *dll* events and associated data.

## Interface

Even though this tutorial is only about Windows implementation we will hide the implementation details via an interface API using namespaces. 

So, let's define the requirements:

* Track load/unload events

* Provide the user with a way to receive call-backs

* Give information like the name/path of the loaded/unloaded *dll*, the base address and its size

The user interface should look like this:

```c++
namespace qcstudio::dll_tracker {

    using namespace std;

    /*
        Callback params...

        bool:      true if load false if unload
        wstring:   full path of the dll
        wstring:   name of the dll
        uintptr_t: base address of the dll
        size_t:    size of the dll
    */

    using callback_t = function<void(bool, const wstring&, const wstring&, uintptr_t, size_t)>;

    // start / stop

    bool start(callback_t&& _callback);
    void stop();

}  // namespace qcstudio::dll_tracker
```

Let's talk about implementation details for Windows.

## Windows

Two functions allow us to get registered to *dll* events: ***[LdrRegisterDllNotification](https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrregisterdllnotification)*** and ***[LdrUnregisterDllNotification](https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrunregisterdllnotification)***. Both can be found inside *ntdll.dll*. 

:warning: However, you will **NOT** find any Windows header file containing the definition of these functions according to the [Remarks](https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrregisterdllnotification#remarks)!

Instead, we have to use the well known mechanism of loading the *dll* with ***LoadLibraryA*** Windows function and then get the function address with ***GetProcAddress***.

But before, we need to write down all the data structures involved as well as the function signatures. This is what we will be using in our Windows implementation straightaway (notice that I use *using* rather than *typedef* as after 25+ years programming I still get confused).

When it comes to *load* and *unload* events these data structures are used to inform the call-back:

```C++
using LDR_DLL_LOADED_NOTIFICATION_DATA = struct {
    ULONG            Flags;         // Reserved.
    PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
    PVOID            DllBase;       // A pointer to the base address for the DLL in memory.
    ULONG            SizeOfImage;   // The size of the DLL image, in bytes.
};
using PLDR_DLL_LOADED_NOTIFICATION_DATA = LDR_DLL_LOADED_NOTIFICATION_DATA*;

using LDR_DLL_UNLOADED_NOTIFICATION_DATA = struct {
    ULONG            Flags;         // Reserved.
    PCUNICODE_STRING FullDllName;   // The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName;   // The base file name of the DLL module.
    PVOID            DllBase;       // A pointer to the base address for the DLL in memory.
    ULONG            SizeOfImage;   // The size of the DLL image, in bytes.
};
using PLDR_DLL_UNLOADED_NOTIFICATION_DATA = LDR_DLL_UNLOADED_NOTIFICATION_DATA*;
```

As you can see both have exactly the same fields ¯\\_(ツ)_/¯

The actual generic data received by any call-back is this:

```C++
using LDR_DLL_NOTIFICATION_DATA = union {
    LDR_DLL_LOADED_NOTIFICATION_DATA   Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
};
using PLDR_DLL_NOTIFICATION_DATA = LDR_DLL_NOTIFICATION_DATA*;
using PCLDR_DLL_NOTIFICATION_DATA = const LDR_DLL_NOTIFICATION_DATA*;
```

Then, we have the notification function definition and associated values:

```C++
using LDR_DLL_NOTIFICATION_FUNCTION = VOID NTAPI (
  _In_     ULONG                       NotificationReason,
  _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
  _In_opt_ PVOID                       Context
);
using PLDR_DLL_NOTIFICATION_FUNCTION = LDR_DLL_NOTIFICATION_FUNCTION*;

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2
```

After all this, now, we can define the actual function signatures for register, un-register and the call-back:

```C++
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
```

:information_source: Check out the beginning of the file *tutorialdll-tracker-windows.cpp* in this repo

As we mentioned before, when it comes to actually retrieving the library and the functions, we use ***LoadLibraryA*** and ***GetProcAddress***.

```c++
auto ntdll = LoadLibraryA("ntdll.dll");
auto reg   = (LdrRegisterDllNotification)GetProcAddress(ntdll, "LdrRegisterDllNotification");
auto unreg = (LdrUnregisterDllNotification)GetProcAddress(ntdll, "LdrUnregisterDllNotification");
```

> Notice that for this example we did not handle errors for simplicity reasons.

We are going to implement this interface inside a namespace as two global functions. Before we start coding we need to define our data. What we need in Windows is the following:
* A cookie which is basically a handle that is provided for future calls (It is just a ***void\****)
* The pointers to the functions (the ***reg*** and ***unreg*** variables from the previous example)
* The user-passed call-back

It looks like this:

```c++
auto cookie   = (void*)nullptr;
auto callback = callback_t{};
auto reg      = LdrRegisterDllNotification{nullptr};
auto unreg    = LdrUnregisterDllNotification{nullptr};
```

As we want a generic interface we need to have two call-backs. One for the Windows one that will receive all the Windows data structures and another one provided by the user with no platform-dependent data structures. 

The internal one looks like this:

```C++
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
```

Regarding the _start_ function:

```c++
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

    if (reg(0, internal_callback, nullptr, &cookie) != STATUS_SUCCESS) {
        return false;
    }

    callback = move(_callback);

    return true;
}
```

> Here we do check errors

In this order, we:
* Check if we already started. If we just call _stop_ and proceed
* We load the _ntdll.dll_ dynamic library where the Windows functions live.
* We register our internal call-back with the registry function
* Store the user call-back

After calling this, we are ready to receive any _dll_ event.

Similarly, _stop_ code is like this:

```c++
void stop() {
    if (cookie && unreg) {
        unreg(cookie);
    }
    cookie = nullptr;
}
```


This one is obvious.

## Example of usage

In the repo you will find a premake5 script with a workspace (VS solution) with three projects:
* ***_foo_*** that is a dynamic library
* ***_bar_*** that is a dynamic library too
* ***_launcher_*** that is the main

The _main_ function has a local lambda as the call-back:

```c++
const auto cb = [](bool _load, const wstring& _path, const wstring& _name, uintptr_t _base_addr, size_t _size) {
    if (_load) {
        wcout << L"Loading ";
    } else {
        wcout << L"Unload ";
    }

    wcout << "\"" << _name.c_str() << "\" at \"" << _path.c_str() << "\" with";
    wcout << "base addr 0x" << hex << _base_addr << " and size " << dec << _size;
    wcout << endl;
};
```

Then...

```c++
if (qcstudio::dll_tracker::start(cb)) {
    auto foo_module = LoadLibrary(L"foo.dll");
    auto bar_module = LoadLibrary(L"bar.dll");

    if (foo_module) {
        if (auto foo_function = (void (*)())GetProcAddress(foo_module, "foo")) {
            foo_function();
        }
    }
    
    if (bar_module) {
        if (auto bar_function = (void (*)())GetProcAddress(bar_module, "bar")) {
            bar_function();
        }
    }

    if (foo_module) {
        FreeLibrary(foo_module);
    }
    
    if (bar_module) {
        FreeLibrary(bar_module);
    }

    qcstudio::dll_tracker::stop();
}
```

...it just
* Starts tracking
* Loads the two dynamic libraries
* Calls two functions from the libraries
* Unloads the dynamic libraries
* Stop tracking

A possible output of this example is

```
Loading "foo.dll" at "tutorial-dll-tracker\.out\x64\Debug\foo.dll" with base addr 0x7ff8320a0000 and size 159744
Loading "bar.dll" at "tutorial-dll-tracker\.out\x64\Debug\bar.dll" with base addr 0x7ff82f8f0000 and size 159744
This is foo
This is bar
Unload "foo.dll" at "tutorial-dll-tracker\.out\x64\Debug\foo.dll" with base addr 0x7ff8320a0000 and size 159744
Unload "bar.dll" at "tutorial-dll-tracker\.out\x64\Debug\bar.dll" with base addr 0x7ff82f8f0000 and size 159744
```

_Easy peasy lemon squeezy!_

## Other platforms

As we mentioned earlier, we will not cover other platforms. Nevertheless, if you want to go further and investigate how to make this mini-library really multi-platform and as a starting point check out the following tables that show us the correspondence of functions :

How to load/unload dynamic libraries and query for specific functions inside:

| Windows          | Linux     |
| ---------------- | --------- |
| *LoadLibrary*    | *Dlopen*  |
| *GetProcAddress* | *Dlsys*   |
| *FreeLibrary*    | *Dlclose* |

