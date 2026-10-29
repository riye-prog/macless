import hashlib
import html
import json
import pathlib
import re
import shutil
import zipfile
from datetime import datetime, timezone


ROOT=pathlib.Path(__file__).resolve().parents[1]
VERSION=json.loads((ROOT/'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
REVIEW=ROOT/'verification'/('macless-'+VERSION)


def hashes(path):
    data=path.read_bytes()
    return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'sha512':hashlib.sha512(data).hexdigest()}


def source_files():
    files=[]
    for folder in ['native','src','shared','legacy/src','tools','tests','gradle','.github','approval']:
        files.extend(path for path in (ROOT/folder).rglob('*') if path.is_file() and '__pycache__' not in path.parts and path.name!='.DS_Store')
    files.extend(ROOT/name for name in ['CMakeLists.txt','build.gradle','legacy/build.gradle','settings.gradle','gradle.properties','gradlew','.gitignore','readme.md','LICENSE'])
    return sorted(set(files))


def package():
    artifacts=json.loads((REVIEW/'artifacts.json').read_text())
    assert artifacts['version']==VERSION
    build=(REVIEW/'final-build.log').read_text()
    assert 'BUILD SUCCESSFUL' in build and re.search(r'Ran (\d+) tests',build) and re.search(r'\nOK\s*\n',build)
    desktop=re.search(r'MACLESS_DESKTOP_PASS checks=(\d+)',build)
    assert desktop
    live={backend:json.loads((REVIEW/(backend+'-qa.json')).read_text()) for backend in ['opengl','vulkan','metal']}
    for result in live.values():
        assert result['version']==VERSION and result['profiler_lifecycle'] and result['consecutive_mode_clicks']==8
        assert result['native_sha256']==hashes(ROOT/'build/native/libtoolscreen.dylib')['sha256']
    release_checks=json.loads((REVIEW/'release-boot.json').read_text())
    assert release_checks['passed'] and release_checks['version']==VERSION
    legacy=json.loads((REVIEW/'legacy-live.json').read_text())
    assert legacy['passed'] and legacy['native_sha256']==hashes(ROOT/'build/native/libtoolscreen.dylib')['sha256']
    for result in release_checks['clients']:
        name='macless-'+VERSION+'+'+result['minecraft']+'.jar'
        assert result['jar_sha256']==artifacts['jars'][name]['sha256']
    destination=ROOT/'build'/('approval-package-'+VERSION)
    if destination.exists():shutil.rmtree(destination)
    destination.mkdir(parents=True)
    checksums={'candidate':VERSION,'jars':{},'natives':{},'helpers':{}}
    for name,details in artifacts['jars'].items():
        path=pathlib.Path(details['path'])
        assert hashes(path)['sha256']==details['sha256']
        with zipfile.ZipFile(path) as archive:
            assert archive.testzip() is None
            assert not json.loads(archive.read('macless-build.json'))['verification']
        shutil.copy2(path,destination/name)
        checksums['jars'][name]=hashes(path)
    for target,details in artifacts['natives'].items():
        path=ROOT/'build/natives'/target/details['library']
        assert hashes(path)['sha256']==details['sha256']
        checksums['natives'][target]={'resource':'natives/'+target+'/'+details['library'],**hashes(path)}
    checksums['helpers']['windows-capture']={'resource':'natives/windows-x64/macless-capture.exe',**hashes(ROOT/'build/natives/windows-x64/macless-capture.exe')}
    shutil.copy2(ROOT/'readme.md',destination/'readme.md')
    shutil.copytree(ROOT/'approval',destination/'approval')
    shutil.copytree(ROOT/'native/licenses',destination/'licenses')
    shutil.copy2(ROOT/'build/native/_deps/imgui-src/LICENSE.txt',destination/'licenses/imgui.txt')
    source=destination/f'macless-{VERSION}-source.zip'
    source_manifest={}
    with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as archive:
        for path in source_files():
            name=path.relative_to(ROOT).as_posix()
            source_manifest[name]=hashes(path)
            info=zipfile.ZipInfo(name,(2026,10,28,0,0,0))
            info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=(0o100755 if name=='gradlew' else 0o100644)<<16
            archive.writestr(info,path.read_bytes())
    checksums['source']={source.name:hashes(source)}
    (destination/'approval/source-files.json').write_text(json.dumps(source_manifest,indent=2)+'\n')
    (destination/'checksums.json').write_text(json.dumps(checksums,indent=2)+'\n')
    captures=json.loads((REVIEW/'gallery-captures.json').read_text())
    screenshots=destination/'screenshots'
    screenshots.mkdir()
    capture_receipt={}
    for name,caption in captures.items():
        path=REVIEW/'review'/name
        shutil.copy2(path,screenshots/name)
        capture_receipt[name]={'caption':caption,**hashes(path)}
    shutil.copy2(screenshots/'gui-modes.png',destination/'preview.png')
    logs=destination/'logs'
    logs.mkdir()
    for name in ['final-build.log','assemble-universal.log','cross-build.log','release-boot.log']+['build-'+target+'.log' for target in artifacts['natives'] if target!='macos-arm64']+['opengl-qa.log','vulkan-qa.log','metal-qa.log']:
        path=REVIEW/name
        (logs/name).write_text(path.read_text().replace(str(pathlib.Path.home()),'/users/build'))
    receipt={'version':VERSION,'purpose':'moderator approval submission candidate','approved':False,'submitted':False,'public_source_published':json.loads((ROOT/'approval/rulings.json').read_text())['public_source_published'],'generated_at':datetime.now(timezone.utc).isoformat(),'compiled_platforms':list(artifacts['natives']),'native_tests':int(re.search(r'Ran (\d+) tests',build).group(1)),'desktop_contract_checks':int(desktop.group(1)),'release_boot':release_checks,'live_26_3':live,'legacy':legacy,'screenshots':capture_receipt,'pending_rulings':json.loads((ROOT/'approval/rulings.json').read_text())['questions'],'limits':['Windows, Linux and Intel macOS natives compiled; full Minecraft runtime checks in this candidate were on Apple Silicon macOS only.','QA world controllers are absent from release jars; the exact final release jars received separate startup checks.','macOS 14 deployment target is configured, but host static dependencies emitted macOS 15 warnings; macOS 14 runtime compatibility remains unverified.','Metal requires the separately reviewed external mcopt backend; it is not included.','Arbitrary mod combinations, long sessions, Linux desktop portals and Windows Minecraft sessions remain unverified.'],'interface_review':json.loads((REVIEW/'interface-review.json').read_text()),'source_manifest_sha256':hashes(destination/'approval/source-files.json')['sha256']}
    (destination/'verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
    cards=''.join('<figure><a href="screenshots/'+html.escape(name)+'"><img loading="lazy" src="screenshots/'+html.escape(name)+'" alt="'+html.escape(caption)+'"></a><figcaption>'+html.escape(caption)+'</figcaption></figure>' for name,caption in captures.items())
    page='<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Macless '+VERSION+'</title><style>:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#111214;color:#f1f2f4;font:15px system-ui}main{max-width:1500px;margin:auto;padding:36px 24px}h1{font-size:32px}p,figcaption{color:#a6abb3;line-height:1.6}a{color:inherit}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(440px,1fr));gap:20px}figure{margin:0;background:#18191b;border-radius:12px;overflow:hidden}img{display:block;width:100%}figcaption{padding:12px 16px}@media(max-width:520px){.grid{grid-template-columns:1fr}}</style><main><h1>Macless '+VERSION+'</h1><p>Approval candidate · Minecraft 1.16.1 and 26.3 · macOS, Windows and Linux</p><p>A smoother profiler legend with shadow-free typography, a rounded charcoal background, subtle row separators, and aligned percentage columns. Live world screenshots below use the separate QA build. Release binaries omit the controllers and fixtures.</p><p><a href="approval/readme.md">Submission notes and pending rulings</a> · <a href="checksums.json">Exact binary hashes</a> · <a href="verification.json">Validation scope</a> · <a href="'+source.name+'">Source archive</a></p><div class="grid">'+cards+'</div></main></html>'
    (destination/'gallery.html').write_text(page)
    files=sorted(path for path in destination.rglob('*') if path.is_file())
    for algorithm in ['sha256','sha512']:
        (destination/(algorithm.upper()+'SUMS')).write_text(''.join(getattr(hashlib,algorithm)(path.read_bytes()).hexdigest()+'  '+path.relative_to(destination).as_posix()+'\n' for path in files))
    bundle=destination/f'macless-{VERSION}-multiplatform.zip'
    with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(destination.rglob('*')):
            if path.is_file() and path!=bundle:archive.write(path,pathlib.Path('macless-'+VERSION)/path.relative_to(destination))
    with zipfile.ZipFile(bundle) as archive:assert archive.testzip() is None
    release=ROOT/'dist'
    if release.exists():shutil.move(release,ROOT/'verification'/('previous-dist-'+datetime.now().strftime('%Y%m%d-%H%M%S')))
    shutil.move(destination,release)
    print(json.dumps({'version':VERSION,'bundle':str(release/bundle.name),'bundle_hashes':hashes(release/bundle.name),'jars':list(checksums['jars'])}))


if __name__=='__main__':
    package()
