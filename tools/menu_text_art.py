"""Draw English menu text in the style of an original Japanese sprite.

measure() records how the original sprite colors each pixel, keyed by:
- the sprite row, which carries vertical gradients
- the signed distance to the edge of the solid glyph area, which carries
  outlines inside the edge and glows outside it
- the direction the nearest edge faces, which carries bevel highlights
render() colors new glyphs by looking up the same keys. English text drawn at
the original size and baseline therefore takes the original's gradient,
outline, bevel and glow.
"""
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

SS = 4
SOLID = 238
# Signed distance buckets in half pixels, from 6 px outside to 4 px inside.
DISTANCES = np.arange(-12, 9)


def features(area_hi, sub_pixels=False):
    """Signed distance and edge direction for a supersampled mask.

    Returns one value per pixel, or one per sub-pixel when sub_pixels is true.
    """
    inside = ndimage.distance_transform_edt(area_hi)/SS
    outside = ndimage.distance_transform_edt(~area_hi)/SS
    smooth = ndimage.gaussian_filter(area_hi.astype(float), 1.5*SS)
    gy, gx = np.gradient(smooth)
    # Sub-pixel distances are measured from the edge between solid and clear
    # sub-pixels, so shift them by half a sub-pixel toward zero.
    signed = inside - outside + np.where(area_hi, -.5, .5)/SS
    if not sub_pixels:
        signed = (inside - outside)[SS//2::SS, SS//2::SS]
        gy, gx = gy[SS//2::SS, SS//2::SS], gx[SS//2::SS, SS//2::SS]
    length = np.hypot(gx, gy) + 1e-9
    # Coverage rises toward the glyph, so the gradient points inward.
    # direction 0: edge faces up, 1: sideways, 2: edge faces down.
    ny = gy/length
    direction = np.where(ny > .4, 0, np.where(ny < -.4, 2, 1))
    # Edge direction matters only near the edge. Deeper pixels share one key.
    direction = np.where(np.abs(signed) <= 1.5, direction, 1)
    bucket = np.clip(np.round(signed*2).astype(int), DISTANCES[0], DISTANCES[-1]) - DISTANCES[0]
    return bucket, direction


def measure(sprite, ignore=None):
    """Build the color table of an original sprite.

    Pixels where ignore is true belong to other text and are left out.
    """
    pixels = np.asarray(sprite.convert('RGBA')).astype(float)
    alpha = pixels[..., 3:]/255
    premultiplied = pixels[..., :3]*alpha
    area = np.kron(pixels[..., 3] >= SOLID, np.ones((SS, SS))).astype(bool)
    bucket, direction = features(area)
    valid = np.ones(alpha.shape[:2], bool) if ignore is None else ~ignore
    height = len(pixels)
    sums = np.zeros((height, len(DISTANCES), 3, 5))
    for y in range(height):
        for x in np.nonzero(valid[y])[0]:
            sums[y, bucket[y, x], direction[y, x]] += [*premultiplied[y, x], alpha[y, x, 0], 1]
    return sums


def lookup(sums, y, bucket, direction):
    """Mean premultiplied color and alpha, widening the search when a key is empty."""
    for rows in (1, 2, 4, 8, 64):
        lo, hi = max(0, y - rows), y + rows + 1
        for dirs in ((direction,), (0, 1, 2)):
            for spread in (0, 1, 2):
                b0, b1 = max(0, bucket - spread), bucket + spread + 1
                total = sums[lo:hi, b0:b1][:, :, list(dirs)].sum((0, 1, 2))
                if total[4] > 0:
                    return total[:4]/total[4]
    return np.zeros(4)


def glyphs(text, font_path, size, x, y, width, height, scale_x=1.0, track=0.0):
    """Supersampled glyph mask with the text origin at (x, y).

    track adds that many pixels between characters.
    """
    font = ImageFont.truetype(str(font_path), round(size*SS))
    line = Image.new('L', (int(font.getlength(text) + max(track, 0)*SS*len(text)) + 16*SS, height*SS))
    draw = ImageDraw.Draw(line)
    if track:
        cursor = 0.0
        for character in text:
            draw.text((round(cursor), round(y*SS)), character, font=font, fill=255)
            cursor += font.getlength(character) + track*SS
    else:
        draw.text((0, round(y*SS)), text, font=font, fill=255)
    if scale_x != 1.0:
        line = line.resize((max(1, round(line.width*scale_x)), line.height), Image.Resampling.LANCZOS)
    mask = Image.new('L', (width*SS, height*SS))
    mask.paste(line, (round(x*SS), 0))
    return np.asarray(mask) > 127


def solid(glyph, outline):
    radius = round(outline*SS)
    if radius <= 0:
        return glyph
    y, x = np.mgrid[-radius:radius + 1, -radius:radius + 1]
    return ndimage.binary_dilation(glyph, structure=x*x + y*y <= radius*radius)


def coverage(mask):
    h, w = mask.shape[0]//SS, mask.shape[1]//SS
    return mask.reshape(h, SS, w, SS).mean((1, 3))


def ink_width(text, font_path, size, scale_x=1.0, track=0.0):
    mask = glyphs(text, font_path, size, 0, 0, 1024, round(size*2), scale_x, track)
    columns = np.nonzero(mask.any(0))[0]
    return (columns.max() + 1 - columns.min())/SS, columns.min()/SS


def fit(sprite, text, font_path, sizes, outlines=(0, .5, 1, 1.5), stretches=(1.0,), tracks=(0.0,)):
    """Find size, stretch, spacing, origin and outline that best cover the original solid area."""
    target = np.asarray(sprite.convert('RGBA'))[..., 3] >= SOLID
    height, width = target.shape
    rows, cols = np.nonzero(target.any(1))[0], np.nonzero(target.any(0))[0]
    best = (-1,)
    for size, stretch, track in ((s, st, tr) for s in sizes for st in stretches for tr in tracks):
            probe = glyphs(text, font_path, size, 0, 0, width + 64, height + 32, stretch, track)
            pr, pc = np.nonzero(probe.any(1))[0], np.nonzero(probe.any(0))[0]
            for outline in outlines:
                x0 = cols.min() + outline - pc.min()/SS
                y0 = rows.min() + outline - pr.min()/SS
                for dx in (-.5, 0, .5):
                    for dy in (-.5, 0, .5):
                        area = solid(glyphs(text, font_path, size, x0 + dx, y0 + dy, width, height, stretch, track), outline)
                        mask = coverage(area) >= .5
                        score = (mask & target).sum()/max((mask | target).sum(), 1)
                        if score > best[0]:
                            best = (score, size, stretch, track, x0 + dx, y0 + dy, outline)
    return dict(zip(('score', 'size', 'stretch', 'track', 'x', 'y', 'outline'), best))


def render(glyph, outline, sums, sharp=False):
    """Color a glyph mask from a measured color table. Returns an RGBA image.

    sharp colors each pixel at its center and blends the two nearest distance
    buckets, which keeps the crisp highlight and outline rows of the originals.
    Otherwise each sub-pixel takes its own color and the result is averaged down.
    """
    area = solid(glyph, outline)
    height, width = area.shape[0]//SS, area.shape[1]//SS
    cache = {}
    def color(y, bucket, direction):
        key = (y, bucket, direction)
        if key not in cache:
            cache[key] = lookup(sums, *key)
        return cache[key]
    if sharp:
        inside = ndimage.distance_transform_edt(area)/SS
        outside = ndimage.distance_transform_edt(~area)/SS
        signed = (inside - outside)[SS//2::SS, SS//2::SS]
        _, direction = features(area)
        position = np.clip(signed*2, DISTANCES[0], DISTANCES[-1]) - DISTANCES[0]
        low = np.floor(position).astype(int)
        high = np.minimum(low + 1, len(DISTANCES) - 1)
        weight = (position - low)[..., None]
        out = np.zeros((height, width, 4))
        for y in range(height):
            for x in range(width):
                d = direction[y, x]
                out[y, x] = color(y, low[y, x], d)*(1 - weight[y, x]) + color(y, high[y, x], d)*weight[y, x]
    else:
        bucket, direction = features(area, sub_pixels=True)
        rows = np.repeat(np.arange(height), SS)[:, None]*np.ones((1, width*SS), int)
        keys = np.stack([rows, bucket, direction], -1)
        unique, inverse = np.unique(keys.reshape(-1, 3), axis=0, return_inverse=True)
        table = np.array([color(*key) for key in unique])
        out = table[inverse.ravel()].reshape(height, SS, width, SS, 4).mean((1, 3))
    alpha = out[..., 3]
    rgb = np.where(alpha[..., None] > 1e-4, out[..., :3]/np.maximum(alpha[..., None], 1e-4), 0)
    def channel(v): return np.clip(np.round(v), 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack([channel(rgb), channel(alpha*255)]), 'RGBA')


def redraw(sprite, original_text, text, font_path, sizes, max_width=None, align='left',
           stretches=(1.0, 1.05, 1.1, 1.15, 1.2)):
    """Measure the original sprite and draw text in its place and style.

    The English text uses the fitted size, stretch and baseline. Text wider
    than max_width is condensed horizontally.
    """
    sums = measure(sprite)
    geometry = fit(sprite, original_text, font_path, sizes, stretches=stretches)
    width, height = sprite.size
    max_width = max_width or width - 2*geometry['x']
    ink, offset = ink_width(text, font_path, geometry['size'], geometry['stretch'])
    scale = min(1.0, max_width/ink)
    stretch = geometry['stretch']*scale
    if align == 'left':
        x = geometry['x']
    elif align == 'center':
        x = (width - ink*scale)/2 - offset*scale
    else:
        x = width - geometry['x'] - ink*scale - offset*scale
    glyph = glyphs(text, font_path, geometry['size'], x, geometry['y'], width, height, stretch)
    return render(glyph, geometry['outline'], sums), geometry, scale
