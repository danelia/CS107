#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

#include <pthread.h>

#include "bank.h"
#include "account.h"
#include "branch.h"
#include "teller.h"
#include "action.h"
#include "report.h"
#include "error.h"

#include "debug.h"


int testfailurecode = 0;  /* non-zero if we should test failure code */
int testbankbalance = 0; /* Run a test of the bank balance code */
int racechecker = 0;     /* non-zero if we should reduce tests for race checker. */
int numWorkers = 1; /* by default no concurrency. */
int testRunNum = 1; /* Default test is test 1. */
int actionControl = 0; /* Control flags for the action generator. */
unsigned int randSeed = 0;   /* Random number generator seed - Default is use time */
Bank *bank;

/*
 * Support for testing bank balance commands. 
 */

/* What bank balance should be */
static AccountAmount fixedBankBalance = 0; 

/* Number of errors detected per worker */
static int workerBalanceErrors[MAX_WORKERS] = {0};


static Bank *CreateBank(int testRunNum, int numWorkers, unsigned int initSeed,
                        int verbose);
static int MultipleWorkers(int numWorkers);
static void *Worker(void *threadarg);
static void TestBank(int testRunNumber, unsigned int initSeed,
                     uint64_t totalTime);
static int64_t  GetTimeInMicrosecs(void);
static void PrintUsageAndExit(char *progname);

int
main(int argc, char *argv[])
{
  int opt;
  char nullString[1] = {0};
  char *debugFlagArgs = nullString;
  int yieldpercent = 0;

  while ((opt = getopt(argc, argv, "w:d:t:s:hfbry::")) != -1) {
    switch (opt) {
    case 'w':
      numWorkers = atoi(optarg);
      break;
    case 't':
      testRunNum = atoi(optarg);
      break;
    case 's':
      randSeed = atoi(optarg);
      break;
    case 'f':
      testfailurecode = 1;
      break;
    case 'b':
      testbankbalance = 1;
      break;
    case 'r':
      racechecker = 1;
      break;
    case 'd':
      debugFlagArgs = optarg;
      break;
    case 'y':
      if (optarg) {
        yieldpercent = atoi(optarg);
      } else {
        yieldpercent = 5;  /* default to 5% yields */
      }
      if ((yieldpercent < 0) || (yieldpercent > 100)) {
        fprintf(stderr, "Invaid -y option (%s) must be a precentage (0-100)\n", optarg);
        PrintUsageAndExit(argv[0]);
      }
      break;
    case 'h':
    default:
      PrintUsageAndExit(argv[0]);
    }
  }

  /* If we are using the repeatable random number generator we can only handle these */
  if (!((numWorkers == 1) || (numWorkers == 2) || (numWorkers == 4) ||
        (numWorkers == 8) ||
        (numWorkers == 16))) {
    fprintf(stderr, "Number of workers must be a 1,2,4,8 or 16\n");
    PrintUsageAndExit(argv[0]);
  }

  if ((numWorkers < 1) || (numWorkers > MAX_WORKERS)) {
    PrintUsageAndExit(argv[0]);
  }

  if (randSeed == 0) {
    randSeed = time(0);
  }

  Debug_Init(debugFlagArgs, yieldpercent, randSeed);

  bank = CreateBank(testRunNum, numWorkers, randSeed, 1);

  uint64_t startTime = GetTimeInMicrosecs();

  if (numWorkers > 1) {
    int err = MultipleWorkers(numWorkers);
    if(err < 0)
      exit(-1);
  } else {
    int w = -1;
    Worker(&w);
  }

  uint64_t endTime = GetTimeInMicrosecs();
  uint64_t totalTime = (endTime - startTime);

  printf("All workers done in %.02f seconds.\nComparing with sequential run ...\n",
         totalTime/1000000.0);

  TestBank(testRunNum, randSeed, totalTime);

  Bank_Dispose(bank);
  pthread_exit(NULL);
  
  exit(EXIT_SUCCESS);
  return 0;
}

/*
 * pre-assign actions for each workers
 * also create and initilize the bank
 */
static Bank *
CreateBank(int testRunNum, int numWorkers, unsigned int initSeed, int verbose)
{
  int numBranches;
  int numAccounts;
  int numCommands;
  int maxTransactionSize;
  int initialAmount;
  int reportingAmount;

  switch (testRunNum ) {

  case 7: {
    /* Test the bank balance */
    /* Large number of branches so that most transfers will 
     * need to update the bank balance */
    testbankbalance = 1;
    numBranches = 256;
    numAccounts = numBranches * 64;
    numCommands = 4*1024*1024;
    maxTransactionSize = 20;
    initialAmount = 5000;
    reportingAmount = 19;
    break;
  }

  case 6: {
    /* Branches with few accounts */
    /* no cross branch transfers or bank balance. */
    actionControl |= ACTION_NO_CROSS_TRANSFER;
    actionControl |= ACTION_NO_BANK_BALANCE;
    numBranches = 32;
    numAccounts = numBranches * 4;
    numCommands = 16*1024*1024;
    maxTransactionSize = 20;
    initialAmount = 5000;
    reportingAmount = 19;
    break;
  }


  case 5: {
    /* Large number of branches to ease concurrency */
    /* no cross branch transfers or bank balance. */
    actionControl |= ACTION_NO_CROSS_TRANSFER;
    actionControl |= ACTION_NO_BANK_BALANCE;
    numBranches = 256;
    numAccounts = numBranches * 64;
    numCommands = 16*1024*1024;
    maxTransactionSize = 20;
    initialAmount = 5000;
    reportingAmount = 19;
    break;
  }


  case 3: {
    /* Large number of branches and accounts to ease concurrency */
    actionControl |= ACTION_NO_BANK_BALANCE;
    numBranches = 256;
    numAccounts = numBranches * 4096;
    numCommands = 16*1024*1024;
    maxTransactionSize = 20;
    initialAmount = 5000;
    reportingAmount = 19;
    break;
  }
  case 2: {
    /* Small number of branches and accounts to really stress concurrency */
    numBranches = 4;
    numAccounts = numBranches * 2;
    numCommands = 8*1024*1024;
    maxTransactionSize = 128;
    initialAmount = 10000;
    reportingAmount = 124;
    break;
  }

  case 4:
    /* "Normal" run with failures*/
    testfailurecode = 1;
  case 1:
  default: {
    /* "Normal" run */
    numBranches = 16;
    numAccounts = numBranches * 1024;
    numCommands = 8*1024*1024;
    maxTransactionSize = 1024;
    initialAmount = 100000;
    reportingAmount = 1010;
    break;
  }

  }
  if (racechecker) {
    // Reduce the number of commands if we're running under Helgrind.
    numCommands = 1024*4;
  }
  if (verbose) {
    printf(
      "BANK test %d: branches %d accounts %d commands %d workers %d reporting %d initial seed %u\n",
      testRunNum, numBranches, numAccounts, numCommands, numWorkers,
      reportingAmount, initSeed);
  }
  if (testbankbalance) {
    actionControl |= ACTION_NO_FUNDS_FLOW;
  }
  Action_Init(numBranches, numAccounts, numCommands, maxTransactionSize,
              numWorkers,
              initSeed);

  bank = Bank_Init(numBranches, numAccounts, initialAmount, reportingAmount,
                   numWorkers);

  if (testbankbalance) {
    int err = Bank_Balance(bank, &fixedBankBalance);
    if (err < 0) { 
      fprintf(stderr, "Error computing Bank_Balance - balance check not performed.\n"); 
      testbankbalance = 0;
    }
  }

  return bank;
}

/*
 * create multiple threads acting as the workers
 */
static int
MultipleWorkers(int numWorkers)
{
  struct {
    pthread_t thread;
    int id;
  } workers[MAX_WORKERS];

  pthread_attr_t attr;
  int rc;

  /* For portability, explicitly create threads in a joinable state */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for(int w = 0; w < numWorkers; w++) {
    workers[w].id = w;
    rc = pthread_create(&workers[w].thread, &attr,
                        Worker, (void *)&(workers[w].id));
    if (rc) {
      fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
      return -1;
    }
  }

  for (int w = 0; w  < numWorkers; w++) {
    pthread_join(workers[w].thread, NULL);
  }
  pthread_attr_destroy(&attr);

  return 0;
}

/*
 * passed-in function of each thread; it exits until all actions assigned
 * to this worker is done.
 */
static void *
Worker(void *threadarg)
{
  int workerNum = *(int *) threadarg;
  int numBalanceErrors = 0;
  int noexit = 0;

  if (workerNum < 0) {
    workerNum = 0;
    noexit = 1;
  }

  DPRINTF('w', ("Worker(%d) starting\n", workerNum));

  while (1) {
    Action action;
    int err = Action_GetNext(workerNum, &action, actionControl);

    if (err < 0) {
      break;
    }

    DPRINTF('x', ("W%d:", workerNum));

    AccountAmount balance;

    switch (action.cmd) {
    case ACTION_DONE:
      err = -1;
      break;
    case ACTION_DEPOSIT:
      err = Teller_DoDeposit(bank, action.u.depwithArg.accountNum,
                             action.u.depwithArg.amount);
      if (err == ERROR_SUCCESS) {
        Report_Transfer(bank, workerNum,  action.u.depwithArg.accountNum,
                        action.u.depwithArg.amount);
      }

      break;
    case ACTION_WITHDRAW:
      err = Teller_DoWithdraw(bank, action.u.depwithArg.accountNum,
                              action.u.depwithArg.amount);
      if (err == ERROR_SUCCESS) {
        Report_Transfer(bank, workerNum,  action.u.depwithArg.accountNum,
                        -action.u.depwithArg.amount);
      }
      break;
    case ACTION_TRANSFER:
      err = Teller_DoTransfer(bank,
                              action.u.transArg.srcAccountNum,
                              action.u.transArg.dstAccountNum,
                              action.u.transArg.amount);
      if (err == ERROR_SUCCESS) {
        Report_Transfer(bank, workerNum,  action.u.transArg.srcAccountNum,
                        -action.u.transArg.amount);
        Report_Transfer(bank, workerNum,  action.u.transArg.dstAccountNum,
                        action.u.transArg.amount);
      }

      break;
    case ACTION_BRANCH_BALANCE:
      err = Branch_Balance(bank,action.u.branchArg.branchID, &balance);
      DPRINTF('b', ("Branch %"PRIu64" balance is %"PRId64"\n",
                    action.u.branchArg.branchID, balance));
      break;
    case ACTION_BANK_BALANCE:
      err = Bank_Balance(bank, &balance);
      DPRINTF('b', ("Bank balance is %"PRId64"\n", balance));
      if (testbankbalance && (fixedBankBalance != balance)) {
        numBalanceErrors++;
        fprintf(stderr, "Bank balance incorrect (%"PRId64" != %"PRId64")\n", 
                balance, fixedBankBalance);

      }
      break;
    case ACTION_REPORT:
      err = Report_DoReport(bank, action.u.reportArg.workerNum);
      if (err != 0) {
        fprintf(stderr, "Report_DoReport(Worker=%d) returns %d\n",
                action.u.reportArg.workerNum,
                err);
        err = 0; // Mask error so we don't abort on a report error
      }
      break;
    default:
      fprintf(stderr, "Unknown action cmd %d\n", action.cmd);
      err = -1;
      break;
    }
    if (err < 0) break;

  }
  workerBalanceErrors[workerNum] = numBalanceErrors;
  DPRINTF('w', ("Worker(%d) exiting\n", workerNum));
  if (!noexit) {
    pthread_exit(NULL);
  }
  return NULL;
}

/*
 * test the data in the bank after all workers are done.
 */
static void
TestBank(int testRunNumber, unsigned int initSeed, uint64_t totalTime)
{
  Bank *bankmulti = bank;
  int balanceCmdErrors = 0;

  for (int i = 0; i < MAX_WORKERS; i++) {
    balanceCmdErrors += workerBalanceErrors[i];
  }

  if (balanceCmdErrors) { 
    fprintf(stderr, "%d bank balance command errors detected.\n", 
            balanceCmdErrors);
  }
    
  bank =  CreateBank(testRunNumber, 1, initSeed, 0);

  uint64_t startTime = GetTimeInMicrosecs();

  int w = -1;
  Worker(&w);

  uint64_t endTime = GetTimeInMicrosecs();

  int err = Bank_Compare(bank, bankmulti);
  if ((err < 0) || balanceCmdErrors) {
    fprintf(stderr,"Bank testrun %d compare FAILED. Time ratio %.02f\n",
            testRunNumber,
            (double)totalTime/(double)(endTime - startTime));

  } else {
    printf("Bank testrun %d PASSED all compare tests. Time ratio %.02f\n",
           testRunNumber,
           (double)totalTime/(double)(endTime - startTime));
  }
  
  Bank_Dispose(bankmulti);   
}


/*
 * get micro second of current time
 */
static int64_t
GetTimeInMicrosecs(void)
{
  struct timeval curtime;

  int err = gettimeofday(&curtime, (struct timezone *) NULL);

  if (err < 0) {
    fprintf(stderr, "Can't read time of day\n");
    return 0;
  }
  return  ((curtime.tv_sec * (int64_t) 1000000) + curtime.tv_usec);

}


void
PrintUsageAndExit(char *progname)
{
    const char *usage =
        "Usage: %s <options>\n"
        "where <options> can be:\n"
        "  -wN        Use N workers. N must be 1, 2, 4, 8, or 16.\n"
        "  -tN        Run test N, where N is 1-7. Each test varies bank\n"
        "             parameters such as number of branches, accounts, and\n"
        "             commands, and maximum transaction amount. See CreateBank\n"
        "             for how each test N is mapped to a test case.\n"
        "  -dCHARS    Enable DPRINTF calls corresponding to the letters in CHARS.\n"
        "  -b         Verify that the bank balance commands work correctly;\n"
        "             test 7 does this implicitly.\n"
        "  -yN        Cause the thread to yield in N percent of cases. If N is\n"
        "             omitted, this defaults to 5. This helps expose race\n"
        "             conditions.\n"
        "  -r         Regardless of which test is run, severely reduce the total\n"
        "             number of actions. This allows your code to run in a\n"
        "             reasonable amount time under a race-checker like Helgrind.\n"
        "  -sN        Seed the random number generator with N. If no -s argument\n"
        "             is included, or -s0 is used, the seed will be based on the\n"
        "             clock.\n"
        "  -f         Initialize the bank such that some transfers are\n"
        "             guaranteed to fail.\n"
        "  -h         Print this help message.\n";
    fprintf(stderr, usage, progname);
    exit(EXIT_FAILURE);
}
