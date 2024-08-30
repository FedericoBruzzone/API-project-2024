#/usr/bin bash

set -e

# Run the tests
echo "Running example.txt"
time ./main < ./test_cases/example.txt > example.out
diff example.out ./test_cases/example.output.txt
rm example.out
echo -e "----------------------\n"

echo -e "Running open1.txt"
time ./main < ./test_cases/open1.txt > open1.out
diff open1.out ./test_cases/open1.output.txt
rm open1.out
echo -e "----------------------\n"

echo "Running open2.txt"
time ./main < ./test_cases/open2.txt > open2.out
diff open2.out ./test_cases/open2.output.txt
rm open2.out
echo -e "----------------------\n"

echo "Running open3.txt"
time ./main < ./test_cases/open3.txt > open3.out
diff open3.out ./test_cases/open3.output.txt
rm open3.out
echo -e "----------------------\n"

echo "Running open4.txt"
time ./main < ./test_cases/open4.txt > open4.out
diff open4.out ./test_cases/open4.output.txt
rm open4.out
echo -e "----------------------\n"

echo "Running open5.txt"
time ./main < ./test_cases/open5.txt > open5.out
diff open5.out ./test_cases/open5.output.txt
rm open5.out
echo -e "----------------------\n"

echo "Running open6.txt"
time ./main < ./test_cases/open6.txt > open6.out
diff open6.out ./test_cases/open6.output.txt
rm open6.out
echo -e "----------------------\n"

echo "Running open7.txt"
time ./main < ./test_cases/open7.txt > open7.out
diff open7.out ./test_cases/open7.output.txt
rm open7.out
echo -e "----------------------\n"

echo "Running open8.txt"
time ./main < ./test_cases/open8.txt > open8.out
diff open8.out ./test_cases/open8.output.txt
rm open8.out
echo -e "----------------------\n"

echo "Running open9.txt"
time ./main < ./test_cases/open9.txt > open9.out
diff open9.out ./test_cases/open9.output.txt
rm open9.out
echo -e "----------------------\n"

echo "Running open10.txt"
time ./main < ./test_cases/open10.txt > open10.out
diff open10.out ./test_cases/open10.output.txt
rm open10.out
echo -e "----------------------\n"

echo "Running open11.txt"
time ./main < ./test_cases/open11.txt > open11.out
diff open11.out ./test_cases/open11.output.txt
rm open11.out
echo -e "----------------------\n"
