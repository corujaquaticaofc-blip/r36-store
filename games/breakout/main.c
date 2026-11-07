#include <SDL2/SDL.h>
#include <stdlib.h>
#define W 640
#define H 480
int main(void){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK)<0)return 1;SDL_Window*w=SDL_CreateWindow("R36 STORE - BREAKOUT",0,0,W,H,SDL_WINDOW_FULLSCREEN);if(!w){SDL_Quit();return 1;}SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);
 SDL_GameController*g=NULL;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){g=SDL_GameControllerOpen(i);if(g)break;}
 SDL_Rect blocks[50];int alive[50];for(int i=0;i<50;i++){blocks[i]=(SDL_Rect){30+(i%10)*58,45+(i/10)*25,50,16};alive[i]=1;}
 float px=280,bx=320,by=390,vx=3,vy=-3;int started=0,quit=0,remaining=50;
 while(!quit){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)quit=1;if(e.type==SDL_CONTROLLERBUTTONDOWN&&e.cbutton.button==SDL_CONTROLLER_BUTTON_B)quit=1;if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE)quit=1;}
 const Uint8*k=SDL_GetKeyboardState(NULL);Sint16 ax=g?SDL_GameControllerGetAxis(g,SDL_CONTROLLER_AXIS_LEFTX):0;int left=(g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_DPAD_LEFT))||ax<-6000||k[SDL_SCANCODE_LEFT],right=(g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_DPAD_RIGHT))||ax>6000||k[SDL_SCANCODE_RIGHT];
 if(left)px-=6;if(right)px+=6;if(px<0)px=0;if(px>W-80)px=W-80;if(!started&&((g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_A))||k[SDL_SCANCODE_SPACE]))started=1;
 if(started){bx+=vx;by+=vy;if(bx<7||bx>W-7)vx=-vx;if(by<7)vy=-vy;if(by>H){started=0;bx=px+40;by=390;vx=3;vy=-3;}
 SDL_Rect paddle={(int)px,420,80,12};if(vy>0&&by>410&&by<440&&bx>px&&bx<px+80){vy=-abs((int)vy);vx+=(bx-(px+40))*0.05f;}
 for(int i=0;i<50;i++)if(alive[i]){SDL_Rect ball={(int)bx-6,(int)by-6,12,12};if(SDL_HasIntersection(&ball,&blocks[i])){alive[i]=0;remaining--;vy=-vy;}}
 }
 SDL_SetRenderDrawColor(r,10,14,24,255);SDL_RenderClear(r);SDL_SetRenderDrawColor(r,80,210,240,255);for(int i=0;i<50;i++)if(alive[i])SDL_RenderFillRect(r,&blocks[i]);SDL_Rect p={(int)px,420,80,12},b={(int)bx-6,(int)by-6,12,12};SDL_SetRenderDrawColor(r,240,240,255,255);SDL_RenderFillRect(r,&p);if(started)SDL_RenderFillRect(r,&b);SDL_RenderPresent(r);SDL_Delay(8);
 if(remaining==0){for(int i=0;i<50;i++)alive[i]=1;remaining=50;started=0;bx=px+40;by=390;}
 }
 if(g)SDL_GameControllerClose(g);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}