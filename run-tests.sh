#!/data/data/com.termux/files/usr/bin/bash

set -u

for f in tests/*.sh; do
	printf "Running $f..."

	EXPECTED_FILE=$f-expected
	ACTUAL_FILE=$f-actual

	rm -f $ACTUAL_FILE
	$f $ACTUAL_FILE > $ACTUAL_FILE

	if cmp --silent $ACTUAL_FILE $EXPECTED_FILE; then
		printf " OK\n"
	else
		printf " FAILED - compare expected $EXPECTED_FILE with ${ACTUAL_FILE}\n"
	fi
done

