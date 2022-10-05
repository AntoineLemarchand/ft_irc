#include <vector>
#include "../inc/IRC_Logic.hpp"

IRC_Logic::IRC_Logic(const std::string &password) : _password(password) {
}

IRC_Logic::IRC_Logic(const IRC_Logic &other) {
	(void) other;
}

IRC_Logic &IRC_Logic::operator=(const IRC_Logic &) {
	return *this;
}

IRC_Logic::~IRC_Logic() {

}

void IRC_Logic::removeMessageTermination(std::string *message) {
	message->erase(message->size() - 3, 3);
}

void IRC_Logic::cleanupName(std::string *name) {
	name->erase(0, 1);
	name->erase(name->size() - 1, 1);
}

bool IRC_Logic::isUserMessage(const std::vector<std::string> &splitMessageVector) const {
	return splitMessageVector.size() >= 5 && splitMessageVector[0] == "USER";
}

std::string IRC_Logic::processInput(int fd, const std::string &input, const std::string &hostName) {
	std::vector<std::string> splitMessageVector;
	IRC_User *currentUser = getUserByFd(fd);
	std::string	result;

	if (currentUser == NULL) {
		_users.push_back(IRC_User(fd, hostName));
		currentUser = &_users.back();
	}
	currentUser->receivedCharacters += input;
	while (currentUser->receivedCharacters.find("\r\n") != std::string::npos) {
		splitMessageVector = extractFirstMessage(currentUser);
		result += processIncomingMessage(splitMessageVector, currentUser);
	}
	return result;
}

std::string IRC_Logic::processIncomingMessage(const std::vector<std::string> &splitMessageVector, IRC_User *user) {
	std::string	result;

	if (isAuthenticationMessage(splitMessageVector))
		result += processPassMessage(user, splitMessageVector);
	else if (user->isAuthenticated == false)
		return result; // todo: handle error here
	else if (isNickMessage(splitMessageVector))
		result += processNickMessage(user, splitMessageVector);
	else if (isUserMessage(splitMessageVector))
		result += processUserMessage(user, splitMessageVector);
	return result;
}

//todo: inline function
bool IRC_Logic::isAuthenticationMessage(const std::vector<std::string> &splitMessageVector) const {
	return splitMessageVector[0] == "PASS";
}

std::string IRC_Logic::processPassMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
	if (splitMessageVector.size() == 2 && splitMessageVector[1] == _password) {
		user->isAuthenticated = true;
		return "";
	}
	return ""; // todo: handle error here
}

std::string IRC_Logic::processUserMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
	if (!IRC_User::isValidCreationString(splitMessageVector))
		return ""; // todo
	user->name = splitMessageVector[1];
	user->fullName = buildFullName(splitMessageVector);
	if (userIsRegistered(user)) {
		return rplWelcome(*user) + rplYourHost() + rplCreated() + rplMyInfo() + rplISupport();
	}
	return ""; // todo: handle error code here
}

std::string IRC_Logic::processNickMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
	std::string result;

	if (splitMessageVector.size() == 2 && !isNickAlreadyPresent(splitMessageVector[1]))
		user->nick = splitMessageVector[1];
	return ""; // todo
}

//todo: inline function and move check to processNickMessage
bool IRC_Logic::isNickMessage(const std::vector<std::string> &splitMessageVector) const {
	return splitMessageVector[0] == "NICK"
		   && IRC_User::isNickValid(splitMessageVector[1]);
}

bool IRC_Logic::isNickAlreadyPresent(const std::string &nick) {
	for (std::vector<IRC_User>::iterator user = _users.begin(); user != _users.end(); ++user)
		if (stringToLower(user->nick) == stringToLower(nick))
			return true;
	return false;
}

std::string IRC_Logic::stringToLower(const std::string &input) const {
	std::string result;

	for (std::string::const_iterator it = input.begin();
		 it != input.end(); ++it) {
		result += tolower(*it);
	}
	return result;
}

std::string IRC_Logic::buildFullName(const std::vector<std::string> &splitMessageVector) {
	std::string name;

	for (size_t i = 4; i < splitMessageVector.size(); i++) {
		name += splitMessageVector[i] + " ";
	}
	cleanupName(&name);
	return name;
}

std::vector<std::string> IRC_Logic::extractFirstMessage(IRC_User *user) {
	size_t pos;
	std::string message;
	std::vector<std::string> result;

	message = user->receivedCharacters.substr(0, user->receivedCharacters.find("\r\n"));
	while ((pos = message.find(" ")) != std::string::npos) {
		result.push_back(message.substr(0, pos));
		message.erase(0, pos + 1);
	}
	result.push_back(message);
	user->receivedCharacters.erase(0, user->receivedCharacters.find("\r\n") + 2);
	return result;
}

std::vector<IRC_User> IRC_Logic::getRegisteredUsers() {
	return _users;
}

std::vector<int> IRC_Logic::getReturnCodes() {
	return _returnCodes;
}

IRC_User *IRC_Logic::getUserByFd(const int &fd) {
	for (std::vector<IRC_User>::iterator it = _users.begin();
		 it != _users.end(); it++) {
		if (it->fd == fd)
			return &(*it);
	}
	return NULL;
}

bool IRC_Logic::userIsRegistered(IRC_User *user) {
	if (!user || user->nick == "" || user->name == "" || user->fullName == "")
		return false;
	return true;
}
