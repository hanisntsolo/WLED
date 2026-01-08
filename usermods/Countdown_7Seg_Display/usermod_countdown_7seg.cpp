#include "usermod_countdown_7seg.h"

#ifdef USERMOD_COUNTDOWN_7SEG

// Global usermod instance
static UsermodCountdown7Seg usermod_countdown_7seg;

// Register the usermod with WLED
REGISTER_USERMOD(usermod_countdown_7seg);

#endif
