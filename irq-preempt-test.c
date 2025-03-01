// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/preempt.h>

#define NUM_RUNS 30

static int __init mod_init(void)
{
	int i;
	s64 results[2];
	u64 t0, t1;

	// Done as a simple (and flawed) irqs-off vs preemption-off test for
	// memory allocation purposes (linux disables IRQs on SLAB/SLUB code
	// paths, FreeBSD does not).

	// Notes:
	// rdtsc_ordered expands to rdtsc, lfence; rdtsc, or rdtscp
	// (for new CPUs), depending on CPU features.
	// preempt_disable and preempt_enable are percpu inc's and dec's with a
	// suitable compiler barrier.
	// arch_local_irq_{save, restore} are "pushf; pop; cli" and
	// if (flags & IF) sti, respectively.

	t0 = rdtsc_ordered();
	for (i = 0; i < NUM_RUNS; i++) {
		preempt_disable();
		preempt_enable();
	}

	t1 = rdtsc_ordered();
	results[0] = t1 - t0;

	t0 = rdtsc_ordered();
	for (i = 0; i < NUM_RUNS; i++) {
		unsigned long flags;
		flags = arch_local_irq_save();
		arch_local_irq_restore(flags);
	}
	t1 = rdtsc_ordered();

	results[1] = t1 - t0;
	for (i = 0; i < 2; i++) {
		pr_info("%s = %lld tsc cycles\n", i == 0 ? "PREEMPT" : "IRQ", results[i]);
	}

	// Note: We can't do this because none of the cyc2ns stuff is exported from tsc.c :/
	pr_info("Check your TSC frequency and convert to nanoseconds.\n");

	return 0;
}

static void mod_fini(void)
{
}

module_init(mod_init);
module_exit(mod_fini);

MODULE_AUTHOR("Pedro Falcato");
MODULE_DESCRIPTION("Benchmarks low level preemption counter manipulation vs irqs-off-on");
MODULE_LICENSE("GPL");
