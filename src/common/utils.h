#pragma once

#include <iostream>

struct static_lifespan_tracker_t {
    const char* class_name_;

    static_lifespan_tracker_t(const char* _class_name)
        : class_name_(_class_name) {
        std::cout << "\"" << class_name_ << "\" static init" << std::endl;
    }

    ~static_lifespan_tracker_t() {
        std::cout << "\"" << class_name_ << "\" static end" << std::endl;
    }
};
