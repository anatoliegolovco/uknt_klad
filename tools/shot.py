import struct, sys, subprocess, os
from PIL import Image

def grab(wid, out, crop=None, scale=1):
    xwd = subprocess.run(['xwd','-id',wid,'-silent'],capture_output=True,
                         env={**os.environ,'DISPLAY':':0'}).stdout
    if len(xwd) < 200:
        # fallback to file method
        subprocess.run(['xwd','-id',wid,'-silent','-out','/tmp/_s.xwd'],env={**os.environ,'DISPLAY':':0'})
        xwd = open('/tmp/_s.xwd','rb').read()
    be = '>I'
    hdr   = struct.unpack(be, xwd[0:4])[0]
    w     = struct.unpack(be, xwd[16:20])[0]
    h     = struct.unpack(be, xwd[20:24])[0]
    bpp   = struct.unpack(be, xwd[44:48])[0]
    bpl   = struct.unpack(be, xwd[48:52])[0]
    rmask = struct.unpack(be, xwd[56:60])[0]
    gmask = struct.unpack(be, xwd[60:64])[0]
    bmask = struct.unpack(be, xwd[64:68])[0]
    Bpp = bpp//8
    def shift(m):
        s=0
        if m==0: return 0,0
        while not (m>>s)&1: s+=1
        width=0; mm=m>>s
        while (mm>>width)&1: width+=1
        return s,width
    rs,_=shift(rmask); gs,_=shift(gmask); bs,_=shift(bmask)
    px=xwd[hdr:]
    img=Image.new('RGB',(w,h)); pd=img.load()
    for y in range(h):
        row=y*bpl
        for x in range(w):
            o=row+x*Bpp
            if Bpp==4: v=struct.unpack('<I',px[o:o+4])[0]
            elif Bpp==3: v=px[o]|(px[o+1]<<8)|(px[o+2]<<16)
            else: v=struct.unpack('<H',px[o:o+2])[0]
            r=(v&rmask)>>rs; g=(v&gmask)>>gs; b=(v&bmask)>>bs
            pd[x,y]=(r&0xff,g&0xff,b&0xff)
    if crop: img=img.crop(crop)
    if scale!=1: img=img.resize((img.width*scale,img.height*scale),Image.NEAREST)
    img.save(out)
    print(f"{w}x{h} bpp={bpp} masks R={rmask:#x} G={gmask:#x} B={bmask:#x} -> {out}")

if __name__=='__main__':
    wid=sys.argv[1]; out=sys.argv[2]
    crop=None; scale=1
    if len(sys.argv)>3: crop=tuple(int(x) for x in sys.argv[3].split(','))
    if len(sys.argv)>4: scale=int(sys.argv[4])
    grab(wid,out,crop,scale)
