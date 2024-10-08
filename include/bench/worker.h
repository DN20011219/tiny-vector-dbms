// Copyright (c) 2024 by dingning
//
// file  : worker.h
// since : 2024-07-TODO
// desc  : this class will handle the sql and return execute result to client.

#ifndef VDBMS_BENCH_WORKER_H_
#define VDBMS_BENCH_WORKER_H_

#include <iostream>
#include <string>
#include <thread>

#include "./session.h"
#include "./queue_msg.h"
#include "../sql/sql_struct.h"

// 
#include "../sql/executer/operator.h"
#include "../sql/executer/optimizer.h"
#include "../sql/parser/parser.h"
#include "../sql/parser/ast.h"

using std::string;
using std::cout;
using std::endl;

namespace tiny_v_dbms {

class Worker
{
    friend class Connector;

private:
    // cache session
    Session* user_session;
    bool working;

    // shared base db
    DB* base_db;

    // information about sql
    string executing_sql;
    SqlResponse* executing_sql_response;
    
    // execute tools
    Parser* parser;
    Operator* op;
    Optimizer* opt;

public:

    Worker(Session* user, Operator* used_op, DB* base_db) : op(used_op), base_db(base_db)
    {
        user_session = user;
        working = true;
        parser = new Parser();
        opt = new Optimizer(op);
    }

    ~Worker()
    {
        working = false;

        // delete pointer resource
        if (executing_sql_response != nullptr)
            delete executing_sql_response;

        delete parser;
        delete opt;
    }

    void Work()
    {
        // execute sql
        AST* ast = parser->BuildAST(executing_sql);
        
        // if sql can not be parsed, return false
        if (ast == nullptr)
        {
            executing_sql_response = new SqlResponse();
            executing_sql_response->sql_state = FAILURE;
            executing_sql_response->information = "Sql can not be parsed!";
            return;
        }

        // can not execute sql about database
        if (ast->GetType() == CREATE_DATABASE_NODE || ast->GetType() == DROP_DATABASE_NODE)
        {
            executing_sql_response = new SqlResponse();
            executing_sql_response->sql_state = FAILURE;
            executing_sql_response->information = "This connection has no power to change db construct!";
            return;
        }
        
        // CREATE TABLE
        if (ast->GetType() == CREATE_TABLE_NODE)
        {
            default_amount_type column_nums = ast->create_table_sql->columns.size();
            ColumnTable new_table(column_nums);
            new_table.table_name = ast->create_table_sql->table_name;
            new_table.table_type = 0;   // no use
            // init col
            for (default_amount_type i = 0; i < ast->create_table_sql->columns.size(); i++)
            {
                new_table.columns.column_name_array[i] = ast->create_table_sql->columns[i].col_name;
                new_table.columns.column_length_array[i] = ast->create_table_sql->columns[i].col_length;
                new_table.columns.column_type_array[i] = ast->create_table_sql->columns[i].value_type;
                new_table.columns.column_index_type_array[i] = IndexType::NONE_INDEX;   // no use
                // new_table.columns.column_storage_address_array[i]
            }
            executing_sql_response = op->CreateTable(user_session->cached_db, &new_table);

            return;
        }

        // INSERT
        if (ast->GetType() == INSERT_INTO_TABLE_NODE)
        {   
            executing_sql_response = op->Insert(
                user_session->cached_db, 
                ast->insert_into_table_sql->table_name, 
                ast->insert_into_table_sql->columns, 
                ast->insert_into_table_sql->values
            );

            return;
        }

        // SELECT todo:
        if (ast->GetType() == SELECT_FROM_ONE_TABLE_NODE)
        {   
            executing_sql_response = opt->ExecuteSelect(
                user_session->cached_db, 
                ast->select_from_one_table_sql
            );
            return;
        }


        // TODO: execute other type of sql
        
        // write back result  
        executing_sql_response = new SqlResponse();
        executing_sql_response->sql_state = SqlState::SUCCESS;
        executing_sql_response->information = "success";  
    }

    void RootIdentityWork()
    {
        // execute sql
        cout << "input RootIdentityWork sql: " << executing_sql << endl;
        
        AST* ast = parser->BuildAST(executing_sql);

        // if sql can not be parsed, return false
        if (ast == nullptr)
        {
            executing_sql_response = new SqlResponse();
            executing_sql_response->sql_state = FAILURE;
            executing_sql_response->information = "Sql can not be parsed!";
            return;
        }

        // execute sql about database
        if (ast->GetType() == CREATE_DATABASE_NODE)
        {
            WorkMsg database_msg;
            database_msg.msg_type = BASE_DB_WORKER_RECEIVE_QUEUE_ID;
            memcpy(database_msg.msg_data, &user_session->msg_queue_id, sizeof(long));
            msgsnd(base_db_worker_msg_key, &database_msg, WORK_MSG_DATA_LENGTH, 0);

            database_msg.msg_type = user_session->msg_queue_id;
            database_msg.SerializeCreateDropDBMessage(true, ast->create_database_sql->db_name);
            msgsnd(base_db_worker_msg_key, &database_msg, WORK_MSG_DATA_LENGTH, 0);
            
            msgrcv(base_db_worker_msg_key, &database_msg, WORK_MSG_DATA_LENGTH, user_session->msg_queue_id + 1, 0);
            executing_sql_response = new SqlResponse();
            executing_sql_response->Deserialize(database_msg.msg_data);
            return;
        }
        else if (ast->GetType() == DROP_DATABASE_NODE)
        {
            WorkMsg database_msg;
            database_msg.msg_type = BASE_DB_WORKER_RECEIVE_QUEUE_ID;
            memcpy(database_msg.msg_data, &user_session->msg_queue_id, sizeof(long));
            msgsnd(base_db_worker_msg_key, &database_msg, WORK_MSG_DATA_LENGTH, 0);

            database_msg.msg_type = user_session->msg_queue_id;
            database_msg.SerializeCreateDropDBMessage(true, ast->create_database_sql->db_name);
            msgsnd(base_db_worker_msg_key, &database_msg, WORK_MSG_DATA_LENGTH, 0);

            msgrcv(base_db_worker_msg_key, &database_msg, WORK_MSG_DATA_LENGTH, user_session->msg_queue_id + 1, 0);
            executing_sql_response = new SqlResponse();
            executing_sql_response->Deserialize(database_msg.msg_data);
            return;
        }
        else
        {
            // can not execute other type of sql!
            executing_sql_response = new SqlResponse();
            executing_sql_response->sql_state = SqlState::FAILURE;
            executing_sql_response->information = "Can not execute this type sql";
        }
    }

    void ListenThread()
    {
        // check msg queue is ready
        if (worker_msg_key < 0)
        {
            throw std::runtime_error("worker_msg_key queue not start successfully");
        }

        WorkMsg work_msg;
        long send_back_id = user_session->msg_queue_id + 1; // store the send back queue id

        while(working)
        {   
            // receive msg id
            work_msg.msg_type = user_session->msg_queue_id; // use special receive queue id
            msgrcv(worker_msg_key, &work_msg, WORK_MSG_DATA_LENGTH, user_session->msg_queue_id, 0);
            
            // get sql length
            int sql_length;
            memcpy(&sql_length, work_msg.msg_data, sizeof(sql_length));
            assert(sql_length < SQL_MAX_LENGTH);

            // store sql
            char* sql_cache = new char[sql_length];
            memcpy(sql_cache, work_msg.msg_data + sizeof(sql_length), sql_length);
            executing_sql.assign(sql_cache);
            delete[] sql_cache;

            // execute sql
            cout << "input Work sql: " << executing_sql << endl;
            Work();
            cout << "input executing_sql_response: " << executing_sql_response->sql_state << " " << executing_sql_response->information << endl;

            // write response to queue
            executing_sql_response->Serialize(work_msg.msg_data);
            work_msg.msg_type = send_back_id;
            msgsnd(worker_msg_key, &work_msg, executing_sql_response->GetLength(), 0);

            // free resource
            delete executing_sql_response;
            executing_sql_response = nullptr;
        }
    }
    
    void RootIdentityListenThread()
    {
        // check msg queue is ready
        if (worker_msg_key < 0)
        {
            throw std::runtime_error("worker_msg_key queue not start successfully");
        }
        
        cout << "worker_msg_key: " << worker_msg_key << endl;

        WorkMsg work_msg;
        long send_back_id = user_session->msg_queue_id + 1; // store the send back queue id

        while(working)
        {   
            // receive msg id
            work_msg.msg_type = user_session->msg_queue_id; // use special receive queue id
            msgrcv(worker_msg_key, &work_msg, WORK_MSG_DATA_LENGTH, user_session->msg_queue_id, 0);

            // get sql length
            int sql_length;
            memcpy(&sql_length, work_msg.msg_data, sizeof(sql_length));
            assert(sql_length < SQL_MAX_LENGTH);

            // store sql
            char* sql_cache = new char[sql_length];
            memcpy(sql_cache, work_msg.msg_data + sizeof(sql_length), sql_length);
            executing_sql.assign(sql_cache);
            delete[] sql_cache;

            // execute sql
            RootIdentityWork();

            // write response to queue
            executing_sql_response->Serialize(work_msg.msg_data);
            work_msg.msg_type = send_back_id;
            msgsnd(worker_msg_key, &work_msg, executing_sql_response->GetLength(), 0);

            // free resource
            delete executing_sql_response;
            executing_sql_response = nullptr;
        }
    }
    
    void BaseDBListenThread()
    {   
        // check msg queue is ready
        if (base_db_worker_msg_key < 0)
        {
            throw std::runtime_error("base_db_worker_msg_key queue not start successfully");
        }
        
        cout << "base_db_worker_msg_key: " << base_db_worker_msg_key << endl;

        WorkMsg work_msg;

        while(working)
        {   
            // receive one command msg, include the customized queue id 
            msgrcv(base_db_worker_msg_key, &work_msg, WORK_MSG_DATA_LENGTH, user_session->msg_queue_id, 0);
            long receive_queue_id;
            memcpy(&receive_queue_id, work_msg.msg_data, sizeof(long));
            long send_back_id = receive_queue_id + 1; // store the send back queue id

            // receive 
            msgrcv(base_db_worker_msg_key, &work_msg, WORK_MSG_DATA_LENGTH, receive_queue_id, 0);
            bool is_create; // true is create
            string db_name;
            work_msg.DeserializeCreateDropDBMessage(is_create, db_name);

            // execute 
            if (is_create)
            {
                // create db
                executing_sql_response = op->CreateDB(base_db, db_name);
                cout << "create db: " << db_name << endl;
            }
            else
            {
                // drop db
                cout << "drop db: " << db_name << endl;
            }

            // write response to queue
            executing_sql_response->Serialize(work_msg.msg_data);

            work_msg.msg_type = send_back_id;
            msgsnd(base_db_worker_msg_key, &work_msg, executing_sql_response->GetLength(), 0);
            delete executing_sql_response;
        }
    }

};


}

#endif // VDBMS_BENCH_WORKER_H_