import hashlib
import json
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import time


ROOT=pathlib.Path(__file__).resolve().parents[1]
VERSION=json.loads((ROOT/'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
REVIEW=ROOT/'verification'/('macless-'+VERSION)


def modern():
    jar=ROOT/'build/libs'/('macless-'+VERSION+'+26.3.jar')
    directory=REVIEW/'release-client-26.3'
    (directory/'mods').mkdir(parents=True,exist_ok=True)
    for old in (directory/'mods').glob('*.jar'):old.unlink()
    shutil.copy2(jar,directory/'mods'/jar.name)
    paths=shlex.split((ROOT/'build/loom-cache/argFiles/runClient').read_text())[1].split(':')
    paths=[path for path in paths if not path.startswith(str(ROOT/'build')) and not path.endswith('mcopt-0.3.0-alpha.2.jar')]
    assert all(pathlib.Path(path).exists() for path in paths)
    command=[str(pathlib.Path(os.environ['JAVA_HOME'])/'bin/java') if os.environ.get('JAVA_HOME') else shutil.which('java'),'-XstartOnFirstThread','-Xmx2048m','-Dfabric.development=false','-cp',':'.join(paths),'net.fabricmc.loader.impl.launch.knot.KnotClient','--username','MaclessReview','--version','26.3','--gameDir',str(directory),'--assetsDir',str(pathlib.Path.home()/'.gradle/caches/fabric-loom/assets'),'--assetIndex','26.3-34','--accessToken','0','--width','1440','--height','900','--graphicsBackend','opengl']
    log=REVIEW/'release-boot.log'
    with log.open('w') as output:
        process=subprocess.Popen(command,cwd=directory,stdout=output,stderr=subprocess.STDOUT)
        try:
            deadline=time.monotonic()+90
            while time.monotonic()<deadline:
                text=log.read_text()
                if 'Macless native core loaded' in text and 'minecraft:textures/atlas/gui.png-atlas' in text:
                    time.sleep(3)
                    break
                if process.poll() is not None:raise RuntimeError('Release client exited; see '+str(log))
                time.sleep(.2)
            else:raise TimeoutError('Release boot timed out; see '+str(log))
        finally:
            if process.poll() is None:process.terminate()
            process.wait(timeout=15)
    text=log.read_text()
    assert not re.search(r'NoClassDefFoundError|NoSuchMethodError|Toolscreen renderer failed|could not start',text)
    assert 'TOOLSCREEN_VISUAL_READY' not in text and 'TOOLSCREEN_WORLD_CAPTURE' not in text
    result={'passed':True,'version':VERSION,'minecraft':'26.3','variant':'release','jar_sha256':hashlib.sha256(jar.read_bytes()).hexdigest(),'scope':'exact universal jar loaded from isolated mods folder; production Fabric launch; title-screen startup; intentional termination','log_sha256':hashlib.sha256(log.read_bytes()).hexdigest()}
    (REVIEW/'modern-release-boot.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


if __name__=='__main__':
    results=[modern()]
    command=['python3','tools/verify_prism.py','--launcher-log',os.environ['MACLESS_PRISM_LOG'],'--instance',os.environ['MACLESS_PRISM_INSTANCE'],'--jar',str(ROOT/'legacy/build/libs'/('macless-'+VERSION+'+1.16.1.jar')),'--boot']
    with (REVIEW/'legacy-release-boot-launch.log').open('w') as output:subprocess.run(command,cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True,timeout=120)
    results.append(json.loads((REVIEW/'legacy-release-boot.json').read_text()))
    (REVIEW/'release-boot.json').write_text(json.dumps({'passed':True,'version':VERSION,'clients':results},indent=2)+'\n')
    print('Both exact universal release JARs passed isolated production startup')
