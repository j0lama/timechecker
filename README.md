# timechecker

## Dependencies
```bash
sudo apt-get install linux-headers-$(uname -r)
```

## How to use
### Run
This tool will consume one CPU
```
sudo ./run.sh <CPU ID> <Threshold (microseconds)>
```

### Check logs
```bash
sudo dmesg -w
```

### Stop
```bash
sudo rmmod timechecker
```

### Access values calculated by timechecker
Timechecker exports two symbols: **offset** and **tsc_cycles**. These two symbols should be accessibles from other parts of the kernels as *extern* variables.

**offset**: contains the cumulative cycles detected by timechecker that should be corrected/adjusted in KVM. This variable is atomic64_t, so it must be access using the asm/atomic.h functions.

**tsc_cycles**: contains the number of TSC cycles per *threshold* (value passed to timechecker) microseconds. This variable is u64.

Calculate offset as microseconds:
```
offset_time = offset/(tsc_cycles/threshold)
```