#ifndef OGREMATH_H
#define OGREMATH_H

#include "OgrePrerequisites.h"
#include <cmath>

namespace Ogre
{
    class Math
    {
    public:
        template <typename T>
        static T Clamp(const T& value, const T& minValue, const T& maxValue)
        {
            return value < minValue ? minValue : (value > maxValue ? maxValue : value);
        }

        static Real Pow(Real base, Real exp)
        {
            return std::pow(base, exp);
        }

        static Real Abs(Real v)
        {
            return std::fabs(v);
        }

        template <typename T>
        static T Abs(T v)
        {
            return v < static_cast<T>(0) ? -v : v;
        }

        static Real Log2(Real v)
        {
            return std::log2(v);
        }

        static Real Sin(Real v)
        {
            return std::sin(v);
        }

        static Real Cos(Real v)
        {
            return std::cos(v);
        }

        static Real Sqrt(Real v)
        {
            return std::sqrt(v);
        }

        static Real Sign(Real v)
        {
            return v < 0.0f ? -1.0f : (v > 0.0f ? 1.0f : 0.0f);
        }

        static Real Sqr(Real v)
        {
            return v * v;
        }

        static const Real PI;
        static const Real TWO_PI;
    };

    inline const Real Math::PI = 3.14159265358979323846f;
    inline const Real Math::TWO_PI = 6.28318530717958647692f;
}

#endif
