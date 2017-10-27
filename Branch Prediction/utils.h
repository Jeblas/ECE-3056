#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace std;

#define UINT32      unsigned int
#define INT32       int
#define UINT64      unsigned long long
#define COUNTER     unsigned long long


#define NOT_TAKEN 0
#define TAKEN 1

#define FAILURE 0
#define SUCCESS 1




static inline UINT32 SatIncrement(UINT32 x, UINT32 max)
{
  if(x<max) return x+1;
  return x;
}

static inline UINT32 SatDecrement(UINT32 x)
{
  if(x>0) return x-1;
  return x;
}



#endif

