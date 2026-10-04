#!/bin/sh
for f in scenarios/trabalho/rr/*.csv; do ./build/application "$f" & done
wait
