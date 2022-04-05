#!/bin/bash

cd extern/Code_c_log
./b.sh
cd -
make clean; make; make install
