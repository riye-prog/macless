import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import re
import shutil
import time
import urllib.error
import urllib.request


ROOT = pathlib.Path(__file__).resolve().parents[1]
CACHE = pathlib.Path.home() / '.gradle/caches'
MAVEN = 'https://repo.maven.apache.org/maven2/'


def fetch(url):
    failure = None
    for attempt in range(4):
        try:
            with urllib.request.urlopen(url, timeout=30) as response:
                return response.read()
        except (OSError, urllib.error.URLError) as error:
            failure = error
            time.sleep(.25 * (attempt + 1))
    raise failure


def sha1(data):
    return hashlib.sha1(data).hexdigest()


def allowed(library):
    result = not library.get('rules')
    for rule in library.get('rules', []):
        system = rule.get('os', {})
        if system.get('name', 'linux') != 'linux':
            continue
        if 'arch' in system and not re.search(system['arch'], 'aarch64'):
            continue
        if any(value for value in rule.get('features', {}).values()):
            continue
        result = rule['action'] == 'allow'
    return result


def prepare(version):
    folder = ROOT / 'build/linux-runtime' / version
    folder.mkdir(parents=True, exist_ok=True)
    metadata = json.loads((CACHE / 'fabric-loom' / version / 'mojang_minecraft_info.json').read_text())
    profile = json.loads(fetch('https://meta.fabricmc.net/v2/versions/loader/' + version + '/0.19.5/profile/json'))
    cached = {}
    for path in (CACHE / 'modules-2/files-2.1').rglob('*.jar'):
        cached.setdefault(path.name, []).append(path)
    jobs = {}

    def artifact(name, base=MAVEN, expected=None, url=None):
        group, module, release, *classifier = name.split(':')
        suffix = '-' + classifier[0] if classifier else ''
        relative = group.replace('.', '/') + '/' + module + '/' + release + '/' + module + '-' + release + suffix + '.jar'
        destination = folder / 'libraries' / relative
        jobs[str(destination)] = (destination, url or base + relative, expected)
        return str(pathlib.Path('libraries') / relative)

    classpath = []
    for library in metadata['libraries']:
        if not allowed(library):
            continue
        name = library['name']
        parts = name.split(':')
        if parts[0] == 'org.lwjgl':
            if version == '1.16.1':
                parts[2] = '3.3.6'
            if len(parts) > 3:
                if not parts[3].startswith('natives-linux'):
                    continue
                parts[3] = 'natives-linux-arm64'
            classpath.append(artifact(':'.join(parts)))
        elif version == '1.16.1' and parts[0] == 'net.java.dev.jna':
            classpath.append(artifact('net.java.dev.jna:' + ('jna-platform' if parts[1] == 'platform' else parts[1]) + ':5.17.0'))
        elif version == '1.16.1' and parts[0] == 'org.apache.logging.log4j':
            parts[2] = '2.24.3'
            classpath.append(artifact(':'.join(parts)))
        elif name.startswith('com.mojang:jtracy:') and 'natives' in name:
            continue
        else:
            item = library.get('downloads', {}).get('artifact')
            if item:
                classpath.append(artifact(name, expected=item.get('sha1'), url=item['url']))
            for classifier, item in library.get('downloads', {}).get('classifiers', {}).items():
                if classifier.startswith('natives-linux') and parts[0] != 'org.lwjgl':
                    classpath.append(artifact(name + ':' + classifier, expected=item.get('sha1'), url=item['url']))
    if version == '1.16.1':
        for module in ['lwjgl', 'lwjgl-glfw', 'lwjgl-opengl', 'lwjgl-openal', 'lwjgl-stb', 'lwjgl-jemalloc', 'lwjgl-tinyfd']:
            classpath.append(artifact('org.lwjgl:' + module + ':3.3.6:natives-linux-arm64'))
    for library in profile['libraries']:
        classpath.append(artifact(library['name'], base=library.get('url', MAVEN)))
    client = folder / 'minecraft.jar'
    jobs[str(client)] = (client, metadata['downloads']['client']['url'], metadata['downloads']['client']['sha1'])
    classpath.append('minecraft.jar')
    index = json.loads(fetch(metadata['assetIndex']['url']))
    destination = folder / 'assets/indexes' / (metadata['assetIndex']['id'] + '.json')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(index))
    for name, item in index['objects'].items():
        if '/sounds/' in name or name.endswith(('.ogg', '.mp3', '.wav', '.fsb')):
            continue
        digest = item['hash']
        path = folder / 'assets/objects' / digest[:2] / digest
        old = CACHE / 'fabric-loom/assets/objects' / digest[:2] / digest
        if old.exists() and sha1(old.read_bytes()) == digest:
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(old, path)
        else:
            jobs[str(path)] = (path, 'https://resources.download.minecraft.net/' + digest[:2] + '/' + digest, digest)

    def download(job):
        path, url, expected = job
        if path.exists() and (expected is None or sha1(path.read_bytes()) == expected):
            return
        if expected is None:
            expected = fetch(url + '.sha1').decode().strip().split()[0]
        for old in cached.get(path.name, []):
            if sha1(old.read_bytes()) == expected:
                path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(old, path)
                return
        data = fetch(url)
        if sha1(data) != expected:
            raise RuntimeError('Game dependency checksum mismatch: ' + path.name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(download, jobs.values()))
    result = {'version': version, 'main_class': profile['mainClass'], 'classpath': list(dict.fromkeys(classpath)), 'asset_index': metadata['assetIndex']['id'], 'audio_assets': 'omitted', 'runtime_native_override': 'Linux ARM64 LWJGL'}
    (folder / 'launch.json').write_text(json.dumps(result, indent=2) + '\n')
    return {'version': version, 'libraries': len(result['classpath']), 'folder': str(folder), 'download_jobs': len(jobs)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('versions', nargs='+', choices=['26.3', '1.16.1'])
    arguments = parser.parse_args()
    for version in arguments.versions:
        print(json.dumps(prepare(version)), flush=True)
