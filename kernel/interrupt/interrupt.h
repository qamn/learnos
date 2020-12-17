#ifndef LEARNOS_INTERRUPT_H
#define LEARNOS_INTERRUPT_H

#include "../lib/linkage.h"

void init_interrupt();

void do_IRQ(unsigned long regs,unsigned long nr);


#endif //LEARNOS_INTERRUPT_H
