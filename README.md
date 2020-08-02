# CP386 Assignment 4 Readme

## Project Title: Assignment 4

## Features: 
* Read resources from File
* Print sequence states
* Run current sequence
* Request resources (RQ)
* Release resources (RL)

## Motivation: 
<pre>To achieve a <b>12</b> in this course</pre>

## Tests:
	osc@ubuntu:~/CP386A4/$ ./a.out 10 10 10 10
    Number of Customers : 5
    Currently Available resources: 10 10 10 10
    Maximum resources from file
    6,4,7,3
    4,2,3,2
    2,5,3,3
    6,3,3,2
    5,6,7,5
    RQ 3 5 2 7
    Request is satisfied
    Enter Command: R1 1 3 2 4 3
## Installation: -
To get this working locally you need to clone the repository and enter the folder
```bash
git clone https://github.com/ThePark0ur/CP386A4.git
cd CP386A4/
```
After you enter the folder you need to use make to make the file and then run it.
```bash
make
./a.out 10 10 10 10
```

## Code Example: -
```bash
./a.out 10 10 10 10
```
## Screenshots: -

## Authors:
```
-Matthew Dietrich
-Kamran Tayyab
```

## Individual Contributions:
Matthew: - ```Request(), Release(), ReadFile(), SilentRequest(), SilentRelease(), InputParser();```

Kamran: - ```SafetyAlgo(), ClientExec(), Run();```

## Credits:
Kamran and Matthew

## License:
This repository is licensed under the Apache License 2.0, explained further in the license file.
