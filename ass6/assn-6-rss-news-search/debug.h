#ifndef _DEBUG_H
#define _DEBUG_H

extern char debugFlags[];
extern int  debugYieldOn;

#define DPRINTF(_flag, _args) if (debugFlags[_flag]) printf _args;

void Debug_Init(char *flags, int yieldprecent, unsigned int randseed);
void Debug_SetFlag(char ch, int val);
void  Debug_Yield(void);

#define Y do { if (debugYieldOn) Debug_Yield();} while(0)

#endif /* _DEBUG_H */
