#pragma once
#include "../performance.h"
// Soft work budgets use Skeeto's shared QueryPerformanceCounter helpers.
namespace budget_clock {
inline long long Now(){return performance::Now();}
inline unsigned Microseconds(long long elapsed){return performance::Microseconds(elapsed);}
}
