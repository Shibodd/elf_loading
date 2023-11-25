#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include <cstring>
#include <cstdio>
#include <cstdlib>

#define MY_ASSERT(cond, s, ...) do { if (!(cond)) { fprintf(stderr, s "\n" __VA_OPT__(,) __VA_ARGS__); exit(1); } } while(0)
#define MY_ASSERT_PERROR(cond, s, ...) MY_ASSERT(cond, s " (%s)" __VA_OPT__(,)  __VA_ARGS__, strerror(errno));


#endif // !OUTPUT_HPP