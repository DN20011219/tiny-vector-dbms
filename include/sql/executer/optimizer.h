// Copyright (c) 2024 by dingning
//
// file  : optimizer.h
// since : 2024-08-06
// desc  : This file contains all optimizer for sql's excution plan. Mainly for select sql.

#ifndef VDBMS_SQL_EXECUTER_PLAN_H_
#define VDBMS_SQL_EXECUTER_PLAN_H_

#include "../parser/ast.h"
#include "operator.h"

namespace tiny_v_dbms {

struct ProjectionOp
{
    string table_name;
    vector<Column*> cols;

    
};

struct Condition
{
    Column* col;
    Comparator comparator;
    Value* compare_val;
};


struct OperationPlan
{

};

class Optimizer
{
private:
    Operator* op;

public:

    SqlResponse* ExecuteSelect(DB* db, SelectFromOneTableSql* sql)
    {
        SqlResponse* response = new SqlResponse();

        ColumnTable* table;
        if (!op->GetTable(*db, sql->table_name, table))
        {   
            response->sql_state = FAILURE;
            response->information = "Db don't have table named " + sql->table_name;
            return response;
        }
        
        if (sql->columns.size() == 1 && sql->columns[0].col_name == "*")
        {
            // select all cols

        }
        else
        {
            // need additional check
            if (!op->CheckColsExists(table, sql->columns))
            {
                response->sql_state = FAILURE;
                response->information = "Select column not exist!";
                return response;
            }
            
        }

        delete sql;
    }

    vector<Row*>* SelectAll(DB* db, string table_name)
    {

    }

    vector<value_tag>* Projection(DB* db, string table_name, string col_name, Comparator* comparator, Value* compare_value)
    {
        vector<value_tag>* col_values = new vector<value_tag>();
        op->FilterLoad(db, table_name, col_name, comparator, compare_value, *col_values);
        return col_values;
    }

    vector<Row*>* Projection(DB* db, SelectFromOneTableSql* sql)
    {
        
    }


};

}

#endif // VDBMS_SQL_EXECUTER_PLAN_H_