from pathlib import Path
import json
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '10daysjam/Resoures/images/mouth_v2'
OUT.mkdir(parents=True, exist_ok=True)
source = Image.open(ROOT / 'design/pixel_snapper/mouth_modules_v2_snapped.png').convert('RGBA')
NEAR = Image.Resampling.NEAREST
outline = (23, 21, 28, 255)
palettes = {'peach': [(255, 179, 168, 255), (239, 126, 131, 255), (184, 82, 102, 255)],
            'lavender': [(233, 193, 233, 255), (198, 143, 198, 255), (154, 97, 158, 255)]}

def isolated(row, col):
    tile = source.crop((col*32+1, row*32+1, col*32+31, row*32+31))
    mask = Image.new('L', tile.size)
    for y in range(tile.height):
        for x in range(tile.width):
            r,g,b,a = tile.getpixel((x,y))
            if max(r,g,b)-min(r,g,b)>30 and r>80:
                mask.putpixel((x,y),255)
    return mask.crop(mask.getbbox())

def paint(mask, palette):
    # Restore a consistent one-pixel contour after snapping; preserve the generated silhouette.
    expanded = mask.filter(ImageFilter.MaxFilter(3))
    result = Image.new('RGBA',(32,32))
    for y in range(32):
        for x in range(32):
            if expanded.getpixel((x,y)):
                color=outline
                if mask.getpixel((x,y)):
                    color=palette[1]
                    if y==0 or not mask.getpixel((x,y-1)): color=palette[0]
                    elif y==31 or not mask.getpixel((x,y+1)): color=palette[2]
                result.putpixel((x,y),color)
    return result

all_tiles={}
for row,(name,palette) in enumerate(palettes.items()):
    straight=Image.new('L',(32,32))
    straight.paste(isolated(row,0).resize((32,6),NEAR),(0,13))
    # Repeat a clean interior cross-section to remove source-frame marks at seams.
    for x in range(32):
        for y in range(13,19): straight.putpixel((x,y),255)
    elbow=Image.new('L',(32,32))
    elbow.paste(isolated(row,1).resize((19,19),NEAR),(0,13))
    # Register both open ports to the same six-pixel interior (13..18).
    for x in range(11):
        for y in range(32): elbow.putpixel((x,y),255 if 13<=y<19 else 0)
    for y in range(23,32):
        for x in range(32): elbow.putpixel((x,y),255 if 13<=x<19 else 0)
    root=Image.new('L',(32,32))
    rootshape=isolated(row,2).resize((32,12),NEAR)
    root.paste(rootshape,(0,10))
    last_width=12
    for x in range(32):
        width=sum(root.getpixel((x,y))>0 for y in range(32))
        width=max(6,min(last_width,width))
        width=2*(width//2)
        last_width=width
        for y in range(32): root.putpixel((x,y),255 if 16-width//2<=y<16+width//2 else 0)
    for x in range(28,32):
        for y in range(32): root.putpixel((x,y),255 if 13<=y<19 else 0)
    h=paint(straight,palette)
    c=paint(elbow,palette)
    tiles={'straight_h':h,'straight_v':h.transpose(Image.Transpose.ROTATE_270),
           'corner_left_down':c,'corner_up_left':c.transpose(Image.Transpose.ROTATE_270),
           'corner_right_up':c.transpose(Image.Transpose.ROTATE_180),
           'corner_down_right':c.transpose(Image.Transpose.ROTATE_90),
           'root_right':paint(root,palette)}
    sheet=Image.new('RGBA',(96*len(tiles),96))
    for i,(part,tile) in enumerate(tiles.items()):
        tile.save(OUT/f'{name}_{part}_32.png')
        enlarged=tile.resize((96,96),NEAR)
        enlarged.save(OUT/f'{name}_{part}.png')
        sheet.paste(enlarged,(i*96,0))
    sheet.save(OUT/f'{name}_sheet.png')
    all_tiles[name]=tiles

# Preview against the actual floor and lips. This is an asset assembly, not a game screenshot.
preview=Image.new('RGBA',(960,432),(21,25,35,255))
floor=Image.open(ROOT/'10daysjam/Resoures/images/floor2.png').convert('RGBA').crop((6,6,90,90)).resize((96,96),NEAR)
for row,(name,tiles) in enumerate(all_tiles.items()):
    ox,oy=48,24+row*216
    for x,y in [(0,0),(1,0),(2,0),(3,0),(3,1),(4,1),(5,1),(6,1),(7,1),(8,1)]:
        preview.alpha_composite(floor,(ox+x*96,oy+y*96))
    placements=[('straight_h',1,0),('straight_h',2,0),('corner_left_down',3,0),
                ('corner_right_up',3,1),('straight_h',4,1),('straight_h',5,1),
                ('straight_h',6,1),('straight_h',7,1)]
    for part,x,y in placements:
        preview.alpha_composite(tiles[part].resize((96,96),NEAR),(ox+x*96,oy+y*96))
    # Keep the original face and lip art, aligning their existing mouth center (y=57).
    character=Image.open(ROOT/'10daysjam/Resoures/images/playerBoy.png').convert('RGBA').crop((0,0,96,96))
    root_preview=tiles['root_right'].resize((96,96),NEAR)
    preview.alpha_composite(root_preview.crop((72,0,96,96)),(ox+72,oy))
    preview.alpha_composite(character,(ox,oy-9))
    lip=Image.open(ROOT/'10daysjam/Resoures/images/Lips.png').convert('RGBA')
    preview.alpha_composite(lip,(ox+7*96+33,oy+96-9))
preview.save(ROOT/'design/mouth-v2/assembly_preview.png')

manifest={'frame_size':[96,96],'logical_size':[32,32],'pixel_scale':3,
          'frame_order':list(all_tiles['peach']),'sheet_size':[672,96],
          'ports':{'center':[48,48],'outer_width':24,'interior_width':18},
          'source':'Built-in ImageGen followed by official Pixel Snapper and port/outline cleanup',
          'status':'asset package only; not loaded by main.cpp'}
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(manifest))
