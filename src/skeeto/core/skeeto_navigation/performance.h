#pragma once
#include "sdk.h"
namespace performance {
inline long long Frequency(){static long long value=0;if(!value)QueryPerformanceFrequency((LARGE_INTEGER*)&value);return value;}
inline long long Now(){long long value=0;QueryPerformanceCounter((LARGE_INTEGER*)&value);return value;}
inline unsigned Microseconds(long long elapsed){return Frequency()?unsigned(double(elapsed)*1000000.0/double(Frequency())):0;}
struct Metric {unsigned count=0,peak=0;double total=0;void Add(long long start){unsigned us=Microseconds(Now()-start);++count;total+=us;if(us>peak)peak=us;}unsigned Average()const{return count?unsigned(total/count):0;}};
struct Measure {Metric& metric;long long start;Measure(Metric& m):metric(m),start(Now()){}~Measure(){metric.Add(start);}};
}

