#include <stdio.h>
#include <time.h>

#include "dco.h"

#define SWITCH_TIME (1*1024*1024)

static void foo(struct dco *co, void *clientData) {
  int i;
  for (i = 0; i < SWITCH_TIME; i += 2) {
    // printf("%d\n", i);
    dcoYield(co);
  }
}

static void bar(struct dco *co, void *clientData) {
  int i;
  for (i = 1; i < SWITCH_TIME; i += 2) {
    // printf("%d\n", i);
    dcoYield(co);
  }
}

int main() {
  struct dco *co = dcoCreateCo();

  int routine1 = dcoCreateRoutine(co, foo, NULL);
  int routine2 = dcoCreateRoutine(co, bar, NULL);

  clock_t t;
  t = clock();

  while (dcoGetRoutineStatus(co, routine1) != DCO_ROUTINE_DEAD ||
         dcoGetRoutineStatus(co, routine2) != DCO_ROUTINE_DEAD) {
    dcoResume(co, routine1);
    dcoResume(co, routine2);
  }

  t = clock() - t;
  double time_taken = ((double)t) / CLOCKS_PER_SEC;

  printf("it took %f seconds to execute \n", time_taken);

  dcoDeleteCo(co);

  return 0;
}
