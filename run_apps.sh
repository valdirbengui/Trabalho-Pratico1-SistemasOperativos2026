#!/bin/sh
./build/application scenarios/1/A.csv &
./build/application scenarios/1/B.csv &
./build/application scenarios/1/C.csv &
wait
