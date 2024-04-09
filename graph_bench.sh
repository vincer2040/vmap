#!/usr/bin/env bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <10 | 100 | 1000 | 10000>"
    exit 1
fi

value=$1

make -s bench | grep "find" | awk -v val="$value" '{if ($5 == val) print $3, $8}' | sed s/,// | ./create_graph.py
