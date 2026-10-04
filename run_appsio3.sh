#!/bin/sh
./build/application scenarios/5/A.csv &
./build/application scenarios/5/B.csv &
./build/application scenarios/5/C.csv &
wait
