#!/bin/bash

find . -type d -empty -exec rmdir {} \;

rm -rf ./todo
mkdir todo
	
cp ./tests/* ./todo

cd ./todo

tests=13

# part 1
gcc -Werror -lm -std=c99 virtmem.c -o test
comp_one=$?
if [[ "$comp_one" -ne 0 ]]; then
	echo "Compilation Error in my shell"
else
	for ((i = 1; $i <= $tests; i++ )); do
		./test BACKING_STORE.bin test$i.in > test_temp$i.out &
		PID=$!
		sleep 3
		if ps -p $PID > /dev/null
		then
			# PROCESS HANGS, we will kill it but it's childs might stay
			kill -9 $PID
			echo "Creating test $i failed due to process hang"
		else
			wait $PID
			result=$?
			if [[ "$result" -ge 126 ]]; then
				echo "Creating test $i failed"
			fi
		fi
	done
fi

cd ..



for ((i = 1; $i < $tests; i++ ))
do
	cp ./todo/test_temp$i.out ./tests/test$i.out
done

rm -rf ./todo
