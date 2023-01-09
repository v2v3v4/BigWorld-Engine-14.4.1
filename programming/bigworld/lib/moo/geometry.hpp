
#pragma once

#include "math/vector2.hpp"

BW_BEGIN_NAMESPACE

namespace Moo {

    class Line
    {
    public:
        Vector2 p1, p2;
        Line() {}
        Line(const Vector2& p1_, const Vector2& p2_) : p1(p1_), p2(p2_) {};
        void init(const Vector2& p1_, const Vector2& p2_) { p1 = p1_; p2 = p2_; };
        virtual ~Line() {};
        bool intersect(Line& l, Vector2& pos)
        {
            float Ax = p1.x; float Ay = p1.y;
            float Bx = p2.x; float By = p2.y;
            float Cx = l.p1.x; float Cy = l.p1.y;
            float Dx = l.p2.x; float Dy = l.p2.y;
            float  distAB, theCos, theSin, newX, ABpos;
            //  Fail if either line segment is zero-length.
            if ((Ax == Bx && Ay == By) || (Cx == Dx && Cy == Dy))
                return false;
            //  Fail if the segments share an end-point.
            if ((Ax == Cx && Ay == Cy) || (Bx == Cx && By == Cy) || (Ax == Dx && Ay == Dy) || (Bx == Dx && By == Dy))
                return false;
            //  (1) Translate the system so that point A is on the origin.
            Bx -= Ax; By -= Ay;
            Cx -= Ax; Cy -= Ay;
            Dx -= Ax; Dy -= Ay;
            //  Discover the length of segment A-B.
            distAB = sqrt(Bx * Bx + By * By);
            //  (2) Rotate the system so that point B is on the positive X axis.
            theCos = Bx / distAB;
            theSin = By / distAB;
            newX = Cx * theCos + Cy * theSin;
            Cy = Cy * theCos - Cx * theSin; Cx = newX;
            newX = Dx * theCos + Dy * theSin;
            Dy = Dy * theCos - Dx * theSin; Dx = newX;
            //  Fail if segment C-D doesn't cross line A-B.
            if ((Cy < 0.f && Dy < 0.f) || (Cy >= 0.f && Dy >= 0.f)) return false;
            //  (3) Discover the position of the intersection point along line A-B.
            ABpos = Dx + (Cx - Dx) * Dy / (Dy - Cy);
            //  Fail if segment C-D crosses line A-B outside of segment A-B.
            if (ABpos<0.0f || ABpos>distAB) return false;
            //  (4) Apply the discovered position to line A-B in the original coordinate system.
            pos.x = Ax + ABpos * theCos;
            pos.y = Ay + ABpos * theSin;
            //  Success.
            return true;
        }
    };

    class Rect
    {
    public:
        Line l1, l2, l3, l4;
        Rect(const Line l1_, const Line l2_, const Line l3_, const Line l4_) : l1(l1_), l2(l2_), l3(l3_), l4(l4_) {};
        virtual ~Rect() {};
        bool isInside(Vector2& p)
        {
            if ((l1.p2 - l1.p1).crossProduct(p - l1.p1) > 0.0) return false;
            if ((l2.p2 - l2.p1).crossProduct(p - l2.p1) > 0.0) return false;
            if ((l3.p2 - l3.p1).crossProduct(p - l3.p1) > 0.0) return false;
            if ((l4.p2 - l4.p1).crossProduct(p - l4.p1) > 0.0) return false;
            return true;
        }
    };

}

BW_END_NAMESPACE
