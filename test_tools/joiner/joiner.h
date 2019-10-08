#pragma once

#include <string>
#include <iostream>

std::ifstream get_stream(const std::string& file);
void join(std::string const& decisions_file_name, std::string const& outcomes_file_name);
