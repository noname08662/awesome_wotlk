#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

class WuQuantizer {
    template <typename T>
    class Array3D {
    public:
        Array3D() : data_(33 * 33 * 33, T{}) {}

        T& operator()(size_t r, size_t g, size_t b) { return data_[(r * 33 + g) * 33 + b]; }

        const T& operator()(size_t r, size_t g, size_t b) const { return data_[(r * 33 + g) * 33 + b]; }

    private:
        std::vector<T> data_;
    };

    class Moment {
        friend class WuQuantizer;

        int64_t w_ = 0;
        int64_t r_ = 0;
        int64_t g_ = 0;
        int64_t b_ = 0;
        double m2_ = 0.0;

    public:
        Moment() = default;

        Moment(int64_t w, int64_t r, int64_t g, int64_t b, double m2) : w_(w), r_(r), g_(g), b_(b), m2_(m2) {}

        Moment& operator+=(const Moment& o) {
            w_ += o.w_;
            r_ += o.r_;
            g_ += o.g_;
            b_ += o.b_;
            m2_ += o.m2_;
            return *this;
        }

        Moment operator+(const Moment& o) const { return {w_ + o.w_, r_ + o.r_, g_ + o.g_, b_ + o.b_, m2_ + o.m2_}; }

        Moment operator-(const Moment& o) const { return {w_ - o.w_, r_ - o.r_, g_ - o.g_, b_ - o.b_, m2_ - o.m2_}; }
    };

public:
    struct Box {
        int r0 = 0, r1 = 0;
        int g0 = 0, g1 = 0;
        int b0 = 0, b1 = 0;
        int vol = 0;
    };

    static void quantize(const std::vector<uint8_t>& rgba_data, std::span<uint32_t, 256> palette, int& out_num_colors,
        std::vector<uint8_t>& out_indices, int requested_colors, std::vector<uint8_t>* out_classify_table) {
        size_t pixel_count = rgba_data.size() / 4;
        if (pixel_count == 0) {
            out_num_colors = 0;
            out_indices.clear();
            return;
        }

        Array3D<Moment> mom;

        for (size_t i = 0; i < pixel_count; ++i) {
            uint8_t r = rgba_data[i * 4 + 0];
            uint8_t g = rgba_data[i * 4 + 1];
            uint8_t b = rgba_data[i * 4 + 2];
            uint8_t a = rgba_data[i * 4 + 3];

            if (a > 0) {
                int inr = (r >> 3) + 1;
                int ing = (g >> 3) + 1;
                int inb = (b >> 3) + 1;

                Moment& m = mom(inr, ing, inb);
                m.w_ += 1;
                m.r_ += r;
                m.g_ += g;
                m.b_ += b;
                m.m2_ += static_cast<double>(r) * r + static_cast<double>(g) * g + static_cast<double>(b) * b;
            }
        }

        for (int r = 1; r <= 32; ++r) {
            std::array<Moment, 33> area = {};

            for (int g = 1; g <= 32; ++g) {
                Moment line{};

                for (int b = 1; b <= 32; ++b) {
                    line += mom(r, g, b);
                    area[static_cast<size_t>(b)] += line;
                    mom(r, g, b) = mom(r - 1, g, b) + area[static_cast<size_t>(b)];
                }
            }
        }

        std::vector<Box> cube(256);
        cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
        cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;

        std::vector vv(256, 0.0);
        int kk = std::min(std::max(1, requested_colors), 256);
        int next = 0;

        for (int i = 1; i < kk; ++i) {
            if (cut(cube[next], cube[i], mom)) {
                vv[next] = (cube[next].vol > 1) ? var(cube[next], mom) : 0.0;
                vv[i] = (cube[i].vol > 1) ? var(cube[i], mom) : 0.0;
            } else {
                vv[next] = 0.0;
                i--;
            }

            next = 0;
            double temp = vv[0];
            for (int k = 1; k <= i; ++k) {
                if (vv[k] > temp) {
                    temp = vv[k];
                    next = k;
                }
            }

            if (temp <= 0.0) {
                kk = i + 1;
                break;
            }
        }

        out_num_colors = kk;

        std::vector<uint8_t> tag(33 * 33 * 33, 0);

        for (int k = 0; k < kk; ++k) {
            for (int r = cube[k].r0 + 1; r <= cube[k].r1; ++r) {
                for (int g = cube[k].g0 + 1; g <= cube[k].g1; ++g) {
                    for (int b = cube[k].b0 + 1; b <= cube[k].b1; ++b) {
                        tag[(r * 33 + g) * 33 + b] = static_cast<uint8_t>(k);
                    }
                }
            }

            Moment v = vol(cube[k], mom);
            uint8_t r = 0, g = 0, b = 0;
            if (v.w_ > 0) {
                r = static_cast<uint8_t>((v.r_ + (v.w_ / 2)) / v.w_);
                g = static_cast<uint8_t>((v.g_ + (v.w_ / 2)) / v.w_);
                b = static_cast<uint8_t>((v.b_ + (v.w_ / 2)) / v.w_);
            }

            palette[static_cast<size_t>(k)] = 0xFF000000U | (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
        }

        for (int k = kk; k < 256; ++k) {
            palette[static_cast<size_t>(k)] = 0;
        }

        out_indices.resize(pixel_count);
        for (size_t i = 0; i < pixel_count; ++i) {
            int inr = (rgba_data[i * 4 + 0] >> 3) + 1;
            int ing = (rgba_data[i * 4 + 1] >> 3) + 1;
            int inb = (rgba_data[i * 4 + 2] >> 3) + 1;
            out_indices[i] = tag[(inr * 33 + ing) * 33 + inb];
        }

        if (out_classify_table != nullptr) { *out_classify_table = std::move(tag); }
    }

    static uint8_t classify(const std::vector<uint8_t>& classify_table, uint8_t r, uint8_t g, uint8_t b) {
        int inr = (r >> 3) + 1;
        int ing = (g >> 3) + 1;
        int inb = (b >> 3) + 1;
        return classify_table[(inr * 33 + ing) * 33 + inb];
    }

private:
    static Moment vol(const Box& cube, const Array3D<Moment>& mom) {
        return mom(cube.r1, cube.g1, cube.b1) - mom(cube.r1, cube.g1, cube.b0) - mom(cube.r1, cube.g0, cube.b1) +
            mom(cube.r1, cube.g0, cube.b0) - mom(cube.r0, cube.g1, cube.b1) + mom(cube.r0, cube.g1, cube.b0) +
            mom(cube.r0, cube.g0, cube.b1) - mom(cube.r0, cube.g0, cube.b0);
    }

    static Moment bottom(const Box& cube, int dir, const Array3D<Moment>& mom) {
        switch (dir) {
            case 2:  // RED
                return (mom(cube.r0, cube.g1, cube.b0) + mom(cube.r0, cube.g0, cube.b1)) -
                    (mom(cube.r0, cube.g1, cube.b1) + mom(cube.r0, cube.g0, cube.b0));
            case 1:  // GREEN
                return (mom(cube.r1, cube.g0, cube.b0) + mom(cube.r0, cube.g0, cube.b1)) -
                    (mom(cube.r1, cube.g0, cube.b1) + mom(cube.r0, cube.g0, cube.b0));
            case 0:  // BLUE
            default:
                return (mom(cube.r1, cube.g0, cube.b0) + mom(cube.r0, cube.g1, cube.b0)) -
                    (mom(cube.r1, cube.g1, cube.b0) + mom(cube.r0, cube.g0, cube.b0));
        }
    }

    static Moment top(const Box& cube, int dir, int pos, const Array3D<Moment>& mom) {
        switch (dir) {
            case 2:  // RED
                return (mom(pos, cube.g1, cube.b1) + mom(pos, cube.g0, cube.b0)) -
                    (mom(pos, cube.g1, cube.b0) + mom(pos, cube.g0, cube.b1));
            case 1:  // GREEN
                return (mom(cube.r1, pos, cube.b1) + mom(cube.r0, pos, cube.b0)) -
                    (mom(cube.r1, pos, cube.b0) + mom(cube.r0, pos, cube.b1));
            case 0:  // BLUE
            default:
                return (mom(cube.r1, cube.g1, pos) + mom(cube.r0, cube.g0, pos)) -
                    (mom(cube.r1, cube.g0, pos) + mom(cube.r0, cube.g1, pos));
        }
    }

    static double var(const Box& cube, const Array3D<Moment>& mom) {
        Moment v = vol(cube, mom);
        if (v.w_ <= 0) { return 0.0; }

        auto dr = static_cast<double>(v.r_);
        auto dg = static_cast<double>(v.g_);
        auto db = static_cast<double>(v.b_);

        return v.m2_ - (dr * dr + dg * dg + db * db) / static_cast<double>(v.w_);
    }

    static double maximize(
        const Box& cube, int dir, int first, int last, int* cut, const Moment& whole, const Array3D<Moment>& mom) {
        Moment base = bottom(cube, dir, mom);
        double max_val = 0.0;
        *cut = -1;

        for (int i = first; i < last; ++i) {
            Moment half = base + top(cube, dir, i, mom);
            if (half.w_ == 0) { continue; }

            double temp = (static_cast<double>(half.r_) * static_cast<double>(half.r_) +
                              static_cast<double>(half.g_) * static_cast<double>(half.g_) +
                              static_cast<double>(half.b_) * static_cast<double>(half.b_)) /
                static_cast<double>(half.w_);

            int64_t rem_w = whole.w_ - half.w_;
            if (rem_w == 0) { continue; }

            int64_t rem_r = whole.r_ - half.r_;
            int64_t rem_g = whole.g_ - half.g_;
            int64_t rem_b = whole.b_ - half.b_;

            temp += (static_cast<double>(rem_r) * static_cast<double>(rem_r) +
                        static_cast<double>(rem_g) * static_cast<double>(rem_g) +
                        static_cast<double>(rem_b) * static_cast<double>(rem_b)) /
                static_cast<double>(rem_w);

            if (temp > max_val) {
                max_val = temp;
                *cut = i;
            }
        }
        return max_val;
    }

    static bool cut(Box& set1, Box& set2, const Array3D<Moment>& mom) {
        int cutr = -1, cutg = -1, cutb = -1;
        Moment whole = vol(set1, mom);

        double maxr = maximize(set1, 2, set1.r0 + 1, set1.r1, &cutr, whole, mom);
        double maxg = maximize(set1, 1, set1.g0 + 1, set1.g1, &cutg, whole, mom);
        double maxb = maximize(set1, 0, set1.b0 + 1, set1.b1, &cutb, whole, mom);

        int dir;
        if (maxr >= maxg && maxr >= maxb) {
            dir = 2;  // RED
            if (cutr < 0) { return false; }
        } else if (maxg >= maxr && maxg >= maxb) {
            dir = 1;  // GREEN
            if (cutg < 0) { return false; }
        } else {
            dir = 0;  // BLUE
            if (cutb < 0) { return false; }
        }

        set2.r1 = set1.r1;
        set2.g1 = set1.g1;
        set2.b1 = set1.b1;

        switch (dir) {
            case 2:
                set2.r0 = set1.r1 = cutr;
                set2.g0 = set1.g0;
                set2.b0 = set1.b0;
                break;
            case 1:
                set2.g0 = set1.g1 = cutg;
                set2.r0 = set1.r0;
                set2.b0 = set1.b0;
                break;
            case 0:
                set2.b0 = set1.b1 = cutb;
                set2.r0 = set1.r0;
                set2.g0 = set1.g0;
                break;
            default:
                break;
        }

        set1.vol = (set1.r1 - set1.r0) * (set1.g1 - set1.g0) * (set1.b1 - set1.b0);
        set2.vol = (set2.r1 - set2.r0) * (set2.g1 - set2.g0) * (set2.b1 - set2.b0);
        return true;
    }
};
