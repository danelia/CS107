#ifndef _ACTION_H
#define _ACTION_H


#define MAX_WORKERS 16

typedef enum {
  ACTION_DONE,
  ACTION_DEPOSIT,
  ACTION_WITHDRAW,
  ACTION_TRANSFER,
  ACTION_BRANCH_BALANCE,
  ACTION_BANK_BALANCE,
  ACTION_REPORT
} ActionType;

typedef struct Action {
  ActionType cmd;
  union {
    struct {
      AccountNumber accountNum;
      AccountAmount amount;
    } depwithArg;
    struct  {
      AccountNumber srcAccountNum;
      AccountNumber dstAccountNum;
      AccountAmount amount;
    } transArg;
    struct  {
      BranchID branchID;
    } branchArg;
    struct {
      int workerNum;
    } reportArg;
  } u;
} Action;

#define ACTION_NO_BANK_BALANCE  (1 << 0)
#define ACTION_NO_CROSS_TRANSFER (1 << 1)
#define ACTION_NO_FUNDS_FLOW     (1 << 2)


int Action_GetNext(int workerNum, Action *action, int control);

void Action_Init(int configNumBranches, int configNumAccounts, int numCommands,
                 int maxTransaction,
                 int numWorkers,
                 unsigned int initSeed);


#endif /* _ACTION_H */
