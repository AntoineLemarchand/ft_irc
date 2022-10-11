#include <vector>
#include "../inc/IRC_Logic.hpp"

std::string concatenateContentFromIndex(int startIndex, const std::vector<std::string> &splitMessageVector);

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

std::string IRC_Logic::processInput(int fd, const std::string &input) {
    std::vector<std::string> splitMessageVector;
    IRC_User::UserIterator currentUser = getUserByFd(fd);
    std::string result;

    if (currentUser == _users.end()) {
        _users.push_back(IRC_User(fd));
        currentUser = _users.end() - 1;
    }
    currentUser->receivedCharacters += input;
    while (currentUser->receivedCharacters.find("\r\n") != std::string::npos) {
        splitMessageVector = extractFirstMessage(&(*currentUser));
        processIncomingMessage(&(*currentUser), splitMessageVector);
    }
    result.swap(_returnMessage);
    return result;
}

void IRC_Logic::processIncomingMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    if (splitMessageVector[0] == "PASS")
        processPassMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "CAP") {}
    else if (splitMessageVector[0] == "PING")
        processPingMessage(splitMessageVector);
    else if (user->isAuthenticated == false)
        _returnMessage += generateResponse(ERR_CONNECTWITHOUTPWD,
                                           "This server is password protected. Have you forgotten to send PASS?");
    else if (splitMessageVector[0] == "MODE")
        processModeMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "NICK")
        processNickMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "USER")
        processUserMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "PRIVMSG")
        processPrivMsgMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "WHOIS")
        processWhoIsMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "WHOWAS")
        processWhoWasMessage(user, splitMessageVector);
    else if (splitMessageVector[0] == "JOIN")
        processJoinMessage(user, splitMessageVector);
}

void IRC_Logic::processModeMessage(const IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    _returnMessage += ":" + user->nick + " " + splitMessageVector[0] + " " + splitMessageVector[1] + " :" + splitMessageVector[2] + "\r\n";
}

void IRC_Logic::processPrivMsgMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    std::queue<int> recipients;

    if (splitMessageVector.size() < 3)
        _returnMessage += generateResponse(ERR_NOTEXTTOSEND, "Missing message text");
    else {
        if (splitMessageVector[1][0] == '#')
            recipients = fetchChannelRecipients(user, splitMessageVector);
        else
            recipients = fetchSingleRecipient(splitMessageVector);
    }
    if (!recipients.empty())
        _messageQueue.push(IRC_Message(recipients, splitMessageVector, user));
}

bool IRC_Logic::isUserInChannel(IRC_User *user, std::vector<IRC_Channel>::iterator &channelCandidate) const {
    for (std::vector<int>::iterator it = channelCandidate->membersFd.begin();
         it != channelCandidate->membersFd.end(); ++it)
        if (*it == user->fd)
            return true;
    return false;
}

std::queue<int> IRC_Logic::fetchSingleRecipient(const std::vector<std::string> &splitMessageVector) {
    IRC_User::UserIterator recipient;
    std::queue<int> recipients;

    recipient = getUserByNick(splitMessageVector[1]);
    if (recipient != _users.end())
        recipients.push(recipient->fd);
    else
        _returnMessage += generateResponse(ERR_NOSUCHNICK, "Get an addressbook! This nick does not exist!");
    return recipients;
}

std::queue<int> IRC_Logic::fetchChannelRecipients(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    std::queue<int> recipients;
    std::vector<IRC_Channel>::iterator channelCandidate = getChannelByName(splitMessageVector[1]);

    if (channelCandidate == _channels.end())
        _returnMessage += generateResponse(ERR_NOSUCHCHANNEL,
                                           "Get an addressbook! This channel does not exist!");
    else if (!isUserInChannel(user, channelCandidate))
        _returnMessage += generateResponse(ERR_CANNOTSENDTOCHAN, "You're not part of this channel");
    else {
        for (std::vector<int>::iterator it = channelCandidate->membersFd.begin();
             it != channelCandidate->membersFd.end(); it++)
            if (user->fd != *it)
                recipients.push(*it);
    }
    return recipients;
}

void IRC_Logic::processPassMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    if (user->isAuthenticated)
        _returnMessage += generateResponse(ERR_ALREADYREGISTERED, "Do you really like to type the pw that much?");
    else if (splitMessageVector.size() == 2 && splitMessageVector[1] == _password)
        user->isAuthenticated = true;
    else if (splitMessageVector.size() == 1)
        _returnMessage += generateResponse(ERR_NEEDMOREPARAMS, "You did not enter a password");
    else
        _returnMessage += generateResponse(ERR_PASSWDMISMATCH, "You did not enter the correct password");
}

void IRC_Logic::processPingMessage(const std::vector<std::string> &splitMessageVector) {
    if (splitMessageVector.size() == 1)
        _returnMessage += generateResponse(ERR_NEEDMOREPARAMS, "Ping must have a token");
    else
        _returnMessage += ("PONG " + splitMessageVector[1] + "\r\n");
}

void IRC_Logic::processNickMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    if (splitMessageVector.size() == 1)
        _returnMessage += generateResponse(ERR_NONICKNAMEGIVEN, "You should consider having a nickname");
    else if (IRC_User::isNickValid(splitMessageVector[1]) == false || splitMessageVector.size() != 2)
        _returnMessage += generateResponse(ERR_ERRONEOUSNICK, "Sigh, think again. NO FORBIDDEN CHARACTERS!");
    else if (isNickAlreadyPresent(splitMessageVector[1]))
        _returnMessage += generateResponse(ERR_NICKNAMEINUSE,
                                           "nickname " + splitMessageVector[1] + " is already taken.");
    else {
        _prevUsers.push_back(*user);
        user->nick = splitMessageVector[1].substr(0, 9);
        _returnMessage += welcomeNewUser(user);
    }
}

void IRC_Logic::processUserMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    if (user->userName != "" || user->fullName != "")
        _returnMessage += generateResponse(ERR_ALREADYREGISTERED, "Call USER once. Not twice.");
    else if (splitMessageVector.size() < 5)
        _returnMessage += generateResponse(ERR_NEEDMOREPARAMS,
                                           "You need to send username, username, unused and fullname.");
    else {
        user->userName = splitMessageVector[1];
        user->fullName = IRC_User::buildFullName(splitMessageVector);
        _returnMessage += welcomeNewUser(user);
    }
}

std::string IRC_Logic::welcomeNewUser(IRC_User *user) {
    if (isUserRegistered(user)) {
        return generateResponse(RPL_WELCOME,
                                user->nick + " :Welcome to the land of fromage " + user->nick + "!~" + user->userName +
                                "@127.0.0.1")
               + generateResponse(RPL_YOURHOST, "This is a ft_irc server")
               + generateResponse(RPL_CREATED, "The server was probably created in 2022")
               + generateResponse(RPL_MYINFO, "This server has got channels, believe me")
               + generateResponse(RPL_ISUPPORT, "Moderate demands");
    }
    return "";
}

bool IRC_Logic::isNickAlreadyPresent(const std::string &nick) {
    for (std::vector<IRC_User>::iterator user = _users.begin(); user != _users.end(); ++user)
        if (stringToLower(user->nick) == stringToLower(nick))
            return true;
    return false;
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

std::vector<IRC_User> &IRC_Logic::getRegisteredUsers() {
    return _users;
}

std::vector<IRC_Channel> &IRC_Logic::getChannels() {
    return _channels;
}

std::queue<IRC_Message> &IRC_Logic::getMessageQueue() {
    return _messageQueue;
}

IRC_User::UserIterator IRC_Logic::getUserByFd(const int &fd) {
    return IRC_User::findUserByFdInVector(&_users, fd);
}

IRC_User::UserIterator IRC_Logic::getUserByNick(const std::string &nick) {
    return IRC_User::findUserByNickInVector(&_users, nick);
}

bool IRC_Logic::isUserRegistered(IRC_User *user) {
    if (!user || user->nick == "" || user->userName == "" || user->fullName == "")
        return false;
    return true;
}

// todo idle check
void IRC_Logic::processWhoIsMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    IRC_User::UserIterator result;
    (void) user;

    if (splitMessageVector.size() == 1) {
        _returnMessage += generateResponse(ERR_NONICKNAMEGIVEN, "Please provide a nickname");
        return;
    }
    result = getUserByNick(splitMessageVector[1]);
    if (result == _users.end())
        _returnMessage += generateResponse(ERR_NOSUCHNICK, "This user is not registered");
    else
        _returnMessage += generateResponse(RPL_WHOISUSER, result->toString());
}

void IRC_Logic::disconnectUser(int fd) {
    IRC_User::UserIterator userToBeDeleted = getUserByFd(fd);

	for (std::vector<IRC_Channel>::iterator channel = _channels.begin(); channel != _channels.end(); channel++) {
        removeMemberFromChannel(fd, channel);
    }
    _prevUsers.push_back(*userToBeDeleted);
    _users.erase(userToBeDeleted);
}

void IRC_Logic::removeMemberFromChannel(int fd, std::vector<IRC_Channel>::iterator &channel) const {
    for (std::vector<int>::iterator recipientFd = channel->membersFd.begin(); recipientFd != channel->membersFd.end(); recipientFd++) {
        if (*recipientFd == fd)
                recipientFd = channel->membersFd.erase(recipientFd) - 1;
    }
}

void IRC_Logic::processWhoWasMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    std::string result;
    (void) user;

    if (splitMessageVector.size() == 1) {
        _returnMessage += generateResponse(ERR_NONICKNAMEGIVEN, "Please provide a nickname");
        return ;
    }
    result = generateWhoWasMessage(splitMessageVector);
    if (result.empty())
        _returnMessage += generateResponse(ERR_WASNOSUCHNICK, "This user was never registered");
    else
        _returnMessage += result + RPL_ENDOFWHOWAS + "\r\n";
}

std::string
IRC_Logic::generateWhoWasMessage(const std::vector<std::string> &splitMessageVector) const {
    std::string result;

    for (std::vector<IRC_User>::const_reverse_iterator rit = _prevUsers.rbegin(); rit != _prevUsers.rend(); ++rit) {
        if (rit->nick == splitMessageVector[1])
            result += generateResponse(RPL_WHOWASUSER, rit->toString());
    }
    return result;
}

void IRC_Logic::processJoinMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector) {
    (void) user;
    if (splitMessageVector.size() == 1) {
        _returnMessage += generateResponse(ERR_NEEDMOREPARAMS, "Join requires <channel_name> and <password>");
        return;
    }
    IRC_Channel channelCandidate = IRC_Channel(splitMessageVector[1]);
    if (getChannelByName(channelCandidate.name) == _channels.end())
        _channels.push_back(channelCandidate);
    getChannelByName(channelCandidate.name)->membersFd.push_back(user->fd);
}

IRC_Channel::ChannelIterator IRC_Logic::getChannelByName(const std::string &name) {
    std::string nameWithoutPrefix = IRC_Channel::getChannelName(name);

    for (std::vector<IRC_Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        if (nameWithoutPrefix == it->name)
            return it;
    return _channels.end();
}
