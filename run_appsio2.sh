#!/bin/sh
./build/application scenarios/4/A.csv &
./build/application scenarios/4/B.csv &
./build/application scenarios/4/C.csv &
wait
