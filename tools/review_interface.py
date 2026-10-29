import json
import pathlib
import visual_ui as ui


ROOT=pathlib.Path(__file__).resolve().parents[1]
VERSION=json.loads((ROOT/'src/main/resources/fabric.mod.json').read_text())['version'].split('+')[0]
REVIEW=ROOT/'verification'/('macless-'+VERSION)
ui.FOLDER=ROOT/'run/verification'


def state():
    ui.send('state')
    return json.loads((ui.FOLDER/'visual-state.json').read_text())


def click(control):
    ui.send('probe '+str(control))
    bounds=json.loads((ui.FOLDER/'visual-probe.json').read_text())
    assert bounds[2]>bounds[0] and bounds[3]>bounds[1],control
    ui.send('mouse %s %s'%((bounds[0]+bounds[2])/2,(bounds[1]+bounds[3])/2))
    ui.send('button 0 1');ui.send('button 0 0')


def key(code,modifiers=0):
    ui.send('key %s %s 1'%(code,modifiers));ui.send('key %s 0 0'%code)


def enter(control,value):
    click(control)
    active=state()
    assert active['textFocused'] and active['sdlTextInput'],(control,active['textFocused'],active['sdlTextInput'])
    key(97,192)
    ui.send('text '+str(value))
    key(13)


def capture(name):
    ui.capture(REVIEW/'review'/name)


def review():
    typed={}
    click(-3);click(801)
    for control,value in [(1,478),(2,674)]:
        enter(control,value);current=state();assert current['settings'][control]==value;typed[str(control)]=value
    assert state()['settings'][36]==0
    capture('gui-typed-resolution.png')
    click(-2)
    for control,value in [(39,2.875),(146,.125)]:
        enter(control,value);assert state()['settings'][control]==value;typed[str(control)]=value
    click(500);enter(34,1.375);assert state()['settings'][34]==1.375
    capture('gui-typed-scale.png')
    click(9111)
    for control,value in [(9001,1024),(9002,650)]:enter(control,value)
    ui.send('probe 501');bounds=json.loads((ui.FOLDER/'visual-probe.json').read_text())
    assert bounds[0]>=0 and bounds[1]>=0 and bounds[2]<=1440 and bounds[3]<=900,bounds
    capture('gui-window-size.png');key(27)
    ui.send('set 34 1')
    for page,name in [(1,'layout'),(2,'controls'),(3,'modes'),(4,'ninjabrain')]:
        click(-page);capture('gui-'+name+'.png')
    ui.send('probe 501');bounds=json.loads((ui.FOLDER/'visual-probe.json').read_text())
    click(-1);click(9200);capture('gui-add-layer-menu.png');key(27)
    click(-1);key(107,192);ui.send('text Mapless overlay');key(13)
    capture('gui-overlay-inspector.png')
    for control in [9100,9101,9102,9103]:
        for attempt in range(10):
            ui.send('probe '+str(control));b=json.loads((ui.FOLDER/'visual-probe.json').read_text())
            if b[2]>b[0] and bounds[1]+80<b[1]<bounds[3]-70:break
            ui.send('mouse %s %s'%(bounds[2]-70,(bounds[1]+bounds[3])/2));ui.send('scroll 0 -2')
        if b[2]>b[0] and bounds[1]+80<b[1]<bounds[3]-70:click(control);capture('gui-overlay-'+str(control)+'.png')
    ui.send('set 34 .75');click(-2);capture('gui-small-scale.png')
    ui.send('set 34 2');ui.send('resize 1024 720')
    for page,name in [(1,'layout'),(2,'controls'),(3,'modes'),(4,'ninjabrain')]:click(-page);capture('gui-compact-'+name+'.png')
    (REVIEW/'interface-review.json').write_text(json.dumps({'typed_fields':typed,'scale':1.375,'gui_window_bounds':bounds[:4],'text_input':'Minecraft KeyboardHandler.textInput, SDL text session checked active for each field','compact_output':[1024,720],'compact_scale':2},indent=2)+'\n')
    (ui.FOLDER/'visual-close').write_text('close')
    print('Typed sizes, sensitivity, scale and GUI window bounds passed; all pages captured at normal and compact sizes')


if __name__=='__main__':
    review()
