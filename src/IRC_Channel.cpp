#include <iostream>
#include "../inc/IRC_Channel.hpp"

IRC_Channel::IRC_Channel(const std::string &channelName) :
	name(getChannelName(channelName)) {
	}

bool IRC_Channel::operator==(const IRC_Channel &rhs) const {
	return name == rhs.name;
}

bool IRC_Channel::operator!=(const IRC_Channel &rhs) const {
	return !(rhs == *this);
}

bool IRC_Channel::isUserInChannel(const IRC_User &user) const {
	for (std::vector<IRC_User>::const_iterator it = members.begin();
			it != members.end(); ++it)
		if (user.nick == it->nick)
			return true;
	return false;
}

std::string IRC_Channel::getChannelName(const std::string &name) {
	std::string result = stringToLower(name);
	std::string prefix = "#&+!";
    if (prefix.find(result[0]) != std::string::npos)
		return result.substr(1, 49);
	return result.substr(0, 49);
}

void IRC_Channel::appendJoinMessages(
		std::queue<IRC_Message> *messageQueue, const IRC_User &newMember) {
	appendJoinNotificationToAllMembers(messageQueue, newMember);
	appendJoinReplyToNewMember(messageQueue, newMember);
}

void IRC_Channel::appendJoinReplyToNewMember(
		std::queue<IRC_Message> *messageQueue, const IRC_User &newMember) {
	IRC_Message reply(newMember.fd, "", "");

	reply.content += std::string(RPL_NAMREPLY) + " " + newMember.nick +
		" = " + name + " :";
	for (std::vector<IRC_User>::reverse_iterator
			it = members.rbegin(); it != members.rend(); ++it)
		reply.content += " " + it->nick;
	reply.content += "\r\n";
	reply.content += std::string(RPL_ENDOFNAMES) + " "
		+ newMember.nick + " #" + name +
		" :End of NAMES list.\r\n";
	messageQueue->push(reply);
}

void IRC_Channel::appendJoinNotificationToAllMembers(
		std::queue<IRC_Message> *messageQueue, const IRC_User &newMember) {
	std::string notifyText;
	std::queue<int> recipients;

	notifyText = " JOIN :#" + name + "\r\n";
	for (std::vector<IRC_User>::reverse_iterator
			it = members.rbegin(); it != members.rend(); ++it)
		recipients.push(it->fd);
	IRC_Message notify(recipients, notifyText, &newMember);
	messageQueue->push(notify);
}

std::queue<int> IRC_Channel::getRecipientFdsForSender(
		const IRC_User &user) const {
	std::queue<int> recipients;

	for (std::vector<IRC_User>::const_iterator
			it = members.begin();
			it != members.end(); it++)
		if (user.nick != it->nick) {
			recipients.push((it)->fd);
		}
	return recipients;
}

void IRC_Channel::removeMember(
		const IRC_User &user,
		std::queue<IRC_Message> *messageQueue,
		const std::string &reason) {
	if (!isUserInChannel(user))
		return;
	std::queue<int> recipients = getRecipientFdsForSender(user);
	messageQueue->push(
			IRC_Message(recipients, reason, &user));
	for (std::vector<IRC_User>::iterator
			it = members.begin(); it != members.end(); ++it)
		if (it->nick == user.nick)
			it = members.erase(it) - 1;
}
