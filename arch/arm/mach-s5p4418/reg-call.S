#include <linux/linkage.h>
#include <asm/opcodes-sec.h>
#include <asm/opcodes-virt.h>

/* int __invoke_nexell_fn_smc(u32 function_id, u32 arg0, u32 arg1, u32 arg2) */
ENTRY(__invoke_nexell_fn_smc)
	__SMC(0)
	bx      lr
ENDPROC(__invoke_nexell_fn_smc)
