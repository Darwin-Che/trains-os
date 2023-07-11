#include "math.h"

struct MathQuadrRoots math_solve_quadr(int64_t a, int64_t b, int64_t c)
{
    struct MathQuadrRoots roots;

    int64_t delta = b * b - 4 * a * c;

    if (delta < 0)
    {
        roots.cnt = 0;
    }
    else if (delta == 0)
    {
        roots.cnt = 1;
        roots.r1 = -2 * c / b;
    }
    else
    {
        int64_t delta_q = math_sqrt(delta);
        roots.cnt = 2;
        roots.r1 = 2 * c / (-b - delta_q);
        roots.r2 = 2 * c / (-b + delta_q);
        if (roots.r1 < roots.r2)
        {
            int64_t tmp = roots.r1;
            roots.r1 = roots.r2;
            roots.r2 = tmp;
        }
    }

    return roots;
}