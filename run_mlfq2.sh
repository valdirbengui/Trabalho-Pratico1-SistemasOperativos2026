#!/bin/sh
for f in scenarios/trabalho/mlfq2/*.csv; do ./build/application "$f" & done
wait
