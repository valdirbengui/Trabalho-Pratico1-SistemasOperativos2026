#!/bin/sh
./build/application scenarios/6/A.csv &
./build/application scenarios/6/B.csv &
./build/application scenarios/6/C.csv &
./build/application scenarios/6/D.csv &
./build/application scenarios/6/E.csv &
wait
