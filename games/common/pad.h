#ifndef R36_PAD_H
#define R36_PAD_H
#include <SDL2/SDL.h>
typedef struct { SDL_GameController *gc; SDL_Joystick *joy; } R36Pad;
static void r36_pad_open(R36Pad *p) {
 p->gc=NULL; p->joy=NULL;
 for(int i=0;i<SDL_NumJoysticks();i++) {
  if(SDL_IsGameController(i)) { p->gc=SDL_GameControllerOpen(i); if(p->gc) { p->joy=SDL_GameControllerGetJoystick(p->gc); break; } }
 }
 if(!p->joy && SDL_NumJoysticks()>0) p->joy=SDL_JoystickOpen(0);
}
static int r36_axis(R36Pad *p,int axis) {
 if(p->gc) return SDL_GameControllerGetAxis(p->gc,axis==0?SDL_CONTROLLER_AXIS_LEFTX:SDL_CONTROLLER_AXIS_LEFTY);
 if(p->joy && SDL_JoystickNumAxes(p->joy)>axis) return SDL_JoystickGetAxis(p->joy,axis);
 return 0;
}
static int r36_hat(R36Pad *p,int dir) {
 if(p->gc) return SDL_GameControllerGetButton(p->gc,dir==0?SDL_CONTROLLER_BUTTON_DPAD_LEFT:dir==1?SDL_CONTROLLER_BUTTON_DPAD_RIGHT:dir==2?SDL_CONTROLLER_BUTTON_DPAD_UP:SDL_CONTROLLER_BUTTON_DPAD_DOWN);
 if(p->joy && SDL_JoystickNumHats(p->joy)>0) { Uint8 h=SDL_JoystickGetHat(p->joy,0); return dir==0?(h&SDL_HAT_LEFT):dir==1?(h&SDL_HAT_RIGHT):dir==2?(h&SDL_HAT_UP):(h&SDL_HAT_DOWN); }
 return 0;
}
static int r36_dir(R36Pad *p,int dir) {
 int axis=r36_axis(p,dir<2?0:1);
 return r36_hat(p,dir) || (dir==0?axis<-6000:dir==1?axis>6000:dir==2?axis<-6000:axis>6000);
}
static int r36_button(R36Pad *p,int b) {
 if(p->gc) return SDL_GameControllerGetButton(p->gc,b==0?SDL_CONTROLLER_BUTTON_A:b==1?SDL_CONTROLLER_BUTTON_B:SDL_CONTROLLER_BUTTON_START);
 return p->joy && SDL_JoystickNumButtons(p->joy)>b ? SDL_JoystickGetButton(p->joy,b):0;
}
static void r36_pad_close(R36Pad *p) { if(p->gc) SDL_GameControllerClose(p->gc); else if(p->joy) SDL_JoystickClose(p->joy); p->gc=NULL;p->joy=NULL; }
#endif
