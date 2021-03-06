#pragma once
#ifndef ZARUWORKS_HOOLIB_HPP
#define ZARUWORKS_HOOLIB_HPP

#include <cstring>
#include <array>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace HooLib {

const double PI = 3.14159265358979323846, PI_2 = 1.57079632679489661923, PI_4 = 0.785398163397448309616;

inline double rad2deg(double rad) { return rad * 180.0 / PI; }
inline double deg2rad(double deg) { return deg * PI / 180.0; }

inline std::string createErrorMsg(const std::string& what, const char *file, int line)
{
    std::stringstream ss;
    ss << "(<" << file << "," << line << ">" << what << ")";
    return ss.str();
}

#define HOOLIB_ERROR(msg) HooLib::createErrorMsg((msg), __FILE__, __LINE__)
#define HOOLIB_THROW(msg) { throw std::runtime_error(HOOLIB_ERROR(msg)); };
#define HOOLIB_THROW_IF(ret, msg) if((ret)){HOOLIB_THROW((msg));}
#define HOOLIB_THROW_UNLESS(ret, msg) HOOLIB_THROW_IF(!(ret), msg);

#define unless(cond) if(!(cond))
#define until(cond) while(!(cond))

#define HOOLIB_RANGE(con) std::begin(con), std::end(con)

inline double divd(double lhs, double rhs) { return lhs / rhs; }

inline std::string fok(const std::string& str)
{
    return str;
}

template<class... Args>
inline std::string fok(const std::string& head, Args... args)
{
    return head + fok(args...);
}

template<class Head, class... Args>
inline Head max(Head head, Args... args)
{
    auto tmp = max(args...);
    return head < tmp ? tmp : head;
}

template<class Head, class Tail>
inline Head max(Head head, Tail tail)
{
    return head < tail ? tail : head;
}

template<class Head, class... Args>
inline Head min(Head head, Args... args)
{
    auto tmp = max(args...);
    return tmp < head ? tmp : head;
}

template<class Head, class Tail>
inline Head min(Head head, Tail tail)
{
    return tail < head ? tail : head;
}

template<class T>
inline std::string to_str(T t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

int str2int(const std::string& str)
{
    HOOLIB_THROW_UNLESS(!str.empty(), "str is empty.");

    bool isMinus = false;
    if(str[0] == '-')   isMinus = true;

    int res = 0;
    for(int i = (isMinus ? 1 : 0);i < str.size();i++){
        char ch = str[i];
        HOOLIB_THROW_UNLESS('0' <= ch && ch <= '9', "not number");
        res = res * 10 + (ch - '0');
    }

    return isMinus ? -res : res;
}


// min <= x < sup
int randomInt(int min, int sup)
{
    static std::mt19937 engine = std::mt19937(std::random_device()());
    std::uniform_int_distribution<> dist(min, sup - 1);
    return dist(engine);
}

double randomFloat(double min, double sup)
{
    static std::mt19937 engine = std::mt19937(std::random_device()());
    std::uniform_real_distribution<> dist(min, sup);
    return dist(engine);
}

class RGB
{
private:
    unsigned char r_, g_, b_;

public:
    RGB(unsigned char r, unsigned char g, unsigned char b)
        : r_(r), g_(g), b_(b)
    {}

    template<class T = int> T r() const { return r_; }
    template<class T = int> T g() const { return g_; }
    template<class T = int> T b() const { return b_; }

    static RGB white() { return RGB(255, 255, 255); }
    static RGB black() { return RGB(  0,   0,   0); }
    static RGB gray()  { return RGB(127, 127, 127); }
    static RGB blue()  { return RGB(  0,   0, 255); }
    static RGB red()   { return RGB(255,   0,   0); }
    static RGB green() { return RGB(  0, 255,   0); }
    static RGB sora()  { return RGB( 88, 178, 220); }
    static RGB momo()  { return RGB(245, 150, 170); }
};
template<> double RGB::r<double>() const { return divd(r_, 0xff); }
template<> double RGB::g<double>() const { return divd(g_, 0xff); }
template<> double RGB::b<double>() const { return divd(b_, 0xff); }

class Vec2d
{
private:
    int x_, y_;

public:
    Vec2d(int x, int y)
        : x_(x), y_(y)
    {}

    int x() const { return x_; }
    int y() const { return y_; }
};

class Rect
{
private:
    int x_, y_, w_, h_;

public:
    struct XYWH {};
    struct LTRB {};

    Rect()
        : x_(0), y_(0), w_(0), h_(0)
    {}
    Rect(int x, int y, int w, int h, XYWH)
        : x_(x), y_(y), w_(w), h_(h)
    {}
    Rect(int l, int t, int r, int b, LTRB)
        : x_(l), y_(t), w_(r - l), h_(b - t)
    {}

    int left()   const { return x_; }
    int top()    const { return y_; }
    int right()  const { return x_ + w_; }
    int bottom() const { return y_ + h_; }
    int x()      const { return x_; }
    int y()      const { return y_; }
    int width()  const { return w_; }
    int height() const { return h_; }
};

inline Rect XYWH(int x, int y, int w, int h) { return Rect(x, y, w, h, Rect::XYWH()); }
inline Rect LTRB(int l, int t, int r, int b) { return Rect(l, t, r, b, Rect::LTRB()); }

// multi-dimention array made of std::array
// thanks to https://www.ruche-home.net/boyaki/2013-12-28/Carray
template<typename T, std::size_t Size, std::size_t ...Sizes>
struct multi_array_type
{
    using type = std::array<typename multi_array_type<T, Sizes...>::type, Size>;
};
template<typename T, std::size_t Size>
struct multi_array_type<T, Size>
{
    using type = std::array<T, Size>;
};
template<typename T, std::size_t Size, std::size_t ...Sizes>
using multi_array = typename multi_array_type<T, Size, Sizes...>::type;

//

std::vector<std::string> splitStrByChars(const std::string& src, const std::string& delimChars)
{
    std::shared_ptr<char> data(new char[src.size() + 1], std::default_delete<char[]>());
    std::vector<std::string> ret;

    std::strcpy(data.get(), src.c_str());

    char *p = std::strtok(data.get(), delimChars.c_str());
    while(p != nullptr){
        ret.emplace_back(p);
        p = std::strtok(nullptr, delimChars.c_str());
    }

    return std::move(ret);
}
}

#endif
