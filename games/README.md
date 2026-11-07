# Jogos do R36 STORE — lote 01

Este diretório reúne jogos originais em C/SDL2, sem ROMs, assets comerciais ou dependências do PortMaster.

## Jogos

- **Pong** — partida contra CPU; direcional/analógico move a raquete.
- **Snake** — cobra clássica; direcional/analógico muda a direção.
- **Breakout** — rebata a bola e destrua os blocos.
- **2048** — deslize as peças para combinar números.

## Estado

Estes arquivos são o código-fonte inicial. Ainda **não são pacotes instaláveis**: cada jogo precisa ser compilado para AArch64, receber seu launcher privado em `/roms/.r36store/ports/<id>`, ser testado fisicamente no R36S e só então ganhar uma entrada instalável no catálogo.

Todos os jogos deste lote são implementações originais para o projeto, usando SDL2 e sem conteúdo protegido de terceiros.

## Compilação no R36S

```sh
gcc main.c -o <nome> -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lm -Wall -Wextra
```
