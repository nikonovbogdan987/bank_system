#include "account_system.h"

  
int randomize(int begin, int end)  {
	static thread_local std::random_device rd;
	static thread_local std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(begin, end);
	return distrib(gen);
}

Operation::Operation(int new_id, float new_sum, std::string new_phone, TypeOperation new_type, CategoryOperation new_category) : id(new_id), sum(new_sum), phone(new_phone), type(new_type),
																															category(new_category) {}


Account::Account() : login(), password(), phone_number(""), name(""), surname(), balance(0.0) {}

Account::Account(std::string new_login, std::string new_password, std::string phone_num, std::string new_name, std::string new_surname, float new_balance) : login(new_login), 
															password(new_password), phone_number(phone_num), name(new_name), surname(new_surname), balance(new_balance) {}

std::string Account::GetLogin() const {
	return login;
}
std::string Account::GetPassword() const {
	return password;
}
bool Account::CheckPassword(const std::string& pass) const {
	if (password == pass) {
		return true;
	}
	return false;
}
std::string Account::GetName() const {
	return name;
}
std::string Account::GetSurname() const {
	return surname;
}
float Account::GetBalance() const {
	return balance;
}


bool Account::SetLogin(const std::string& new_login) {
	login = new_login;
	return true;
}
bool Account::SetPassword(const std::string& new_password) {
	password = new_password;
	return true;
}
bool Account::SetName(const std::string& new_name) {
	name = new_name;
	return true;
}
bool Account::SetSurname(const std::string& new_surname) {
	surname = new_surname;
	return true;
}
bool Account::SetBalance(float amount) {
	balance = amount;
	return true;
}


BankSystem::BankSystem() : db(), bank_name("") {}

BankSystem::BankSystem(const std::string& name) : db(), bank_name(name) {}

BankSystem::BankSystem(const char* filename, const std::string& name) {
	int rc = sqlite3_open(filename, &db);
	sqlite3_stmt* stmt;
	const char* sql;
	const char* next_sql;

	if (rc != SQLITE_OK) {
		std::cout << "ERROR" << sqlite3_errmsg(db);
		sqlite3_close(db);
		return;
	}

	sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
	sql = "CREATE TABLE IF NOT EXISTS bank_user (phone TEXT PRIMARY KEY, name TEXT, surname TEXT, login TEXT, password TEXT, balance REAL);"
		"CREATE TABLE IF NOT EXISTS history_operation (id INTEGER PRIMARY KEY AUTOINCREMENT, sum REAL, phone TEXT, type INTEGER, category INTEGER, FOREIGN KEY(phone) REFERENCES bank_user(phone) ON DELETE CASCADE);";
	next_sql = sql;
	while (next_sql != NULL && next_sql[0] != '\0') {
		rc = sqlite3_prepare_v2(db, next_sql, -1, &stmt, &next_sql);
		if (rc != SQLITE_OK) {
			std::cout << "ERROR" << sqlite3_errmsg(db);
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			return;
		}
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
	

	bank_name = name;
}

BankSystem::~BankSystem() {
	sqlite3_close(db);
}


std::unique_ptr<Account> BankSystem::FindAccountByPhone(const std::string& phn_num) {
	const char* sql;
	int rc;
	Account user;
	sqlite3_stmt* stmt;

	sql = "SELECT * FROM bank_user WHERE phone = ?";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_text(stmt, 1, phn_num.c_str(), static_cast<int>(phn_num.length()), SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		user.phone_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		user.SetLogin(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
		user.SetPassword(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
		user.SetName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
		user.SetSurname(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
		user.SetBalance(static_cast<float>(sqlite3_column_double(stmt, 5)));
		sqlite3_finalize(stmt);
		return std::make_unique<Account>(user);
	}
	else {
		sqlite3_finalize(stmt);
		return nullptr;
	}


	
}

int BankSystem::EnterAccount(const std::string& phn_num, const std::string& lgn, const std::string& passd) {
	std::unique_ptr<Account> ptr = FindAccountByPhone(phn_num);
	if (ptr == nullptr) {
		return ACCOUNT_NOT_FOUND;
	}
	else {
		if ((ptr->GetLogin() == lgn) && (ptr->CheckPassword(passd))) {
			return OPERATION_OK;
		}
		else {
			return WRONG_DATA;
		}
	}
}

int BankSystem::TransferMoney(const std::string& from_phone, const std::string& to_phone, float amount) {
	std::unique_ptr<Account> ptr1 = FindAccountByPhone(from_phone);
	if (ptr1 == nullptr) [[unlikely]]  {
		return ACCOUNT_NOT_FOUND;
	}

	if ((ptr1->GetBalance() - amount) < 0) {
		return NOT_ENOUGH_BALANCE;
	}
	
	std::unique_ptr<Account> ptr2 = FindAccountByPhone(to_phone);
	if (ptr2 == nullptr) {
		return ACCOUNT_NOT_FOUND;
	}


	sqlite3_stmt* stmt;
	bool success = true;

	sqlite3_prepare_v2(db, "BEGIN", -1, &stmt, NULL);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	const char* sql1 = "UPDATE bank_user SET balance = balance - ? WHERE phone = ?;";
	if (sqlite3_prepare_v2(db, sql1, -1, &stmt, NULL) == SQLITE_OK) {
		sqlite3_bind_double(stmt, 1, static_cast<double>(amount));
		sqlite3_bind_text(stmt, 2, from_phone.c_str(), static_cast<int>(from_phone.length()), SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			success = false;
		}
		sqlite3_finalize(stmt);
	}
	else {
		success = false;
	}
			
	if (success) {
		const char* sql2 = "UPDATE bank_user SET balance = balance + ? WHERE phone = ?;";

		if (sqlite3_prepare_v2(db, sql2, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_double(stmt, 1, static_cast<double>(amount));
			sqlite3_bind_text(stmt, 2, to_phone.c_str(), static_cast<int>(to_phone.length()), SQLITE_TRANSIENT);
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				success = false;
			}
			sqlite3_finalize(stmt);
		}
		else {
			success = false;
		}
	} 
	
	if (success) {
		const char* sql_op1 = "INSERT INTO history_operation (sum, phone, type, category) VALUES (?, ?, ?, ?)";
		if (sqlite3_prepare_v2(db, sql_op1, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_double(stmt, 1, static_cast<double>(amount));
			sqlite3_bind_text(stmt, 2, from_phone.c_str(), static_cast<int>(from_phone.length()), SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt, 3, static_cast<int>(TypeOperation::transfer_out));
			sqlite3_bind_int(stmt, 4, static_cast<int>(CategoryOperation::other));
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				success = false;
			}
			sqlite3_finalize(stmt);
		}
		else {
			success = false;
		}
	}

	if (success) {
		const char* sql_op2 = "INSERT INTO history_operation (sum, phone, type, category) VALUES (?, ?, ?, ?)";
		if (sqlite3_prepare_v2(db, sql_op2, -1, &stmt, NULL) == SQLITE_OK) {
			sqlite3_bind_double(stmt, 1, static_cast<double>(amount));
			sqlite3_bind_text(stmt, 2, to_phone.c_str(), static_cast<int>(to_phone.length()), SQLITE_TRANSIENT);
			sqlite3_bind_int(stmt, 3, static_cast<int>(TypeOperation::transfer_in));
			sqlite3_bind_int(stmt, 4, static_cast<int>(CategoryOperation::other));
			if (sqlite3_step(stmt) != SQLITE_DONE) {
				success = false;
			}
			sqlite3_finalize(stmt);
		}
		else {
			success = false;
		}
	}

	
	if (success) [[likely]] {
		sqlite3_prepare_v2(db, "COMMIT", -1, &stmt, NULL);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		return OPERATION_OK;
	}
	else [[unlikely]] {
		sqlite3_prepare_v2(db, "ROLLBACK", -1, &stmt, NULL);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		return SYSTEM_ERROR;
	}
			
}
		
void BankSystem::GetHistoryList(std::multimap<float, TypeOperation>& history, const std::string& phone_num, int lim) {
	sqlite3_stmt* stmt;
	history.clear();
	
	const char* sql = "SELECT sum, type FROM history_operation WHERE phone = ? LIMIT ?";
		
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) [[unlikely]] {
		std::cout << sqlite3_errmsg(db);
		return;
	}
		
	sqlite3_bind_text(stmt, 1, phone_num.c_str(), static_cast<int>(phone_num.length()), SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, lim);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		history.insert(std::make_pair(static_cast<float>(sqlite3_column_double(stmt, 0)), static_cast<TypeOperation>(sqlite3_column_int(stmt, 1))));
	}
	sqlite3_finalize(stmt);
	
}

int BankSystem::CreateAccount(Account&& new_acc) {
	if (FindAccountByPhone(new_acc.phone_number) != nullptr) {
		return ACCOUNT_HAS_CREATED;
	}
	sqlite3_stmt* stmt;
	const char* sql1 = "INSERT INTO bank_user VALUES (?, ?, ?, ?, ?, ?)";
		
	if (sqlite3_prepare_v2(db, sql1, -1, &stmt, NULL) != SQLITE_OK) {
		std::cout << "ERROR: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return SYSTEM_ERROR;
	}
	sqlite3_bind_text(stmt, 1, new_acc.phone_number.c_str(), static_cast<int>(new_acc.phone_number.length()), SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, new_acc.GetName().c_str(), static_cast<int>(new_acc.GetName().length()), SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, new_acc.GetSurname().c_str(), static_cast<int>(new_acc.GetSurname().length()), SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, new_acc.GetLogin().c_str(), static_cast<int>(new_acc.GetLogin().length()), SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, new_acc.GetPassword().c_str(), static_cast<int>(new_acc.GetPassword().length()), SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 6, static_cast<double>(new_acc.GetBalance()));
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
		
	return OPERATION_OK;
	
}

void BankSystem::PrintUserList() {
	sqlite3_stmt* stmt;
	const char* sql = "SELECT name, surname, phone FROM bank_user";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		std::cout << "ERROR: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		std::cout << "name: " << sqlite3_column_text(stmt, 0) << std::endl;
		std::cout << "surname: " << sqlite3_column_text(stmt, 1) << std::endl;
		std::cout << "phone: " << sqlite3_column_text(stmt, 2) << std::endl;
	}
	sqlite3_finalize(stmt);
}
