#!/bin/sh
./build/application scenarios/7/A.csv &
./build/application scenarios/7/B.csv &
./build/application scenarios/7/C.csv &
./build/application scenarios/7/D.csv &
./build/application scenarios/7/E.csv &
wait
