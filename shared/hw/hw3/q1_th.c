int pid = getpid();
while (1) {
	x = pid;
	if (y && y != pid) continue;
	y = pid;
	if (x != pid) continue;
	/* critical section */
}