#!/bin/bash

HW_FOLDER="$1"
rm -rf ./todo

current_datetime=$(date +'%Y-%m-%d_%H-%M-%S')
output_file="${current_datetime}.csv"
echo "ID,Penalty,Penalty Notes,Wet grade,Wet Notes,Dry grade,Dry Notes,final" > "$output_file"

tests_num=15
test_weight=6
penalty=10

count=0
find $HW_FOLDER -type f -mindepth 1 -maxdepth 1 | while read filename
do

	let count+=1
	echo "We are in $count test"
	
	ids=($(echo "$filename" | grep -oE '[0-9]{9}'))
	
	mkdir todo
	cp ./tests/* ./todo
	cp $filename ./todo/encdec.c 2>/dev/null

	grade_one=0	
	note=""
	
	make_output=$(make -C todo 2>&1)
	comp_one=$? # Get return code of compilation
	if [[ "$comp_one" -ne 0 ]]; then
		make_output=$(echo "$make_output" | tr '\n' ' ' | tr ',' ' ')
		make_output=$(printf '%q' "$make_output")
		note="Compilation Error in my shell: $make_output"
		
	else
		cd ./todo
		../load
		cd ..
		if [[ "$?" -ne 0 ]]; then
			note="Module Loading ERROR"
		else
			for ((i = 1; $i <= $tests_num; i++ )); do
				./todo/test < ./todo/test$i.in > ./todo/test_temp$i.out &
				PID=$!
				sleep 4 # for waiting on stupid linux
				if ps -p $PID > /dev/null
				then
					# PROCESS HANGS, we will kill it but it's childs might stay
					kill -9 $PID
					note="$note --newline-- test number $i failed due to process hang"
				else
					wait $PID
					result=$?
					if [[ "$result" -ge 126 ]]; then
						note="$note --newline-- test number $i failed due to Module SIGNAL ERROR"
						#rm -rf $filename		#remove, its his problem.
						
						grade_one=0
						for id in ${ids[@]}; do
							echo "$id,$penalty,,$grade_one,$note,0,,0" >> "$output_file"
						done
						
						rm -rf ./todo
						echo "SIGNAL ERROR in Module"
						sleep 3
						/sbin/reboot
						exit 1
					else
						diff -B -w --strip-trailing-cr ./todo/test$i.out ./todo/test_temp$i.out > /dev/null
						result1=$?
						if [[ "$result1" -ne 0 ]]; then
							note="$note --newline-- test number $i failed"
						else
							let grade_one+=test_weight
						fi
					fi
				fi
			done	
			./unload
			#rm -rf $filename		#remove only if compiled
		fi
	fi
	
	for id in ${ids[@]}; do
		echo "$id,$penalty,,$grade_one,$note,0,,0" >> "$output_file"
	done
	
	rm -rf ./todo
done;
