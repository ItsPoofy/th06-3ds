"""Extract native menu motion and a persistent logo from the original ANMs."""
from pathlib import Path
import math,struct
from PIL import Image

def animation(root,name):
    data=(root/'romfs/data'/name).read_bytes();fields=struct.unpack_from('<16I',data)
    sprites=[]
    for i in range(fields[0]):
        offset=struct.unpack_from('<I',data,64+i*4)[0]
        sprites.append(struct.unpack_from('<I4f',data,offset))
    scripts=[]
    entries=[struct.unpack_from('<2I',data,64+fields[0]*4+i*8) for i in range(fields[1])]
    for _,offset in entries:
        end=min([o for _,o in entries if o>offset]+[len(data)])
        instructions=[]
        while offset+4<=end:
            time,opcode,size=struct.unpack_from('<hBB',data,offset)
            instructions.append((opcode,data[offset+4:offset+4+size]))
            offset+=4+size
        scripts.append(instructions)
    image_name=data[fields[7]:].split(b'\0',1)[0].decode().replace('\\','/').rsplit('/',1)[-1]
    image=Image.open(root/'romfs/data'/image_name).convert('RGBA')
    if fields[9]:
        alpha_name=data[fields[9]:].split(b'\0',1)[0].decode().replace('\\','/').rsplit('/',1)[-1]
        image.putalpha(Image.open(root/'romfs/data'/alpha_name).convert('RGB').getchannel('R'))
    return scripts,sprites,image

def position(script,interrupt):
    active=False;result=None
    for opcode,args in script:
        if opcode==22:
            if active and result is not None:return result
            active=struct.unpack_from('<i',args)[0]==interrupt
        if active and opcode in (17,18,19,20):result=struct.unpack_from('<3f',args)[:2]
    if result is not None:return result
    raise ValueError(f'No position for interrupt {interrupt}')

def menu_motion(root):
    scripts=[]
    for name in ('title01.anm','title02.anm','title03.anm','title04.anm'):
        scripts.extend(animation(root,name)[0])
    header=['struct Motion {float initialX,initialY,restX,restY;};']
    for name,indices,interrupt in [('optionMotion',[8,9,10,11,12,72,73,74,13],3),
                                    ('controlMotion',list(range(34,45)),5)]:
        header.append('inline constexpr Motion '+name+'[]={')
        for i in indices:
            initial=next(struct.unpack_from('<3f',args)[:2] for op,args in scripts[i] if op==17)
            rest=position(scripts[i],interrupt)
            header.append('{'+','.join(str(v)+'f' for v in (*initial,*rest))+'},')
        header.append('};')
    return header

def logo(root):
    canvas=Image.new('RGBA',(400,320))
    for name in ('title02.anm','title03.anm'):
        scripts,sprites,sheet=animation(root,name)
        for i,script in enumerate(scripts):
            sprite_id=struct.unpack_from('<I',next(args for op,args in script if op==1))[0]
            _,x,y,w,h=next(s for s in sprites if s[0]==sprite_id)
            sprite=sheet.crop((int(x),int(y),int(x+w),int(y+h)))
            sprite=sprite.resize((round(w*.5),round(h*.5)),Image.Resampling.LANCZOS)
            cx,cy=position(script,2)
            angle=0
            if name=='title03.anm':angle=-math.pi/8 if i==0 else -3*math.pi/8
            if angle:sprite=sprite.rotate(-math.degrees(angle),Image.Resampling.BICUBIC,expand=True)
            canvas.alpha_composite(sprite,(round(40+cx*.5-sprite.width/2),round(cy*.5-sprite.height/2)))
    bounds=canvas.getbbox();ink=canvas.crop(bounds)
    scale=min(1,210/ink.height)
    ink=ink.resize((round(ink.width*scale),round(ink.height*scale)),Image.Resampling.LANCZOS)
    result=Image.new('RGBA',(400,240))
    result.alpha_composite(ink,(round(96-ink.width/2),15))
    result.save(root/'assets/persistent-logo.png')
    return result
