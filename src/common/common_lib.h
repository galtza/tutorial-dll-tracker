#pragma once

#include <iostream>
#include <thread>
#include <cstdint>
#include <intrin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <minwindef.h>
#include <winnt.h>
#include <subauth.h>
#include <libloaderapi.h>

__declspec(noinline) inline auto get_ip() {
    return _ReturnAddress();
}

__declspec(noinline) inline void common_lib_foo() {
    std::cout << "common_lib_foo (addr = 0x" << std::hex << &common_lib_foo << "; pc = 0x" << (uintptr_t)get_ip() << ")" << std::endl;
}

static uint32_t INDEX;

struct test {
    __declspec(noinline) test() {
        auto x = TlsGetValue(1);
        if (!x) {
            TlsSetValue(1, this);
        }
        auto id = std::this_thread::get_id();
        auto _1 = &INDEX;
        std::cout << "test::test() -> 0x" << std::hex << (uintptr_t)this << "; thread = 0x" << id << std::endl;
        std::cout << "_1 -> 0x" << std::hex << (uintptr_t)_1 << std::endl;
        value = 12;
    }

    static int get_tls_index() {
        return 0;
    }

    static test& get() {
        static test* ret = nullptr;
        if (!ret) {
            // Try to access the TLS
        }
        return *ret;
    }

    void foo() {
        std::cout << "foo() -> 0x" << std::hex << (uintptr_t)this << std::endl;
    }

    int value;
};

// extern test g_test;