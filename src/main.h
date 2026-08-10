#pragma once

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "misc/cpp/imgui_stdlib.h"
#include "misc/cpp/imgui_stdlib.cpp"
#include "account_system.h" 

#pragma comment(lib, "d3d11.lib")

#define EMPTY_INPUT 50
#define HAVE_SPECIAL_CHARACTERS 100
#define CORRECT_INPUT 15


BankSystem Ego_bank("data.db", "Ego_bank");
static std::unique_ptr<Account> user = NULL;
static std::multimap<float, TypeOperation> user_history;


void PrintHistory(const std::multimap<float, TypeOperation>& history, bool max, int limit);
[[nodiscard]] int CheckCoorectInput(const std::string& name, const std::string& surname, const std::string& phone_number, const std::string& login, const std::string& password);
[[nodiscard]] bool FindSpecChar(const std::string& str);