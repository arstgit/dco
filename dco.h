#ifndef __DCO_H__
#define __DCO_H__


#include <ucontext.h>
#include <stddef.h>
#include <setjmp.h>

#define DCO_ROUTINE_DEAD (1 << 1)
#define DCO_ROUTINE_READY (1 << 2)
#define DCO_ROUTINE_ACTIVE (1 << 3)
#define DCO_ROUTINE_SUSPENDED (1 << 4)

 // Default shared stack size is 4MB.
#define DCO_DEFAULT_STACK_SIZE (4 * 1024 * 1024)
#define DCO_ROUTINE_INITIAL_MAX_NUM 8

struct dco;

typedef void (*routineProc)(struct dco *, void *clientData);

typedef struct routine {
  ucontext_t *ctx;
  jmp_buf jmpctx;
  routineProc proc;
  void *clientData;
  struct dco *co;
  int status;
  ptrdiff_t size;
  ptrdiff_t used;
  char *stack;
} routine;

typedef struct dco {
  ucontext_t *ctx;
  jmp_buf jmpctx;
  int activeid;
  int routinemaxnum;
  int routinenum;
  routine **routines;
  char stack[DCO_DEFAULT_STACK_SIZE];
} dco;

dco * dcoCreateCo(void);
void dcoDeleteCo( dco *co);

int dcoCreateRoutine( dco *co, routineProc, void *clientData);
void dcoResume( dco *co, int rid);
void dcoYield( dco *co);

int dcoGetRoutineStatus( dco *co, int rid);
int dcoGetActiveid( dco *co);

#endif