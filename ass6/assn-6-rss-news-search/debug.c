#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sched.h>

#include "debug.h"


char debugFlags[256];
int debugYieldOn = 0;

static unsigned int randYieldSeed;
static unsigned int yieldPercent;

void
Debug_Init(char *flags, int yieldpercentage, unsigned int randseed)
{
  char *c = flags;
  while (*c) {
    Debug_SetFlag(*c, 1);
    c++;
  }
  if (yieldpercentage) {
    debugYieldOn = 1;
    randYieldSeed = randseed;
    yieldPercent= yieldpercentage;
  }

}


void
Debug_SetFlag(char ch, int val)
{
  debugFlags[(int)ch] = val;
}

void
Debug_Yield(void)
{
  unsigned int rand = rand_r(&randYieldSeed) % 100;
  if (rand < yieldPercent) sched_yield();
}
