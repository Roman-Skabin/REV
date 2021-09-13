//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/common.h"
#include <functional>

namespace REV
{
    // @NOTE(Roman): I don't know how to implement Function adequately,
    //               so I'm good with STL's one (no). C++ is a terrible language
    //               in these moments. I mean... I can't even inspect function's return type
    //               and arguments' types without writing hundreds lines of templated code
    //               with all that C++'s freaking metaprogramming stuff!

    // @Important(Roman): DO NOT USE IT IN MULTITHREADED CODE WITHOUT CRITICAL SECTIONS OR MUTEXES!!!
    //                    Use function pointers wherever you can. std::function is not thread safety!
    //                    So if you store std::function in some global (common for several threads) storage
    //                    and you want to change it from time to time (like in the WorkQueue::AddWork)
    //                    you have to synchronize that (re)assignment with a critical section or mutex
    //                    because your other thread can call it before it will be assigned (yeah STL is freaking slow).
    //                    Ok, maybe the only good way to synchronize it is with some kind of condition variable
    //                    or with some flag which state you change with interlocked functions.

    // @Issue(Roman): So maybe just get rid of std::function?
    //                But then we won't be able to pass lambdas as callbacks.
    //                Hmm...

    template<typename SomethingCallable>
    using Function = std::function<SomethingCallable>;
}
