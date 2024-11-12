#ifndef GLOBALCONFIG_HPP
#define GLOBALCONFIG_HPP

#include "Singleton.hpp"

class GlobalConfig : public Singleton<GlobalConfig> {
public:
	inline int GetMaxFramesInFlight() const { return _MAX_FRAMES_IN_FLIGHT
		; }


private:
	const int _MAX_FRAMES_IN_FLIGHT = 3;
};



#endif // GLOBALCONFIG_HPP