#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#define W 640
#define H 480
static int down(SDL_GameController*g,SDL_GameControllerButton b){return g&&SDL_GameControllerGetButton(g,b);}
int main(void){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK)<0)return 1;
 SDL_Window*w=SDL_CreateWindow("R36 STORE - PONG",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_FULLSCREEN);
 if(!w){SDL_Quit();return 1;} SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);
 SDL_GameController*g=NULL;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){g=SDL_GameControllerOpen(i);if(g)break;}
 float py=H/2-35,cy=H/2-35,bx=W/2,by=H/2,vx=4,vy=3;int score=0,quit=0;Uint32 last=SDL_GetTicks();
 while(!quit){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)quit=1;if(e.type==SDL_CONTROLLERBUTTONDOWN&&(e.cbutton.button==SDL_CONTROLLER_BUTTON_B||e.cbutton.button==SDL_CONTROLLER_BUTTON_BACK))quit=1;if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE)quit=1;}
 const Uint8*k=SDL_GetKeyboardState(NULL);float a=g?SDL_GameControllerGetAxis(g,SDL_CONTROLLER_AXIS_LEFTY):0;int up=down(g,SDL_CONTROLLER_BUTTON_DPAD_UP)||a<-7000||k[SDL_SCANCODE_UP];int dn=down(g,SDL_CONTROLLER_BUTTON_DPAD_DOWN)||a>7000||k[SDL_SCANCODE_DOWN];
 if(up)py-=6;if(dn)py+=6;if(py<0)py=0;if(py>H-70)py=H-70;
 cy+=(by-(cy+35))*0.055f;if(cy<0)cy=0;if(cy>H-70)cy=H-70;
 Uint32 now=SDL_GetTicks();float dt=(now-last)/16.0f;last=now;if(dt>3)dt=3;bx+=vx*dt;by+=vy*dt;
 if(by<8||by>H-8)vy=-vy;if(bx<28&&by>py&&by<py+70){vx=abs((int)vx)+0.25f;vx=-vx;vy+=(by-(py+35))*0.06f;}
 if(bx>W-28&&by>cy&&by<cy+70)vx=-vx;
 if(bx<0){bx=W/2;by=H/2;vx=4;score++;}if(bx>W){bx=W/2;by=H/2;vx=-4;}
 SDL_SetRenderDrawColor(r,10,14,22,255);SDL_RenderClear(r);SDL_Rect p={12,(int)py,12,70},c={W-24,(int)cy,12,70},ball={(int)bx-7,(int)by-7,14,14};
 SDL_SetRenderDrawColor(r,235,240,255,255);SDL_RenderFillRect(r,&p);SDL_RenderFillRect(r,&c);SDL_RenderFillRect(r,&ball);SDL_RenderPresent(r);SDL_Delay(8);
 }
 if(g)SDL_GameControllerClose(g);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return score<0;
}