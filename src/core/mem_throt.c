/**
 * SPDX-License-Identifier: Apache-2.0 
 * Copyright (c) Bao Project and Contributors. All rights reserved
 */

#include <mem_throt.h>
#include <cpu.h>
#include <vm.h>


/**
 * @brief Callback function for memory throttling period timer interrupt.
 *
 * This function is called when the memory throttling period timer interrupt occurs.
 * It performs the following actions:
 * - Disables the timer.
 * - Disables the event counter associated with memory throttling.
 * - Reschedules the timer interrupt based on the configured period counts.
 * - Sets the event counter to the configured budget.
 * - If the memory throttling is active, enables the event counter interrupt and marks
 *   the throttling as inactive.
 * - Enables the event counter.
 * - Enables the timer.
 * - Prints a debug message indicating the timer callback execution.
 *
 * @param int_id The interrupt ID associated with the timer interrupt.
 */
void mem_throt_period_timer_callback(irqid_t int_id) {

    timer_disable();
    events_cntr_disable(cpu()->vcpu->vm->mem_throt.counter_id);
    timer_reschedule_interrupt(cpu()->vcpu->vm->mem_throt.period_counts);
    events_cntr_set(cpu()->vcpu->vm->mem_throt.counter_id, cpu()->vcpu->vm->mem_throt.budget);

    if (cpu()->vcpu->vm->mem_throt.throttled) {
        events_cntr_irq_enable(cpu()->vcpu->vm->mem_throt.counter_id);
        cpu()->vcpu->vm->mem_throt.throttled = false;
    }
    

    events_cntr_enable(cpu()->vcpu->vm->mem_throt.counter_id);
    timer_enable();
    console_printk("Hypervisor Timer callback %d\n", cpu()->id);
}

/**
 * @brief Callback function for handling memory overflow events.
 *
 * This function is triggered when a memory overflow event occurs. It performs
 * the following actions:
 * - Prints a message indicating a memory overflow on the current CPU.
 * - Clears the overflow counter for the current virtual machine's memory throttle.
 * - Disables the event counter for the current virtual machine's memory throttle.
 * - Disables the interrupt for the event counter of the current virtual machine's memory throttle.
 * - Sets the throttled flag to true for the current virtual machine's memory throttle.
 * - Puts the CPU into standby mode.
 *
 * @param int_id The interrupt ID associated with the memory overflow event.
 */
void mem_throt_event_overflow_callback(irqid_t int_id) {
    console_printk("Memory Overflow on cpu %d\n", cpu()->id);
    events_clear_cntr_ovs(cpu()->vcpu->vm->mem_throt.counter_id);
    events_cntr_disable(cpu()->vcpu->vm->mem_throt.counter_id);
    events_cntr_irq_disable(cpu()->vcpu->vm->mem_throt.counter_id);
    cpu()->vcpu->vm->mem_throt.throttled = true;
    
    cpu_standby();
}


/**
 * @brief Initializes the memory throttling timer for the current CPU.
 *
 * This function sets up the memory throttling timer by printing a message to the console,
 * defining the IRQ callback handler, and initializing the timer with the specified period.
 *
 * @param handler The IRQ handler to be called when the timer expires.
 */
void mem_throt_timer_init(irq_handler_t handler) {
    console_printk("Memory Throttling Timer Init on cpu %d\n", cpu()->id);
    timer_define_irq_callback(handler);
    cpu()->vcpu->vm->mem_throt.period_counts = timer_init(cpu()->vcpu->vm->mem_throt.period_us);
}


/**
 * @brief Initializes memory throttling events.
 *
 * This function sets up the memory throttling events by allocating an event counter,
 * setting the event type, budget, and interrupt handler, and enabling the necessary
 * interrupts and counters.
 *
 * @param event The event type to be monitored.
 * @param budget The budget for the event counter.
 * @param handler The interrupt handler to be called when the event occurs.
 *
 * @note This function assumes that the CPU, VCPU, and VM structures are properly initialized
 *       and accessible through the `cpu()` function.
 *
 * @warning If no more event counters are available, an error message is logged.
 */
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

/**
 * @brief Initializes memory throttling for the current CPU.
 *
 * This function sets up memory throttling parameters and initializes the
 * necessary timers and events for the current CPU's virtual machine.
 *
 * @param budget The memory budget in bytes.
 * @param period_us The throttling period in microseconds.
 *
 * This function performs the following steps:
 * 1. Prints a message indicating the initialization of memory throttling on the current CPU.
 * 2. Sets the `throttled` flag to false for the current CPU's virtual machine.
 * 3. Assigns the provided `budget` and `period_us` values to the current CPU's virtual machine.
 * 4. Initializes the memory throttling timer with the specified callback function.
 * 5. Initializes memory throttling events with the specified parameters and callback function.
 */
void mem_throt_init(uint64_t budget, uint64_t period_us) {

    console_printk("Memory Throttling Init on cpu %d\n", cpu()->id);
    cpu()->vcpu->vm->mem_throt.throttled = false;
    cpu()->vcpu->vm->mem_throt.budget = budget / cpu()->vcpu->vm->cpu_num;
    cpu()->vcpu->vm->mem_throt.period_us = period_us;

    mem_throt_timer_init(mem_throt_period_timer_callback);
    mem_throt_events_init(bus_access, cpu()->vcpu->vm->mem_throt.budget, mem_throt_event_overflow_callback);
}
