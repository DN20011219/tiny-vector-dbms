// Copyright (c) 2024 by dingning
//
// file  : client.h
// since : 2024-08-09
// desc  : This is the client, can send sql to server, and show the execute result.

#ifndef VDBMS_BENCH_CLIENT_H_
#define VDBMS_BENCH_CLIENT_H_

#include <string>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "../config.h"
#include "../bench/queue_msg.h"
#include "../sql/sql_struct.h"

using std::cin;

namespace tiny_v_dbms {

const long queue_list_range[] = {1, 3, 5, 7, 9};  // skip 1, because of the msg sent by server use this queue id + 1

class Client
{
public:

    void RunClient(std::string ip, int port, int connect_identity_type, std::string db_name, long communicate_queue_id)
    {
        if (connector_msg_key < 0)
        {
            throw std::runtime_error("msg queue not start successfully");
        }
        if (worker_msg_key < 0)
        {
            throw std::runtime_error("msg queue not start successfully");
        }

        ConnectMsg msg;

        // set information about the first msg(send_back_queue_id and queue_id)
        msg.msg_type = CONNECTOR_MSG_TYPE_RECV; // use public queue
        memcpy(msg.msg_data, &communicate_queue_id, sizeof(long)); // special queue id
        msgsnd(connector_msg_key, &msg, MSG_DATA_LENGTH, 0);

        // set information about the second msg
        msg.msg_type = communicate_queue_id; // use special queue
        msg.Serialize(ip, port, connect_identity_type, db_name); // serialize data
        msgsnd(connector_msg_key, &msg, MSG_DATA_LENGTH, 0);

        // receive connect result
        msg.msg_type = communicate_queue_id + 1; // use special queue
        msgrcv(connector_msg_key, &msg, MSG_DATA_LENGTH, communicate_queue_id + 1, 0);

        bool state;
        string connect_result = ConnectMsg::DeserializeConnectResult(msg, state);

        // check connection state
        if (!state)
        {   
            throw std::runtime_error("Client connect to server fail! db_name: " + db_name);
        }

        std::cout << std::endl;
        std::cout << "-----------Connect to " << db_name << " result-----------" << std::endl;
        std::cout << connect_result << std::endl;
        std::cout << "-----------Connect to " << db_name << " result-----------" << std::endl;
        std::cout.flush();

        // handle sql
        while(true)
        {
            std::string sql;

            std::cin.sync();

            std::getline(std::cin, sql);

            sql.erase(0, sql.find_first_not_of(" \t\n\r\f\v"));
            sql.erase(sql.find_last_not_of(" \t\n\r\f\v") + 1);

            if (sql.empty()) {
                continue;
            }

            if (sql == "quit")
            {
                break;
            }

            assert(sql.length() < SQL_MAX_LENGTH);

            // serialize sql
            int sql_length = sql.length();
            memcpy(msg.msg_data, &sql_length, sizeof(int));
            memcpy(msg.msg_data + sizeof(int), sql.c_str(), sql_length);

            // send sql to server
            msg.msg_type = communicate_queue_id; // use special queue
            msgsnd(worker_msg_key, &msg, sizeof(int) + sql_length, 0);

            // receive sql handle result
            msgrcv(worker_msg_key, &msg, WORK_MSG_DATA_LENGTH, communicate_queue_id + 1, 0); // use special receive queue

            // check execute result
            SqlResponse response;
            response.Deserialize(msg.msg_data);

            // show result
            switch (response.sql_state)
            {
            case UNSUBMIT:
            {
                std::cout << "UNSUBMIT" << std::endl;
                break;
            }
            case PARSE_ERROR:
            {
                std::cout << "PARSE_ERROR" << std::endl;
                break;
            }
            case SUCCESS:
            {
                if (response.information.size() > 0)
                {
                    std::cout << response.information << std::endl;
                }
                else
                {
                    std::cout << "SUCCESS" << std::endl;
                }
                break;
            }
            case FAILURE:
            {
                std::cout << "FAILURE" << std::endl;
                if (response.information.size() > 0)
                {
                    std::cout << " " << response.information << std::endl;
                }
                break;
            }
            default:
                break;
            }
        }
    }
};

class ClientManager
{

private:
    long used_queue_id;

public:

    ~ClientManager()
    {
        msgctl(connector_msg_key, IPC_RMID, nullptr);
        msgctl(worker_msg_key, IPC_RMID, nullptr);
    }

    void Run()
    {
        used_queue_id = 0;
        while(true)
        {
            std::string command;
            getline(cin, command);

            if (command == "quit")
            {
                break;
            }

            if (command == "root")
            {
                BuildRootClient();
                continue;
            }

            if (command == "user")
            {
                BuildClient();
                continue;
            }
        }
    }

    void BuildRootClient()
    {
        Client* cli = new Client();
        cli->RunClient("192.0.0.0", 0, 0, "base_db", queue_list_range[used_queue_id++]);
        delete cli;
    }

    void BuildDefaultUserClient()
    {
        Client* cli = new Client();
        cli->RunClient("192.0.0.0", 0, 0, "first_db", queue_list_range[used_queue_id++]);
        delete cli;
    }

    // build new client
    void BuildClient()
    {
        std::string ip;
        int port;
        int connect_identity_type;
        std::string db_name;
        long communicate_queue_id;

        std::cout << "Enter IP: ";
        std::cin >> ip;

        std::cout << "Enter port: ";
        std::cin >> port;

        std::cout << "Enter connect identity type: ";
        std::cin >> connect_identity_type;

        std::cout << "Enter database name: ";
        std::cin >> db_name;

        communicate_queue_id = queue_list_range[used_queue_id++];

        Client* cli = new Client();
        cli->RunClient(ip, port, connect_identity_type, db_name, communicate_queue_id);
        delete cli;
    }

};

}

#endif // VDBMS_BENCH_CLIENT_H_