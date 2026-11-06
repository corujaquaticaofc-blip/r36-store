# R36 STORE

Loja independente de jogos e aplicativos para R36S com ArkOS/dArkOS.

## Objetivo

Permitir que o R36 STORE consulte um catálogo remoto, mostre informações dos itens e, futuramente, baixe e instale pacotes compatíveis diretamente no aparelho.

## Estrutura

- `catalog/catalog.json`: catálogo remoto em JSON.
- `docs/FORMAT.md`: formato e regras dos itens do catálogo.
- `assets/covers/`: capas e imagens da loja.
- Pacotes distribuíveis serão publicados separadamente em Releases, com URL, tamanho e SHA-256 registrados no catálogo.

## Segurança e compatibilidade

- Um item só deve ser marcado como instalável quando o pacote tiver sido testado no R36S/ArkOS.
- O instalador deverá validar o SHA-256 antes de extrair ou instalar qualquer pacote.
- Não distribuir ROMs, executáveis comerciais ou dados protegidos por direitos autorais. Quando necessário, o usuário deverá fornecer seus próprios arquivos legais.
- URLs, comandos e caminhos de instalação devem ser validados antes da execução.

## Estado atual

Projeto em desenvolvimento. O catálogo inicial está vazio; nenhum download é anunciado como disponível até que exista um pacote legítimo, testado e publicado.
