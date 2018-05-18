#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "teller.h"
#include "account.h"
#include "branch.h"
#include "error.h"
#include "debug.h"

#include "action.h"

static int numBranches;
static int numAcctPerBranch;
static int amountMax;
static int cmdsPerReportPerWorker;

static struct {

  int cmdCount;
  int nextReport;
  int nextSeed;
  unsigned int seed;
  int seedList[MAX_WORKERS];

} workerState[MAX_WORKERS];

/*
 * pre-assign actions to the workers by initilizing the worker states
 */
void
Action_Init(int configNumBranches, int configNumAccounts, int numCommands,
            int maxTransaction, int numWorkers, unsigned int initSeed)
{
  numBranches = configNumBranches;
  numAcctPerBranch = configNumAccounts/configNumBranches;
  amountMax = maxTransaction;

  const int COMMANDS_PER_REPORT = 4;
  cmdsPerReportPerWorker = (numCommands / COMMANDS_PER_REPORT)/numWorkers;

  int perWorker = numCommands / numWorkers;
  for (int w = 0; w < numWorkers; w++) {
    workerState[w].cmdCount = perWorker;
    workerState[w].nextReport = perWorker - cmdsPerReportPerWorker;

    numCommands -= perWorker;

    if (numCommands <= 0)
      break;
  }

  for (int w = 0; w < MAX_WORKERS; w++) {
    workerState[w].nextSeed = 0;
    workerState[w].seed = initSeed + w; /* Give each worker a different start seed */

    for (int i = 0; i < MAX_WORKERS; i++) {
      workerState[w].seedList[i] = (w + (i * numWorkers)) % MAX_WORKERS;
    }
  }
}

/*
 * get a random integer based on the worker number
 */
static int
GetRandom(int workerNum, int isBellDistribution, unsigned int maxValue)
{
  int ns = workerState[workerNum].nextSeed % MAX_WORKERS;
  int ws = workerState[workerNum].seedList[ns];

  DPRINTF('r', ("GetRandom(%d,%d) from seed %d\n",
                workerNum, isBellDistribution, ws));

  if (isBellDistribution) {
    // sum of 3 random numbers gives us something like a bell curve disribution
    unsigned int u;
    do {
      unsigned int sum = 0;
      for (int i = 0; i < 3; i++)
        sum += rand_r(&workerState[ws].seed) % maxValue;

      u = sum / 3;
    } while (u >= maxValue);

    return u;

  } else {
    // Uniform random distribution
    return rand_r(&workerState[ws].seed);
  }
}

/*
 * get the next action command for the worker
 */
int
Action_GetNext(int workerNum, Action *action, int control)
{
  extern int testfailurecode;

  if (workerState[workerNum].cmdCount <= 0) {
    action->cmd = ACTION_DONE;
    return 0;
  }

  if (workerState[workerNum].cmdCount == workerState[workerNum].nextReport) {
    workerState[workerNum].nextReport -= cmdsPerReportPerWorker;
    action->cmd = ACTION_REPORT;
    action->u.reportArg.workerNum = workerNum;
    return 0;
  }

  workerState[workerNum].cmdCount--;

  int sel = GetRandom(workerNum,0,0) & 0x7;
  switch (sel) {
  case 0:
  case 1:
  case 2: {
    int account = GetRandom(workerNum,0,0) % numAcctPerBranch;
    int branch = GetRandom(workerNum,0,0) % numBranches;
    int amount   = GetRandom(workerNum,1,amountMax);
    action->cmd = (sel == 0) ? ACTION_WITHDRAW : ACTION_DEPOSIT;
    action->u.depwithArg.accountNum = Account_MakeAccountNum(branch, account);
    action->u.depwithArg.amount = amount % amountMax;
    if ((control & ACTION_NO_FUNDS_FLOW) || 
        (testfailurecode && (sel != 0) && ((account & 0x3) == 0))) {
      /* Either we are told to have no money flows in/out or we need 
       * to test error handling, so we zero out all requests.  */
      action->u.depwithArg.amount = 0;
    }
    break;
  }

  case 3:
  case 4:
  case 5: {
    int srcAccount = GetRandom(workerNum,0,0) % numAcctPerBranch;
    int srcBranch = GetRandom(workerNum,0,0) % numBranches;
    int dstBranch = GetRandom(workerNum,0,0)  % numBranches;
    int dstAccount = GetRandom(workerNum,0,0) % numAcctPerBranch;
    int amount   = GetRandom(workerNum,1,amountMax);
    int noCross = (control & ACTION_NO_CROSS_TRANSFER);
    BranchID branchID = ((sel == 5) || noCross) ? srcBranch : dstBranch;

    action->cmd =  ACTION_TRANSFER;
    action->u.transArg.srcAccountNum = Account_MakeAccountNum(srcBranch,
                                                              srcAccount);
    action->u.transArg.dstAccountNum = Account_MakeAccountNum(branchID,
                                                              dstAccount);
    action->u.transArg.amount = (amount % amountMax);
    if (testfailurecode && ((dstAccount & 0x3) == 0)) {
      action->u.transArg.amount = 0;
    }

    break;
  }
  case 6: {
    action->cmd = ACTION_BRANCH_BALANCE;
    action->u.branchArg.branchID = (GetRandom(workerNum,0,0) % numBranches);
    break;
  }
  case 7: {
    if (control & ACTION_NO_BANK_BALANCE) {
      //do not generate Bank_Balance
      DPRINTF('z', ("GetRandom(%d) skipping Bank_Balance\n", workerNum));
      //instead generate branch balance
      action->cmd = ACTION_BRANCH_BALANCE;
      action->u.branchArg.branchID = (GetRandom(workerNum,0,0) % numBranches);
    } else {
      action->cmd = ACTION_BANK_BALANCE;
    }
    break;
  }
  }
  workerState[workerNum].nextSeed++;
  return 0;
}
