#!/bin/bash

find . -type d -empty -exec rmdir {} \;

rm -rf ./todo
mkdir todo
	
cp ./tests/* ./todo

cd ./todo

tests=15


# part 1
make_output=$(make 2>&1)
comp_one=$? # Get return code of compilation
if [[ "$comp_one" -ne 0 ]]; then
	echo "Compilation Error in my shell"
	echo $make_output
else
	../load
	if [[ "$?" -ne 0 ]]; then
		echo "Module Loading ERROR"
		cd ..
		rm -rf ./todo
		exit 1
	fi

	for ((i = 1; $i <= $tests; i++ )); do
		./test < test$i.in > test_temp$i.out &
		PID=$!
		sleep 4
		if ps -p $PID > /dev/null
		then
			# PROCESS HANGS, we will kill it but it's childs might stay
			kill -9 $PID
			echo "Creating test $i failed due to process hang"
		else
			wait $PID
			result=$?
			if [[ "$result" -ge 126 ]]; then
				echo Creating test number $i failed due to Module SIGNAL ERROR >> ../results.txt
				cd ..
				rm -rf ./todo
				sleep 3
				/sbin/reboot
				exit 1
			fi
		fi
	done
	../unload
fi

cd ..



for ((i = 1; $i <= $tests; i++ ))
do
	cp ./todo/test_temp$i.out ./tests/test$i.out
done

rm -rf ./todo
