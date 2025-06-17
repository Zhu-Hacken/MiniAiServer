#pragma once
#include <iostream>
#include "config/configs.h"



void initAllModules(ServerConfig config, 
                    std::string& db_username, 
                    std::string& db_password, 
                    std::string& db_name, 
                    int db_port = 3306);