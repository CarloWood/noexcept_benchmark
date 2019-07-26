#ifndef noexcept_benchmark_h
#define noexcept_benchmark_h

/*
Copyright Niels Dekker, LKEB, Leiden University Medical Center

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0.txt

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "cwds/benchmark.h"
#include <cassert>
#include <chrono>
#include <ctime>
#include <climits>
#include <exception>


#ifdef SPECIFY_NOEXCEPT
#  if SPECIFY_NOEXCEPT == 0
#    define OPTIONAL_EXCEPTION_SPECIFIER
#    define LIB_NAME implicit_lib
#  endif
#  if SPECIFY_NOEXCEPT == 1
#    define OPTIONAL_EXCEPTION_SPECIFIER noexcept
#    define LIB_NAME noexcept_lib
#  endif
#endif

#ifndef NOEXCEPT_BENCHMARK_NUMBER_OF_ITERATIONS
#  define NOEXCEPT_BENCHMARK_NUMBER_OF_ITERATIONS 10
#endif

#ifndef NOEXCEPT_BENCHMARK_THROW_EXCEPTION
#  define NOEXCEPT_BENCHMARK_THROW_EXCEPTION 1
#endif


#ifdef NDEBUG
#  ifndef NOEXCEPT_BENCHMARK_NUMBER_OF_INLINE_FUNC_CALLS
#    define NOEXCEPT_BENCHMARK_NUMBER_OF_INLINE_FUNC_CALLS 2147483647 // INT32_MAX (about two billion)
#  endif
#  ifndef NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS
#    define NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS 10 // two hundred million
#  endif
#  ifndef NOEXCEPT_BENCHMARK_NUMBER_OF_CATCHING_RECURSIVE_FUNC_CALLS
#    define NOEXCEPT_BENCHMARK_NUMBER_OF_CATCHING_RECURSIVE_FUNC_CALLS 10 // ten thousand
#endif
#  ifndef NOEXCEPT_BENCHMARK_INC_AND_DEC_FUNC_CALLS
#    define NOEXCEPT_BENCHMARK_INC_AND_DEC_FUNC_CALLS 10 // INT32_MAX (about two billion)
#  endif
#  ifndef NOEXCEPT_BENCHMARK_STACK_UNWINDING_FUNC_CALLS
// Note: On Windows 10, x64, stack overflow occurred with N = 15000
#    define NOEXCEPT_BENCHMARK_STACK_UNWINDING_FUNC_CALLS 10 // ten thousand
#  endif
#  ifndef NOEXCEPT_BENCHMARK_STACK_UNWINDING_OBJECTS
// Note: On Windows 10, x64, stack overflow occurred with N = 1280000
#    define NOEXCEPT_BENCHMARK_STACK_UNWINDING_OBJECTS 10 // a million
#  endif
#  ifndef NOEXCEPT_BENCHMARK_INITIAL_VECTOR_SIZE
#    define NOEXCEPT_BENCHMARK_INITIAL_VECTOR_SIZE 10000000 // ten million
#  endif
#else
#  define NOEXCEPT_BENCHMARK_NUMBER_OF_INLINE_FUNC_CALLS 42
#  define NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS 42
#  define NOEXCEPT_BENCHMARK_NUMBER_OF_CATCHING_RECURSIVE_FUNC_CALLS 42
#  define NOEXCEPT_BENCHMARK_INC_AND_DEC_FUNC_CALLS 42
#  define NOEXCEPT_BENCHMARK_STACK_UNWINDING_FUNC_CALLS 42
#  define NOEXCEPT_BENCHMARK_STACK_UNWINDING_OBJECTS 42
#  define NOEXCEPT_BENCHMARK_INITIAL_VECTOR_SIZE 42
#endif

extern benchmark::Stopwatch stopwatch;

namespace noexcept_benchmark
{
  constexpr double cpu_frequency = 3612059050.0;        // In cycles per second.
  constexpr int cpu = 0;                                // The CPU to run on.
  constexpr size_t loopsize = 1000;                     // We'll be measuring the number of clock cylces needed for this many iterations of the test code.
  constexpr size_t minimum_of = 3;                      // All but the fastest measurement of this many measurements are thrown away (3 is normally enough).
  constexpr int nk = 3;                                 // The number of buckets of FrequencyCounter (with the highest counts) that are averaged over.

  inline void throw_exception_if(const bool do_throw_exception)
  {
    if (do_throw_exception)
    {
      assert(!"This function should only be called with do_throw_exception = false!");
#if NOEXCEPT_BENCHMARK_THROW_EXCEPTION
      throw std::exception{};
#endif
    }
  }


  inline bool get_false()
  {
    // The compiler may not assume that std::time returns non-zero,
    // but in practise, it always does!
    return std::time(nullptr) == 0;
  }


  template <typename T>
  double profile_func_call(T func)
  {
    // The lambda is marked mutable because of the asm() that claims to change m,
    // however - you should not *really* change the input variables!
    // The [m = m] is needed because our m is const and doing just [m] weirdly
    // enough makes the type of the captured m also const, despite the mutable.
    auto result = stopwatch.measure<nk>(loopsize, [/*m = m*/func]() mutable {
        //IACA_START                      // Optional; needed when you want to analyse the generated assembly code with IACA
                                          // (https://software.intel.com/en-us/articles/intel-architecture-code-analyzer).
//      asm volatile ("" : "+r" (m));     // See https://stackoverflow.com/a/54245040/1487069 for an explanation and discussion.
        asm volatile ("");

        // Code under test.
        func();

        asm volatile ("");
//      asm volatile ("" :: "r" (lsb));   // Same.
        //IACA_END                        // Optional; needed when you want to analyse the generated assembly code with IACA.
    }, minimum_of);

    return (result / cpu_frequency * 1e9 / loopsize);
  }
}

#define NOEXCEPT_BENCHMARK_EXCEPTION_SPECIFIER noexcept
#define NOEXCEPT_BENCHMARK_LIB_NAME noexcept_lib
#include "lib/lib.h"
#undef NOEXCEPT_BENCHMARK_LIB_NAME
#undef NOEXCEPT_BENCHMARK_EXCEPTION_SPECIFIER
#define NOEXCEPT_BENCHMARK_EXCEPTION_SPECIFIER
#define NOEXCEPT_BENCHMARK_LIB_NAME implicit_lib
#include "lib/lib.h"
#undef NOEXCEPT_BENCHMARK_LIB_NAME
#undef NOEXCEPT_BENCHMARK_EXCEPTION_SPECIFIER



#endif
