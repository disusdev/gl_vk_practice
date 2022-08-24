// #ifndef __PERF_H__
// #define __PERF_H__

// #include "defines.h"

// typedef struct
// t_perf
// {
//   const char* name;
//   f64 total_time;
// }
// t_perf;


// t_perf perf_start(const char* name);
// void perf_accumulate(const t_perf* perf);
// void perf_end(const t_perf* perf);
// void perf_print(const t_perf* perf);

// // make vscode happy
// #if defined(__INTELLISENSE__)
// #define PERF_IMPLEMENTATION
// #endif

// #ifdef PERF_IMPLEMENTATION

// // #ifdef PERF_STDLIB
// // #include <malloc.h>
// // #include <string.h>
// // #include <assert.h>
// // #endif

// t_perf perf_start(const char* name)
// {
//   t_perf perf;
//   perf.name = name;
//   perf.total_time = 0.0f;
//   return perf;
// }

// void perf_accumulate(const t_perf* perf)
// {

// }
// void perf_end(const t_perf* perf);
// void perf_print(const t_perf* perf);

// #endif

// #endif