#ifndef _REPORT_H
#define _REPORT_H

#include <stdint.h>


int Report_Init(struct Bank *bank, AccountAmount reportAmount,
                int maxNumWorkers);

int Report_DoReport(struct Bank *bank, int workerNum);
int Report_Transfer(struct Bank *bank, int workerNum, AccountNumber accountNum,
                    AccountAmount amount);

int Report_Compare(struct Bank *bank1, struct Bank *bank2);



#endif /* _REPORT_H */
