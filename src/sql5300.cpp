/*
    File: main.cpp
    Authors: Kevin Lundeen, Lisa Lomidze
    Date: January 27, 2024
    Description: Main program file for the SQL interpreter. 
    Initializes Berkeley DB environment, takes user input for SQL statements, 
    parses and prints the statements using the SQLprinting class. Allows the user 
    to interactively input SQL statements until the user enters "quit". Allows to test 
    functionality of heap storage if user enters "test".
*/
#include <cstdlib>
#include <iostream>
#include <string>
#include "db_cxx.h"
#include "SQLParser.h"
#include "ParseTreeToString.h"
#include "SQLExec.h"
#include "btree.h"

using namespace std;
using namespace hsql;

/*
 * we allocate and initialize the _DB_ENV global
 */
DbEnv *_DB_ENV;

/**
 * Main entry point of the sql5300 program
 * @args dbenvpath  the path to the BerkeleyDB database environment
 */
int main(int argc, char *argv[]) {

    // Open/create the db enviroment
    if (argc != 2) {
        cerr << "Usage: cpsc5300: dbenvpath" << endl; // /home/st/llomidze/cpsc5300/data
        return 1;
    }

    char *envHome = argv[1];
    cout << "(sql5300: running with database environment at " << envHome << ")" << endl;
    DbEnv env(0U);
    env.set_message_stream(&cout);
    env.set_error_stream(&cerr);

    try {
        env.open(envHome, DB_CREATE | DB_INIT_MPOOL, 0);
    } catch (DbException &exc) {
        cerr << "(sql5300: " << exc.what() << ")";
        exit(1);
    }

    _DB_ENV = &env;

    initialize_schema_tables();
    
    while (true) 
    {
        cout << "SQL> ";
        string query;
        getline(cin, query);

        if (query.length() == 0)
            continue;  // blank line -- just skip

        if (query == "quit")
            break;  // only way to get out

        if (query == "test") {
            cout << "test_heap_storage: " << (test_heap_storage() ? "ok" : "failed") << endl;
            cout << "test_btree: " << (test_btree() ? "ok" : "failed") << endl;
            continue;
        }

        // use the Hyrise sql parser to get us our AST
        SQLParserResult *parse = SQLParser::parseSQLString(query);
        if (!parse->isValid()) {
            cout << "invalid SQL: " << query << endl;
            cout << parse->errorMsg() << endl;
            delete parse;
            continue;
        }
        // execute the statement
        for (uint i = 0; i < parse->size(); ++i) {
            const SQLStatement *statement = parse->getStatement(i);
            try {
                cout << ParseTreeToString::statement(statement) << endl;
                QueryResult *result = SQLExec::execute(statement);
                cout << *result << endl;
                delete result;
            } catch (SQLExecError &e) {
                cout << "Error: " << e.what() << endl;
            }
        }
        delete parse;

    }

    return EXIT_SUCCESS;
}
