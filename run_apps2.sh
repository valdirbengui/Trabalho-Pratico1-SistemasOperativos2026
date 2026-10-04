#!/bin/sh
for f in scenarios/2/*.csv; do
    ./build/application "$f" &
done
wait
