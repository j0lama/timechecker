#include <linux/delay.h> /* usleep_range */
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <asm/atomic.h>

#define TSC_CALIBRATION_CYCLES 10000

/* From: https://elixir.bootlin.com/linux/v5.9.2/source/include/uapi/linux/sched/types.h#L7 */
struct sched_param {
	int sched_priority;
};


/* Globals */
/* CPU ID for where the timechecker will run */
static int cpu_id = 0;
module_param(cpu_id, int, 0660);
static int threshold = 0;
module_param(threshold, int, 0660);
/* kthread structure */
static struct task_struct * kthread;
/* Offset value that is exported to KVM */
atomic64_t offset = ATOMIC64_INIT(0);
EXPORT_SYMBOL(offset);
/* Number of TSC cycles per 'threshold' microseconds (us). This is exported to KVM to calculate the offset as time */
u64 tsc_cycles = 0xFFFFFFFFFFFFFFFF;


static inline u64 get_rdtsc(void) __attribute__((always_inline));
static inline u64 get_rdtsc(void) {
  register u64 a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

/* Returns the cycles per usec */
static u64 get_tsc_freq(void)
{
    u64 start, cycles;
    start = get_rdtsc();
    udelay(threshold);
    cycles = get_rdtsc() - start;
    return cycles;
}

static int timechecker_thread(void *data)
{
    u64 t0, t1, tmp;
    int inner = 0, outer = 0;
    int i;

    /* Calibrate TSC counter frequency (cycles per usec) */
    printk(KERN_INFO "Calibrating TSC frequency...");
    for (i = 0; i < TSC_CALIBRATION_CYCLES; i++) {
        tmp = get_tsc_freq();
        if(tmp < tsc_cycles)
            tsc_cycles = tmp;
    }
    printk(KERN_INFO "TSC calibrated (%llu cycles per %d microseconds)\n", tsc_cycles, threshold);
    
    t1 = get_rdtsc();
    while (!kthread_should_stop()) {
        t0 = get_rdtsc();
        if((tmp = (t0-t1)) > (tsc_cycles)) { /* Outer */
            outer += 1;
            atomic64_add(tmp, &offset);
            printk(KERN_INFO "Outer preemption detected (%llu > %llu)\n", t0-t1, tsc_cycles);
        }
        t1 = get_rdtsc();
        if((tmp = (t1-t0)) > (tsc_cycles)) { /* Inner */
            inner += 1;
            atomic64_add(tmp, &offset);
            printk(KERN_INFO "Inner preemption detected (%llu > %llu)\n", t1-t0, tsc_cycles);
        }
    }
    printk(KERN_INFO "Exiting timechecker...\n");
    printk(KERN_INFO "Timechecker summary: inner %d, outer %d\n", inner, outer);
    printk(KERN_INFO "Accumulated offset: %llu cycles\n", atomic64_read(&offset));

    return 0;
}

static int __exit  timechecker_init(void)
{
    struct sched_param sp;

    printk(KERN_INFO "timechecker init: cpu_id(%d), threshold(%d)\n", cpu_id, threshold);
    kthread = kthread_create(timechecker_thread, NULL, "mykthread");

    /* Pin kthread to the specified CPU */
    kthread_bind(kthread, cpu_id);

    /* Asign max FIFO priority */
    /* This only work for latest kernels */
    sp.sched_priority = MAX_RT_PRIO;
    sched_setscheduler_nocheck(kthread, SCHED_FIFO, &sp);

    /* Start kthread */
    wake_up_process(kthread);

    return 0;
}

static void __exit timechecker_exit(void)
{
    /* Waits for thread to return. */
    kthread_stop(kthread);
}

module_init(timechecker_init);
module_exit(timechecker_exit);

MODULE_AUTHOR("Jon Larrea <jon.larrea@ed.ac.uk>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Scheduling detector");
