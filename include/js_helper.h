/**
 * created by aincvy(aincvy@gmail.com) on 2022/10/3
 */

#pragma once

#include <string_view>

namespace cdf {

    void initJsEngine(std::string_view execPath);

    void destroyJsEngine();

}