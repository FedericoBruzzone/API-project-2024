#/usr/bin bash

set -e

# Run the tests
echo "Running example.txt"
./main < ./test_cases/example.txt > example.out
diff example.out ./test_cases/example.output.txt
rm example.out

echo "Running open1.txt"
./main < ./test_cases/open1.txt > open1.out
diff open1.out ./test_cases/open1.output.txt
rm open1.out

echo "Running open2.txt"
./main < ./test_cases/open2.txt > open2.out
diff open2.out ./test_cases/open2.output.txt
rm open2.out

echo "Running open3.txt"
./main < ./test_cases/open3.txt > open3.out
diff open3.out ./test_cases/open3.output.txt
rm open3.out

echo "Running open4.txt"
./main < ./test_cases/open4.txt > open4.out
diff open4.out ./test_cases/open4.output.txt
rm open4.out

echo "Running open5.txt"
./main < ./test_cases/open5.txt > open5.out
diff open5.out ./test_cases/open5.output.txt
rm open5.out

echo "Running open6.txt"
./main < ./test_cases/open6.txt > open6.out
diff open6.out ./test_cases/open6.output.txt
rm open6.out
