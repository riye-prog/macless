import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import time
import visual_ui as ui


ROOT=pathlib.Path(__file__).resolve().parents[1]
VERSION=json.loads((ROOT/'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
REVIEW=ROOT/'verification'/('macless-'+VERSION)


def verify(backend):
    ui.FOLDER=ROOT/'run/verification'
    ui.FOLDER.mkdir(parents=True,exist_ok=True)
    for name in ['visual-input','visual-ack','visual-close','visual-window.png']:
        (ui.FOLDER/name).unlink(missing_ok=True)
    command=['./gradlew',':runClient','-Pworld','-Pvisual','-Pbackend='+('opengl' if backend=='metal' else backend),'--console=plain']
    if backend=='metal':
        command.append('-Pmcopt='+os.environ.get('MACLESS_MCOPT',str(pathlib.Path.home()/'Downloads/mcopt-0.3.0-alpha.2.jar')))
    log=REVIEW/(backend+'-qa.log')
    with log.open('w') as output:
        process=subprocess.Popen(command,cwd=ROOT,stdout=output,stderr=subprocess.STDOUT)
        try:
            deadline=time.monotonic()+180
            while time.monotonic()<deadline:
                if 'TOOLSCREEN_VISUAL_READY' in log.read_text():
                    break
                if process.poll() is not None:
                    raise RuntimeError('Preview exited; see '+str(log))
                time.sleep(.2)
            else:
                raise TimeoutError('Preview not ready; see '+str(log))
            states=[]
            for mode in [1,2,3,4,0,2,1,0]:
                ui.send('probe '+str(100+mode))
                bounds=json.loads((ui.FOLDER/'visual-probe.json').read_text())
                ui.send('mouse %s %s'%((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2))
                ui.send('button 0 1')
                ui.send('button 0 0')
                ui.send('state')
                state=json.loads((ui.FOLDER/'visual-state.json').read_text())
                assert state['mode']==mode,(mode,state)
                states.append(state)
            assert states[-1]['profiler'][1]>0
            ui.send('key 51 192 1')
            ui.send('key 51 0 0')
            ui.capture(REVIEW/'review'/('26.3-'+backend+'-modes.png'))
            ui.send('game')
            ui.capture(REVIEW/'review'/('26.3-'+backend+'-profiler.png'))
            ui.send('profiler off')
            ui.send('state')
            assert json.loads((ui.FOLDER/'visual-state.json').read_text())['profiler'][1]==0
            ui.send('profiler on')
            ui.send('state')
            assert json.loads((ui.FOLDER/'visual-state.json').read_text())['profiler'][1]>0
            for wanted in ['frame','extract','level']:
                ui.send('profileinfo')
                lines=(ui.FOLDER/'visual-profiler.txt').read_text().splitlines()
                selected=next(int(line.split(' ',1)[0]) for line in lines[1:] if line.split(' ',1)[1]==wanted)
                assert selected<=9
                ui.send('profile '+str(selected))
            ui.send('state')
            assert json.loads((ui.FOLDER/'visual-state.json').read_text())['profiler'][2]==1
            ui.capture(REVIEW/'review'/('26.3-'+backend+'-mapless.png'))
            ui.send('profile 0')
            ui.send('state')
            assert json.loads((ui.FOLDER/'visual-state.json').read_text())['profiler'][2]==0
            if backend=='opengl':
                ui.send('studio')
                ui.send('key 49 192 1')
                ui.send('key 49 0 0')
                for value in [.75,1,1.5,2]:
                    ui.send('set 34 '+str(value))
                    ui.capture(REVIEW/'review'/('modern-scale-'+str(value)+'.png'))
                ui.send('set 34 1')
                ui.send('probe 500')
                bounds=json.loads((ui.FOLDER/'visual-probe.json').read_text())
                ui.send('mouse %s %s'%((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2))
                ui.send('button 0 1');ui.send('button 0 0')
                ui.send('probe 34')
                bounds=json.loads((ui.FOLDER/'visual-probe.json').read_text())
                ui.send('mouse %s %s'%((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2))
                ui.send('button 0 1');ui.send('button 0 0')
                ui.send('key 97 192 1');ui.send('key 97 0 0')
                ui.send('text 1.375');ui.send('key 13 0 1');ui.send('key 13 0 0')
                ui.capture(REVIEW/'review'/'modern-scale-exact.png')
            if backend=='opengl':
                ui.send('game');ui.send('studio');ui.send('set 34 1')
                import review_interface
                review_interface.review()
            (ui.FOLDER/'visual-close').write_text('close')
            process.wait(timeout=40)
            assert process.returncode==0
        finally:
            if process.poll() is None:
                (ui.FOLDER/'visual-close').write_text('close')
                try:process.wait(timeout=20)
                except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=10)
    text=log.read_text()
    assert 'Toolscreen renderer failed' not in text and 'TOOLSCREEN_WORLD_FAIL' not in text
    expected={'metal':'Metal','vulkan':'Vulkan','opengl':'OpenGL'}[backend]
    assert 'TOOLSCREEN_VISUAL_READY backend='+expected in text
    native=ROOT/'build/native/libtoolscreen.dylib'
    result={'version':VERSION,'minecraft':'26.3','backend':expected,'variant':'qa','native_sha256':hashlib.sha256(native.read_bytes()).hexdigest(),'consecutive_mode_clicks':8,'mouse_path':'ToolscreenScreen.mouseClicked/mouseReleased','profiler_lifecycle':True,'mapless_selected_section_only':True,'states':states,'log_sha256':hashlib.sha256(log.read_bytes()).hexdigest()}
    (REVIEW/(backend+'-qa.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(expected+' live profiler and eight GUI mode changes passed',flush=True)


if __name__=='__main__':
    arguments=argparse.ArgumentParser()
    arguments.add_argument('backends',nargs='+',choices=['opengl','vulkan','metal'])
    for backend in arguments.parse_args().backends:
        verify(backend)
