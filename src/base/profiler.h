#ifndef PROFILER_H__
#define PROFILER_H__
#include "helper.h"

/* Overview:
  - Regions are the actual profiling regions (e.g My_Calculation_Region_Of_Code), there we put inclusive/exclusive timings in cycles.
  - Blocks just mark the execution of one begin_block(..) end_block(..) pair, they contribute to final region timings.
The scheme here is that we have inclusive and exclusive timings
 - for inclusive timings we take the region's current inclusive timing and cache it inside our block, now when end_block(..) is called the region's
  inclusive timing will be over-written to cached inclusive timing + the blocks duration (delta), even if we have recursive function calls the 'root' block will overwrite last, so the timing will be correct
 - for exclusive timings, we just sum the block's duration (delta) to the region's exclusive timing AND if the block had a parent, we remove the delta from the parent region's exclusive timing (to be exclusive !)
 */

//  Sample execution:
//    Fn1
//   |`` Fn2
//   |   |``  Fn1
//   |   |   |``
// t2| t1| t0|
//   |   |   |..
//   |   |..
//   |..
// Functions Fn1 and Fn2, each taking t0, t1, t2 to complete
//  begin(fn1) -> b0_fn1.begin = os_now(), b0_fn1.old_inc = reg[fn1].inc, b0_fn1.parent = nil
//  begin(fn2) -> b0_fn2.begin = os_now(), b0_fn2.old_inc = reg[fn2].inc, b0_fn2.parent = fn1
//  begin(fn1) -> b1_fn1.begin = os_now(), b1_fn1.old_inc = reg[fn1].inc, b1_fn1.parent = fn2
//  end(fn1) -> delta = b1_fn1.begin - os_now(), region[fn1].inc = b1_fn1.old_inc + delta, region[fn1].exc += delta, region[b1_fn1.parent].exc -= delta
//  end(fn2) -> delta = b0_fn2.begin - os_now(), region[fn2].inc = b0_fn2.old_inc + delta, region[fn2].exc += delta, region[b0_fn2.parent].exc -= delta
//  end(fn1) -> delta = b0_fn1.begin - os_now(), region[fn1].inc = b0_fn1.old_inc + delta, region[fn1].exc += delta  <- inclusive timing is overwritten to have root F1's timing(!!), that's the shit


// TODO: make this profiler not work on release builds.. for SPEED
// TODO: change xxx_cyc they are not really cycles, just intervals of the performance counter
// TODO: DONT use printf please.. we DONT like the c standard library

typedef struct {
  u32 hit_count;
  u64 elapsed_inclusive_cyc; // cycles in this region + all children
  u64 elapsed_exclusive_cyc; // includes ONLY this region ( not children )
  const char *label;
} Profiler_Region;

typedef struct {
  u64 begin_cyc;
  u64 end_cyc;
  u32 parent_idx;
  u32 region_idx;
  const char *region_label;
  // the root time for our region when the block started, will be updated at the end_block incrementing actual delta
  u64 old_elapsed_inclusive_cyc;
} Profiler_Block;

typedef struct {
  // this contains all block timings corresponding to region
  #define MAX_PROFILE_REGIONS 256
  Profiler_Region regions[MAX_PROFILE_REGIONS];

  u64 begin_cyc;
  u64 end_cyc;

} Profiler;

#define NAME_CONCAT2(A, B) A##B
#define NAME_CONCAT(A, B) NAME_CONCAT2(A, B)
#define TIME_BLOCK(Name) Profiler_Block NAME_CONCAT(Block,__LINE__) __attribute__ ((__cleanup__(profiler_block_end)))  = profiler_block_begin(Name, __COUNTER__ + 1);
#define TIME_FUNC TIME_BLOCK(__func__)

#ifdef PROFILER_IMPLEMENTATION
  extern u64 platform_read_cpu_timer();

  // Whoops!
  global_var Profiler g_prof;
  global_var u32 g_parent_idx;

  void profiler_begin() {
    g_prof.begin_cyc = platform_read_cpu_timer();
  }

  void profiler_end() {
    g_prof.end_cyc = platform_read_cpu_timer();
  }

  void profiler_print() {
    printf("---- PROFILER INFO ----\n");
    u64 total_cyc = g_prof.end_cyc - g_prof.begin_cyc;
    printf("Total duration: %lu cycles, (100%%)\n", total_cyc);

    for (u64 i = 0; i < MAX_PROFILE_REGIONS; ++i) {
      Profiler_Region *region = &g_prof.regions[i];
      if (region->elapsed_inclusive_cyc) {
      printf("Region %s(%u):", region->label, region->hit_count);
      printf("%lu cycles, (%.2f%%", region->elapsed_exclusive_cyc, 100.0 * ((f64)region->elapsed_exclusive_cyc / (f64)total_cyc));
      if (region->elapsed_inclusive_cyc != region->elapsed_exclusive_cyc) {
        f64 percent_elapsed_inclusive = 100.0 * ((f64)region->elapsed_inclusive_cyc / (f64)total_cyc);
        printf(", %.2f%% w/ children", percent_elapsed_inclusive);
      }
      printf(")\n");
      }
    }
    printf("-----------------------\n");
  }

  void profiler_end_and_print() {
    profiler_end();
    profiler_print();
  }

  Profiler_Block profiler_block_begin(const char *region_label, u64 region_idx) {
    Profiler_Region *region = &g_prof.regions[region_idx];

    Profiler_Block block = (Profiler_Block){
      .region_label = region_label,
      .region_idx = region_idx,
      .begin_cyc = platform_read_cpu_timer(),
      .old_elapsed_inclusive_cyc = region->elapsed_inclusive_cyc,
      .parent_idx = g_parent_idx, // By default its zero, which encodes no parent! we want NO invalid Region pointers OK!?!?!?
    };
    g_parent_idx = region_idx;

    return block;
  }

  void profiler_block_end(Profiler_Block *block) {
    Profiler_Region *region = &g_prof.regions[block->region_idx];
    Profiler_Region *parent = &g_prof.regions[block->parent_idx];

    block->end_cyc = platform_read_cpu_timer();
    u64 delta = block->end_cyc - block->begin_cyc;

    region->hit_count += 1;
    region->elapsed_inclusive_cyc = block->old_elapsed_inclusive_cyc + delta;
    region->elapsed_exclusive_cyc += delta;
    parent->elapsed_exclusive_cyc -= delta;

    region->label = block->region_label;

    g_parent_idx = block->parent_idx; // reset the parent index to the blocks actual parent (It might have changed through execution)
  }
#else
  void profiler_begin();
  void profiler_end_and_print();
  Profiler_Block profiler_block_begin(const char *region_label, u64 region_idx);
  void profiler_block_end(Profiler_Block *block);
#endif 

#endif
