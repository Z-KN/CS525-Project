#!/bin/bash
# this has a problem: can't execute in parallel; adding "&" does not help
ssh kainingz@sp24-cs525-0201.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 1" > logs/log1.txt 
ssh kainingz@sp24-cs525-0202.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 2" > logs/log2.txt
ssh kainingz@sp24-cs525-0203.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 3" > logs/log3.txt
ssh kainingz@sp24-cs525-0204.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 4" > logs/log4.txt
