# H-MBR: Hypervisor-level Memory Bandwidth Reservation for Mixed-Criticality Systems

## Overview

H-MBR (Hypervisor-level Memory Bandwidth Reservation) is a VM-centric memory bandwidth reservation mechanism built on the [Bao hypervisor](https://github.com/bao-project/bao-hypervisor). It enables robust memory bandwidth isolation among virtual machines (VMs) on shared multicore platforms, making it particularly valuable for Mixed-Criticality Systems (MCS).

### What is H-MBR?

H-MBR provides fine-grained control over memory bandwidth allocation at the VM level, ensuring critical VMs maintain predictable performance while coexisting with non-critical VMs. The mechanism leverages hardware Performance Monitoring Units (PMU) and generic timers to enforce bandwidth limits with minimal overhead.

### Key Features

- **VM-Centric Bandwidth Allocation**
  - Configure memory bandwidth budgets per VM rather than per CPU
  - Ideal for mixed-criticality workloads with varying bandwidth requirements
  - Supports both balanced and unbalanced distribution across vCPUs

- **Architecture & OS Support**
  - OS-Agnostic: Compatible with Linux, FreeRTOS, Zephyr, and other operating systems
  - Platform-Agnostic: Supports ARMv8-A, ARMv8-R, and RISC-V architectures
  - Built on Bao's minimal hypervisor design

- **Performance**
  - Zero overhead on unregulated (critical) workloads
  - <1% overhead for regulated workloads with periods ≥2 µs
  - Efficient interrupt handling through Bao's optimized design

## Implementation Details

### Memory Bandwidth Control

H-MBR uses two key hardware components:
1. **PMU (Performance Monitoring Unit)**
   - Tracks memory access activities
   - Triggers overflow interrupts when budget is exceeded
   - Provides per-CPU monitoring capabilities

2. **Generic Timer**
   - Manages regulation periods
   - Resets bandwidth budgets periodically
   - Controls VM execution based on budget consumption

### Budget Calculation

The effective memory bandwidth is determined by:
```
Bandwidth = Budget / Period
```
where:
- `Budget`: Maximum allowed memory accesses per period
- `Period`: Time interval in microseconds

## Getting Started

### Hardware Requirements

- **Tested Platform**: Xilinx Zynq UltraScale+ ZCU104
  - ARM Cortex-A53 quad-core processor
  - Integrated PMU and timer support
  - 1MB unified L2 cache

### Configuration Guide

1. **VM Configuration**
   Add the following to your VM description in Bao:
   ```c
   .mem_throth = {
       .period_us = x,  // Period in microseconds (e.g., 1000 for 1ms)
       .budget = y,     // Maximum memory accesses per period per VM
       .cpu_num_buget = {x, y, z} //Percentage of VM budget each CPU takes
   },
   ```

2. **Recommended Settings**
   - For critical VMs: Leave unregulated
   - For non-critical VMs:
     - Minimum period: 2 µs (to maintain <1% overhead)
     - Budget: Tune based on workload requirements
     - Start with larger periods (e.g., 100 µs) and adjust based on performance

### Building the System

1. **Repository Setup**
   ```bash
   git clone https://github.com/<user>/H-MBR.git
   cd H-MBR/
   ```

2. **Configuration**
   - Configure MBR parameters in your VM description
   - Adjust settings based on your specific mixed-criticality requirements

3. **Deployment**
   - Deploy as normally would for bao depending on your platform.
   - For more information check https://github.com/bao-project/bao-demos/


## Publications

If you use H-MBR in your research, please cite our paper:

> **H-MBR: Hypervisor-level Memory Bandwidth Reservation for Mixed-Criticality Systems**  
> Afonso Oliveira, Diogo Costa, Gonçalo Moreira, José Martins, Sandro Pinto  
> Workshop on Next Generation Real-Time Embedded Systems (NG-RES 2025)

## Contributing

We welcome contributions! Please submit pull requests or open issues for:
- Bug fixes and performance improvements
- Feature enhancements
- Documentation updates
- Platform support extensions