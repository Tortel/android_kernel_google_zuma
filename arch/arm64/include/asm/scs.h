/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SCS_H
#define _ASM_SCS_H

#ifdef __ASSEMBLY__

#include <asm/asm-offsets.h>
#include <asm/sysreg.h>

#ifdef CONFIG_SHADOW_CALL_STACK
	scs_sp	.req	x18

	.macro scs_load tsk
	ldr	scs_sp, [\tsk, #TSK_TI_SCS_SP]
	.endm

	.macro scs_save tsk
	str	scs_sp, [\tsk, #TSK_TI_SCS_SP]
	.endm
#else
	.macro scs_load tsk
	.endm

	.macro scs_save tsk
	.endm
#endif /* CONFIG_SHADOW_CALL_STACK */


#else

#include <linux/scs.h>
#include <asm/cpufeature.h>

#ifdef CONFIG_UNWIND_PATCH_PAC_INTO_SCS
static inline bool should_patch_pac_into_scs(void)
{
	u64 reg;

	/*
	 * We only enable the shadow call stack dynamically if we are running
	 * on a system that does not implement PAC or BTI. PAC and SCS provide
	 * roughly the same level of protection, and BTI relies on the PACIASP
	 * instructions serving as landing pads, preventing us from patching
	 * those instructions into something else.
	 */
	reg = read_sysreg_s(SYS_ID_AA64ISAR1_EL1);
	if ((reg & (0xf << ID_AA64ISAR1_EL1_APA_SHIFT)) |
	    (reg & (0xf << ID_AA64ISAR1_EL1_API_SHIFT)))
		return false;

	reg = read_sysreg_s(SYS_ID_AA64ISAR2_EL1);
	if (reg & (0xf << ID_AA64ISAR2_EL1_APA3_SHIFT))
		return false;

	if (IS_ENABLED(CONFIG_ARM64_BTI_KERNEL)) {
		reg = read_sysreg_s(SYS_ID_AA64PFR1_EL1);
		if (reg & (0xf << ID_AA64PFR1_EL1_BT_SHIFT))
			return false;
	}
	return true;
}

static inline void dynamic_scs_init(void)
{
	if (should_patch_pac_into_scs()) {
		pr_info("Enabling dynamic shadow call stack\n");
		static_branch_enable(&dynamic_scs_enabled);
	}
}
#else
static inline void dynamic_scs_init(void) {}
#endif

int scs_patch(const u8 eh_frame[], int size);

#endif /* __ASSEMBLY __ */

#endif /* _ASM_SCS_H */
