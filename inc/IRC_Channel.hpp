#ifndef INC_IRC_CHANNEL_HPP_
#define INC_IRC_CHANNEL_HPP_
#include <string>
#include "utils.hpp"

struct IRC_Channel{
	std::string name;

	IRC_Channel(const std::string& channelName);
	bool operator==(const IRC_Channel &rhs) const;
	bool operator!=(const IRC_Channel &rhs) const;

private:
	IRC_Channel();
};



#endif // INC_IRC_CHANNEL_HPP_
