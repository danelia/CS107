#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"
#include "bank.h"
#include "branch.h"

/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  BranchID branchID = AccountNum_GetBranchID(accountNum);
  
  sem_wait(&(account->accLock));
  sem_wait(&(bank->branches[branchID].branchLock));
  
  if (account == NULL) {
	sem_post (&(account->accLock));
	sem_post(&(bank->branches[branchID].branchLock));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account_Adjust(bank,account, amount, 1);
  
  sem_post (&(account->accLock));
  sem_post(&(bank->branches[branchID].branchLock));
  
  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);
  
  BranchID branchID = AccountNum_GetBranchID(accountNum);
  
  sem_wait(&(account->accLock));
  sem_wait(&(bank->branches[branchID].branchLock));

  if (account == NULL) {
	sem_post (&(account->accLock));
	sem_post(&(bank->branches[branchID].branchLock));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(account)) {
	sem_post (&(account->accLock));
	sem_post(&(bank->branches[branchID].branchLock));
    return ERROR_INSUFFICIENT_FUNDS;
  }
  
  Account_Adjust(bank,account, -amount, 1);
  
  sem_post (&(account->accLock));
  sem_post(&(bank->branches[branchID].branchLock));

  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);
  
  if(srcAccountNum == dstAccountNum) 
    return ERROR_SUCCESS;

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);
  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if(srcAccount > dstAccount){
    sem_wait(&dstAccount->accLock);
    sem_wait(&srcAccount->accLock);
  } else {
    sem_wait(&srcAccount->accLock);
    sem_wait(&dstAccount->accLock);
  }
  
  if (amount > Account_Balance(srcAccount)) {
	sem_post(&srcAccount->accLock);
    sem_post(&dstAccount->accLock);
    return ERROR_INSUFFICIENT_FUNDS;
  }

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);
  
  int srcBranchID, dstBranchID;
  srcBranchID = AccountNum_GetBranchID(srcAccountNum);
  dstBranchID = AccountNum_GetBranchID(dstAccountNum);

  if(updateBranch){
    if(srcAccountNum > dstAccountNum){
      sem_wait(&bank->branches[dstBranchID].branchLock);
      sem_wait(&bank->branches[srcBranchID].branchLock);
    } else {
      sem_wait(&bank->branches[srcBranchID].branchLock);
      sem_wait(&bank->branches[dstBranchID].branchLock);
    }
  }
  Account_Adjust(bank, srcAccount, -amount, 0);
  Account_Adjust(bank, dstAccount, amount, 0);
  if(updateBranch){
    sem_post(&bank->branches[srcBranchID].branchLock);
    sem_post(&bank->branches[dstBranchID].branchLock);
  }
  sem_post(&srcAccount->accLock);
  sem_post(&dstAccount->accLock);

  return ERROR_SUCCESS;
  
}
