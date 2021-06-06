# 5300-Echidna 

5300-Echidna's DB Relation Manager project

## Tags
- <code>Milestone1</code> is playing around with the AST returned by the HyLine parser and general setup of the command loop.
- <code>Milestone2</code> has the requirement files for Milestone2.
- <code>Milestone3</code> Implemented CREATE TABLE, DROP TABLE, SHOW TABLES, SHOW COLUMNS
- <code>Milestone4</code> Implemented CREATE INDEX, SHOW INDEX, DROP INDEX
- <code>Milestone5</code> Implemented INSERT, DELETE, SELECTED Queries
- <code>Milestone6</code> Implemented BTREE, and Lookup
*Sprint Verano:*

To run milestone1 on cs1:

$ clone git repositiory: git clone https://github.com/klundeen/5300-Echidna

$ cd 5300-Echidna

$ make

$ ./sql5300 ~/cpsc5300/data

$ SQL> *enter SQL command here to see parser output*

To run milestone2 on cs1:

$ make

$ ./sql5300 ~/cpsc5300/data

$ SQL> test

Note: make sure to delete the contents of ~/cpsc5300/data between each run or File Exists DbException will occur.

Use the following commands to empty data folder

$ rm -f ~/cpsc5300/data/*


Sprint 1 Handoff Video:

https://user-images.githubusercontent.com/49925867/115344024-947c3600-a161-11eb-94fd-1de4e2958235.mp4

*Sprint Otono:*

To run milestone3 and milestone4 on cs1:

$ clone git repositiory: git clone https://github.com/klundeen/5300-Echidna

$ cd 5300-Echidna

Note: Remove the old data between each run to prevent DbException error for File Exists
$ rm -f ~/cpsc5300/data/*

$ make

$ ./sql5300 ~/cpsc5300/data

$ SQL> *enter SQL command here*

*Supported SQL commnads for Sprint Otono:*

- <code>show tables</code>
- <code>create table goober (x integer, y integer, z integer)</code>
- <code>show columns from goober </code>
- <code>create index fx on goober (x,y)</code>
- <code>show index from goober</code>
- <code>drop index fx from goober</code>
- <code>drop table goober</code>
- <code>quit</code>

*Sprint Invierno:*

To run milestone5 and milestone6 on cs1:


$ clone git repositiory: git clone https://github.com/klundeen/5300-Echidna

$ cd 5300-Echidna

Note: Remove the old data between each run to prevent DbException error for File Exists
$ rm -f ~/cpsc5300/data/*

$ make

$ ./sql5300 ~/cpsc5300/data

$ SQL> *enter SQL command here*

- <code>show tables</code>
- <code>create table goober (x integer, y integer, z integer)</code>
- <code>show columns from goober </code>
- <code>create index fx on goober (x,y)</code>
- <code>show index from goober</code>
- <code>drop index fx from goober</code>
- <code>drop table goober</code>
- <code>insert into goober value (1,2,2);</code>
- <code>select * from goober </code>
- <code>delete from goober where y = 2</code>
- <code>quit</code>
