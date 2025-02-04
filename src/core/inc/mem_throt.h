/**
 * SPDX-License-Identifier: Apache-2.0 
 * Copyright (c) Bao Project and Contributors. All rights reserved
 */

#ifndef __mem_throt_H__
#define __mem_throt_H__

#include <timer.h>
#include <events.h>
#include <bitmap.h>


typedef struct mem_throt_info {
	bool is_initialized;
	bool throttled;			 
	size_t counter_id;
	size_t period_us;
	size_t period_counts;
	size_t budget; 
	int64_t budget_left;
	size_t assign_ratio;
	size_t c_vm;
}mem_throt_t;

extern const size_t DT[3];

extern volatile size_t c_bus_access;

extern size_t global_num_ticket_hypervisor;

void mem_throt_config(size_t period_us, size_t vm_budget, size_t* cpu_ratio);

void mem_throt_init();

// void mem_throt_period_timer_callback_c(irqid_t);
void mem_throt_period_timer_callback_nc(irqid_t);

/* budget is used up. PMU generate an interrupt */
void mem_throt_event_overflow_callback(irqid_t); 
void mem_throt_process_overflow(void);

void mem_throt_timer_init(irq_handler_t hander);
void mem_throt_events_init(events_enum event, unsigned long budget, irq_handler_t handler);
void mem_throt_budget_change(uint64_t budget);
void perf_monitor_setup_event_counters(size_t counter_id);


static inline uint64_t atomic_load64_acquire(const volatile uint64_t *addr)
{
    uint64_t val;
    __asm__ volatile (
        "ldar %0, [%1]"
        : "=r" (val)
        : "r" (addr)
        : "memory"
    );
    return val;
}

static inline void atomic_store64_release(volatile uint64_t *addr, uint64_t val)
{
    __asm__ volatile (
        "stlr %0, [%1]"
        :
        : "r" (val), "r" (addr)
        : "memory"
    );
}


#endif /* __mem_throt_H__ */