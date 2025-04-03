#ifndef USER_H
#define USER_H

#include <string>

using namespace std;

class User {

public:
	User();

	string getUserId() { return this->userId; }
	string getUserPassword() { return this->userPassword; }
	string getUserPhone() { return this->userPhone; }
	string getUserName() { return this->userName; }
	string getUserEmail() { return this->userEmail; }
	string getUserAvatar() { return this->userAvatar; }
	string getUserStatus() { return this->userStatus; }
	string getUserCreated() { return this->userCreated; }

	void setUserId(string id) { this->userId = id; return; }
	void setUserPassword(string password) { this->userPassword = password;  return; }
	void setUserPhone(string phone) { this->userPhone = phone;  return; }
	void setUserName(string name) { this->userName = name; return; }
	void setUserEmail(string email) { this->userEmail = email;  return; }
	void setUserAvatar(string avatar) { this->userAvatar = avatar;  return;}
	void setUserStatus(string status) { this->userStatus = status;  return; }
	void setUserCreated(string created) { this->userCreated = created;  return; }

private:

	string userId;
	string userPassword;
	string userPhone;
	string userName;
	string userEmail;
	string userAvatar;
	string userStatus;
	string userCreated;
};

#endif