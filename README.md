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
