

```
cd pyttelog/
rm pytte || true
make
cp pytte ../
cd ..
go build faller.go
./faller | ./pytte 7 faller.log
```

use dstat, htop and iotop to check cpu usage of pytte and io throughput to disk
