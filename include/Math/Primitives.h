#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

template <typename T>
struct Vec2 {
    union {
        struct {
            T x, y;
        };

        struct {
            T u, v;
        };

        struct {
            T width, height;
        };

        struct {
            T w, h;
        };

        T data[2];
        T raw[2];
    };

    constexpr Vec2() noexcept : x{}, y{} {}

    constexpr Vec2(T x, T y) noexcept : x(x), y(y) {}

    template <typename U>
    constexpr explicit Vec2(const Vec2<U>& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr Vec2 operator+(const Vec2& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y};
    }

    [[nodiscard]]
    constexpr Vec2 operator-(const Vec2& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y};
    }

    [[nodiscard]]
    constexpr Vec2 operator*(T scalar) const noexcept {
        return {x * scalar, y * scalar};
    }

    [[nodiscard]]
    constexpr Vec2 operator/(T scalar) const noexcept {
        return {x / scalar, y / scalar};
    }

    [[nodiscard]]
    constexpr Vec2 operator*(const Vec2& rhs) const noexcept {
        return {x * rhs.x, y * rhs.y};
    }

    [[nodiscard]]
    constexpr Vec2 operator-() const noexcept {
        return {-x, -y};
    }

    constexpr Vec2& operator+=(const Vec2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2& operator*=(T scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2& operator/=(T scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    constexpr bool operator==(const Vec2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }

    [[nodiscard]]
    constexpr T dot(const Vec2& rhs) const noexcept {
        return x * rhs.x + y * rhs.y;
    }

    [[nodiscard]]
    constexpr T lengthSq() const noexcept {
        return dot(*this);
    }

    [[nodiscard]]
    auto length() const noexcept {
        return std::sqrt(static_cast<double>(lengthSq()));
    }

    [[nodiscard]]
    auto distance(const Vec2& rhs) const noexcept {
        return (*this - rhs).length();
    }

    [[nodiscard]]
    Vec2 normalized() const noexcept {
        auto len = length();
        return (len > 0) ? *this / static_cast<T>(len) : Vec2{};
    }
};

template <typename T>
constexpr Vec2<T> operator*(T scalar, const Vec2<T>& v) noexcept {
    return v * scalar;
}

template <typename T>
struct Vec3 {
    union {
        struct {
            T x, y, z;
        };

        struct {
            T r, g, b;
        };

        struct {
            T u, v, w;
        };

        struct {
            T width, height, depth;
        };

        struct {
            T w_dim, h_dim, d_dim;
        };

        struct {
            T pitch, yaw, roll;
        };

        T data[3];
        T raw[3];
    };

    constexpr Vec3() noexcept : x{}, y{}, z{} {}

    constexpr Vec3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}

    constexpr Vec3(const Vec2<T>& xy, T z) noexcept : x(xy.x), y(xy.y), z(z) {}

    template <typename U>
    constexpr explicit Vec3(const Vec3<U>& other) noexcept
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr Vec3 operator+(const Vec3& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    [[nodiscard]]
    constexpr Vec3 operator-(const Vec3& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    [[nodiscard]]
    constexpr Vec3 operator*(T scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar};
    }

    [[nodiscard]]
    constexpr Vec3 operator/(T scalar) const noexcept {
        return {x / scalar, y / scalar, z / scalar};
    }

    [[nodiscard]]
    constexpr Vec3 operator*(const Vec3& rhs) const noexcept {
        return {x * rhs.x, y * rhs.y, z * rhs.z};
    }

    [[nodiscard]]
    constexpr Vec3 operator-() const noexcept {
        return {-x, -y, -z};
    }

    constexpr Vec3& operator+=(const Vec3& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vec3& operator*=(T scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3& operator/=(T scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    constexpr bool operator==(const Vec3& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z; }

    [[nodiscard]]
    constexpr T dot(const Vec3& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    [[nodiscard]]
    constexpr Vec3 cross(const Vec3& rhs) const noexcept {
        return {y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
    }

    [[nodiscard]]
    constexpr T lengthSq() const noexcept {
        return dot(*this);
    }

    [[nodiscard]]
    auto length() const noexcept {
        return std::sqrt(static_cast<double>(lengthSq()));
    }

    [[nodiscard]]
    auto distance(const Vec3& rhs) const noexcept {
        return (*this - rhs).length();
    }

    [[nodiscard]]
    Vec3 normalized() const noexcept {
        auto len = length();
        return (len > 0) ? *this / static_cast<T>(len) : Vec3{};
    }
};

template <typename T>
constexpr Vec3<T> operator*(T scalar, const Vec3<T>& v) noexcept {
    return v * scalar;
}

template <typename T>
struct Vec4 {
    union {
        struct {
            T x, y, z, w;
        };

        struct {
            T r, g, b, a;
        };

        struct {
            T x0, y0, x1, y1;
        };

        struct {
            T left, top, right, bottom;
        };

        struct {
            T u0, v0, u1, v1;
        };

        T data[4];
        T raw[4];
    };

    constexpr Vec4() noexcept : x{}, y{}, z{}, w{} {}

    constexpr Vec4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

    constexpr Vec4(const Vec3<T>& xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

    template <typename U>
    constexpr explicit Vec4(const Vec4<U>& other) noexcept
        : x(static_cast<T>(other.x)),
          y(static_cast<T>(other.y)),
          z(static_cast<T>(other.z)),
          w(static_cast<T>(other.w)) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr Vec4 operator+(const Vec4& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
    }

    [[nodiscard]]
    constexpr Vec4 operator-(const Vec4& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
    }

    [[nodiscard]]
    constexpr Vec4 operator*(T scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }

    [[nodiscard]]
    constexpr Vec4 operator/(T scalar) const noexcept {
        return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    [[nodiscard]]
    constexpr Vec4 operator*(const Vec4& rhs) const noexcept {
        return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w};
    }

    [[nodiscard]]
    constexpr Vec4 operator-() const noexcept {
        return {-x, -y, -z, -w};
    }

    constexpr Vec4& operator+=(const Vec4& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    constexpr Vec4& operator-=(const Vec4& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    constexpr Vec4& operator*=(T scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    constexpr Vec4& operator/=(T scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    constexpr bool operator==(const Vec4& rhs) const noexcept {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

    [[nodiscard]]
    constexpr T dot(const Vec4& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    [[nodiscard]]
    constexpr T lengthSq() const noexcept {
        return dot(*this);
    }

    [[nodiscard]]
    auto length() const noexcept {
        return std::sqrt(static_cast<double>(lengthSq()));
    }

    [[nodiscard]]
    auto distance(const Vec4& rhs) const noexcept {
        return (*this - rhs).length();
    }

    [[nodiscard]]
    Vec4 normalized() const noexcept {
        auto len = length();
        return (len > 0) ? *this / static_cast<T>(len) : Vec4{};
    }
};

template <typename T>
constexpr Vec4<T> operator*(T scalar, const Vec4<T>& v) noexcept {
    return v * scalar;
}

template <typename T = uint8_t>
struct ColorBGRA {
    union {
        struct {
            T b, g, r, a;
        };

        struct {
            T blue, green, red, alpha;
        };

        T data[4];
        T raw[4];
    };

    constexpr ColorBGRA() noexcept : b{}, g{}, r{}, a{static_cast<T>(255)} {}

    constexpr ColorBGRA(T b, T g, T r, T a = static_cast<T>(255)) noexcept : b(b), g(g), r(r), a(a) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    constexpr bool operator==(const ColorBGRA& rhs) const noexcept {
        return b == rhs.b && g == rhs.g && r == rhs.r && a == rhs.a;
    }
};

template <typename T = uint8_t>
struct ColorRGBA {
    union {
        struct {
            T r, g, b, a;
        };

        struct {
            T red, green, blue, alpha;
        };

        struct {
            T x, y, z, w;
        };

        T data[4];
        T raw[4];
    };

    constexpr ColorRGBA() noexcept : r{}, g{}, b{}, a{static_cast<T>(255)} {}

    constexpr ColorRGBA(T r, T g, T b, T a = static_cast<T>(255)) noexcept : r(r), g(g), b(b), a(a) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    constexpr bool operator==(const ColorRGBA& rhs) const noexcept {
        return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
    }
};

template <typename T>
struct Quaternion {
    union {
        struct {
            T x, y, z, w;
        };

        struct {
            T i, j, k, real;
        };

        T data[4];
        T raw[4];
    };

    constexpr Quaternion() noexcept : x{}, y{}, z{}, w{static_cast<T>(1)} {}

    constexpr Quaternion(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr Quaternion conjugate() const noexcept {
        return {-x, -y, -z, w};
    }

    [[nodiscard]]
    constexpr T normSq() const noexcept {
        return x * x + y * y + z * z + w * w;
    }

    [[nodiscard]]
    auto norm() const noexcept {
        return std::sqrt(static_cast<double>(normSq()));
    }

    [[nodiscard]]
    Quaternion normalized() const noexcept {
        auto n = norm();
        return (n > 0)
            ? Quaternion{static_cast<T>(x / n), static_cast<T>(y / n), static_cast<T>(z / n), static_cast<T>(w / n)}
            : Quaternion{};
    }

    [[nodiscard]]
    constexpr Quaternion operator*(const Quaternion& q) const noexcept {
        return {w * q.x + x * q.w + y * q.z - z * q.y, w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w, w * q.w - x * q.x - y * q.y - z * q.z};
    }
};

template <typename T, size_t Rows, size_t Cols>
struct Matrix {
    union {
        T m[Rows][Cols];
        T data[Rows * Cols];
    };

    constexpr Matrix() noexcept : m{} {}

    [[nodiscard]]
    static constexpr Matrix identity() noexcept {
        static_assert(Rows == Cols, "Identity matrix requires square dimensions.");
        Matrix res{};
        for (size_t i = 0; i < Rows; ++i) {
            res.m[i][i] = static_cast<T>(1);
        }
        return res;
    }

    [[nodiscard]]
    constexpr T* operator[](size_t row) noexcept {
        return m[row];
    }

    [[nodiscard]]
    constexpr const T* operator[](size_t row) const noexcept {
        return m[row];
    }

    template <size_t OtherCols>
    [[nodiscard]]
    constexpr Matrix<T, Rows, OtherCols> operator*(const Matrix<T, Cols, OtherCols>& rhs) const noexcept {
        Matrix<T, Rows, OtherCols> res{};
        for (size_t r = 0; r < Rows; ++r) {
            for (size_t c = 0; c < OtherCols; ++c) {
                T sum{};
                for (size_t k = 0; k < Cols; ++k) {
                    sum += m[r][k] * rhs.m[k][c];
                }
                res.m[r][c] = sum;
            }
        }
        return res;
    }

    [[nodiscard]]
    constexpr Matrix<T, Cols, Rows> transposed() const noexcept {
        Matrix<T, Cols, Rows> res{};
        for (size_t r = 0; r < Rows; ++r) {
            for (size_t c = 0; c < Cols; ++c) {
                res.m[c][r] = m[r][c];
            }
        }
        return res;
    }
};

template <typename T>
using Mat3x3 = Matrix<T, 3, 3>;

template <typename T>
using Mat4x4 = Matrix<T, 4, 4>;

template <typename T>
using Mat4x3 = Matrix<T, 4, 3>;

template <typename T>
[[nodiscard]]
constexpr Vec3<T> operator*(const Mat3x3<T>& mat, const Vec3<T>& v) noexcept {
    return {mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z,
        mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z,
        mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z};
}

template <typename T>
[[nodiscard]]
constexpr Vec4<T> operator*(const Mat4x4<T>& mat, const Vec4<T>& v) noexcept {
    return {mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z + mat.m[0][3] * v.w,
        mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z + mat.m[1][3] * v.w,
        mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z + mat.m[2][3] * v.w,
        mat.m[3][0] * v.x + mat.m[3][1] * v.y + mat.m[3][2] * v.z + mat.m[3][3] * v.w};
}

#pragma pack(push, 2)

template <typename T>
struct Rect {
    union {
        struct {
            T min_y, min_x, max_y, max_x;
        };

        struct {
            T bottom, left, top, right;
        };

        struct {
            T v1, u0, v0, u1;
        };

        struct {
            T y0, x0, y1, x1;
        };

        T data[4];
        T raw[4];
    };

    constexpr Rect() noexcept : bottom{}, left{}, top{}, right{} {}

    constexpr Rect(T bottom, T left, T top, T right) noexcept : bottom(bottom), left(left), top(top), right(right) {}

    [[nodiscard]]
    constexpr T& operator[](size_t index) noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr const T& operator[](size_t index) const noexcept {
        return data[index];
    }

    [[nodiscard]]
    constexpr T width() const noexcept {
        return right >= left ? (right - left) : (left - right);
    }

    [[nodiscard]]
    constexpr T height() const noexcept {
        return bottom >= top ? (bottom - top) : (top - bottom);
    }

    [[nodiscard]]
    constexpr T area() const noexcept {
        return width() * height();
    }

    [[nodiscard]]
    constexpr Vec2<T> minPoint() const noexcept {
        return {(std::min)(left, right), (std::min)(top, bottom)};
    }

    [[nodiscard]]
    constexpr Vec2<T> maxPoint() const noexcept {
        return {(std::max)(left, right), (std::max)(top, bottom)};
    }

    [[nodiscard]]
    constexpr bool contains(const Vec2<T>& pt) const noexcept {
        auto [min_x, min_y] = minPoint();
        auto [max_x, max_y] = maxPoint();
        return pt.x >= min_x && pt.x <= max_x && pt.y >= min_y && pt.y <= max_y;
    }

    // clamps into [min_x, max_x] x [min_y, max_y] as stored, without normalizing
    [[nodiscard]]
    constexpr Vec2<T> clamp(const Vec2<T>& pt) const noexcept {
        return {std::clamp(pt.x, min_x, max_x), std::clamp(pt.y, min_y, max_y)};
    }

    [[nodiscard]]
    constexpr bool intersects(const Rect& other) const noexcept {
        auto [min_x1, min_y1] = minPoint();
        auto [max_x1, max_y1] = maxPoint();
        auto [min_x2, min_y2] = other.minPoint();
        auto [max_x2, max_y2] = other.maxPoint();

        return !(min_x1 > max_x2 || max_x1 < min_x2 || min_y1 > max_y2 || max_y1 < min_y2);
    }
};

#pragma pack(pop)

using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2i = Vec2<int32_t>;
using Vec2u = Vec2<uint32_t>;
using Vec2i16 = Vec2<int16_t>;
using Vec2u16 = Vec2<uint16_t>;
using Vec2i8 = Vec2<int8_t>;
using Vec2u8 = Vec2<uint8_t>;

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Vec3i = Vec3<int32_t>;
using Vec3u = Vec3<uint32_t>;
using Vec3i16 = Vec3<int16_t>;
using Vec3u16 = Vec3<uint16_t>;
using Vec3i8 = Vec3<int8_t>;
using Vec3u8 = Vec3<uint8_t>;

using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;
using Vec4i = Vec4<int32_t>;
using Vec4u = Vec4<uint32_t>;
using Vec4i16 = Vec4<int16_t>;
using Vec4u16 = Vec4<uint16_t>;
using Vec4i8 = Vec4<int8_t>;
using Vec4u8 = Vec4<uint8_t>;

using Quatf = Quaternion<float>;
using Quatd = Quaternion<double>;

using Mat3f = Mat3x3<float>;
using Mat4f = Mat4x4<float>;

using Rectf = Rect<float>;
using Recti = Rect<int32_t>;
using Rectu = Rect<uint32_t>;
using Recti16 = Rect<int16_t>;
using Rectu16 = Rect<uint16_t>;
using Recti8 = Rect<int8_t>;
using Rectu8 = Rect<uint8_t>;

struct AaBox {
    Vec3f min;
    Vec3f max;

    constexpr AaBox() noexcept : min{}, max{} {}

    constexpr AaBox(const Vec3f& min, const Vec3f& max) noexcept : min(min), max(max) {}

    [[nodiscard]]
    constexpr bool operator==(const AaBox& rhs) const noexcept {
        return min == rhs.min && max == rhs.max;
    }

    [[nodiscard]]
    constexpr Vec3f center() const noexcept {
        return (min + max) * 0.5f;
    }

    [[nodiscard]]
    constexpr Vec3f extents() const noexcept {
        return (max - min) * 0.5f;
    }

    [[nodiscard]]
    constexpr Vec3f size() const noexcept {
        return max - min;
    }

    [[nodiscard]]
    constexpr float volume() const noexcept {
        auto s = size();
        return s.x * s.y * s.z;
    }

    [[nodiscard]]
    constexpr bool contains(const Vec3f& pt) const noexcept {
        return pt.x >= min.x && pt.x <= max.x && pt.y >= min.y && pt.y <= max.y && pt.z >= min.z && pt.z <= max.z;
    }

    [[nodiscard]]
    constexpr bool contains(const AaBox& other) const noexcept {
        return contains(other.min) && contains(other.max);
    }

    [[nodiscard]]
    constexpr bool intersects(const AaBox& other) const noexcept {
        return !(min.x > other.max.x || max.x < other.min.x || min.y > other.max.y || max.y < other.min.y ||
            min.z > other.max.z || max.z < other.min.z);
    }

    [[nodiscard]]
    constexpr AaBox merged(const AaBox& other) const noexcept {
        return {
            {(std::min)(min.x, other.min.x), (std::min)(min.y, other.min.y), (std::min)(min.z, other.min.z)},
            {(std::max)(max.x, other.max.x), (std::max)(max.y, other.max.y), (std::max)(max.z, other.max.z)}
        };
    }

    [[nodiscard]]
    constexpr AaBox merged(const Vec3f& pt) const noexcept {
        return {
            {(std::min)(min.x, pt.x), (std::min)(min.y, pt.y), (std::min)(min.z, pt.z)},
            {(std::max)(max.x, pt.x), (std::max)(max.y, pt.y), (std::max)(max.z, pt.z)}
        };
    }
};

static_assert(sizeof(AaBox) == 0x18);

struct AaSphere {
    Vec3f position;
    float radius;

    constexpr AaSphere() noexcept : position{}, radius{} {}

    constexpr AaSphere(const Vec3f& position, float radius) noexcept : position(position), radius(radius) {}

    [[nodiscard]]
    constexpr bool operator==(const AaSphere& rhs) const noexcept {
        return position == rhs.position && radius == rhs.radius;
    }

    [[nodiscard]]
    constexpr bool contains(const Vec3f& pt) const noexcept {
        return (pt - position).lengthSq() <= radius * radius;
    }

    [[nodiscard]]
    constexpr bool intersects(const AaSphere& other) const noexcept {
        auto r = radius + other.radius;
        return (other.position - position).lengthSq() <= r * r;
    }

    [[nodiscard]]
    constexpr bool intersects(const AaBox& box) const noexcept {
        Vec3f closest{std::clamp(position.x, box.min.x, box.max.x), std::clamp(position.y, box.min.y, box.max.y),
            std::clamp(position.z, box.min.z, box.max.z)};
        return (closest - position).lengthSq() <= radius * radius;
    }
};

static_assert(sizeof(AaSphere) == 0x10);

struct Bounds {
    AaBox extent;
    float radius;

    constexpr Bounds() noexcept : extent{}, radius{} {}

    constexpr Bounds(const AaBox& extent, float radius) noexcept : extent(extent), radius(radius) {}

    [[nodiscard]]
    constexpr bool operator==(const Bounds& rhs) const noexcept {
        return extent == rhs.extent && radius == rhs.radius;
    }

    [[nodiscard]]
    constexpr Vec3f center() const noexcept {
        return extent.center();
    }

    [[nodiscard]]
    constexpr AaSphere boundingSphere() const noexcept {
        return {extent.center(), radius};
    }

    [[nodiscard]]
    constexpr bool contains(const Vec3f& pt) const noexcept {
        return extent.contains(pt);
    }

    [[nodiscard]]
    constexpr bool intersects(const Bounds& other) const noexcept {
        return extent.intersects(other.extent);
    }
};

static_assert(sizeof(Bounds) == 0x1C);

struct Plane {
    Vec3f normal;
    float distance;

    constexpr Plane() noexcept : normal{}, distance{} {}

    constexpr Plane(const Vec3f& normal, float distance) noexcept : normal(normal), distance(distance) {}

    [[nodiscard]]
    constexpr bool operator==(const Plane& rhs) const noexcept {
        return normal == rhs.normal && distance == rhs.distance;
    }

    [[nodiscard]]
    constexpr float signedDistance(const Vec3f& pt) const noexcept {
        return normal.dot(pt) + distance;
    }

    [[nodiscard]]
    constexpr bool isFront(const Vec3f& pt) const noexcept {
        return signedDistance(pt) > 0.0f;
    }

    [[nodiscard]]
    Plane normalized() const noexcept {
        auto len = normal.length();
        return (len > 0) ? Plane{normal / static_cast<float>(len), static_cast<float>(distance / len)} : Plane{};
    }
};

static_assert(sizeof(Plane) == 0x10);
