#!/bin/bash

mkdir data
cd data
i=0
while [ $i -lt $1 ]; do
    mkdir $i
    let i=i+1
done
