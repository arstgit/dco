#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dco.h"

static void _deleteRoutine(routine *co) {
  free(co->ctx);
  free(co->stack);
  free(co);
}

dco *dcoCreateCo() {
  dco *co = malloc(sizeof(*co));
  co->ctx = malloc(sizeof(*co->ctx));
  co->routinenum = 0;
  co->routinemaxnum = DCO_ROUTINE_INITIAL_MAX_NUM;
  co->activeid = -1;
  co->routines = malloc(co->routinemaxnum * sizeof(routine *));
  memset(co->routines, 0, sizeof(routine *) * co->routinemaxnum);
  return co;
}

void dcoDeleteCo(dco *co) {
  int i;
  for (i = 0; i < co->routinemaxnum; i++) {
    routine *routine = co->routines[i];
    if (routine != NULL) {
      _deleteRoutine(routine);
    }
  }

  free(co->ctx);
  free(co->routines);
  free(co);
}

int dcoCreateRoutine(dco *co, routineProc proc, void *clientData) {
  routine *routine = malloc(sizeof(*routine));
  routine->ctx = malloc(sizeof(*routine->ctx));
  routine->proc = proc;
  routine->clientData = clientData;
  routine->co = co;
  routine->size = 0;
  routine->used = 0;
  routine->status = DCO_ROUTINE_READY;
  routine->stack = NULL;

  if (co->routinenum >= co->routinemaxnum) {
    int rid = co->routinemaxnum;
    co->routines =
        realloc(co->routines, co->routinemaxnum * 2 * sizeof(*routine));
    memset(co->routines + co->routinemaxnum, 0,
           sizeof(*routine) * co->routinemaxnum);

    co->routinemaxnum *= 2;
    co->routines[co->routinemaxnum] = routine;
    co->routinenum++;
    return rid;
  } else {
    for (int i = 0; i < co->routinemaxnum; i++) {
      int rid = (i + co->routinenum) % co->routinemaxnum;
      if (co->routines[rid] == NULL) {
        co->routines[rid] = routine;
        co->routinenum++;
        return rid;
      }
    }
  }

  assert(0);
  return -1;
}

static void bootstrapRoutine(dco *co) {
  int rid = co->activeid;
  routine *routine = co->routines[rid];
  routine->proc(co, routine->clientData);
  _deleteRoutine(routine);
  co->routines[rid] = NULL;
  co->routinenum--;
  co->activeid = -1;
}

void dcoResume(dco *co, int rid) {
  assert(co->activeid == -1);
  assert(rid >= 0 && rid < co->routinemaxnum);

  routine *routine = co->routines[rid];
  if (routine == NULL)
    return;

  switch (routine->status) {
  case DCO_ROUTINE_READY:
    getcontext(routine->ctx);
    routine->ctx->uc_stack.ss_sp = co->stack;
    routine->ctx->uc_stack.ss_size = DCO_DEFAULT_STACK_SIZE;
    routine->ctx->uc_link = co->ctx;
    makecontext(routine->ctx, (void (*)(void))bootstrapRoutine, 1, co);

    co->activeid = rid;
    routine->status = DCO_ROUTINE_ACTIVE;

    if (setjmp(co->jmpctx) == 0) {
      swapcontext(co->ctx, routine->ctx);
    }

    break;
  case DCO_ROUTINE_SUSPENDED:
    memcpy(co->stack + DCO_DEFAULT_STACK_SIZE - routine->used, routine->stack,
           routine->used);

    co->activeid = rid;
    routine->status = DCO_ROUTINE_ACTIVE;

    if (setjmp(co->jmpctx) == 0) {
      longjmp(routine->jmpctx, 1);
    }

    break;
  default:
    assert(0);
  }
}

static void _saveStack(routine *routine, char *stackTop) {
  // Scout here to mark current stack bottom position.
  char scout = 0;
  assert(stackTop - &scout <= DCO_DEFAULT_STACK_SIZE);

  if (routine->size < stackTop - &scout) {
    free(routine->stack);
    routine->size = stackTop - &scout;
    routine->stack = malloc(routine->size);
  }
  routine->used = stackTop - &scout;
  memcpy(routine->stack, &scout, routine->used);
}

void dcoYield(dco *co) {
  int rid = co->activeid;
  assert(rid >= 0);

  routine *routine = co->routines[rid];
  assert((char *)&routine > co->stack);

  _saveStack(routine, co->stack + DCO_DEFAULT_STACK_SIZE);
  routine->status = DCO_ROUTINE_SUSPENDED;
  co->activeid = -1;

  // Using setjmp()/longjmp() instead of swapcontext() for performance concern.
  // swapcontext() additionally involves system call overhead dealing with signal.
  if (setjmp(routine->jmpctx) == 0) {
    longjmp(co->jmpctx, 1);
  }
}

int dcoGetRoutineStatus(dco *co, int rid) {
  assert(rid >= 0 && rid < co->routinemaxnum);

  if (co->routines[rid] == NULL) {
    return DCO_ROUTINE_DEAD;
  }
  return co->routines[rid]->status;
}

int dcoGetActiveid(dco *co) { return co->activeid; }