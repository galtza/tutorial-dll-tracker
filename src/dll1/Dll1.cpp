#include "Dll1.h"
#include "common_lib.h"
#include <iostream>

void dll1_foo() {
    common_lib_foo();
    test::get().foo();
    // g_test.foo();
}