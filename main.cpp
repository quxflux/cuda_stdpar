#include <algorithm>
#include <chrono>
#include <execution>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <numbers>
#include <random>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

#include "point3d.h"

namespace
{
  namespace qf = quxflux;

  namespace layout
  {
    struct structure_of_arrays
    {};

    struct array_of_structures
    {};

    static inline constexpr structure_of_arrays soa;
    static inline constexpr array_of_structures aos;

    template<typename Layout>
    std::string_view name() = delete;

    template<>
    constexpr std::string_view name<structure_of_arrays>()
    {
      return "structure_of_arrays";
    }

    template<>
    constexpr std::string_view name<array_of_structures>()
    {
      return "array_of_structures";
    }
  }  // namespace layout

  std::vector<qf::point3d> generate_random_point_cloud(const std::size_t n)
  {
    std::vector<qf::point3d> points(n);

    std::ranges::generate(points, [rd = std::mt19937{42}]() mutable -> qf::point3d {
      std::uniform_real_distribution<float> dis{};

      return {dis(rd), dis(rd), dis(rd)};
    });

    return points;
  }

  template<typename F, typename... Ts>
  void for_each(const std::tuple<Ts...>& tuple, F&& f)
  {
    [&]<std::size_t... Indices>(const std::index_sequence<Indices...>&)
    {
      (std::invoke(f, std::get<Indices>(tuple)), ...);
    }
    (std::make_index_sequence<sizeof...(Ts)>());
  }

  template<typename Layout>
  auto generate_data(const std::size_t n) = delete;

  template<>
  auto generate_data<layout::array_of_structures>(const std::size_t n)
  {
    return generate_random_point_cloud(n);
  }

  template<>
  auto generate_data<layout::structure_of_arrays>(const std::size_t n)
  {
    struct
    {
      std::vector<float> x;
      std::vector<float> y;
      std::vector<float> z;
    } values;

    for (auto& vec : {std::ref(values.x), std::ref(values.y), std::ref(values.z)})
      vec.get().resize(n);

    const auto cloud = generate_random_point_cloud(n);

    std::ranges::transform(cloud, std::begin(values.x), &qf::point3d::x);
    std::ranges::transform(cloud, std::begin(values.y), &qf::point3d::y);
    std::ranges::transform(cloud, std::begin(values.z), &qf::point3d::z);

    return values;
  }

  template<typename Layout>
  void benchmark(const std::size_t n, const Layout&)
  {
    const auto data = generate_data<Layout>(n);
    const float n_recip = 1.f / n;

    using clock = std::chrono::high_resolution_clock;
    using duration = std::chrono::duration<float, std::milli>;

    std::array<duration, 100u> execution_times;

    std::ranges::generate(execution_times, [&] {
      const auto start = clock::now();

      const auto calculation = [=](const qf::point3d& cartesian) {
        auto [r, theta, phi] = qf::to_spherical(qf::normalize(cartesian));

        theta += std::numbers::pi_v<float> / 8.f;
        phi += std::numbers::pi_v<float> / 4.f;

        return qf::to_cartesian({r, theta, phi}) * n_recip;
      };

      qf::point3d reduced;

      if constexpr (std::same_as<Layout, layout::structure_of_arrays>)
      {
        const std::span data_x{data.x};
        const std::span data_y{data.y};
        const std::span data_z{data.z};

        const auto v = std::views::iota(0, std::int32_t{n});
        reduced = std::transform_reduce(std::execution::par_unseq, std::begin(v), std::end(v), qf::point3d{},
                                        std::plus<>{}, [=](const auto i) {
                                          const auto x = data_x[i];
                                          const auto y = data_y[i];
                                          const auto z = data_z[i];
                                          return calculation({x, y, z});
                                        });
      } else
      {
        const std::span data_view{data};

        reduced = std::transform_reduce(std::execution::par_unseq, std::begin(data_view), std::end(data_view),
                                        qf::point3d{}, std::plus<>{},
                                        [=](const auto& coord) { return calculation(coord); });
      }

      const auto exec_time = std::chrono::duration_cast<duration>(clock::now() - start);

      // print to cout to prevent compiler from outsmarting us
      std::cout << std::fixed << std::setprecision(5) << exec_time.count() << " ms [" << reduced.x << ", " << reduced.y
                << ", " << reduced.z << "]\r";

      return exec_time;
    });

    std::ranges::nth_element(execution_times, std::begin(execution_times) + execution_times.size() / 2);
    const auto median_duration = execution_times[execution_times.size() / 2];

    const float mega_items_per_second =
      (static_cast<float>(n) / std::mega::num) /
      (std::chrono::duration_cast<std::chrono::duration<float>>(median_duration).count());
    std::cout << n << ", " << layout::name<Layout>() << ", " << median_duration.count() << " ms, "
              << mega_items_per_second << " MItems/s" << std::endl;
  }
}  // namespace

int main(int, char**)
{
  for_each(std::tuple{layout::aos, layout::soa}, [](const auto layout) {
    for (const auto n : std::views::iota(1u, 8u) | std::views::transform([](const auto exp) {
                          return static_cast<std::size_t>(std::pow(10u, exp));
                        }))
      benchmark(n, layout);
  });

  return 0;
}
