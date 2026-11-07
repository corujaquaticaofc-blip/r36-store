#!/usr/bin/env python3
"""Adaptador inicial e restrito para pacotes PortMaster no R36 STORE.

Nao executa launchers upstream nem importa control.txt/mod_*.txt.
Primeiro perfil suportado: ports libretro com core ARM64 e launcher RetroArch.
"""
import hashlib
import json
import os
import re
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path, PurePosixPath
from urllib.parse import urlparse

ROOT = Path("/roms/r36store")
CATALOG_URL = "https://raw.githubusercontent.com/corujaquaticaofc-blip/r36-store/main/catalog/community-portmaster.json"
REGISTRY = ROOT / "catalog/installed.json"
PORTS = Path("/roms/.r36store/ports")
CANCEL = ROOT / "cache/install.cancel"
MAX_DOWNLOAD = 1024 *  1024 * 1024
MAX_EXTRACT = 2 * 1024 * 1024 * 1024
MAX_FILES = 20000


def progress(stage, current=0, total=0, message=""):
    print("R36_PROGRESS " + json.dumps({
        "stage": stage, "current": current, "total": total,
        "message": message
    }), flush=True)


def check_cancel():
    if CANCEL.exists():
        raise RuntimeError("Instalacao cancelada pelo usuario.")


def load_item(item_id):
    if not re.fullmatch(r"pm-[a-zA-Z0-9_-]{1,44}", item_id):
        raise ValueError("ID comunitario invalido.")
    req = urllib.request.Request(CATALOG_URL, headers={"User-Agent": "R36-STORE"})
    with urllib.request.urlopen(req, timeout=45) as response:
        raw = response.read(16 * 1024 * 1024 + 1)
    if len(raw) > 16 * 1024 * 1024:
        raise ValueError("Catalogo comunitario excedeu o limite.")
    data = json.loads(raw.decode("utf-8"))
    for item in data.get("items", []):
        if item.get("id") == item_id:
            return item
    raise ValueError("Jogo nao encontrado no catalogo comunitario.")


def download(item, tempdir):
    info = item.get("download") or {}
    url = info.get("url")
    size = info.get("size_bytes")
    md5 = str(info.get("md5", "")).lower()
    if not isinstance(url, str) or urlparse(url).scheme != "https":
        raise ValueError("URL do pacote invalida.")
    if urlparse(url).hostname not in ("github.com", "www.github.com"):
        raise ValueError("Origem de download nao autorizada.")
    if not isinstance(size, int) or not 0 < size <= MAX_DOWNLOAD:
        raise ValueError("Tamanho do pacote fora do limite do adaptador.")
    if not re.fullmatch(r"[0-9a-f]{32}", md5):
        raise ValueError("MD5 oficial ausente ou invalido.")
    if shutil.disk_usage(PORTS.parent).free < size + 64 * 1024 * 1024:
        raise ValueError("Espaco livre insuficiente para baixar o pacote.")
    target = tempdir / "community.zip"
    digest = hashlib.md5()
    received = 0
    progress("download", 0, size, "Baixando pacote comunitario")
    request = urllib.request.Request(url, headers={"User-Agent": "R36-STORE"})
    with urllib.request.urlopen(request, timeout=45) as response, target.open("wb") as out:
        while True:
            check_cancel()
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            received += len(chunk)
            if received > size:
                raise ValueError("Download maior que o tamanho oficial.")
            digest.update(chunk)
            out.write(chunk)
            progress("download", received, size, "Baixando pacote comunitario")
    if received != size or digest.hexdigest() != md5:
        raise ValueError("Tamanho ou MD5 nao confere; pacote rejeitado.")
    progress("verify", 1, 1, "Pacote oficial verificado")
    return target


def safe_extract(package, destination):
    total = 0
    with zipfile.ZipFile(package) as archive:
        members = archive.infolist()
        if not members or len(members) > MAX_FILES:
            raise ValueError("ZIP vazio ou com arquivos demais.")
        seen = {}
        for m in members:
            name = m.filename
            if "\\" in name or "\x00" in name or ":" in name:
                raise ValueError("Nome de arquivo ZIP invalido.")
            if m.flag_bits & 1:
                raise ValueError("ZIP criptografado nao permitido.")
            p = PurePosixPath(name)
            if p.is_absolute() or any(x in ("", ".", "..") for x in p.parts):
                raise ValueError("Caminho inseguro dentro do ZIP.")
            mode = m.external_attr >> 16
            kind = stat.S_IFMT(mode)
            if kind == stat.S_IFLNK or kind not in (0, stat.S_IFREG, stat.S_IFDIR):
                raise ValueError("Link ou tipo de arquivo nao permitido.")
            key = "/".join(p.parts)
            if key in seen:
                raise ValueError("Arquivo duplicado no ZIP.")
            seen[key] = m.is_dir()
            if not m.is_dir():
                total += m.file_size
                if total > MAX_EXTRACT:
                    raise ValueError("Pacote excede limite de extracao.")
        if shutil.disk_usage(destination).free < total + 32 * 1024 * 1024:
            raise ValueError("Espaco livre insuficiente para extrair o pacote.")
        for key, is_dir in seen.items():
            parts = key.split("/")
            for i in range(1, len(parts)):
                parent = "/".join(parts[:i])
                if parent in seen and not seen[parent]:
                    raise ValueError("Conflito arquivo/pasta no ZIP.")
            if not is_dir and any(x.startswith(key + "/") for x in seen):
                raise ValueError("Conflito arquivo/pasta no ZIP.")
        done = 0
        progress("extract", 0, total, "Validando e extraindo arquivos")
        for m in members:
            check_cancel()
            if m.is_dir():
                continue
            p = PurePosixPath(m.filename)
            target = destination.joinpath(*p.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(m) as src, target.open("xb") as out:
                while True:
                    chunk = src.read(65536)
                    if not chunk:
                        break
                    out.write(chunk)
                    done += len(chunk)
                    if done > total:
                        raise ValueError("Extracao excedeu tamanho declarado.")
            if target.stat().st_size != m.file_size:
                raise ValueError("Arquivo extraido com tamanho incorreto.")
            permissions = (m.external_attr >> 16) & 0o777
            if permissions:
                target.chmod(permissions & 0o755)
            else:
                target.chmod(0o644)
        if done != total:
            raise ValueError("Tamanho total extraido incorreto.")


def adapt_libretro(root):
    scripts = list(root.glob("*.sh"))
    cores = sorted(root.rglob("*.so.aarch64"))
    if len(cores) != 1:
        return None
    # Exige evidencia de que o launcher oficial usa RetroArch/libretro.
    if not any("retroarch" in p.read_text(encoding="utf-8", errors="ignore").lower()
               and "-l " in p.read_text(encoding="utf-8", errors="ignore").lower()
               for p in scripts):
        return None
    core = cores[0].relative_to(root).as_posix()
    wrapper = root / "launch-r36store.sh"
    quoted = "'" + core.replace("'", "'\\''") + "'"
    wrapper.write_text(
        "#!/bin/bash\n"
        "set -eu\n"
        'ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"\n'
        f"CORE=\"$ROOT\"/{quoted}\n"
        "for RA in /usr/local/bin/retroarch /usr/bin/retroarch /opt/retroarch/bin/retroarch; do\n"
        '  if [ -x "$RA" ]; then exec "$RA" -L "$CORE" "$@"; fi\n'
        "done\n"
        'echo "R36 STORE: RetroArch nao encontrado neste firmware."\n'
        "exit 127\n",
        encoding="utf-8"
    )
    wrapper.chmod(0o755)
    return {"type": "script", "target": "launch-r36store.sh", "cwd": ".", "args": [], "env": {}}



def safe_relative(path):
    value = path.as_posix()
    if not re.fullmatch(r"[A-Za-z0-9_./ +()-]+", value):
        raise ValueError("Caminho do pacote contem caracteres nao suportados.")
    if value.startswith("/") or ".." in PurePosixPath(value).parts:
        raise ValueError("Caminho relativo invalido.")
    return value


def is_aarch64_elf(path):
    try:
        header = path.read_bytes()[:20]
        return (len(header) >= 20 and header[:4] == b"\x7fELF"
                and header[4] == 2 and header[5] == 1
                and struct.unpack_from("<H", header, 18)[0] == 183)
    except Exception:
        return False


def native_dependencies_ok(binary, root):
    readelf = shutil.which("readelf")
    if not readelf:
        return False
    result = subprocess.run([readelf, "-d", str(binary)], capture_output=True,
                            text=True, timeout=8, check=False)
    if result.returncode != 0:
        return False
    needed = re.findall(r"\(NEEDED\).*?\[([^\]]+)\]", result.stdout)
    system_dirs = [
        Path("/lib/aarch64-linux-gnu"), Path("/usr/lib/aarch64-linux-gnu"),
        Path("/usr/local/lib"), Path("/lib"), Path("/usr/lib")
    ]
    package_dirs = [p for p in root.rglob("libs") if p.is_dir()]
    for lib in needed:
        if not any((directory / lib).exists() for directory in system_dirs + package_dirs):
            return False
    program = subprocess.run([readelf, "-l", str(binary)], capture_output=True,
                             text=True, timeout=8, check=False)
    interpreter = re.search(r"Requesting program interpreter:\s*([^\]]+)", program.stdout)
    if interpreter and not Path(interpreter.group(1).strip()).exists():
        return False
    return True


def adapt_native(root):
    scripts = list(root.glob("*.sh"))
    profiles = list(root.rglob("*.gptk"))
    if len(scripts) != 1 or len(profiles) != 1:
        return None
    script = scripts[0].read_text(encoding="utf-8", errors="replace")
    candidates = []
    for line in script.splitlines():
        stripped = line.strip()
        match = re.search(r"(?:^|\s)(?:exec\s+)?(?:\$TASKSET\s+)?\./([A-Za-z0-9_.+-]+)(?=\s|$)", stripped)
        if not match:
            continue
        tail = stripped[match.end():].strip()
        if tail and not re.match(r"^(?:2>&1|>|&|\|)", tail):
            continue
        for candidate in root.rglob(match.group(1)):
            if candidate.is_file() and is_aarch64_elf(candidate):
                candidates.append(candidate)
    unique = {str(p.resolve()): p for p in candidates}
    if len(unique) != 1:
        return None
    binary = next(iter(unique.values()))
    if not native_dependencies_ok(binary, root):
        return None
    binary.chmod(binary.stat().st_mode | 0o111)

    binary_rel = safe_relative(binary.relative_to(root))
    game_dir_rel = safe_relative(binary.parent.relative_to(root))
    profile_rel = safe_relative(profiles[0].relative_to(root))
    lib_dirs = sorted({p.relative_to(root).as_posix() for p in root.rglob("libs") if p.is_dir()})
    lines = [
        "#!/bin/bash",
        "set -e",
        'ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"',
        f'GAME_DIR="$ROOT/{game_dir_rel}"',
        'cd "$GAME_DIR"',
    ]
    if lib_dirs:
        joined = ":".join(f"$ROOT/{x}" for x in lib_dirs)
        lines.append('export LD_LIBRARY_PATH="' + joined + ':$LD_LIBRARY_PATH"')
    lines.extend([
        'KEY_PID=""',
        f'python3 /roms/r36store/app/keymapper.py "$ROOT/{profile_rel}" >/dev/null 2>&1 &',
        'KEY_PID=$!',
        'cleanup() { if [ -n "$KEY_PID" ]; then kill "$KEY_PID" 2>/dev/null || true; wait "$KEY_PID" 2>/dev/null || true; fi; }',
        'trap cleanup EXIT INT TERM',
        'sleep 0.15',
        f'"$ROOT/{binary_rel}" "$@"',
        'exit $?'
    ])
    wrapper = root / "launch-r36store.sh"
    wrapper.write_text("\n".join(lines) + "\n", encoding="utf-8")
    wrapper.chmod(0o755)
    return {"type": "script", "target": "launch-r36store.sh", "cwd": ".", "args": [], "env": {}}


def register(item, destination, launcher):
    if REGISTRY.exists():
        records = json.loads(REGISTRY.read_text(encoding="utf-8"))
    else:
        records = {}
    if not isinstance(records, dict):
        raise ValueError("Registro local invalido.")
    item_id = item["id"]
    records[item_id] = {
        "name": str(item.get("name", item_id))[:95],
        "version": str(item.get("updated") or "PortMaster")[:30],
        "category": str(item.get("category") or "community")[:31],
        "path": str(destination),
        "launcher": launcher,
        "installed_at": __import__("datetime").datetime.now().isoformat(),
        "source": "PortMaster"
    }
    REGISTRY.parent.mkdir(parents=True, exist_ok=True)
    temp = REGISTRY.with_suffix(".tmp")
    temp.write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding="utf-8")
    os.replace(temp, REGISTRY)


def install(item):
    if not item.get("ready_to_run"):
        raise ValueError("Este port exige arquivos ou configuracao adicional.")
    if item.get("requirements") or item.get("runtime"):
        raise ValueError("Este port depende de componentes ainda nao adaptados.")
    item_id = item["id"]
    PORTS.mkdir(parents=True, exist_ok=True)
    destination = PORTS / item_id
    if destination.exists():
        raise ValueError("Este jogo ja possui uma pasta de instalacao.")
    CANCEL.unlink(missing_ok=True)
    progress("prepare", 0, 1, "Preparando adaptador comunitario")
    with tempfile.TemporaryDirectory(prefix="r36pm-", dir=str(ROOT / "cache")) as td:
        tempdir = Path(td)
        package = download(item, tempdir)
        with tempfile.TemporaryDirectory(prefix=f".{item_id}-", dir=str(PORTS)) as stage:
            extracted = Path(stage) / "package"
            extracted.mkdir()
            safe_extract(package, extracted)
            check_cancel()
            launcher = adapt_libretro(extracted)
            profile = "libretro" if launcher else None
            if launcher is None:
                launcher = adapt_native(extracted)
                profile = "native-sdl-keymap" if launcher else None
            if launcher is None:
                raise ValueError("Formato ainda nao suportado. Nenhum arquivo foi instalado.")
            manifest = {
                "schema_version": 1,
                "launcher": launcher
            }
            (extracted / "r36store.json").write_text(
                json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
            )
            if destination.exists():
                raise ValueError("Destino apareceu durante a instalacao.")
            os.replace(extracted, destination)
            try:
                register(item, destination, launcher)
            except Exception:
                shutil.rmtree(destination, ignore_errors=True)
                raise
    progress("complete", 1, 1, "Instalacao comunitaria concluida")
    print("INSTALACAO COMUNITARIA CONCLUIDA!")
    print("Perfil:", profile)
    print("Destino:", destination)


def main():
    if len(sys.argv) != 3 or sys.argv[1] != "install":
        print("Uso: python3 community_adapter.py install pm-ID")
        return 2
    item = load_item(sys.argv[2])
    install(item)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("ERRO NO ADAPTADOR COMUNITARIO:", error, flush=True)
        raise SystemExit(1)

[executed on device: gilbert (f7f1466f-4c0b-4a9b-8c86-4d8bdba1ba83)]