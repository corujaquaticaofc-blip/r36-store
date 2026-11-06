# Formato do catálogo R36 STORE

O arquivo remoto principal é `catalog/catalog.json`.

## Campos de cada item

Cada objeto em `items` deverá conter:

- `id`: identificador único, minúsculo, sem espaços.
- `name`: nome exibido na loja.
- `category`: categoria, por exemplo `game`, `app` ou `tool`.
- `description`: descrição curta.
- `version`: versão publicada.
- `architecture`: arquitetura suportada, inicialmente `aarch64`.
- `firmware`: lista de firmwares testados.
- `download.url`: URL HTTPS do pacote publicado.
- `download.size_bytes`: tamanho exato em bytes.
- `download.sha256`: hash SHA-256 do arquivo.
- `install.type`: método de instalação suportado.
- `install.destination`: destino permitido, relativo à raiz de instalação aprovada.
- `cover`: caminho da capa dentro do repositório, se houver.
- `legal.notice`: observação sobre licença e arquivos de dados exigidos.

## Regras

1. Não cadastrar URL fictícia nem pacote ainda não publicado.
2. Não marcar item como pronto sem teste real no R36S.
3. Todo pacote deve ter SHA-256 verificado antes da instalação.
4. Não incluir ROMs ou dados comerciais protegidos por direitos autorais.
5. O instalador deve rejeitar caminhos absolutos, `..` e destinos fora das pastas autorizadas.
6. O catálogo deve permanecer JSON válido.
