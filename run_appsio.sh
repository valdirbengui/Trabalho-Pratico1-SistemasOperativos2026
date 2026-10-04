#!/bin/sh
./build/application scenarios/3/A.csv &
./build/application scenarios/3/B.csv &
./build/application scenarios/3/C.csv &
wait
