#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct _TTF_Font TTF_Font;

extern int TTF_Init(void);
extern void TTF_Quit(void);
extern TTF_Font *TTF_OpenFont(const char *, int);
extern void TTF_CloseFont(TTF_Font *);
extern SDL_Surface *TTF_RenderUTF8_Blended(
    TTF_Font *, const char *, SDL_Color
);

#define W 640
#define H 480
#define MAX_GAMES 2048

typedef struct {
    char id[48];
    char name[96];
    char category[32];
    char status[48];
    char port[160];
    char cover[64];
} Game;

static Game games[MAX_GAMES];
static int game_count = 0;

static Game library_games[MAX_GAMES];
static int library_count = 0;

static SDL_Window *window;
static SDL_Renderer *renderer;
static TTF_Font *font;
static TTF_Font *bold;

static const char *categories[] = {
    "INICIO", "JOGOS", "APLICATIVOS",
    "FERRAMENTAS", "BIBLIOTECA"
};

static const char *items[][4] = {
    {"Destaques", "Novidades", "Recomendados", "Explorar"},
    {"Catalogo de jogos", "Terror", "Puzzles", "Arcade"},
    {"Moonlight", "Multimedia", "Utilitarios", "Emuladores"},
    {"Sistema", "Rede", "Personalizacao", "Desempenho"},
    {"Instalados", "Downloads", "Atualizacoes", "Historico"}
};

static int category = 0;
static int selected = 0;
static int catalog_scroll = 0;
static bool details = false;
static bool in_catalog = false;
static bool exit_confirm = false;
static bool running = true;
static bool launch_requested = false;
static char launch_id[48] = "";
static char launch_error[128] = "";

static pid_t installer_pid = -1;
static int installer_pipe = -1;
static bool installing = false;
static bool install_finished = false;
static bool install_success = false;
static bool cancel_requested = false;

static char install_stage[32] = "";
static char install_message[128] = "";
static char pipe_buffer[4096];
static size_t pipe_used = 0;
static unsigned long long install_current = 0;
static unsigned long long install_total = 0;

bool selected_is_remote(void);
void refresh_installed_library(void);
void draw_install_screen(void);

static SDL_Color white = {240,245,255,255};
static SDL_Color muted = {135,153,180,255};
static SDL_Color blue = {35,125,255,255};
static SDL_Color panel = {17,29,50,255};

void rect(int x, int y, int w, int h, SDL_Color c) {
    SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);
    SDL_Rect r={x,y,w,h};
    SDL_RenderFillRect(renderer,&r);
}

void text(const char *s,int x,int y,TTF_Font *f,SDL_Color c) {
    SDL_Surface *surface=TTF_RenderUTF8_Blended(f,s,c);
    if(!surface) return;

    SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surface);
    if(texture) {
        SDL_Rect dst={x,y,surface->w,surface->h};
        SDL_RenderCopy(renderer,texture,NULL,&dst);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}


void load_catalog(void) {
    FILE *file=fopen("/roms/r36store/catalog/games.db","r");

    if(!file) {
        printf("Catalogo nao encontrado.\n");
        return;
    }

    char line[600];

    while(fgets(line,sizeof(line),file) && game_count<MAX_GAMES) {
        if(line[0]=='#' || line[0]=='\n' || line[0]=='\r')
            continue;

        Game g={0};

        int fields=sscanf(line,
            "%47[^|]|%95[^|]|%31[^|]|%47[^|]|%159[^|]|%63[^\r\n]",
            g.id,g.name,g.category,g.status,g.port,g.cover);

        if(fields==6) {
            games[game_count++]=g;
        }
    }

    fclose(file);

    FILE *remote=fopen("/roms/r36store/cache/remote-games.db","r");

    if(remote) {
        while(fgets(line,sizeof(line),remote) && game_count<MAX_GAMES) {
            if(line[0]=='#' || line[0]=='\n' || line[0]=='\r')
                continue;

            Game g={0};

            int fields=sscanf(line,
                "%47[^|]|%95[^|]|%31[^|]|%47[^|]|%159[^|]|%63[^\r\n]",
                g.id,g.name,g.category,g.status,g.port,g.cover);

            if(fields==6)
                games[game_count++]=g;
        }

        fclose(remote);
    }


    FILE *community=fopen("/roms/r36store/cache/community-games.db","r");
    if(community) {
        while(fgets(line,sizeof(line),community) && game_count<MAX_GAMES) {
            if(line[0]=='#' || line[0]=='\n' || line[0]=='\r')
                continue;
            Game g={0};
            int fields=sscanf(line,
                "%47[^|]|%95[^|]|%31[^|]|%47[^|]|%159[^|]|%63[^\r\n]",
                g.id,g.name,g.category,g.status,g.port,g.cover);
            if(fields==6)
                games[game_count++]=g;
        }
        fclose(community);
    }

    printf("R36 STORE: %d jogos carregados no total.\n",game_count);

}

void load_library(void) {
    library_count=0;
    FILE *file=fopen("/roms/r36store/cache/installed-games.db","r");

    if(!file) {
        printf("Biblioteca ainda nao disponivel.\n");
        return;
    }

    char line[600];

    while(fgets(line,sizeof(line),file) &&
          library_count<MAX_GAMES) {
        if(line[0]=='#' || line[0]=='\n' || line[0]=='\r')
            continue;

        Game g={0};

        int fields=sscanf(line,
            "%47[^|]|%95[^|]|%31[^|]|%47[^|]|%159[^|]|%63[^\r\n]",
            g.id,g.name,g.category,g.status,g.port,g.cover);

        if(fields==6)
            library_games[library_count++]=g;
    }

    fclose(file);

    printf("R36 STORE: %d itens na biblioteca.\n",library_count);
}

void sync_catalog_installed(void) {
    for(int i=0;i<game_count;i++) {
        bool store_remote =
            strcmp(games[i].port,"ONLINE")==0 ||
            strncmp(games[i].port,"/roms/.r36store/ports/",22)==0;
        if(!store_remote) continue;
        bool found=false;
        for(int j=0;j<library_count;j++) {
            if(strcmp(games[i].id,library_games[j].id)==0) {
                snprintf(games[i].status,sizeof(games[i].status),"%s",
                         library_games[j].status);
                snprintf(games[i].port,sizeof(games[i].port),"%s",
                         library_games[j].port);
                found=true;
                break;
            }
        }
        if(!found) {
            snprintf(games[i].status,sizeof(games[i].status),"DISPONIVEL");
            snprintf(games[i].port,sizeof(games[i].port),"ONLINE");
        }
    }
}

int item_count(void) {
    if(category==1) return game_count;
    if(category==4) return library_count;
    return 4;
}

void draw_game_catalog(void) {
    text("CATALOGO DE JOGOS",202,91,bold,white);

    if(game_count==0) {
        text("Nenhum jogo encontrado.",220,160,font,muted);
        return;
    }

    const int visible=6;
    int max_scroll=game_count-visible;
    if(max_scroll<0) max_scroll=0;
    if(catalog_scroll>max_scroll) catalog_scroll=max_scroll;
    if(catalog_scroll<0) catalog_scroll=0;

    rect(202,120,415,300,panel);

    for(int row=0;row<visible;row++) {
        int i=catalog_scroll+row;
        if(i>=game_count) break;
        int y=130+row*48;

        rect(202,y,415,42,(SDL_Color){24,39,64,255});

        if(selected==i) {
            SDL_SetRenderDrawColor(renderer,55,145,255,255);
            SDL_Rect outline={202,y,415,42};
            SDL_RenderDrawRect(renderer,&outline);
        }

        text(games[i].name,215,y+12,bold,white);

        SDL_Color status_color=muted;
        if(strstr(games[i].status,"REQUER"))
            status_color=(SDL_Color){255,174,75,255};
        else if(strstr(games[i].status,"PRESENTES") ||
                strstr(games[i].status,"INSTALADO"))
            status_color=(SDL_Color){90,210,145,255};
        else if(strstr(games[i].status,"VERIFICAR"))
            status_color=(SDL_Color){255,210,90,255};

        text(games[i].status,435,y+14,font,status_color);
    }

    char page[48];
    snprintf(page,sizeof(page),"JOGO %d/%d",selected+1,game_count);
    text(page,500,423,font,muted);
}

void draw_game_details(void) {
    if(selected<0 || selected>=game_count) return;

    Game *g=&games[selected];

    text("DETALHES DO JOGO",202,91,bold,white);

    rect(202,135,415,245,panel);

    text(g->name,225,160,bold,white);

    text("CATEGORIA",225,210,bold,muted);
    text(g->category,225,233,font,white);

    text("SITUACAO",225,270,bold,muted);
    text(g->status,225,293,font,blue);

    if(strcmp(g->port,"COMMUNITY")==0) {
        text("ORIGEM",225,310,bold,muted);
        text("PORTMASTER / COMUNIDADE",225,333,font,white);
    } else {
        text("CAPA",225,310,bold,muted);
        text(g->cover,225,333,font,white);
    }

    if(strcmp(g->port,"COMMUNITY")==0) {
        text("COMPATIBILIDADE NAO TESTADA",225,360,font,
             (SDL_Color){255,210,90,255});
        text("INSTALADOR EM ADAPTACAO",225,385,bold,blue);
        text("B PARA VOLTAR",425,385,bold,blue);
    } else if(selected_is_remote()) {
        text("A PARA INSTALAR",225,385,bold,blue);
        text("B PARA VOLTAR",425,385,bold,blue);
    } else {
        text("A PARA INICIAR",225,385,bold,blue);
        text("B PARA VOLTAR",425,385,bold,blue);
    }

    if(launch_error[0])
        text(launch_error,225,390,font,
             (SDL_Color){255,174,75,255});
}

void draw_library(void) {
    text("MINHA BIBLIOTECA",202,91,bold,white);

    if(library_count==0) {
        text("Nenhum item instalado pela loja.",220,160,font,muted);
        text("Os pacotes instalados aparecerao aqui.",220,190,font,muted);
        return;
    }

    for(int i=0;i<library_count;i++) {
        int y=130+i*48;

        rect(202,y,415,42,panel);

        if(selected==i) {
            SDL_SetRenderDrawColor(renderer,55,145,255,255);
            SDL_Rect outline={202,y,415,42};
            SDL_RenderDrawRect(renderer,&outline);
        }

        text(library_games[i].name,215,y+12,bold,white);
        text(library_games[i].status,435,y+14,font,
             (SDL_Color){90,210,145,255});
    }
}

void draw_library_details(void) {
    if(selected<0 || selected>=library_count) return;

    Game *g=&library_games[selected];

    text("DETALHES DO INSTALADO",202,91,bold,white);

    rect(202,135,415,245,panel);

    text(g->name,225,160,bold,white);

    text("CATEGORIA",225,210,bold,muted);
    text(g->category,225,233,font,white);

    text("VERSAO / SITUACAO",225,270,bold,muted);
    text(g->status,225,293,font,blue);

    text("LOCAL DE INSTALACAO",225,330,bold,muted);
    text(g->port,225,353,font,white);

    text("A PARA INICIAR",225,395,bold,blue);
    text("Y PARA DESINSTALAR",225,418,bold,(SDL_Color){255,174,75,255});
    text("B PARA VOLTAR",425,395,bold,blue);

    if(launch_error[0])
        text(launch_error,225,420,font,
             (SDL_Color){255,174,75,255});
}

void draw(void) {
    SDL_SetRenderDrawColor(renderer,9,14,27,255);
    SDL_RenderClear(renderer);

    rect(0,0,W,66,(SDL_Color){13,21,39,255});
    rect(22,15,36,36,blue);

    text("R",31,19,bold,white);
    text("R36 STORE",70,17,bold,white);
    text("SUA LOJA, SEU R36S",70,42,font,muted);

    rect(15,82,165,350,(SDL_Color){13,21,39,255});

    for(int i=0;i<5;i++) {
        int y=98+i*61;

        if(category==i)
            rect(23,y,149,47,(SDL_Color){23,72,137,255});

        text(categories[i],35,y+14,bold,
             category==i?white:muted);
    }

    if(installing || install_finished) {
        draw_install_screen();
    } else if(!in_catalog) {
        text("BEM-VINDO AO R36 STORE",202,105,bold,white);

        rect(202,150,415,205,panel);
        text(categories[category],225,178,bold,blue);
        text("Explore a loja usando o direcional.",225,225,font,white);
        text("CIMA / BAIXO  -  Escolher categoria",225,260,font,muted);
        text("DIREITA  -  Entrar na categoria",225,290,font,muted);
        text("B  -  Sair da loja",225,320,font,muted);
    } else if(category==0) {
        text("BEM-VINDO AO R36 STORE",202,91,bold,white);

        rect(202,130,415,125,panel);
        text("SUA LOJA NO R36S",224,151,bold,blue);
        text("Jogos, aplicativos e ferramentas",224,190,font,white);
        text("em um catalogo independente.",224,214,font,muted);

        rect(202,270,198,140,panel);
        text("DESTAQUES",218,290,bold,white);
        text("Espaco para novidades",218,330,font,muted);
        text("e conteudos em destaque.",218,353,font,muted);

        rect(410,270,207,140,panel);
        text("SUA BIBLIOTECA",426,290,bold,white);
        text("Os itens instalados",426,330,font,muted);
        text("pela loja aparecerao",426,353,font,muted);
        text("na categoria Biblioteca.",426,376,font,muted);
    } else if(category==1) {
        if(details)
            draw_game_details();
        else
            draw_game_catalog();
    } else if(category==4) {
        if(details)
            draw_library_details();
        else
            draw_library();
    } else {
        text(details?"DETALHES":"EXPLORE A LOJA",
             202,91,bold,white);

        if(details) {
            rect(202,135,415,220,panel);
            text(items[category][selected],225,160,bold,white);
            text("R36 STORE - CATALOGO",225,205,font,blue);
            text("Esta secao sera conectada ao",225,245,font,muted);
            text("catalogo de aplicativos e ferramentas.",225,268,font,muted);
            text("Novos recursos virao nas proximas",225,300,font,muted);
            text("versoes.",225,323,font,muted);
            text("Pressione B para voltar",225,380,font,white);
        } else {
            for(int i=0;i<4;i++) {
                int x=202+(i%2)*207;
                int y=135+(i/2)*135;

                rect(x,y,194,119,panel);
                rect(x,y,194,4,(SDL_Color){38,67,104,255});

                rect(x+14,y+17,38,38,(SDL_Color){25,55,94,255});
                text("R36",x+20,y+27,bold,blue);
                text(items[category][i],x+14,y+72,bold,white);
                text("ABRIR",x+14,y+96,font,muted);

                if(selected==i) {
                    SDL_SetRenderDrawColor(renderer,55,145,255,255);
                    SDL_Rect outline={x,y,194,119};
                    SDL_RenderDrawRect(renderer,&outline);
                }
            }
        }
    }

    rect(0,445,W,35,(SDL_Color){13,21,39,255});

    if(installing || install_finished) {
        text("B  CANCELAR / VOLTAR",18,455,font,white);
    } else if(exit_confirm) {
        text("A  CONFIRMAR",18,455,font,white);
        text("B  CANCELAR",330,455,font,white);
    } else if(!in_catalog) {
        text("B  SAIR",18,455,font,white);
        text("D-PAD  CATEGORIAS",330,455,font,muted);
        text("DIREITA  ENTRAR",190,455,font,white);
    } else if(category==0) {
        text("ESQUERDA / B  VOLTAR AS CATEGORIAS",18,455,font,white);
    } else if(details && category==1) {
        text(selected_is_remote() ? "A  INSTALAR" : "A  INICIAR",
             18,455,font,white);
        text("B  VOLTAR",190,455,font,white);
    } else if(details && category==4) {
        text("A  INICIAR",18,455,font,white);
        text("Y  DESINSTALAR",190,455,font,(SDL_Color){255,174,75,255});
        text("B  VOLTAR",430,455,font,white);
    } else if(details) {
        text("B  VOLTAR",190,455,font,white);
    } else {
        text("A  DETALHES",18,455,font,white);
        text("B  VOLTAR",190,455,font,white);
        text("D-PAD  NAVEGAR",330,455,font,muted);
    }

    if(exit_confirm && !installing && !install_finished) {
        rect(80,130,480,220,(SDL_Color){5,10,22,255});
        SDL_SetRenderDrawColor(renderer,55,145,255,255);
        SDL_Rect border={80,130,480,220};
        SDL_RenderDrawRect(renderer,&border);

        text("DESEJA SAIR DO R36 STORE?",145,165,bold,white);
        text("Tem certeza que deseja fechar a loja?",125,210,font,muted);
        text("A  SIM - SAIR",155,270,bold,blue);
        text("B  NAO - CONTINUAR",155,310,bold,white);
    }

    SDL_RenderPresent(renderer);
}
void change_category(int direction) {
    if(installing || install_finished) return;
    category=(category+direction+5)%5;
    selected=0;
    catalog_scroll=0;
    details=false;
    if(category==4)
        refresh_installed_library();
}

void move_selection(int direction) {
    if(installing || install_finished) return;
    int count=item_count();

    if(count<=0) {
        selected=0;
        return;
    }

    selected=(selected+direction+count)%count;

    if(category==1) {
        const int visible=6;
        if(selected<catalog_scroll)
            catalog_scroll=selected;
        else if(selected>=catalog_scroll+visible)
            catalog_scroll=selected-visible+1;
        int max_scroll=game_count-visible;
        if(max_scroll<0) max_scroll=0;
        if(catalog_scroll>max_scroll) catalog_scroll=max_scroll;
    }
}

bool selected_is_remote(void) {
    if(category!=1 || selected<0 || selected>=game_count)
        return false;

    Game *g=&games[selected];

    return strcmp(g->status,"DISPONIVEL")==0 &&
           strcmp(g->port,"ONLINE")==0;
}

bool valid_install_id(const char *id) {
    if(!id || !*id) return false;

    for(const char *p=id; *p; p++) {
        if(!((*p>='a' && *p<='z') ||
             (*p>='A' && *p<='Z') ||
             (*p>='0' && *p<='9') ||
             *p=='_' || *p=='-'))
            return false;
    }

    return true;
}

void start_install(void);
void refresh_installed_library(void);

void launch_game_item(Game *g) {
    launch_error[0]='\0';

    if(!g || !valid_install_id(g->id)) {
        snprintf(launch_error,sizeof(launch_error),
                 "ID do jogo invalido.");
        return;
    }


    snprintf(launch_id,sizeof(launch_id),"%s",g->id);
    launch_requested=true;
    running=false;
}

void request_game_launch(void) {
    if(category!=1 || selected<0 || selected>=game_count)
        return;

    Game *g=&games[selected];
    if(strcmp(g->port,"COMMUNITY")==0) {
        snprintf(launch_error,sizeof(launch_error),
                 "Port comunitario: instalador em adaptacao.");
        return;
    }
    if(selected_is_remote()) {
        start_install();
        return;
    }
    launch_game_item(g);
}

void request_library_launch(void) {
    if(category!=4 || selected<0 || selected>=library_count)
        return;

    launch_game_item(&library_games[selected]);
}

void request_library_uninstall(void) {
    if(category!=4 || selected<0 || selected>=library_count)
        return;

    Game *g=&library_games[selected];
    if(!valid_install_id(g->id)) {
        snprintf(launch_error,sizeof(launch_error),"ID invalido.");
        return;
    }

    pid_t child=fork();
    if(child==0) {
        execlp("python3","python3",
               "/roms/r36store/app/installer.py",
               "uninstall",g->id,(char *)NULL);
        _exit(127);
    }

    if(child<0) {
        snprintf(launch_error,sizeof(launch_error),
                 "Falha ao iniciar desinstalacao.");
        return;
    }

    int status=0;
    pid_t result;
    do {
        result=waitpid(child,&status,0);
    } while(result<0 && errno==EINTR);

    if(result==child && WIFEXITED(status) && WEXITSTATUS(status)==0) {
        launch_error[0]='\0';
        details=false;
        selected=0;
        refresh_installed_library();
    } else {
        snprintf(launch_error,sizeof(launch_error),
                 "Falha ao desinstalar. Consulte o log.");
    }
}

void start_install(void) {
    if(installing || install_finished || !selected_is_remote())
        return;

    Game *g=&games[selected];

    if(!valid_install_id(g->id)) {
        snprintf(install_message,sizeof(install_message),
                 "ID do pacote invalido.");
        install_finished=true;
        install_success=false;
        return;
    }

    int descriptors[2];

    if(pipe(descriptors)!=0) {
        snprintf(install_message,sizeof(install_message),
                 "Nao foi possivel iniciar o instalador.");
        install_finished=true;
        install_success=false;
        return;
    }

    pid_t child=fork();

    if(child<0) {
        close(descriptors[0]);
        close(descriptors[1]);
        snprintf(install_message,sizeof(install_message),
                 "Falha ao criar processo.");
        install_finished=true;
        install_success=false;
        return;
    }

    if(child==0) {
        dup2(descriptors[1],STDOUT_FILENO);
        dup2(descriptors[1],STDERR_FILENO);
        close(descriptors[0]);
        close(descriptors[1]);

        execlp("python3","python3",
               "/roms/r36store/app/installer.py",
               "install",g->id,(char *)NULL);

        _exit(127);
    }

    close(descriptors[1]);

    int flags=fcntl(descriptors[0],F_GETFL,0);
    if(flags>=0)
        fcntl(descriptors[0],F_SETFL,flags|O_NONBLOCK);

    installer_pid=child;
    installer_pipe=descriptors[0];
    installing=true;
    install_finished=false;
    install_success=false;
    cancel_requested=false;
    install_stage[0]='\0';
    install_message[0]='\0';
    pipe_used=0;
    install_current=0;
    install_total=0;
}

void request_install_cancel(void) {
    if(!installing || cancel_requested || !install_stage[0])
        return;

    int fd=open("/roms/r36store/cache/install.cancel",
                O_WRONLY|O_CREAT|O_TRUNC,0600);

    if(fd>=0) {
        write(fd,"cancel",6);
        close(fd);
        cancel_requested=true;
        snprintf(install_message,sizeof(install_message),
                 "Solicitando cancelamento...");
    }
}

void parse_installer_line(char *line) {
    char *progress_line=strstr(line,"R36_PROGRESS ");

    if(progress_line) {
        char stage[32]={0};
        char message[128]={0};
        unsigned long long current=0,total=0;

        int fields=sscanf(
            progress_line,
            "R36_PROGRESS {\"stage\": \"%31[^\"]\", "
            "\"current\": %llu, \"total\": %llu, "
            "\"message\": \"%127[^\"]\"}",
            stage,&current,&total,message
        );

        if(fields==4) {
            snprintf(install_stage,sizeof(install_stage),"%s",stage);
            snprintf(install_message,sizeof(install_message),"%s",message);
            install_current=current;
            install_total=total;
        }
    } else if(*line) {
        snprintf(install_message,sizeof(install_message),"%s",line);
    }
}

void refresh_installed_library(void) {
    pid_t child=fork();

    if(child==0) {
        execlp("python3","python3",
               "/roms/r36store/app/library_bridge.py",
               (char *)NULL);
        _exit(127);
    }

    if(child>0) {
        int status=0;
        pid_t result;

        do {
            result=waitpid(child,&status,0);
        } while(result<0 && errno==EINTR);

        if(result==child &&
           WIFEXITED(status) &&
           WEXITSTATUS(status)==0) {
            load_library();
            sync_catalog_installed();
        }
    }
}


void process_installer(void) {
    if(installer_pipe>=0) {
        char incoming[1024];
        ssize_t amount;

        while((amount=read(installer_pipe,incoming,sizeof(incoming)))>0) {
            for(ssize_t i=0;i<amount;i++) {
                char c=incoming[i];

                if(c=='\n') {
                    pipe_buffer[pipe_used]='\0';
                    parse_installer_line(pipe_buffer);
                    pipe_used=0;
                } else if(c!='\r' && pipe_used<sizeof(pipe_buffer)-1) {
                    pipe_buffer[pipe_used++]=c;
                }
            }
        }

        if(amount==0) {
            if(pipe_used>0) {
                pipe_buffer[pipe_used]='\0';
                parse_installer_line(pipe_buffer);
                pipe_used=0;
            }

            close(installer_pipe);
            installer_pipe=-1;
        }
    }

    if(installing && installer_pid>0) {
        int status=0;
        pid_t result=waitpid(installer_pid,&status,WNOHANG);

        if(result==installer_pid) {
            installing=false;
            install_finished=true;
            install_success=WIFEXITED(status) &&
                            WEXITSTATUS(status)==0;
            installer_pid=-1;

            if(install_success) {
                snprintf(install_message,sizeof(install_message),
                         "Instalacao concluida!");
                refresh_installed_library();
            } else if(cancel_requested) {
                snprintf(install_message,sizeof(install_message),
                         "Instalacao cancelada.");
            } else if(!install_message[0]) {
                snprintf(install_message,sizeof(install_message),
                         "Falha na instalacao.");
            }
        }
    }
}

void draw_install_screen(void) {
    text("INSTALACAO DO PACOTE",202,91,bold,white);
    rect(202,135,415,245,panel);

    text(install_finished ?
         (install_success ? "INSTALACAO CONCLUIDA" : "INSTALACAO ENCERRADA") :
         "INSTALANDO...",225,158,bold,white);

    text(install_stage[0] ? install_stage : "prepare",
         225,205,bold,blue);

    text(install_message,225,235,font,muted);

    if(install_total>0) {
        unsigned long long percent=
            install_current>=install_total ? 100 :
            install_current*100/install_total;

        char label[64];
        snprintf(label,sizeof(label),"%llu%%",percent);
        text(label,225,285,bold,white);

        rect(225,315,365,15,(SDL_Color){35,48,70,255});

        int width=(int)(365*percent/100);
        if(width>0)
            rect(225,315,width,15,blue);
    }

    if(installing) {
        text(cancel_requested ?
             "CANCELAMENTO SOLICITADO" :
             "A PARA CANCELAR",225,365,font,white);
    } else {
        text(install_success ?
             "PACOTE ADICIONADO A BIBLIOTECA" :
             "A OU B PARA VOLTAR",225,365,font,white);
    }
}

int main(void) {
    load_catalog();
    load_library();
    sync_catalog_installed();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"1");

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0) {
        fprintf(stderr,"SDL: %s\n",SDL_GetError());
        return 1;
    }

    if(TTF_Init()!=0) {
        fprintf(stderr,"TTF: falha ao iniciar\n");
        SDL_Quit();
        return 1;
    }

    font=TTF_OpenFont(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",12);
    bold=TTF_OpenFont(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",15);

    if(!font || !bold) {
        fprintf(stderr,"Erro ao carregar fontes\n");
        return 1;
    }

    window=SDL_CreateWindow(
        "R36 STORE",SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_FULLSCREEN
    );

    if(!window) {
        fprintf(stderr,"Janela: %s\n",SDL_GetError());
        return 1;
    }

    renderer=SDL_CreateRenderer(
        window,-1,SDL_RENDERER_ACCELERATED|
        SDL_RENDERER_PRESENTVSYNC
    );

    if(!renderer)
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);

    if(!renderer) {
        fprintf(stderr,"Renderizador: %s\n",SDL_GetError());
        return 1;
    }

    SDL_GameController *pad=NULL;

    for(int i=0;i<SDL_NumJoysticks();i++) {
        if(SDL_IsGameController(i)) {
            pad=SDL_GameControllerOpen(i);
            if(pad) break;
        }
    }

    SDL_Event e;

    while(running) {
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT)
                running=false;

            if(e.type==SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        if(!installing && !install_finished)
                            exit_confirm = !exit_confirm;
                        break;

                    case SDLK_UP:
                        if(!exit_confirm && !details && !installing &&
                           !install_finished) {
                            if(in_catalog) {
                                if(category!=0)
                                    move_selection(-1);
                            } else {
                                change_category(-1);
                            }
                        }
                        break;

                    case SDLK_DOWN:
                        if(!exit_confirm && !details && !installing &&
                           !install_finished) {
                            if(in_catalog) {
                                if(category!=0)
                                    move_selection(1);
                            } else {
                                change_category(1);
                            }
                        }
                        break;