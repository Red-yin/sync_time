#!/bin/bash

cd extern/Code_c_log
./m.sh
cd -

mv Makefile.mixpad Makefile
make clean;make;make install
mv Makefile Makefile.mixpad
git checkout Makefile

