#include <linux/delay.h> /* usleep_range */
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/module.h>

#define SLEEP_DELAY 1000 /* usec */

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
    udelay(SLEEP_DELAY);
    cycles = get_rdtsc() - start;
    return div64_u64(cycles, SLEEP_DELAY);
}

static int timechecker_thread(void *data)
{
    u64 start, end, th_cycles;

    /* Calculate TSC counter frequency (cycles per usec) */
    th_cycles = (SLEEP_DELAY+threshold)*get_tsc_freq();

    while (!kthread_should_stop()) {
        start = get_rdtsc();
        udelay(SLEEP_DELAY);
        end = get_rdtsc();
        if((end-start) > (th_cycles)) {
            printk(KERN_INFO "Preemption detected (%llu > %llu)\n", end-start, th_cycles);
        }
    }
    printk(KERN_INFO "Exiting timechecker...\n");
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
