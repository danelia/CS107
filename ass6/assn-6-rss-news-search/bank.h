#ifndef _BANK_H
#define _BANK_H

#include <semaphore.h>

typedef struct Bank {
  unsigned int numberBranches;
  int numberWorkersHasToFinish;
  sem_t forCheck;
  sem_t letMeStartNextDay;
  sem_t lockForReportTransfer;
  struct       Branch  *branches;
  struct       Report  *report;
} Bank;

#include "account.h"

int Bank_Balance(Bank *bank, AccountAmount *balance);

void Bank_Dispose(Bank* bank);
Bank *Bank_Init(int numBranches, int numAccounts, AccountAmount initAmount,
                AccountAmount reportingAmount,
                int numWorkers);

int Bank_Validate(Bank *bank);
int Bank_Compare(Bank *bank1, Bank *bank2);



#endif /* _BANK_H */
