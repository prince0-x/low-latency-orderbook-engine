#!/bin/bash

> data/orders.txt   # clear file

# 1. Random (50k)
for i in {1..75000}
do
  r=$((RANDOM % 3))
  
  if [ $r -eq 0 ]; then
    echo "BUY $((100 + RANDOM % 10)) $((1 + RANDOM % 10))"
  elif [ $r -eq 1 ]; then
    echo "SELL $((100 + RANDOM % 10)) $((1 + RANDOM % 10))"
  else
    echo "CANCEL $((1 + RANDOM % i))"
  fi
done >> data/orders.txt

# 2. Heavy matching (50k)
for i in {1..75000}
do
  echo "BUY 100 5"
  echo "SELL 100 5"
done >> data/orders.txt

# 3. Deterministic (50k)
for i in {1..75000}
do
  if (( i % 2 == 0 ))
  then
    echo "BUY 100 5"
  else
    echo "SELL 100 5"
  fi
done >> data/orders.txt