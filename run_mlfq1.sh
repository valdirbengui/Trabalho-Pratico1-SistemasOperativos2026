#!/bin/sh
for f in scenarios/trabalho/mlfq1/*.csv; do ./build/application "$f" & done
wait
