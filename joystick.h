#ifndef JOYSTICK
#define JOYSTICK

extern int js_init(void);
extern int js_open(char *device);
extern int js_close(void);
extern int js_input(unsigned int *punChannelData);

#endif
