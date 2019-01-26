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
#include <climits>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
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


  void recursive_func(unsigned short number_of_func_calls) noexcept
  {
    if (--number_of_func_calls > 0)
    {
      dummy_class dummy;
      recursive_func(number_of_func_calls);
    }
  }

  template <unsigned number_of_func_calls>
  void recursive_func_template() noexcept
  {
    dummy_class dummy;
    recursive_func_template<number_of_func_calls - 1>();
  }

  template <>
  void recursive_func_template<0>() noexcept
  {
  }

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

  template <unsigned number_of_func_calls>
  void recursive_func_template()
  {
    dummy_class dummy;
    recursive_func_template<number_of_func_calls - 1>();
  }

  template <>
  void recursive_func_template<0>()
  {
  }


  void recursive_func(unsigned short number_of_func_calls)
  {
    if (--number_of_func_calls > 0)
    {
      dummy_class dummy;
      recursive_func(number_of_func_calls);
    }
  }
}

namespace
{
  const int number_of_iterations = NOEXCEPT_BENCHMARK_NUMBER_OF_ITERATIONS;

  struct durations_type
  {
    double duration_noexcept;
    double duration_implicit;
  };

  
  double divide_by_positive(const double x, const double y)
  {
    return x /
      ((y > 0) ? y : std::numeric_limits<double>::denorm_min());
  }

  template <unsigned N>
  class test_result
  {
    unsigned m_number_of_times_noexcept_is_faster = 0;
    unsigned m_number_of_times_implicit_is_faster = 0;
    double m_sum_of_durations_noexcept = 0.0;
    double m_sum_of_durations_implicit = 0.0;
    double m_shortest_duration_noexcept = std::numeric_limits<double>::infinity();
    double m_shortest_duration_implicit = std::numeric_limits<double>::infinity();
    const char* const m_test_case_name;

  public:

    explicit test_result(const char* const test_case_name)
      :
      m_test_case_name{ test_case_name }
    {
      std::cout << "\n[" << test_case_name << " (N = " << N
        << ")]\n  noexcept \t implicit"
        << std::flush;
    }

    void update_test_result(const durations_type& durations)
    {
      m_sum_of_durations_noexcept += durations.duration_noexcept;
      m_sum_of_durations_implicit += durations.duration_implicit;

      m_shortest_duration_noexcept = std::min(m_shortest_duration_noexcept, durations.duration_noexcept);
      m_shortest_duration_implicit = std::min(m_shortest_duration_implicit, durations.duration_implicit);

      if (durations.duration_noexcept < durations.duration_implicit)
      {
        ++m_number_of_times_noexcept_is_faster;
      }
      if (durations.duration_implicit < durations.duration_noexcept)
      {
        ++m_number_of_times_implicit_is_faster;
      }
    }

    ~test_result()
    {
      std::cout
        << "\nShortest duration: "
        << m_shortest_duration_noexcept
        << " sec. (explicit 'noexcept')"
        << "\nShortest duration: "
        << m_shortest_duration_implicit
        << " sec. (implicit exception specification)"
        << "\nSum of durations: "
        << m_sum_of_durations_noexcept
        << " sec. (explicit 'noexcept')"
        << "\nSum of durations: "
        << m_sum_of_durations_implicit
        << " sec. (implicit exception specification)"
        << "\nRatio sum of durations noexcept/implicit: "
        << divide_by_positive(m_sum_of_durations_noexcept, m_sum_of_durations_implicit)
        << "\nRatio sum of durations implicit/noexcept: "
        << divide_by_positive(m_sum_of_durations_implicit, m_sum_of_durations_noexcept)
        << ((m_number_of_times_implicit_is_faster == 0) ?
          "\nIn this case, 'noexcept' specifications always appear faster." : "")
        << ((m_number_of_times_noexcept_is_faster == 0) ?
          "\nIn this case, implicit exception specifications always appear faster." : "")
        << (((m_number_of_times_noexcept_is_faster > 0) && (m_number_of_times_implicit_is_faster > 0)) ?
          "\nIn this case, neither implicit nor noexcept specifications always appear faster." : "")
        << std::endl;
    }
  };


  template <typename T1, typename T2>
  durations_type profile_func_calls(T1 func1, T2 func2)
  {
    durations_type durations;
    durations.duration_noexcept = profile_func_call(func1);
    durations.duration_implicit = profile_func_call(func2);
    return durations;
  }
    

  template <unsigned N>
  void print_durations_and_update_test_result(
    const durations_type& durations,
    test_result<N>& result)
  {
    std::cout
      << "\n  "
      << durations.duration_noexcept
      << " \t "
      << durations.duration_implicit
      << std::flush;

    result.update_test_result(durations);
  }

}

int main()
{
  std::cout
    << std::fixed
    << std::setprecision(10)
    << "The noexcept benchmark from https://github.com/N-Dekker/noexcept_benchmark"
    << "\n__FILE__ = " << __FILE__
    << "\nsizeof(void*) = " << sizeof(void*)
    << " (" << CHAR_BIT * sizeof(void*) << "-bit)"
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
    << "\nNOEXCEPT_BENCHMARK_NUMBER_OF_ITERATIONS = "
    << NOEXCEPT_BENCHMARK_NUMBER_OF_ITERATIONS
    << "\nNOEXCEPT_BENCHMARK_THROW_EXCEPTION = "
    << NOEXCEPT_BENCHMARK_THROW_EXCEPTION
    << std::endl;
  {
    test_result<NOEXCEPT_BENCHMARK_NUMBER_OF_INLINE_FUNC_CALLS> result(
      "inline function calls");

    for (int iteration_number = 0; iteration_number < number_of_iterations; ++iteration_number)
    {
      durations_type durations;
      durations.duration_noexcept = noexcept_test::test_inline_func();
      durations.duration_implicit = implicit_except_test::test_inline_func();
      print_durations_and_update_test_result(durations, result);
    }
  }
  {
    test_result<NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS> result(
      "exported library function calls");

    for (int iteration_number = 0; iteration_number < number_of_iterations; ++iteration_number)
    {
      enum { number_of_func_calls = NOEXCEPT_BENCHMARK_NUMBER_OF_EXPORTED_FUNC_CALLS };

      const auto durations = profile_func_calls(
        []
      {
        for (int i = 0; i < number_of_func_calls; ++i)
        {
          noexcept_test::exported_func(false);
        }
      },
        []
      {
        for (int i = 0; i < number_of_func_calls; ++i)
        {
          implicit_except_test::exported_func(false);
        }
      });

      print_durations_and_update_test_result(durations, result);
    }
  }
  {
    test_result<NOEXCEPT_BENCHMARK_NUMBER_OF_RECURSIVE_FUNC_CALLS> result(
      "recursive function calls");

    for (int iteration_number = 0; iteration_number < number_of_iterations; ++iteration_number)
    {
      enum { number_of_func_calls = NOEXCEPT_BENCHMARK_NUMBER_OF_RECURSIVE_FUNC_CALLS };

      const auto durations = profile_func_calls(
        []
      {
        noexcept_test::recursive_func(number_of_func_calls);
      },
        []
      {
        implicit_except_test::recursive_func(number_of_func_calls);
      });

      print_durations_and_update_test_result(durations, result);
    }
  }
  {
    test_result<NOEXCEPT_BENCHMARK_NUMBER_OF_RECURSIVE_FUNC_TEMPLATE_CALLS> result(
      "template recursion");

    for (int iteration_number = 0; iteration_number < number_of_iterations; ++iteration_number)
    {
      enum { number_of_func_calls = NOEXCEPT_BENCHMARK_NUMBER_OF_RECURSIVE_FUNC_TEMPLATE_CALLS };

      const auto durations = profile_func_calls(
        []
      {
        noexcept_test::recursive_func_template<number_of_func_calls>();
      },
        []
      {
        implicit_except_test::recursive_func_template<number_of_func_calls>();;
      });

      print_durations_and_update_test_result(durations, result);
    }
  }
  {
    test_result<NOEXCEPT_BENCHMARK_INITIAL_VECTOR_SIZE> result(
      "std::vector<my_string> reserve");

    for (int iteration_number = 0; iteration_number < number_of_iterations; ++iteration_number)
    {
      durations_type durations;
      durations.duration_noexcept = noexcept_test::test_vector_reserve();
      durations.duration_implicit = implicit_except_test::test_vector_reserve();
      print_durations_and_update_test_result(durations, result);
    }
  }

  for (int i = 0; i < 80; ++i)
  {
    std::cout << '=';
  }
  std::cout << std::endl;
  return 0;
}
