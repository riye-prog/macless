import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
import zipfile


ROOT = pathlib.Path(__file__).resolve().parents[1]


def verify(launcher_log, instance, jar, visual=False, visual_world=False, boot=False):
    text = launcher_log.read_text()
    with zipfile.ZipFile(jar) as archive:
        version = json.loads(archive.read("fabric.mod.json"))["version"].split("+")[0]
    directory = ROOT / "verification" / (("prism-boot-" if boot else "prism-visual-" if visual else "prism-client-") + version)
    mods = directory / "mods"
    natives = directory / "natives"
    mods.mkdir(parents=True, exist_ok=True)
    natives.mkdir(exist_ok=True)
    for old in mods.glob("*.jar"):
        old.unlink()
    game = next(folder for folder in [instance / "minecraft", instance / ".minecraft"] if (folder / "mods").exists())
    count = 0
    for path in (game / "mods").glob("*.jar"):
        with zipfile.ZipFile(path) as archive:
            if "fabric.mod.json" in archive.namelist() and re.search(r'"id"\s*:\s*"toolscreen-native"', archive.read("fabric.mod.json").decode()):
                continue
        shutil.copy2(path, mods / path.name)
        count += 1
    shutil.copy2(jar, mods / jar.name)
    classpath = []
    for line in text.splitlines()[:200]:
        value = line.strip()
        if not value.startswith("/Users/") or ".jar" not in value:
            continue
        path = pathlib.Path(value[:value.index(".jar") + 4].replace("/Users/********", str(pathlib.Path.home())))
        if not path.exists():
            raise FileNotFoundError(path)
        classpath.append(str(path))
        if "natives-" in path.name:
            with zipfile.ZipFile(path) as archive:
                for name in archive.namelist():
                    if name.endswith(".dylib"):
                        (natives / pathlib.Path(name).name).write_bytes(archive.read(name))
    config = dict(line.split("=", 1) for line in (instance / "instance.cfg").read_text().splitlines() if "=" in line)
    java = config["JavaPath"]
    asset_index = re.search(r"--assetIndex\s+(\S+)", text).group(1)
    assets = instance.parents[1] / "assets"
    log = ROOT / "verification" / (("prism-boot-" if boot else "prism-visual-" if visual else "prism-production-") + version + ".log")
    command = [java, "-XstartOnFirstThread", "-Xms512m", "-Xmx3072m", "-Duser.language=en", "-Dorg.lwjgl.system.stackSize=2048", "-Dtoolscreen.visual=true" if visual else "-Dtoolscreen.world=true", "-Djava.library.path=" + str(natives), "-Dorg.lwjgl.librarypath=" + str(natives), "-cp", ":".join(classpath), "net.fabricmc.loader.impl.launch.knot.KnotClient", "--username", "ToolscreenCheck", "--version", "1.16.1", "--gameDir", str(directory), "--assetsDir", str(assets), "--assetIndex", asset_index, "--uuid", "00000000000000000000000000000001", "--accessToken", "0", "--userType", "mojang", "--width", "1440", "--height", "900"]
    if visual_world:
        command.insert(1,"-Dtoolscreen.visual.world=true")
    if boot:
        import time
        with log.open("w") as output:
            process=subprocess.Popen(command,cwd=directory,stdout=output,stderr=subprocess.STDOUT)
            try:
                deadline=time.monotonic()+90
                while time.monotonic()<deadline:
                    content=log.read_text()
                    if "Macless native core loaded" in content and "Created: 256x256x0 minecraft:textures/atlas/paintings.png-atlas" in content:
                        time.sleep(3)
                        break
                    if process.poll() is not None:raise RuntimeError("Release client exited before startup: "+str(log))
                    time.sleep(.2)
                else:raise TimeoutError("Release boot timed out: "+str(log))
            finally:
                if process.poll() is None:process.terminate()
                process.wait(timeout=15)
        content=log.read_text()
        assert not re.search(r"NoClassDefFoundError|NoSuchMethodError|Toolscreen renderer failed|could not start",content)
        receipt={"passed":True,"version":version,"minecraft":"1.16.1","variant":"release","jar_sha256":hashlib.sha256(jar.read_bytes()).hexdigest(),"java":java,"lwjgl":"3.3.1-mmachina.1","mod_count":count,"scope":"exact packaged jar; title-screen startup; intentional termination", "log_sha256":hashlib.sha256(log.read_bytes()).hexdigest()}
        (ROOT/"verification"/("macless-"+version)/"legacy-release-boot.json").write_text(json.dumps(receipt,indent=2)+"\n")
        print(json.dumps(receipt))
        return
    with log.open("w") as output:
        result = subprocess.run(command, cwd=directory, stdout=output, stderr=subprocess.STDOUT, timeout=900 if visual else 300)
    output = log.read_text()
    if visual:
        print(json.dumps({"exit": result.returncode, "log": str(log), "directory": str(directory)}))
        return
    checks = ["TOOLSCREEN_LEGACY_PASS", "TOOLSCREEN_CAMERA_SCALING_PASS", "TOOLSCREEN_MEASUREMENT_PAUSE_PASS", "TOOLSCREEN_MEASUREMENT_RESUME_PASS", "TOOLSCREEN_SCREEN_CONTEXT_PASS stage=19", "TOOLSCREEN_SCREEN_CONTEXT_PASS stage=20"]
    missing = [check for check in checks if check not in output]
    if result.returncode or missing or re.search(r"TOOLSCREEN_\w*FAIL|Toolscreen renderer failed|NoSuchMethodError", output):
        raise RuntimeError(f"Prism verification failed: exit={result.returncode} missing={missing}; see {log}")
    receipt = {"version": version, "jar_sha256": hashlib.sha256(jar.read_bytes()).hexdigest(), "java": java, "lwjgl": "3.3.1-mmachina.1", "mod_count": count, "checks": checks, "log_sha256": hashlib.sha256(log.read_bytes()).hexdigest()}
    (ROOT / "verification" / ("prism-controls-" + version + ".json")).write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--launcher-log", type=pathlib.Path, required=True)
    parser.add_argument("--instance", type=pathlib.Path, required=True)
    parser.add_argument("--jar", type=pathlib.Path, required=True)
    parser.add_argument("--visual", action="store_true")
    parser.add_argument("--visual-world", action="store_true")
    parser.add_argument("--boot", action="store_true")
    arguments = parser.parse_args()
    verify(arguments.launcher_log, arguments.instance, arguments.jar, arguments.visual, arguments.visual_world, arguments.boot)
