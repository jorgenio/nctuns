#!/bin/bash

for i; do
	echo "$i-objs ?= $i.o"
	echo "$i: \$($i-objs)"
	echo ""
done
