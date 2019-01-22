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

#include "noexcept_benchmark.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <iostream>
#include <string>

using namespace noexcept_benchmark;

namespace noexcept_test
{
  NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT void exported_func(bool do_throw_exception) noexcept;
  NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT double test_inline_func();
  NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT double test_vector_reserve();

  class NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT dummy_class
  {
  public:
    dummy_class() noexcept;
    ~dummy_class();
  };
}

namespace implicit_except_test
{
  NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT void exported_func(bool do_throw_exception);
  NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT double test_inline_func();
  NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT double test_vector_reserve();

  class NOEXCEPT_BENCHMARK_SHARED_LIB_IMPORT dummy_class
  {
  public:
    dummy_class(); // No noexcept
    ~dummy_class();
  };
}

namespace
{
  const int max_number_of_times = 4;

  template <typename T>
  void recursive_func(unsigned short numberOfFuncCalls)
  {
    T dummy;

    if (--numberOfFuncCalls > 0)
    {
      recursive_func<T>(numberOfFuncCalls);
    }
  }

  struct test_result
  {
    unsigned number_of_times_noexcept_is_faster = 0;
    unsigned number_of_times_implicit_is_faster = 0;
    double sum_of_durations_noexcept = 0.0;
    double sum_of_durations_implicit = 0.0;
  };


  template <typename T1, typename T2>
  std::pair<double, double> profile_func_calls(T1 func1, T2 func2)
  {
    return std::make_pair(profile_func_call(func1), profile_func_call(func2));
  }
    

  std::string duration_seconds_to_string(const double seconds)
  {
    return "\tDuration = " + std::to_string(seconds) + " sec. ";
  }
  
  
  void print_durations(const std::pair<double, double> durations)
  {
    std::cout
      << duration_seconds_to_string(durations.first)
      << "(explicitly defined 'noexcept')\n"
      << duration_seconds_to_string(durations.second)
      << "(implicitly defined exception specification)\n"
      << std::flush;
  }


  void update_test_result(test_result& result, const std::pair<double, double> durations)
  {
    result.sum_of_durations_noexcept += durations.first;
    result.sum_of_durations_implicit += durations.second;

    if (durations.first < durations.second)
    {
      ++result.number_of_times_noexcept_is_faster;
    }
    if (durations.second < durations.first)
    {
      ++result.number_of_times_implicit_is_faster;
    }
  }

  double divide_by_positive(const double x, const double y)
  {
    return x /
      ((y > 0) ? y : std::numeric_limits<double>::denorm_min());
  }


  void print_conclusion(const test_result& result)
  {
    if (result.number_of_times_noexcept_is_faster == max_number_of_times)
    {
      std::cout << "So for this test case, 'noexcept' seems approximately "
        << divide_by_positive(result.sum_of_durations_implicit, result.sum_of_durations_noexcept)
        << " x faster.";
    }
    else
    {
      if (result.number_of_times_implicit_is_faster == max_number_of_times)
      {
        std::cout
          << "So for this test case, an implicitly defined exception specification seems approximately "
          << divide_by_positive(result.sum_of_durations_noexcept, result.sum_of_durations_implicit)
          << " x faster.";
      }
      else
      {
        std::cout << "So for this test case, it seems unclear whether 'noexcept' or implicit is faster.";
      }
    }
    std::cout << std::endl;
  }

}

int main()
{
  std::cout
    << "__FILE__ = " << __FILE__
    << "\nsizeof(void*) = " << sizeof(void*)
    << "\n__DATE__ = " << __DATE__
    << "\n__TIME__ = " << __TIME__
#ifdef __VERSION__
    << "\n__VERSION__ = "
    __VERSION__
#endif
#ifdef _MSC_FULL_VER
    << "\n_MSC_FULL_VER = "
    << _MSC_FULL_VER
#endif
#ifdef _MSC_BUILD
    << "\n_MSC_BUILD = "
    << _MSC_BUILD
#endif
#ifdef _DEBUG
    << "\n_DEBUG"
#endif
#ifdef NDEBUG
    << "\nNDEBUG (\"Not Debug\")"
#endif
    << std::endl;


  {
    test_result result;

    std::cout << "\n[test_inline_func (N = "
      << NOEXCEPT_BENCHMARK_NUMBER_OF_INLINE_FUNC_CALLS
      << ")]"
      << std::endl;

    for (int numberOfTimes = 0; numberOfTimes < max_number_of_times; ++numberOfTimes)
    {
      const auto durations = std::make_pair(
        noexcept_test::test_inline_func(),
        implicit_except_test::test_inline_func());
      print_durations(durations);
      update_test_result(result, durations);
    }
    print_conclusion(result);
  }
  {
    test_result result;

    std::cout << "\n[recursive_func (N = "
      << NOEXCEPT_BENCHMARK_NUMBER_OF_RECURSIVE_FUNC_CALLS
      << ")]"
      << std::endl;

    for (int numberOfTimes = 0; numberOfTimes < max_number_of_times; ++numberOfTimes)
    {
      enum { numberOfFuncCalls = NOEXCEPT_BENCHMARK_NUMBER_OF_RECURSIVE_FUNC_CALLS };

      const auto durations = profile_func_calls(
        []
      {
        recursive_func<noexcept_test::dummy_class>(numberOfFuncCalls);
      },
        []
      {
        recursive_func<implicit_except_test::dummy_class>(numberOfFuncCalls);
      });

      print_durations(durations);
      update_test_result(result, durations);
    }
    print_conclusion(result);
  }

  {
    test_result result;

    std::cout << "\n[exported_func(false) calls (N = "
      << NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS
      << ")]"
      << std::endl;

    for (int numberOfTimes = 0; numberOfTimes < max_number_of_times; ++numberOfTimes)
    {
      enum { numberOfFuncCalls = NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS };

      const auto durations = profile_func_calls(
        []
      {
        for (int i = 0; i < numberOfFuncCalls; ++i)
        {
          noexcept_test::exported_func(false);
        }
      },
        []
      {
        for (int i = 0; i < numberOfFuncCalls; ++i)
        {
          implicit_except_test::exported_func(false);
        }
      });
      print_durations(durations);
      update_test_result(result, durations);
    }
    print_conclusion(result);
  }
  {
    test_result result;

    std::cout << "\n[test_vector_reserve (N = "
      << NOEXCEPT_BENCHMARK_INITIAL_VECTOR_SIZE
      << ")]"
      << std::endl;

    for (int numberOfTimes = 0; numberOfTimes < max_number_of_times; ++numberOfTimes)
    {
      const auto durations = std::make_pair(
        noexcept_test::test_vector_reserve(),
        implicit_except_test::test_vector_reserve());
      print_durations(durations);
      update_test_result(result, durations);
    }
    print_conclusion(result);
  }
  return 0;
}