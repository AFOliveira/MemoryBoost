#ifndef __ARCH_PMU_H__
#define __ARCH_PMU_H__

#include <interrupts.h>   // Hypothetical
#include <platform.h>     // Hypothetical
#include <printk.h>       // Hypothetical
#include <bit.h>
#include <riscv_sysregs.h>  // Hypothetical header for RISC-V CSR access

#define PMU_CNTR_MAX_NUM 32

#if !defined(UINT32_MAX)
    #define UINT32_MAX        0xffffffffU /* 4294967295U */
#endif

// RISC-V performance counters typically start at mhpmcounter3.
// Counters: mhpmcounter3 ... mhpmcounter31 (if supported) correspond to 
// hardware performance counters 0 ... 28 (for this code we consider up to PMU_CNTR_MAX_NUM).
// Event selectors: mhpmevent3 ... mhpmevent31.
//
// QEMU’s support for these may vary, but we assume it allows reading/writing them.

// The ARM-specific defines below have no direct equivalent in RISC-V. We leave them as unused.
#define PMCR_EL0_N_POS       (11)
#define PMCR_EL0_N_MASK      (0x1F << PMCR_EL0_N_POS)
#define MDCR_EL2_HPME        (1 << 7)
#define MDCR_EL2_HPMN_MASK   (0x1F)

// Bits used by ARM PMEVTYPER don't apply to RISC-V. We leave them here as unused placeholders.
#define PMEVTYPER_P          31
#define PMEVTYPER_U          30
#define PMEVTYPER_NSK        29
#define PMEVTYPER_NSU        28
#define PMEVTYPER_NSH        27
#define PMEVTYPER_M          26
#define PMEVTYPER_MT         25
#define PMEVTYPER_SH         24

#define PMU_N_CNTR_GIVEN     1
#define PMU_N_CNTR           6
#define ERROR_NO_MORE_EVENT_COUNTERS -10

// Example event codes. These are not standard RISC-V events.
// They must match the platform's event numbering as defined by your firmware or QEMU configuration.
#define DATA_MEMORY_ACCESS          0x13
#define L2D_CACHE_ACCESS            0x16
#define L2D_CACHE_REFILL            0x17
#define BUS_ACCESS                  0x19
#define EXTERNAL_MEMORY_REQUEST     0xC0

static size_t events_array[] = {
    DATA_MEMORY_ACCESS,
    L2D_CACHE_ACCESS,
    BUS_ACCESS,
    EXTERNAL_MEMORY_REQUEST,
    L2D_CACHE_REFILL
};

// Placeholder for allocation/free routines. Implementation dependent.
uint64_t pmu_cntr_alloc();
void pmu_cntr_free(uint64_t);

// On RISC-V/QEMU, enabling PMU might mean clearing inhibit bits in mcountinhibit CSR.
void pmu_enable(void) {
    uint64_t inhibit = read_csr(mcountinhibit);
    // Clear all inhibit bits for counters (assuming counters 3 to 31 are available):
    // The layout of mcountinhibit is implementation dependent. 
    // Typically: bit 0 = cycle, bit 1 = time, bit 2 = instret, bits 3+ = hpm counters.
    // We assume clearing bits 3+ enables them. We'll just clear a range:
    for (int i = 3; i < 3 + PMU_CNTR_MAX_NUM; i++) {
        inhibit = bit_clear(inhibit, i);
    }
    write_csr(mcountinhibit, inhibit);
}

// On RISC-V, enabling interrupts for PMU events is platform-specific.
// QEMU does not provide a standard PMU interrupt mechanism. We leave this as a no-op.
void pmu_interrupt_enable(uint64_t cpu_id) {
    // No standard PMU interrupt in RISC-V spec.
}

// Define an IRQ callback for event counters. Again, platform dependent and may not be supported by QEMU.
void pmu_define_event_cntr_irq_callback(irq_handler_t handler, size_t counter) {
    // Not implemented
}

static inline void pmu_disable(void) {
    // Disable all HPM counters by setting their inhibit bits:
    uint64_t inhibit = read_csr(mcountinhibit);
    for (int i = 3; i < 3 + PMU_CNTR_MAX_NUM; i++) {
        inhibit = bit_set(inhibit, i);
    }
    write_csr(mcountinhibit, inhibit);
}

static inline int pmu_cntr_enable(size_t counter) {
    uint64_t inhibit = read_csr(mcountinhibit);
    size_t hw_counter = counter + 3; 
    inhibit = bit_clear(inhibit, hw_counter);
    write_csr(mcountinhibit, inhibit);
    return 0;
}

static inline void pmu_cntr_disable(size_t counter) {
    uint64_t inhibit = read_csr(mcountinhibit);
    size_t hw_counter = counter + 3;
    inhibit = bit_set(inhibit, hw_counter);
    write_csr(mcountinhibit, inhibit);
}

static inline void pmu_cntr_set(size_t counter, unsigned long value) {
    size_t hw_counter = counter + 3;
    uint64_t val = (uint64_t)UINT32_MAX - (uint64_t)value;

    // Write mhpmcounterX via a helper. This might need inline assembly:
    write_hpmcounter(hw_counter, val);
}

static inline unsigned long pmu_cntr_get(size_t counter) {
    size_t hw_counter = counter + 3;
    return (unsigned long)read_hpmcounter(hw_counter);
}

static inline void pmu_set_evtyper(size_t counter, size_t event) {
    size_t hw_counter = counter + 3;
    uint64_t pmxevtyper = 0;

    // Insert the event code into the lower bits:
    pmxevtyper = bit_insert(pmxevtyper, events_array[event], 0, 10);

    // Write mhpmeventX. This is also implementation dependent. Assume helper functions exist.
    write_hpm_event(hw_counter, pmxevtyper);
}

static inline void pmu_interrupt_disable(uint64_t cpu_id) {
    // No standard PMU interrupt in RISC-V spec / QEMU.
}

static inline void pmu_set_cntr_irq_enable(size_t counter) {
    // Not supported/unknown in QEMU. No-op.
}

static inline void pmu_set_cntr_irq_disable(size_t counter) {
    // Not supported/unknown in QEMU. No-op.
}

static inline void pmu_clear_cntr_ovs(size_t counter) {
    // Clearing overflow is implementation dependent. Not standard in RISC-V.
    // No-op for QEMU unless implemented by the platform.
}

#endif /* __ARCH_PMU_H__ */
