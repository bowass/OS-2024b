#!/bin/bash

HW_FOLDER="$1"
rm -rf ./todo

current_datetime=$(date +'%Y-%m-%d_%H-%M-%S')
output_file="${current_datetime}.csv"
echo "ID,Penalty,Penalty Notes,Wet grade,Wet Notes,Dry grade,Dry Notes,final" > "$output_file"

tests_num=6
test_weight=8
penalty=7

count=0
find $HW_FOLDER -type f -mindepth 1 -maxdepth 1 | while read filename
do

	let count+=1
	echo "We are in $count test"
	
	ids=($(echo "$filename" | grep -oE '[0-9]{9}'))
	
	mkdir todo
	cp ./tests/* ./todo
	cp $filename ./todo/myshell.c 2>/dev/null
	
	grade_one=0
	note=""
	
	# part 1
	gcc_output=$(gcc -Werror -lm -std=c99 ./todo/myshell.c -o ./todo/myshell 2>&1)
	comp_one=$? # Get return code of compilation
	
	if [[ "$comp_one" -ne 0 ]]; then
		gcc_output=$(echo "$gcc_output" | tr '\n' ' ' | tr ',' ' ')
		gcc_output=$(printf '%q' "$gcc_output")
		note="Compilation Error in my shell: $gcc_output"

	else
		for ((i = 1; $i <= $tests_num; i++ )); do
		./todo/myshell < ./todo/test$i.in > ./todo/test_temp$i.out &
		PID=$!
		sleep 3 # for waiting on stupid linux
		if ps -p $PID > /dev/null
		then
			# PROCESS HANGS, we will kill it but it's childs might stay
			kill -9 $PID
			note="$note --newline-- test number $i failed due to process hang"
		else
			wait $PID
			result=$?
			if [[ "$result" -ge 126 ]]; then
				note="$note --newline-- test number $i failed due to segmentation fault"
			else
				echo TRUE:
				cat ./todo/test$i.out
				echo OUT:
				cat ./todo/test_temp$i.out
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
		#rm -rf $filename		#remove only if compiled
	fi
	
	echo -n id =# tmp
	for id in ${ids[@]}; do
		echo $id
		echo "$id,$penalty,,$grade_one,$note,0,,0" >> "$output_file"
	done

	rm -rf ./todo
	echo $note # tmp
done;
