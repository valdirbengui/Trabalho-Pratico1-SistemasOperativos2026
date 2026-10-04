#!/bin/sh
for f in scenarios/trabalho/sjf/*.csv; do ./build/application "$f" & done
wait
