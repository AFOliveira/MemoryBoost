/**
 * SPDX-License-Identifier: Apache-2.0 
 * Copyright (c) Bao Project and Contributors. All rights reserved
 */

#include <mem_throt.h>
#include <cpu.h>
#include <vm.h>
#include <spinlock.h>

spinlock_t lock;


const size_t DT[3] = {100000, 10000, 1000};


volatile size_t c_bus_access = 0;

// void mem_throt_period_timer_callback_c(irqid_t int_id) {
//     timer_disable();
//     c_bus_access += events_get_cntr_value(cpu()->vcpu->vm->mem_throt.counter_id);
//     timer_reschedule_interrupt(cpu()->vcpu->vm->mem_throt.period_counts);
//     timer_enable();
// }

// Selects the appropriate DT value based on 'val'.
static inline size_t select_dt_value(uint64_t val) {
    // The comparisons yield 0 or 1 and by adding them we get the correct index.
    size_t index = (val >= 1000) + (val >= 5000);
    return DT[index];
}

void mem_throt_period_timer_callback_nc(irqid_t int_id) {
    timer_disable();
    if (cpu()->vcpu->vm->mem_throt.c_vm) {
        uint64_t new_val = events_get_cntr_value(cpu()->vcpu->vm->mem_throt.counter_id);
        atomic_store64_release(&c_bus_access, new_val);

        pmu_reset_event_counters();

        events_clear_cntr_ovs(cpu()->vcpu->vm->mem_throt.counter_id);
        events_arch_cntr_enable(cpu()->vcpu->vm->mem_throt.counter_id);
        events_cntr_set(cpu()->vcpu->vm->mem_throt.counter_id, 0);
    } else {
        events_cntr_disable(cpu()->vcpu->vm->mem_throt.counter_id);

        if (cpu()->vcpu->mem_throt.throttled) {
            events_cntr_irq_enable(cpu()->vcpu->vm->mem_throt.counter_id);
            cpu()->vcpu->mem_throt.throttled = false;
        }
        events_cntr_enable(cpu()->vcpu->vm->mem_throt.counter_id);
        
        if (cpu()->vcpu->vm->master)
            cpu()->vcpu->vm->mem_throt.budget_left = cpu()->vcpu->vm->mem_throt.budget;

        // Load the shared value and choose the DT value accordingly.
        uint64_t val = atomic_load64_acquire(&c_bus_access);
        size_t new_budget = select_dt_value(val);

        events_cntr_set(cpu()->vcpu->vm->mem_throt.counter_id, new_budget);
    }
    timer_reschedule_interrupt(cpu()->vcpu->vm->mem_throt.period_counts);
    timer_enable();
}

void mem_throt_event_overflow_callback(irqid_t int_id) {

    events_clear_cntr_ovs(cpu()->vcpu->vm->mem_throt.counter_id);
    events_cntr_disable(cpu()->vcpu->vm->mem_throt.counter_id);
    events_cntr_irq_disable(cpu()->vcpu->vm->mem_throt.counter_id);

    // spin_lock(&lock);
    // cpu()->vcpu->vm->mem_throt.budget_left -= (cpu()->vcpu->vm->mem_throt.budget / cpu()->vcpu->vm->cpu_num);
    // spin_unlock(&lock);
    
    cpu()->vcpu->mem_throt.throttled = true;  
    cpu_standby();

}


void mem_throt_timer_init(irq_handler_t handler) {
    timer_define_irq_callback(handler);
    cpu()->vcpu->vm->mem_throt.period_counts = timer_init(cpu()->vcpu->vm->mem_throt.period_us);
}


void mem_throt_events_init(events_enum event, unsigned long budget, irq_handler_t handler) {

    if ((cpu()->vcpu->vm->mem_throt.counter_id = events_cntr_alloc()) == ERROR_NO_MORE_EVENT_COUNTERS) {
        ERROR("No more event counters!");
    }

    events_set_evtyper(cpu()->vcpu->vm->mem_throt.counter_id, event);
    events_cntr_set(cpu()->vcpu->vm->mem_throt.counter_id, budget);
    events_cntr_set_irq_callback(handler, cpu()->vcpu->vm->mem_throt.counter_id);
    events_clear_cntr_ovs(cpu()->vcpu->vm->mem_throt.counter_id);
    events_interrupt_enable(cpu()->id);
    events_cntr_irq_enable(cpu()->vcpu->vm->mem_throt.counter_id);
    events_enable();
    events_cntr_enable(cpu()->vcpu->vm->mem_throt.counter_id);
}

inline void mem_throt_budget_change(size_t budget) {
    events_cntr_set(cpu()->vcpu->vm->mem_throt.counter_id, cpu()->vcpu->vm->mem_throt.budget);
    events_cntr_enable(cpu()->vcpu->vm->mem_throt.counter_id);
    events_cntr_irq_enable(cpu()->vcpu->vm->mem_throt.counter_id);
}

void perf_monitor_setup_event_counters(size_t counter_id) {
        events_cntr_set(counter_id, 0);
        events_enable();
        events_set_evtyper(counter_id, bus_access);
        events_clear_cntr_ovs(counter_id);
        events_cntr_enable(counter_id);
}

void mem_throt_config(size_t period_us, size_t vm_budget, size_t* cpu_ratio, size_t asil_criticality) {
    
    cpu()->vcpu->vm->mem_throt.c_vm = 0;
    if(period_us == 0) return;

    if (cpu()->id == cpu()->vcpu->vm->master) 
    {   
        vm_budget = vm_budget / cpu()->vcpu->vm->cpu_num;
        cpu()->vcpu->vm->mem_throt.throttled = false;
        cpu()->vcpu->vm->mem_throt.period_us = period_us;
        cpu()->vcpu->vm->mem_throt.budget = vm_budget * cpu()->vcpu->vm->cpu_num ;
        cpu()->vcpu->vm->mem_throt.budget_left = cpu()->vcpu->vm->mem_throt.budget;
        cpu()->vcpu->vm->mem_throt.is_initialized = true;
    }
    cpu()->vcpu->vm->mem_throt.c_vm = asil_criticality;

    while(cpu()->vcpu->vm->mem_throt.is_initialized != true);

    spin_lock(&lock);

    if (cpu_ratio[cpu()->vcpu->id] == 0) {
        cpu_ratio[cpu()->vcpu->id] = cpu()->vcpu->vm->mem_throt.budget / cpu()->vcpu->vm->cpu_num;
    }
    
    cpu()->vcpu->mem_throt.assign_ratio = cpu_ratio[cpu()->vcpu->id]; 
    cpu()->vcpu->mem_throt.budget = cpu()->vcpu->vm->mem_throt.budget * (cpu()->vcpu->mem_throt.assign_ratio) / 100;
    cpu()->vcpu->vm->mem_throt.budget -= cpu()->vcpu->mem_throt.budget;
    cpu()->vcpu->vm->mem_throt.budget_left -= cpu()->vcpu->mem_throt.budget;
    cpu()->vcpu->vm->mem_throt.assign_ratio += cpu()->vcpu->mem_throt.assign_ratio;
    // cpu()->vcpu->vm->mem_throt.counter_id = 1;

    spin_unlock(&lock);


    if(cpu()->vcpu->vm->mem_throt.assign_ratio > 100){
        ERROR("The sum of the ratios is greater than 100");
    }

    
}

void mem_throt_init() {
    if(cpu()->vcpu->vm->mem_throt.is_initialized != true) return;

    if (cpu()->vcpu->vm->mem_throt.budget != 0){
        mem_throt_events_init(bus_access, cpu()->vcpu->mem_throt.budget, mem_throt_event_overflow_callback);
    }
    else{
        // cpu()->vcpu->vm->mem_throt.c_vm = 1;
        perf_monitor_setup_event_counters(cpu()->vcpu->vm->mem_throt.counter_id);
        // mem_throt_timer_init(mem_throt_period_timer_callback_c);
    }
    mem_throt_timer_init(mem_throt_period_timer_callback_nc);

}

