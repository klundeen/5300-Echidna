/**
 * @file SQLExec.cpp - implementation of SQLExec class
 * @authors Kevin Lundeen, Dnyandeep, Samuel
 * @see "Seattle University, CPSC5300, Winter 2024"
 */
#include "SQLExec.h"
#include <sql/DropStatement.h>

using namespace std;
using namespace hsql;

// define static data
Tables* SQLExec::tables = nullptr;
Indices* SQLExec::indices = nullptr;

// make query result be printable
ostream& operator<<(ostream& out, const QueryResult& qres) {
    if (qres.column_names != nullptr) {
        for (Identifier& column_name: *qres.column_names)
            out << column_name << " ";
        out << endl << "+";
        for (unsigned int i = 0; i < qres.column_names->size(); i++)
            out << "----------+";
        out << endl;
        for (ValueDict* row: *qres.rows) {
            for (Identifier& column_name: *qres.column_names) {
                Value value = row->at(column_name);
                switch (value.data_type) {
                    case ColumnAttribute::INT:
                        out << value.n;
                        break;
                    case ColumnAttribute::TEXT:
                        out << "\"" << value.s << "\"";
                        break;
                    case ColumnAttribute::BOOLEAN:
                        out << (value.n == 0 ? "false" : "true");
                        break;
                    default:
                        out << "???";
                }
                out << " ";
            }
            out << endl;
        }
    }
    out << qres.message;
    return out;
}

QueryResult::~QueryResult() {
    if (this->column_names)
        delete this->column_names;
    if (this->column_attributes)
        delete this->column_attributes;
    if (this->rows) {
        for (ValueDict* row: *this->rows)
            delete row;
        delete this->rows;
    }
}

/**
 * Executes a given SQL statement and returns the result as a QueryResult object.
 * It also initializes the schema tables if they haven't been initialized yet.
 *
 * @param statement Pointer to a SQLStatement object representing the SQL statement to execute.
 * @return Pointer to a QueryResult object containing the outcome of the executed statement.
 * @throws SQLExecError if an error occurs during statement execution.
 */

QueryResult* SQLExec::execute(const SQLStatement* statement) {
    if (!SQLExec::tables)
        SQLExec::tables = new Tables();
    if (!SQLExec::indices)
        SQLExec::indices = new Indices();

    try {
        switch (statement->type()) {
            case kStmtCreate:
                return create((const CreateStatement*) statement);
            case kStmtDrop:
                return drop((const DropStatement*) statement);
            case kStmtShow:
                return show((const ShowStatement*) statement);
            case kStmtInsert:
                return insert((const InsertStatement*) statement);
            case kStmtDelete:
                return del((const DeleteStatement*) statement);
            case kStmtSelect:
                return select((const SelectStatement*) statement);
            default:
                return new QueryResult("not implemented");
        }
    } catch (DbRelationError& e) {
        throw SQLExecError("DbRelationError: " + string(e.what()));
    }
}

QueryResult* SQLExec::insert(const InsertStatement* statement) {
    Identifier table_name = statement->tableName;

    // check table exists
    ValueDict where = {{"table_name", Value(table_name)}};
    Handles* tabMeta = SQLExec::tables->select(&where);
    bool tableExists = !tabMeta->empty();
    delete tabMeta;
    if (!tableExists)
        throw SQLExecError("attempting to insert into non-existent table " + table_name);
    DbRelation& table = SQLExec::tables->get_table(table_name);
    
    // create row
    ValueDict row;
    const ColumnNames& cn = table.get_column_names();
    size_t values_n = statement->values->size();
    for (size_t i = 0; i < values_n; i++) {
        Identifier column = cn[i];
        Expr* value = (*statement->values)[i];
        switch (value->type) {
            case kExprLiteralInt:
                row[column] = Value(value->ival);
                break;
            case kExprLiteralString:
                row[column] = Value(value->name);
                break;
            default:
                throw SQLExecError("column attribute unrecognized");
        }
    }

    // insert into table and existing indices
    Handle insertion = table.insert(&row);
    IndexNames indices = SQLExec::indices->get_index_names(table_name);
    for (const Identifier& idx : indices) {
        DbIndex &index = SQLExec::indices->get_index(table_name, idx);
        index.insert(insertion);
    }
    string suffix = indices.size() ? " and into " + to_string(indices.size()) + " indices" : "";
    return new QueryResult("successfully inserted 1 row into " + table_name + suffix);
}

void get_where_conjunction(const Expr* where, ValueDict* conjunction) {
    if (where->opType == Expr::OperatorType::AND) {
        get_where_conjunction(where->expr, conjunction);
        get_where_conjunction(where->expr2, conjunction);
    } else if (where->opType == Expr::OperatorType::SIMPLE_OP && where->opChar == '=') {
        switch (where->expr2->type) {
            case kExprLiteralInt:
                (*conjunction)[where->expr->name] = Value(where->expr2->ival);
                break;
            case kExprLiteralString:
                (*conjunction)[where->expr->name] = Value(where->expr2->name);
                break;
            default:
                throw SQLExecError("unrecognized expression");
        }
    }
}

ValueDict* get_where_conjunction(const Expr* where) {
    ValueDict* conjunction = new ValueDict();
    get_where_conjunction(where, conjunction);
    return conjunction;
}


QueryResult* SQLExec::del(const DeleteStatement* statement) {
    Identifier table_name = statement->tableName;

    // check table exists
    ValueDict where = {{"table_name", Value(table_name)}};
    Handles* tabMeta = SQLExec::tables->select(&where);
    bool tableExists = !tabMeta->empty();
    delete tabMeta;
    if (!tableExists)
        throw SQLExecError("attempting to delete from non-existent table " + table_name);
    DbRelation& table = SQLExec::tables->get_table(table_name);
    
    // evaluation plan
    EvalPlan* plan = new EvalPlan(table);
    if (statement->expr)
        plan = new EvalPlan(get_where_conjunction(statement->expr), plan);
    plan = plan->optimize();

    // get handles to remove tuples from table and indices
    Handles* handles = plan->pipeline().second;
    IndexNames indices = SQLExec::indices->get_index_names(table_name);
    for (const Handle& handle : *handles) {
        // FIXME: Implement index row del
        // for (const Identifier& index : indices)
        //     SQLExec::indices->get_index(table_name, index).del(handle);
        table.del(handle);
    }

    size_t rows_n = handles->size();
    size_t indices_n = indices.size();
    string suffix = indices_n ? " FIXME: and from " + to_string(indices_n) + " indices" : "";
    delete plan;
    delete handles;
    return new QueryResult("successfully deleted " + to_string(rows_n) + " rows" + suffix);
}

QueryResult* SQLExec::select(const SelectStatement* statement) {
    Identifier table_name = statement->fromTable->getName();

    // check table exists
    ValueDict where = {{"table_name", Value(table_name)}};
    Handles* tabMeta = SQLExec::tables->select(&where);
    bool tableExists = !tabMeta->empty();
    delete tabMeta;
    if (!tableExists)
        throw SQLExecError("attempting to select from non-existent table " + table_name);
    DbRelation& table = SQLExec::tables->get_table(table_name);
    ColumnNames* cn = new ColumnNames();
    for (const Expr* expr : *statement->selectList) {
        if (expr->type == kExprStar)
            for (const Identifier& col : table.get_column_names())
                cn->push_back(col);
        else
            cn->push_back(expr->name);
    }

    // start base of plan at a TableScan
    EvalPlan* plan = new EvalPlan(table);

    // enclose in selection if where clause exists
    if (statement->whereClause)
        plan = new EvalPlan(get_where_conjunction(statement->whereClause), plan);
    
    // wrap in project
    plan = new EvalPlan(cn, plan);

    // optimize and evaluate
    plan = plan->optimize();
    ValueDicts* rows = plan->evaluate();
    delete plan;
    return new QueryResult(cn, table.get_column_attributes(*cn), rows, "successfully return " + to_string(rows->size()) + " rows");
}

/**
 * Extracts column definition details from an hsql::ColumnDefinition object.
 *
 * @param col Pointer to an hsql::ColumnDefinition object
 * @param column_name Reference to a string where the column's name will be stored.
 * @param column_attribute Reference to a ColumnAttribute object where the column's data type will be stored.
 * @throws SQLExecError if the column data type is not supported.
 */

void SQLExec::column_definition(const ColumnDefinition* col, Identifier& column_name, ColumnAttribute& column_attribute) {
    column_name = col->name;
    switch (col->type) {
        case ColumnDefinition::DataType::INT:
            column_attribute = ColumnAttribute::DataType::INT;
            break;
        case ColumnDefinition::DataType::TEXT:
            column_attribute = ColumnAttribute::DataType::TEXT;
            break;
        default:
            throw SQLExecError("not implemented");
    }
}

/**
 * Handles the CREATE statement, it supports two types CREATE TABLE and CREATE INDEX
 *
 * @param statement Pointer to a CreateStatement object.
 * @return Pointer to a QueryResult object containing the outcome of the CREATE operation.
 */

QueryResult* SQLExec::create(const CreateStatement* statement) {
    switch(statement->type) {
        case CreateStatement::kTable:
            return create_table(statement);
        case CreateStatement::kIndex:
            return create_index(statement);
        default:
            return new QueryResult("not implemented");
    }
}

/**
 * Handles the CREATE TABLE statement and physically creates a new table in the database based
 * on the specs in the statement. It updates the schema tables (_tables,_columns) accordingly.
 *
 * @param statement Pointer to a CreateStatement object specifying the table to create.
 * @return Pointer to a QueryResult object indicating the success of the operation
 * @throws DbRelationError if an error occurs during table creation.
 */

QueryResult* SQLExec::create_table(const CreateStatement* statement) {
    // update _tables schema
    ValueDict row = {{"table_name", Value(statement->tableName)}};
    Handle tableHandle = SQLExec::tables->insert(&row);
    try {
        // update _columns schema
        Handles columnHandles;
        DbRelation& columns = SQLExec::tables->get_table(Columns::TABLE_NAME);
        try {
            for (ColumnDefinition* column : *statement->columns) {
                Identifier cn;
                ColumnAttribute ca;
                column_definition(column, cn, ca);
                std::string type = ca.get_data_type() == ColumnAttribute::DataType::TEXT ? "TEXT" : "INT";
                ValueDict row = {
                    {"table_name", Value(statement->tableName)},
                    {"column_name", Value(cn)},
                    {"data_type", Value(type)}
                };
                columnHandles.push_back(columns.insert(&row));
            }

            // create table
            DbRelation& table = SQLExec::tables->get_table(statement->tableName);
            if (statement->ifNotExists)
                table.create_if_not_exists();
            else
                table.create();
        } catch (...) {
            // attempt to undo the insertions into _columns
            try {
                for (Handle& columnHandle : columnHandles)
                    columns.del(columnHandle);
            } catch (...) {}
            throw;
        }
    } catch (...) {
        // attempt to undo the insertion into _tables
        try {
            SQLExec::tables->del(tableHandle);
        } catch (...) {}
        throw;
    }

    return new QueryResult("created table " + string(statement->tableName));
}

/**
 * Handles the CREATE INDEX statement and physically creates a new table in the database based on the specs in the statement.
 * It updates schema tables (_tables,_columns,_indices)
 * @param statement Pointer to a CreateStatement object specifying the index to create.
 * @return Pointer to a QueryResult object indicating the success of the index creation.
 * @throws DbRelationError if an error occurs during index creation.
 */

QueryResult* SQLExec::create_index(const CreateStatement* statement) {
    DbRelation& table = SQLExec::tables->get_table(statement->tableName);

    // check that all the index columns exist in the table
    const ColumnNames& cn = table.get_column_names();
    for (char* column_name : *statement->indexColumns)
        if (find(cn.begin(), cn.end(), string(column_name)) == cn.end())
            throw SQLExecError("no such column " + string(column_name) + " in table " + statement->tableName);

    // insert a row for each column in index key into _indices
    ValueDict row = {
        {"table_name", Value(statement->tableName)},
        {"index_name", Value(statement->indexName)},
        {"column_name", Value("")},
        {"seq_in_index", Value()},
        {"index_type", Value(statement->indexType)},
        {"is_unique", Value(string(statement->indexType) == "BTREE")}
    };
    for (char* column_name : *statement->indexColumns) {
        row["column_name"] = Value(column_name);
        row["seq_in_index"].n += 1;
        SQLExec::indices->insert(&row);
    }

    // call get_index to get a reference to the new index and then invoke the create method on it
    DbIndex& index = SQLExec::indices->get_index(string(statement->tableName), string(statement->indexName));
    try {
        index.create();
    } catch (DbRelationError& e) {
        DropStatement drop(DropStatement::kIndex);
        drop.name = statement->tableName;
        drop.indexName = statement->indexName;
        SQLExec::drop_index(&drop);
        drop.name = nullptr;
        drop.indexName = nullptr;
        throw e;
    }

    return new QueryResult("created index " + string(statement->indexName));
}

QueryResult* SQLExec::drop(const DropStatement* statement) {
    switch (statement->type) {
        case DropStatement::kTable:
            return drop_table(statement);
        case DropStatement::kIndex:
            return drop_index(statement);
        default:
            return new QueryResult("not implemented");
    }
}

/**
 * Handles the dropping of tables based on a DROP TABLE statement and handles
 * the necessary updates in schema tables.
 *
 * @param statement Pointer to a DropStatement object specifying the table to drop.
 * @return Pointer to a QueryResult object indicating the success of the table drop.
 * @throws SQLExecError if an error occurs during table drop.
 */

QueryResult* SQLExec::drop_table(const DropStatement* statement) {
    if (statement->type != DropStatement::kTable)
        throw SQLExecError("unrecognized DROP type");
    
    // get table name
    Identifier table_name = statement->name;
    if (table_name == Tables::TABLE_NAME || table_name == Columns::TABLE_NAME || table_name == Indices::TABLE_NAME)
        throw SQLExecError("Cannot drop a schema table!");
    ValueDict where = {{"table_name", Value(table_name)}};

    // check table exists
    Handles* tabMeta = SQLExec::tables->select(&where);
    bool tableExists = !tabMeta->empty();
    delete tabMeta;
    if (!tableExists)
        throw SQLExecError("attempting to drop non-existent table " + table_name);

    // before dropping the table, drop each index on the table
    Handles* selected = SQLExec::indices->select(&where);
    for (Handle& row : *selected)
        SQLExec::indices->del(row);
    delete selected;

    // remove columns    
    DbRelation& columns = SQLExec::tables->get_table(Columns::TABLE_NAME);
    Handles* rows = columns.select(&where);
    for (Handle& row : *rows)
        columns.del(row);
    delete rows;

    // remove table
    DbRelation& table = SQLExec::tables->get_table(table_name);
    table.drop();
    rows = SQLExec::tables->select(&where);
    SQLExec::tables->del(*rows->begin());
    delete rows;

    return new QueryResult("dropped table " + table_name);    
}

/**
 * Handles the dropping of indexes based on a DROP INDEX statement.
 *
 * @param statement Pointer to a DropStatement object specifying the index to drop.
 * @return Pointer to a QueryResult object indicating the outcome of the drop index operation.
 */

QueryResult* SQLExec::drop_index(const DropStatement* statement) {
    // call get_index to get a reference to the index and then invoke the drop method on it
    Identifier table_name = statement->name; 
    Identifier index_name = statement->indexName; 
    DbIndex& index = SQLExec::indices->get_index(table_name, index_name);
    index.drop();

    ValueDict where = {
        {"table_name", Value(table_name)},
        {"index_name", Value(index_name)}
    };

    // check index exists
    Handles* idxMeta = SQLExec::indices->select(&where);
    bool indexExists = !idxMeta->empty();
    delete idxMeta;
    if (!indexExists)
        throw SQLExecError("attempting to drop non-existent index " + index_name + " on " + table_name);

    // remove all the rows from _indices for this index
    Handles* selected = SQLExec::indices->select(&where);
    for (Handle& row : *selected)
        SQLExec::indices->del(row);
    delete selected;

    return new QueryResult("dropped index " + index_name + " on " + table_name);
}

/**
 * Handles the display of tables, columns, or indexes based on a SHOW statement.
 *
 * @param statement Pointer to a ShowStatement object
 * @return Pointer to a QueryResult object containing the requested information.
 * @throws SQLExecError if the operation type is not allowed or not implemented.
 */

QueryResult* SQLExec::show(const ShowStatement* statement) {
    switch(statement->type) {
        case ShowStatement::kTables:
            return show_tables();
        case ShowStatement::kColumns:
            return show_columns(statement);
        case ShowStatement::kIndex:
            return show_index(statement);
        default:
            return new QueryResult("not implemented");
    }
}

/**
 * Handles the SHOW TABLES statement which displays the _tables table that contains
 * all the tables information in the database
 *
 * @return Pointer to a QueryResult object containing the requested information.
 */

QueryResult* SQLExec::show_tables() {
    // get column names and attributes
    ColumnNames* cn = new ColumnNames();
    ColumnAttributes* ca = new ColumnAttributes();
    SQLExec::tables->get_columns(Tables::TABLE_NAME, *cn, *ca);

    // get table names
    Handles* tables = SQLExec::tables->select();
    ValueDicts* rows = new ValueDicts();
    for (Handle& table : *tables) {
        ValueDict* row = SQLExec::tables->project(table, cn);
        Identifier table_name = (*row)["table_name"].s;
        if (table_name != Tables::TABLE_NAME && table_name != Columns::TABLE_NAME && table_name != Indices::TABLE_NAME)
            rows->push_back(row);
        else
            delete row;
    }
    delete tables;
    return new QueryResult(cn, ca, rows, "successfully returned " + to_string(rows->size()) + " rows");
}

/**
 *  Handles the SHOW COLUMNS FROM <TABLE> statement which displays the column data
 *  from the _columns table on the give table
 *
 * @param statement Pointer to a ShowStatement object specifying the table
 * @return Pointer to a QueryResult object containing the column information.
 */

QueryResult *SQLExec::show_columns(const ShowStatement *statement)
{
    ColumnNames *column_names = new ColumnNames({"table_name", "column_name", "data_type"});
    ColumnAttributes *column_attributes = new ColumnAttributes({ColumnAttribute(ColumnAttribute::DataType::TEXT)});

    // Check if specific table is requested
    if (statement->tableName != nullptr)
    {
        ValueDict where = {{"table_name", Value(statement->tableName)}};
        DbRelation &columns = tables->get_table(Columns::TABLE_NAME);
        Handles *rows = columns.select(&where);
        ValueDicts *data = new ValueDicts();
        for (Handle &row : *rows)
        {
            data->push_back(columns.project(row, column_names));
        }
        delete rows;
        return new QueryResult(column_names, column_attributes, data, "successfully returned " + to_string(data->size()) + " rows");
    }
    else
    {
        // If no table specified, retrieve all columns
        Handles *rows = tables->select();
        ValueDicts *data = new ValueDicts();
        for (Handle &row : *rows)
        {
            data->push_back(tables->project(row, column_names));
        }
        delete rows;
        return new QueryResult(column_names, column_attributes, data, "successfully returned " + to_string(data->size()) + " rows");
    }
}

/**
 *  Handles the SHOW INDEX FROM <TABLE> statement which displays the index data
 *  from the _indices table on the give table
 *
 * @param statement Pointer to a ShowStatement object specifying the table
 * @return Pointer to a QueryResult object containing the indices information.
 */

QueryResult *SQLExec::show_index(const ShowStatement *statement)
{
    // SHOW INDEX FROM <table_name>
    Identifier table_name = statement->tableName;

    ColumnNames *column_names = new ColumnNames();
    ColumnAttributes *column_attributes = new ColumnAttributes();
    tables->get_columns(Indices::TABLE_NAME, *column_names, *column_attributes); // get the column names and attr from indices

    ValueDicts *rows = new ValueDicts();
    ValueDict where;
    where["table_name"] = Value(table_name);

    Handles *index_handles = indices->select(&where); // select * from indices where table_name = <table_name>

    for (Handle handle : *index_handles)
    {
        rows->push_back(indices->project(handle));
    }
    string message = "successfully returned " + to_string(index_handles->size()) + " rows";
    delete index_handles;
    return new QueryResult(column_names, column_attributes, rows, message);
}
