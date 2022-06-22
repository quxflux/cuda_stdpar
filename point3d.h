#pragma once

#include <cmath>

namespace quxflux
{
  struct point3d
  {
    float x{};
    float y{};
    float z{};
  };

  [[nodiscard]] constexpr point3d operator+(const point3d& lhs, const point3d& rhs) noexcept
  {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
  }

  [[nodiscard]] point3d to_spherical(const point3d& cartesian) noexcept
  {
    const auto [x, y, z] = cartesian;
    const auto r = std::sqrt(x * x + y * y + z * z);
    return {r, std::acos(z / r), std::atan2(x, y)};
  }

  [[nodiscard]] point3d to_cartesian(const point3d& spherical) noexcept
  {
    const auto [r, inclination, azimuth] = spherical;
    return {
      r * std::sin(inclination) * std::cos(azimuth),
      r * std::sin(inclination) * std::sin(azimuth),
      r * std::cos(inclination),
    };
  }

  [[nodiscard]] point3d normalize(const point3d& cartesian) noexcept
  {
    const auto [x, y, z] = cartesian;
    const auto m_recip = 1.f / std::sqrt(x * x + y + y + z * z);

    return {x * m_recip, y * m_recip, z * m_recip};
  }
}
