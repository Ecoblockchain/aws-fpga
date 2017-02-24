/*
 * Copyright 2015-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#pragma once

#include <fpga_common.h>
#include <stdint.h>

/* resource number (base address register) definitions */
enum {
    APP_PF_BAR0 = 0,
    APP_PF_BAR1 = 1,
    APP_PF_BAR4 = 4,
    APP_PF_BAR_MAX
};

enum {
    MGMT_PF_BAR0 = 0,
    MGMT_PF_BAR2 = 2,
    MGMT_PF_BAR4 = 4,
    MGMT_PF_BAR_MAX
};

/* physical function definitions */
enum {
    FPGA_APP_PF,
    FPGA_MGMT_PF,
    FPGA_MAX_PF
};

typedef int pci_bar_handle_t;

/**
 * Initialize the pci library.
 * Calls fpga_hal_plat_init.
 * @returns
 * 0 on success
 * -1 on failure
 */
int fpga_pci_init(void);

/**
 * Attach to an FPGA memory space.
 *
 * @param[in]  slot_id  logical slot index
 * @param[in]  pf_id    physical function id, e.g. FPGA_APP_PF
 * @param[in]  bar_id   base address register, e.g. APP_PF_BAR4
 * @param[in]  flags    set various options (flags defined below)
 * @param[out] handle   used to identify fpga attachment for future library
 *                      calls
 *
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_attach(int slot_id, int pf_id, int bar_id, uint32_t flags, pci_bar_handle_t *handle);
/**
 * Flags used to specify options for fpga_pci_attach.
 */
enum {
    BURST_CAPABLE = 0x1,
    FPGA_ATTACH_RESERVED = 0xfffffffe,
};

/**
 * Detatch from an FPGA memory space.
 *
 * @param[in]  handle  the value provided by fpga_pci_attach corresponding to
 *                     the memory space to detach
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_detatch(pci_bar_handle_t handle);

/**
 * Write a value to a register.
 *
 * @param[in]  handle  handle provided by fpga_pci_attach
 * @param[in]  offset  memory location offset for register to write
 * @param[in]  value   value to write to the register
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_poke(pci_bar_handle_t handle, uint64_t offset, uint32_t value);

/**
 * Write a value to a register.
 *
 * @param[in]  handle  handle provided by fpga_pci_attach
 * @param[in]  offset  memory location offset for register to write
 * @param[in]  value   64-bit value to write to the register
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_poke64(pci_bar_handle_t handle, uint64_t offset, uint64_t value);

/**
 * Write a burst to a burst capable memory bar.
 *
 * @param[in]  handle  handle provided by fpga_pci_attach
 * @param[in]  offset  memory location offset for  to write
 * @param[in]  datap   pointer to the data to be written
 * @param[in]  dword_len  the length of data to write in burst, in 4-byte DWORDs
 *
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_write_burst(pci_bar_handle_t handle, uint64_t offset, uint32_t* datap, uint32_t dword_len);

/**
 * Read a value from a register.
 *
 * @param[in]  handle  handle provided by fpga_pci_attach
 * @param[in]  offset  memory location offset for register to read
 * @param[out] value   value read from the register (32-bit)
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_peek(pci_bar_handle_t handle, uint64_t offset, uint32_t *value);

/**
 * Read a value from a register.
 *
 * @param[in]  handle  handle provided by fpga_pci_attach
 * @param[in]  offset  memory location offset for register to read
 * @param[out] value   64-bit value read from the register
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_peek64(pci_bar_handle_t handle, uint64_t offset, uint64_t *value);

/**
 * Use a logical slot id to populate a slot spec
 *
 * @param[in]  slot_id  The logical slot id of the FPGA of interest
 * @param[in]  pf_id    physical function id (e.g. FPGA_APP_PF)
 * @param[in]  bar_id   base address register id (e.g. APP_PF_BAR0)
 * @param[out] spec     Pointer to fpga_slot spec to populate
 * @returns 0 on success, non-zero on error
 */
int fpga_pci_get_slot_spec(int slot_id, int pf_id, int bar_id, struct fpga_slot_spec *spec);