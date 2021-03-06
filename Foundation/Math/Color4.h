#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <iostream>

#include "Utils.h"
#include "Color3.h"

namespace Helium
{
    namespace Math
    {
        class FOUNDATION_API Color4
        {
        public:
            u8 r, g, b, a;

            Color4            () : r(0), g(0), b(0), a(255) {}
            explicit Color4   ( const Color3& c) : r( c.r ), g( c.g ), b( c.b ), a( 255 ) {}
            explicit Color4   ( u8 r, u8 g, u8 b, u8 a ) : r( r ), g( g ), b( b ), a( a ) {}
            explicit Color4   ( u8 val ) : r( val ), g( val ), b( val ), a( val ) {}

            Color4&           operator= (const Color4& v) { r = v.r; g = v.g; b = v.b; a = v.a; return *this; }
            Color4&           operator= (const Color3& v) { r = v.r; g = v.g; b = v.b; a = 255; return *this; }
            Color4&           operator+= (const Color4& v) { r += v.r; g += v.g; b += v.b; a += v.a; return *this; }
            Color4&           operator-= (const Color4& v) { r -= v.r; g -= v.g; b -= v.b; a -= v.a; return *this; }
            Color4&           operator*= (const Color4& v) { r *= v.r; g *= v.g; b *= v.b; a *= v.a; return *this; }
            Color4&           operator*= (const u8 v) { r *= v; g *= v; b *= v; a *= v; return *this; }
            Color4&           operator/= (const Color4& v) { r /= v.r; g /= v.g; b /= v.b; a /= v.a; return *this; }
            Color4&           operator/= (const u8 v) { r /= v; g /= v; b /= v; a /= v; return *this; }

            Color4            operator+ (const Color4& v) const { return Color4 (r + v.r, g + v.g, b + v.b, a + v.a); }
            Color4            operator- (const Color4& v) const { return Color4 (r - v.r, g - v.g, b - v.b, a - v.a); }
            Color4            operator* (const Color4& v) const { return Color4 (r * v.r, g * v.g, b * v.b, a * v.a); }
            Color4            operator* (const u8 v) const { return Color4 (r * v, g * v, b * v, a * v); }
            Color4            operator/ (const Color4& v) const { return Color4 (r / v.r, g / v.g, b / v.b, a / v.a); }
            Color4            operator/ (const u8 v) const { return Color4 (r / v, g / v, b / v, a / v); }

            Color4            operator- () const { return Color4( -r, -g, -b, -a ); }

            u8&               operator[] (const u32 i) {  HELIUM_ASSERT(i < 4); return (&r)[i]; }
            const u8&         operator[] (const u32 i) const {  HELIUM_ASSERT(i < 4); return (&r)[i]; }

            bool              operator== (const Color4& v) const { return (r == v.r && g == v.g && b == v.b && a == v.a); }
            bool              operator!= (const Color4& v) const { return !(r == v.r && g == v.g && b == v.b && a == v.a); }

            void Set( u8 vx, u8 vy, u8 vz, u8 vw )
            {
                r = vx;
                g = vy;
                b = vz;
                a = vw;
            }

            void Set( f32 vr, f32 vg, f32 vb, f32 va )
            {
                r = (u8)( vr * 255.0f );
                g = (u8)( vg * 255.0f );
                b = (u8)( vb * 255.0f );
                a = (u8)( va * 255.0f );
            }

            void Get( f32& vr, f32& vg, f32& vb, f32& va ) const
            {
                vr = r / 255.0f;
                vg = g / 255.0f;
                vb = b / 255.0f;
                va = a / 255.0f;
            }

            friend FOUNDATION_API tostream& operator<<(tostream& outStream, const Color4& color);
            friend FOUNDATION_API tistream& operator>>(tistream& inStream, Color4& color);
        };

        typedef std::vector< Color4 > V_Color4;
        typedef std::vector< V_Color4 > VV_Color4;

        inline tostream& operator<<(tostream& outStream, const Color4& color)
        {
            outStream << (u16)color.r << ", " << (u16)color.g << ", " << (u16)color.b << ", " << (u16)color.a;

            return outStream;
        }

        inline tistream& operator>>(tistream& inStream, Color4& color)
        {
            u32 r = 0;
            u32 g = 0;
            u32 b = 0;
            u32 a = 0;

            tstring line;
            std::getline( inStream, line );
            if (4 == _stscanf( line.c_str(), TXT("%u, %u, %u, %u"), &r, &g, &b, &a))
            {
                color.r = (u8)r;
                color.g = (u8)g;
                color.b = (u8)b;
                color.a = (u8)a;
            }

            return inStream;
        }
    }
}