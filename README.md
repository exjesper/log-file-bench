

```
cd pyttelog/
rm pytte || true
make
cp pytte ../
cd ..
go build faller.go
./faller | ./pytte 7 faller.log
```