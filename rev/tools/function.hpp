//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include <functional>

namespace REV
{
    // @NOTE(Roman): Function has been implemented incorrectly
    //               and I don't want to rewrite it in the near future
    //
    //               P.S. C++ is a terrible language in theese moments.
    template<typename FunctionType>
    using Function = std::function<FunctionType>;
}
