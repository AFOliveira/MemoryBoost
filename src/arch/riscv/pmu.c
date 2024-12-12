#include <arch/pmu.h>
#include <cpu.h>
#include <bitmap.h>
#include <printk.h>
#include <interrupts.h>
#include <platform.h>

// On RISC-V, the number of hardware performance counters may be discovered from the platform.
// For simplicity, assume PMU_CNTR_MAX_NUM is defined and is the number of counters supported.
// The original code read implemented counters from PMCR_EL0 in ARM. We don't have that here.
// We'll just assume PMU_CNTR_MAX_NUM counters are available or rely on a platform definition.
#ifndef PMU_CNTR_MAX_NUM
#define PMU_CNTR_MAX_NUM 32
#endif

uint64_t pmu_cntr_alloc()
{
    uint32_t index = PMU_N_CNTR_GIVEN;

    for (int __bit = bitmap_get(cpu()->events_bitmap, index); 
         index < cpu()->implemented_event_counters;
         __bit = bitmap_get(cpu()->events_bitmap, ++index)) {
        if(!__bit)
            break;
    }

    if (index == cpu()->implemented_event_counters)
        return ERROR_NO_MORE_EVENT_COUNTERS;

    bitmap_set(cpu()->events_bitmap, index);
    return index;
}

void pmu_cntr_free(uint64_t counter)
{
    bitmap_clear(cpu()->events_bitmap, counter);
}

void pmu_interrupt_handler(irqid_t int_id)
{
    // On ARM, PMOVSCLR_EL0 contains the overflow status of event counters.
    // On RISC-V, there's no standard equivalent. Overflow detection is implementation-dependent.
    // For now, we assume no overflow status available or that we must track overflows differently.
    
    // Placeholder: If QEMU supported some CSR or memory-mapped register for overflow,
    // we would read it here. We'll just assume no overflows to clear for this example.

    // If we had a hypothetical overflow status bitmap:
    uint64_t pmovsclr = 0;  // No standard RISC-V equivalent.

    for (uint32_t index = 0; index < PMU_N_CNTR; index++)
    {
        if (bit_get(pmovsclr, index))
        {
            // Call the registered handler if this counter is in the allocated range.
            if (index >= PMU_N_CNTR_GIVEN && index < (cpu()->implemented_event_counters - PMU_N_CNTR_GIVEN))
                cpu()->array_interrupt_functions[index](int_id);
            else {
                // Clear overflows not handled by guests
                pmovsclr = bit_set(pmovsclr, index);
            }
        }
    }

    // In ARM: sysreg_pmovsclr_el0_write(pmovsclr); 
    // On RISC-V: no standard equivalent. 
    // We do nothing here as we have no standard CSR to write.
}

/* Enable the PMU */
void pmu_enable(void)
{
    // On ARM, we read PMCR_EL0 and MDCR_EL2 to discover the number of counters and enable them.
    // On RISC-V, no direct equivalent. We'll just assume all PMU_CNTR_MAX_NUM counters are implemented.
    cpu()->implemented_event_counters = PMU_CNTR_MAX_NUM;

    // ARM code:
    // uint32_t pmcr = sysreg_pmcr_el0_read();
    // uint64_t mdcr = sysreg_mdcr_el2_read();
    //
    // cpu()->implemented_event_counters = ((pmcr & PMCR_EL0_N_MASK) >> PMCR_EL0_N_POS);
    //
    // mdcr &= ~MDCR_EL2_HPMN_MASK;
    // mdcr |= MDCR_EL2_HPME + (PMU_N_CNTR_GIVEN);
    //
    // sysreg_mdcr_el2_write(mdcr);

    // On RISC-V, enabling counters typically involves clearing bits in mcountinhibit
    // which can be done in pmu_cntr_enable() calls. For now, nothing special here.
}

void pmu_interrupt_enable(uint64_t cpu_id)
{
    // On ARM, we had platform.arch.events.events_irq_offset + cpu_id.
    // On RISC-V/QEMU, interrupts are platform-specific. We assume the same offset logic applies.
    irqid_t irq_id = platform.arch.events.events_irq_offset + cpu_id;

    if(!interrupts_reserve(irq_id, pmu_interrupt_handler)) {
        ERROR("Failed to assign PMU interrupt id = %d\n", irq_id);
    }

    interrupts_arch_enable(irq_id, true);
}

void pmu_define_event_cntr_irq_callback(irq_handler_t handler, size_t counter)
{
    cpu()->array_interrupt_functions[counter] = handler;
}
