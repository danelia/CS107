#ifndef _BRANCH_H
#define _BRANCH_H

#include <stdint.h>


typedef uint64_t BranchID;

typedef struct Branch {
  BranchID branchID;
  AccountAmount balance;
  sem_t branchLock;
  int numberAccounts;
  Account   *accounts;
} Branch;


int Branch_Balance(struct Bank *bank, BranchID branchID, AccountAmount *balance);
int Branch_UpdateBalance(struct Bank *bank, BranchID branchID,
                         AccountAmount change);


int Branch_Init(struct Bank *bank, int numBranches, int numAccounts,
                AccountAmount initialAmount);

int Branch_Validate(struct Bank *bank, BranchID branchID);
int Branch_Compare(Branch *branch1, Branch *branch2);


#endif /* _BRANCH_H */
