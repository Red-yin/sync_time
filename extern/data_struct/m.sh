#!/bin/bash

mv Makefile.mixpad Makefile
make clean;make;make install
mv Makefile Makefile.mixpad
git checkout Makefile

