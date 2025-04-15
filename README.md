a spreadsheet app prioritizing performance

only works on linux for now

```
./build.sh
./build/main resources/test.csv
```

to test with 1 million lines
```
cd resources
./gen.py --rows 1000000 --output 1M.csv
../build/main 1M.csv
```
