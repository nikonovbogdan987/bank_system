#pragma once

#include <string>
#include <map>
#include <memory>
#include <random>
#include <iostream>
#include "sqlite3.h"
#define ACCOUNT_NOT_FOUND -1
#define NOT_ENOUGH_BALANCE 0
#define OPERATION_OK 1
#define SYSTEM_ERROR 10
#define WRONG_DATA 5
#define ACCOUNT_HAS_CREATED -10

[[maybe_unused]]  int randomize(int begin, int end);
static int count_sql = 1;

enum TypeOperation {
	transfer_in,
	transfer_out,
	purchase,
};

enum CategoryOperation {
	entertainment,
	education, 
	food, 
	utilities, 
	transport,
	medical,
	housing,
	other
};

class Operation {
public:
	int id;
	float sum;
	std::string phone;
	TypeOperation type;
	CategoryOperation category;

	Operation(int new_id, float new_sum, std::string new_phone, TypeOperation new_type, CategoryOperation new_category);
};

class Account {
private:
	std::string login;
	std::string password;
	std::string name;
	std::string surname;
	float balance = 0.0;

public:
	Account();


	Account(std::string new_login, std::string new_password, std::string phone_num, std::string new_name, std::string new_surname, float new_balance);

	std::string phone_number;
	std::string GetLogin() const;
	bool CheckPassword(const std::string& pass) const;
	std::string GetName() const;
	std::string GetSurname() const;
	float GetBalance() const;
	std::string GetPassword() const;

	bool SetLogin(const std::string& new_login);
	bool SetPassword(const std::string& new_password);
	bool SetName(const std::string& new_name);
	bool SetSurname(const std::string& new_surname);
	bool SetBalance(float amount);
};




class BankSystem {
private:
	sqlite3* db;
public:
	std::string bank_name;
	BankSystem();
	BankSystem(const std::string& name);
	BankSystem(const char* filename, const std::string& name);
	~BankSystem();

	std::unique_ptr<Account> FindAccountByPhone(const std::string& phn_num);
	[[nodiscard]]  int EnterAccount(const std::string& phn_num, const std::string& lgn, const std::string& passd);
	[[nodiscard]]  int TransferMoney(const std::string& from_phone, const std::string& to_phone, float amount);
	[[nodiscard]]  int CreateAccount(Account&& new_acc);
	void PrintUserList();
	void GetHistoryList(std::multimap<float, TypeOperation>& history, const std::string& phone_num, int lim = 15);
};