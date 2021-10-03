#!/bin/bash

echo "#define ALL_TCS \\" > test-case-list.h

for FN in `find tst | grep "case_" | grep "\.c$" | sort`
do
	CA=`echo $FN | sed "s/tst\///g" | sed "s/\.c$//g"`
	echo "_T("$CA") \\" >> test-case-list.h
done

echo "" >> test-case-list.h

# eof
