#!/usr/bin/env python3
import json
import re
import urllib.request
from pathlib import Path

URL = "https://raw.githubusercontent.com/corujaquaticaofc-blip/r36-store/main/catalog/community-portmaster.json"
DEST = Path("/roms/r36store/cache/community-games.db")
MAX_BYTES = 8 * 1024 * 1024

SUPPORTED = {
    "pm-2048", "pm-wordlesdl", "pm-sdlsopwith", "pm-cceleste",
    "pm-falling_time", "pm-raptor", "pm-nonny", "pm-opensyobon",
    "pm-shippy", "pm-derclou", "pm-meritous", "pm-worship_vector",
    "pm-escape", "pm-powder", "pm-znax", "pm-colorlines",
    "pm-mutant_tank_knights", "pm-nebulus", "pm-openfodder",
    "pm-gnurobbo", "pm-lbreakouthd", "pm-mreader"
}

def clean(value, limit, fallback=""):
    value = str(value if value is not None else fallback).replace("|", "/").replace("\r", " ").replace("\n", " ").strip()
    value = re.sub(r"\s+", " ", value)
    return value[:limit] or fallback

def main():
    request = urllib.request.Request(URL, headers={"User-Agent": "R36-STORE"})
    with urllib.request.urlopen(request, timeout=45) as response:
        raw = response.read(MAX_BYTES + 1)
    if len(raw) > MAX_BYTES:
        raise ValueError("Catalogo comunitario excedeu o limite.")
    data = json.loads(raw.decode("utf-8"))
    if data.get("schema_version") != 1 or not isinstance(data.get("items"), list):
        raise ValueError("Formato do catalogo comunitario invalido.")

    lines = ["# ID|NOME|CATEGORIA|STATUS|PORT|CAPA"]
    seen = set()
    for item in data["items"]:
        game_id = clean(item.get("id"), 47)
        if not re.fullmatch(r"[a-zA-Z0-9_-]{1,47}", game_id) or game_id in seen:
            continue
        seen.add(game_id)
        name = clean(item.get("name"), 95, game_id)
        category = clean(item.get("category"), 31, "other")
        status = "ADAPTADO" if game_id in SUPPORTED else ("VERIFICAR" if item.get("ready_to_run") else "REQUER ARQUIVOS")
        lines.append(f"{game_id}|{name}|{category}|{status}|COMMUNITY|PortMaster")

    DEST.parent.mkdir(parents=True, exist_ok=True)
    temporary = DEST.with_suffix(".tmp")
    temporary.write_text("\n".join(lines) + "\n", encoding="utf-8")
    temporary.replace(DEST)
    print("CATALOGO COMUNITARIO ATUALIZADO!")
    print("Ports importados:", len(lines) - 1)
    print("Arquivo:", DEST)

if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print("ERRO NO CATALOGO COMUNITARIO:", error)
        raise SystemExit(1)

[executed on device: gilbert (f7f1466f-4c0b-4a9b-8c86-4d8bdba1ba83)]