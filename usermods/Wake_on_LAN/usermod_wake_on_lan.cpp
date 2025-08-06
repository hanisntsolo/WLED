#include "usermod_wake_on_lan.h"

#ifdef USERMOD_WAKE_ON_LAN

// Global usermod instance
static UsermodWakeOnLAN usermod_wake_on_lan;

// Register the usermod
REGISTER_USERMOD(usermod_wake_on_lan);

#endif
