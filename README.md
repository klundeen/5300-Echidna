# 5300-Echidna
Student DB Relation Manager project for CPSC5300 at Seattle U, Winter 2024

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
  - [Interacting with SQL](#interacting-with-sql)
  - [Testing Heap Storage Functionality](#testing-heap-storage-functionality)
- [Project Structure](#project-structure)
- [Clean Up](#clean-up)
- [Authors](#authors)
- [Acknowledgments](#acknowledgments)

## Installation

1. Clone the repository:

    ```bash
    git clone https://github.com/klundeen/5300-Echidna.git
    cd 5300-Echidna
    ```

2. Build the executable using the provided Makefile:

    ```bash
    make
    ```
    
## Usage

Ensure that the Berkeley DB environment is set up by providing the path as a command-line argument when running the executable:

```bash
./main <dbenvpath>
```
Replace <dbenvpath> with the path to the BerkeleyDB database environment. For example, if you are working on cs1 server:

```bash
./main ~/cpsc5300/data
```

### 1.Interacting with SQL
Enter SQL statements interactively:
```bash
SQL> <your_sql_statement>
```
For example:
```bash
SQL> SELECT * FROM table_name;
```
To exit the program, enter:

```bash
SQL> quit
```

### 2.Testing Heap Storage Functionality
To test the functionality of heap storage, enter:

```bash
SQL> test
```
## Clean Up
To clean up the compiled files, use:

```
make clean
```

This will remove the main executable and object files.

## Project Structure

```
5300-Echidna
│   LICENSE
│   Makefile
│   README.md
│
└───src
    │   main.cpp
    │   SQLprinting.cpp
    |   heap_storage.cpp
    │   SQLprinting.h
    │   storage_engine.h
    │   heap_storage.cpp
```



# Authors
Kevin Lundeen
Lisa Lomidze