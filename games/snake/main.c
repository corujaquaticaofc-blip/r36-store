#include <SDL2/SDL.h>
#include <stdlib.h>
#include <time.h>
#define W 640
#define H 480
#define S 20
#define GW (W/S)
#define GH (H/S)
int main(void){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK)<0)return 1;
 SDL_Window*w=SDL_CreateWindow("R36 STORE - SNAKE",0,0,W,H,SDL_WINDOW_FULLSCREEN);if(!w){SDL_Quit();return 1;}SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);
 SDL_GameController*g=NULL;for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){g=SDL_GameControllerOpen(i);if(g)break;}
 int x[GW*GH],y[GW*GH],n=4,dx=1,dy=0,foodx=18,foody=12,over=0,quit=0;for(int i=0;i<n;i++){x[i]=10-i;y[i]=12;}srand((unsigned)time(NULL));Uint32 tick=SDL_GetTicks();
 while(!quit){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)quit=1;if(e.type==SDL_CONTROLLERBUTTONDOWN&&e.cbutton.button==SDL_CONTROLLER_BUTTON_B)quit=1;if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE)quit=1;}
 const Uint8*k=SDL_GetKeyboardState(NULL);int l=(g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_DPAD_LEFT))||k[SDL_SCANCODE_LEFT],rr=(g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_DPAD_RIGHT))||k[SDL_SCANCODE_RIGHT],u=(g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_DPAD_UP))||k[SDL_SCANCODE_UP],d=(g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_DPAD_DOWN))||k[SDL_SCANCODE_DOWN];
 if(l&&dx!=1){dx=-1;dy=0;}if(rr&&dx!=-1){dx=1;dy=0;}if(u&&dy!=1){dx=0;dy=-1;}if(d&&dy!=-1){dx=0;dy=1;}
 Uint32 now=SDL_GetTicks();if(now-tick>115&&!over){tick=now;int nx=x[0]+dx,ny=y[0]+dy;if(nx<0||nx>=GW||ny<0||ny>=GH)over=1;for(int i=0;i<n-1;i++)if(x[i]==nx&&y[i]==ny)over=1;if(!over){for(int i=n;i>0;i--){x[i]=x[i-1];y[i]=y[i-1];}x[0]=nx;y[0]=ny;if(nx==foodx&&ny==foody){n++;foodx=rand()%GW;foody=rand()%GH;}else n--;}}
 SDL_SetRenderDrawColor(r,8,15,20,255);SDL_RenderClear(r);SDL_Rect f={foodx*S,foody*S,S-2,S-2};SDL_SetRenderDrawColor(r,240,90,70,255);SDL_RenderFillRect(r,&f);
 SDL_SetRenderDrawColor(r,90,230,150,255);for(int i=0;i<n;i++){SDL_Rect b={x[i]*S,y[i]*S,S-2,S-2};SDL_RenderFillRect(r,&b);}SDL_RenderPresent(r);SDL_Delay(8);
 if(over&&((g&&SDL_GameControllerGetButton(g,SDL_CONTROLLER_BUTTON_A))||k[SDL_SCANCODE_RETURN])){n=4;for(int i=0;i<n;i++){x[i]=10-i;y[i]=12;}dx=1;dy=0;over=0;}
 }
 if(g)SDL_GameControllerClose(g);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}