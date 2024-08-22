# API project 2024

Project of the course "Algoritmi e Principi dell'Informatica" (API) of Politecnico di Milano, academic year 2023/2024.

<table>
  <tr>
    <th>Problem</th>
    <th>Time Limit [s]</th>
    <th>Memory Limit [MiB]</th>
    <th>Type</th>
  </tr>
  <tr>
    <th>Open</th>
    <th>45</th>
    <th>150</th>
    <th>Batch</th>
  </tr>
  <tr>
    <th>18</th>
    <th>14</th>
    <th>35</th>
    <th>Batch</th>
  </tr>
  <tr>
    <th>21</th>
    <th>11.5</th>
    <th>30</th>
    <th>Batch</th>
  </tr>
  <tr>
    <th>24</th>
    <th>9</th>
    <th>25</th>
    <th>Batch</th>
  </tr>
  <tr>
    <th>27</th>
    <th>6.5</th>
    <th>20</th>
    <th>Batch</th>
  </tr>
  <tr>
    <th>30</th>
    <th>4</th>
    <th>15</th>
    <th>Batch</th>
  </tr>
  <tr>
    <th>Cum Laude</th>
    <th>1.5</th>
    <th>14</th>
    <th>Batch</th>
  </tr>
</table>

## How to run

```bash
rm main && rm test.txt && make main
./main < test_cases/<test_case>.txt > test.txt
diff test.txt test_cases/<test_case>.output.txt
```

### Valgrind

Remove the `-fsanitize=address` flag from the `Makefile` and add the `-g` and `-ggdb` flags at the end of the `CFLAGS` variable.

**Check for memory leaks:**

```bash
valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./main < test_cases/<test_case>.txt
```

**Inspect time of execution:**
```bash
valgrind --tool=callgrind ./main < test_cases/<test_case>.txt
kcachegrind callgrind.out.PID
```

**Inspect memory usage:**
```bash
valgrind --tool=massif ./main < test_cases/<test_case>.txt
ms_print massif.out.PID
```

**Check for cache misses:**
```bash
valgrind --tool=cachegrind ./main < test_cases/<test_case>.txt
cg_annotate cachegrind.out.<pid> # or kcachegrind cachegrind.out.PID
```

## Notes

- [Course site](https://martinenghi.faculty.polimi.it/courses/api/index.html)
- [Project Specification](https://github.com/FedericoBruzzone/API-project-2024/blob/main/2023_2024.pdf)
- [Project Presentation](https://github.com/FedericoBruzzone/API-project-2024/blob/main/PFAPI2023-2024.pdf)
- [Development Tools](https://github.com/FedericoBruzzone/API-project-2024/blob/main/strumenti_progetto_api.pdf)

## Contact

If you have any questions, suggestions, or feedback, do not hesitate to [contact me](https://federicobruzzone.github.io/).

