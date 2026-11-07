#!/bin/bash

cd /roms/r36store

echo "R36 STORE - Atualizando catalogo..."

if python3 app/catalog.py; then
    python3 app/catalog_bridge.py
else
    echo "Sem atualizacao. Usando catalogo salvo."
fi

echo "R36 STORE - Atualizando catalogo da comunidade..."
if ! python3 app/community_bridge.py; then
    echo "Sem atualizacao comunitaria. Usando cache salvo."
fi

echo "R36 STORE - Atualizando biblioteca..."
if ! python3 app/library_bridge.py; then
    echo "Aviso: nao foi possivel atualizar a biblioteca."
fi

exec /roms/r36store/bin/r36store