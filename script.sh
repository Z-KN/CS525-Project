#!/bin/bash
# this has a problem: can't execute in parallel; adding "&" does not help
ssh kainingz@sp24-cs525-0201.cs.illinois.edu -t "cd CS525-Project && python3 src/network_test_churn.py -n 1" 
ssh kainingz@sp24-cs525-0202.cs.illinois.edu -t "cd CS525-Project && python3 src/network_test_churn.py -n 2" 
ssh kainingz@sp24-cs525-0203.cs.illinois.edu -t "cd CS525-Project && python3 src/network_test_churn.py -n 3" 
ssh kainingz@sp24-cs525-0204.cs.illinois.edu -t "cd CS525-Project && python3 src/network_test_churn.py -n 4"
# ssh kainingz@sp24-cs525-0201.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 5" > log5.txt 
# ssh kainingz@sp24-cs525-0202.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 6" > log6.txt
# ssh kainingz@sp24-cs525-0203.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 7" > log7.txt
# ssh kainingz@sp24-cs525-0204.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 8" > log8.txt
# ssh kainingz@sp24-cs525-0204.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 9" > log9.txt
# ssh kainingz@sp24-cs525-0204.cs.illinois.edu -t "pip3 install --user future==0.18.2 pyzmq==19.0.2 && chmod -R go+rw . && cd CS525-Project && python3 network_test.py -n 10" > log10.txt
