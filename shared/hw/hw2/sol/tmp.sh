for i in $(seq 1 15);
do
	 ./test < ../nothing_here/test$i.in > tmp.out
	diff ../nothing_here/test$i.out tmp.out
done
