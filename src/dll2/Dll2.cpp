#include "Dll2.h"
#include "common_lib.h"
#include <iostream>

thread_local int x;

void dll2_foo() {
    common_lib_foo();
    test::get().foo();
    // g_test.foo();
}
