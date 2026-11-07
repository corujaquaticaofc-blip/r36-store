#include <SDL2/SDL.h>
#include "../common/pad.h"
#include <stdlib.h>
#include <time.h>
#define W 640
#define H 480
static int board[4][4];
static void spawn(void){int empty[16],n=0;for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(!board[y][x])empty[n++]=y*4+x;if(n){int p=empty[rand()%n];board[p/4][p%4]=(rand()%10==0)?4:2;}}
static void slide(int dir){for(int a=0;a<4;a++){int v[4]={0},n=0;for(int b=0;b<4;b++){int x=dir==0?b:dir==1?3-b:a,y=dir==2?b:dir==3?3-b:a;int z=dir<2?y:x;(void)z;int val=dir<2?board[a][x]:board[y][a];if(val)v[n++]=val;}for(int i=0;i<3;i++)if(v[i]&&v[i]==v[i+1]){v[i]*=2;v[i+1]=0;}int out[4]={0},k=0;for(int i=0;i<4;i++)if(v[i])out[k++]=v[i];for(int b=0;b<4;b++){int x=dir==0?b:dir==1?3-b:a,y=dir==2?b:dir==3?3-b:a;if(dir<2)board[a][x]=out[b];else board[y][a]=out[b];}}}
static const unsigned char digit[10][5]={{7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
static void draw_number(SDL_Renderer*r,int value,int cx,int cy,int scale){
 char str[16];SDL_snprintf(str,sizeof(str),"%d",value);int len=(int)SDL_strlen(str),width=len*4*scale-scale,x0=cx-width/2;
 SDL_SetRenderDrawColor(r,35,30,25,255);
 for(int n=0;n<len;n++){int d=str[n]-'0';for(int y=0;y<5;y++)for(int x=0;x<3;x++)if(digit[d][y]&(1<<(2-x))){SDL_Rect p={x0+(n*4+x)*scale,cy-2*scale+y*scale,scale,scale};SDL_RenderFillRect(r,&p);}}
}
int main(void){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_JOYSTICK)<0)return 1;
 SDL_Window*w=SDL_CreateWindow("R36 STORE - 2048",0,0,W,H,SDL_WINDOW_FULLSCREEN);
 if(!w){SDL_Quit();return 1;}
 SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED);
 R36Pad pad; r36_pad_open(&pad);
 srand((unsigned)time(NULL)); spawn(); spawn();
 int quit=0; Uint32 last=0;
 while(!quit){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)quit=1;if(e.type==SDL_JOYBUTTONDOWN&&e.jbutton.button==1)quit=1;if(e.type==SDL_JOYBUTTONDOWN&&e.jbutton.button==0){for(int y=0;y<4;y++)for(int x=0;x<4;x++)board[y][x]=0;spawn();spawn();}if(e.type==SDL_CONTROLLERBUTTONDOWN&&e.cbutton.button==SDL_CONTROLLER_BUTTON_B)quit=1;if(e.type==SDL_CONTROLLERBUTTONDOWN&&e.cbutton.button==SDL_CONTROLLER_BUTTON_A){for(int y=0;y<4;y++)for(int x=0;x<4;x++)board[y][x]=0;spawn();spawn();}if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE)quit=1;}
 Uint32 now=SDL_GetTicks();if(now-last>140){last=now;int dir=-1;const Uint8*k=SDL_GetKeyboardState(NULL);if(r36_dir(&pad,0)||k[SDL_SCANCODE_LEFT])dir=0;else if(r36_dir(&pad,1)||k[SDL_SCANCODE_RIGHT])dir=1;else if(r36_dir(&pad,2)||k[SDL_SCANCODE_UP])dir=2;else if(r36_dir(&pad,3)||k[SDL_SCANCODE_DOWN])dir=3;if(dir>=0){slide(dir);spawn();}}
 SDL_SetRenderDrawColor(r,22,25,32,255);SDL_RenderClear(r);for(int y=0;y<4;y++)for(int x=0;x<4;x++){SDL_Rect c={100+x*110,20+y*110,100,100};int v=board[y][x],shade=v?40+(v%180):55;if(shade>220)shade=220;SDL_SetRenderDrawColor(r,shade,shade/2+35,80,255);SDL_RenderFillRect(r,&c);if(v)draw_number(r,v,c.x+c.w/2,c.y+c.h/2,7);}SDL_RenderPresent(r);SDL_Delay(8);
 }
 r36_pad_close(&pad);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}