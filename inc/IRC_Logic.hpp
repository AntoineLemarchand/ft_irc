#ifndef INC_IRC_LOGIC_HPP_
#define INC_IRC_LOGIC_HPP_
#include <string>
#include <vector>
#include <queue>
#include "./IRC_User.hpp"
#include "./return_code.hpp"
#include "./utils.hpp"
#include "IRC_Message.hpp"
#include "IRC_Channel.hpp"

class IRC_Logic {
private:
    std::string _password;
    std::vector<IRC_User> _users;
	std::vector<IRC_Channel> _channels;
	std::vector<IRC_User> _prevUsers;
	std::queue<IRC_Message> _messageQueue;

	IRC_Logic();
public:

    explicit IRC_Logic(const std::string &password);
    IRC_Logic(const IRC_Logic &other);
    IRC_Logic &operator=(const IRC_Logic &);
    ~IRC_Logic();
	std::string processInput(int fd, const std::string &input);
	IRC_User::UserIterator getUserByFd(const int &fd);
	IRC_User::UserIterator getUserByNick(const std::string &nick);
	std::queue<IRC_Message> &getMessageQueue();
	void disconnectUser( int fd );
	std::vector<IRC_User>& getRegisteredUsers();
	std::vector<IRC_Channel>& getChannels();

private:
    void cleanupName(std::string *name);
	void removeMessageTermination(std::string *message);
    std::vector<std::string> extractFirstMessage(IRC_User *user);
	bool isNickAlreadyPresent(const std::string &nick);
	bool isUserRegistered(IRC_User* user);
	std::string welcomeNewUser(IRC_User *user);
	std::string processIncomingMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string processNickMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string processUserMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string processPassMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string processPingMessage(const std::vector<std::string> &splitMessageVector);
	std::string processPrivMsgMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string processWhoIsMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string processWhoWasMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::string generateWhoWasMessage(const std::vector<std::string> &splitMessageVector) const;
	std::string processJoinMessage(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	IRC_Channel::ChannelIterator getChannelByName(const std::string &name);
	std::queue<int> messageToChannel(IRC_User *user, const std::vector<std::string> &splitMessageVector);
	std::queue<int> messageToSingleUser(const std::vector<std::string> &splitMessageVector);
};
#endif //INC_IRC_LOGIC_HPP_
