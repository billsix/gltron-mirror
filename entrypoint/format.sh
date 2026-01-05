#!/bin/env bash

for x in $(find lib3ds/ nebutest/ nebu src/ \( -iname "*.c" -o -iname "*.cpp" -o -iname "*.h" -o -iname "*.hpp" \)) ; do
              clang-format -i $x ;
              done;
