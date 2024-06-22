#ifndef _IRQ_H
#define _IRQ_H
void irq_install();
void irq_remap(void);
void irq_install_handler(int irq, void (*handler));
void irq_uninstall_handler(int irq);

#endif