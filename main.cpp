#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <numeric>
#include <random>
#include <ranges>
#include <span>
#include <string_view>
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
  void benchmark(const Layout&)
  {
    constexpr std::size_t n = 10'000'000;
    const auto data = generate_data<Layout>(n);
    const float n_recip = 1.f / n;

    using clock = std::chrono::high_resolution_clock;

    for (std::size_t i = 0; i < 10; ++i)
    {
      const auto start = clock::now();

      constexpr auto calculation = [](const qf::point3d& cartesian) {
        auto [r, theta, phi] = qf::to_spherical(qf::normalize(cartesian));

        constexpr float pi = 3.1415926535f;

        theta += pi / 8.f;
        phi += pi / 4.f;

        return qf::to_cartesian({r, theta, phi});
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

      reduced.x = reduced.x * n_recip;
      reduced.y = reduced.y * n_recip;
      reduced.z = reduced.z * n_recip;

      const auto duration = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(clock::now() - start);

      const float mega_items_per_second = static_cast<float>(n) / 1'000'000 / (duration.count() / 1000);
      std::cout << layout::name<Layout>() << ": " << reduced.x << ", " << reduced.y << ", " << reduced.z << ", took "
                << duration.count() << " ms, " << mega_items_per_second << " MItems/s" << std::endl;
    }
  }
}  // namespace

int main(int, char**)
{
  benchmark(layout::soa);
  benchmark(layout::aos);

  return 0;
}
