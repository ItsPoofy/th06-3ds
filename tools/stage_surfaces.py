"""Keep the stage-one river beneath the banks at every authored camera angle."""
import math
import struct


def quads(data, object_offset):
    offset = object_offset + 28
    result = []
    while struct.unpack_from('<h', data, offset)[0] >= 0:
        quad = struct.unpack_from('<4h5f', data, offset)
        result.append(quad)
        offset += quad[1]
    return result, offset


def ray_slopes(data, script_offset):
    # Match GameManager::SetupCamera, including its pitch. A margin derived
    # only from FOV ignores parallax when looking along the river.
    tangent = math.tan(math.radians(15))
    distance = 224 / tangent
    facing = [(0, 0, 1)]
    for offset in range(script_offset, len(data) - 19, 20):
        _, opcode, _ = struct.unpack_from('<ihh', data, offset)
        if opcode == 2:
            facing.append(struct.unpack_from('<3f', data, offset + 8))
    slopes = []
    for x, y, z in facing:
        z *= distance
        length = math.sqrt(x*x + y*y + z*z)
        fx, fy, fz = x/length, y/length, z/length
        horizontal = math.hypot(fx, fz)
        rx, rz = fz/horizontal, -fx/horizontal
        ux, uy, uz = fy*rz, fz*rx - fx*rz, -fy*rx
        for sx in (-tangent*384/448, tangent*384/448):
            for sy in (-tangent, tangent):
                dx, dy, dz = fx+sx*rx+sy*ux, fy+sy*uy, fz+sx*rz+sy*uz
                if dz > 0:
                    slopes.append((dx/dz, -dy/dz, z))
    return slopes


def prepare(name, data):
    if name != 'stage1.std':
        return data
    count, faces, instances_offset, script_offset, _ = struct.unpack_from('<hhiii', data)
    if count <= 9:
        return data
    objects = [struct.unpack_from('<I', data, 1168+i*4)[0] for i in range(count)]
    instances = []
    offset = instances_offset
    while struct.unpack_from('<h', data, offset)[0] >= 0:
        instances.append(struct.unpack_from('<hh3f', data, offset))
        offset += 16
    water = sorted((item for item in instances if item[0] == 9), key=lambda item: item[3])
    banks = [item for item in instances if item[0] in (7, 8)]
    if len(water) < 2 or not banks:
        return data
    tiles, tile_end = quads(data, objects[9])
    slopes = ray_slopes(data, script_offset)
    required = []
    for bank in banks:
        bank_tiles, _ = quads(data, objects[bank[0]])
        for tile in bank_tiles:
            dz = water[0][4] - bank[4] - tile[6]
            for dx, dy, _ in slopes:
                required.append((bank[2]+tile[4]+dz*dx, bank[3]+tile[5]+dz*dy))
                required.append((bank[2]+tile[4]+tile[7]+dz*dx, bank[3]+tile[5]+tile[8]+dz*dy))
    width = tiles[0][7]
    left = min(tile[4] for tile in tiles)
    right = max(tile[4]+tile[7] for tile in tiles)
    minimum_x = min(point[0] for point in required) - water[0][2]
    maximum_x = max(point[0] for point in required) - water[0][2]
    # The river itself also has to reach both sides of the viewport, including
    # the space beyond the cutout banks. Bank bounds alone miss those corners.
    camera_positions = [(0, 0, 0)]
    for position in range(script_offset, len(data)-19, 20):
        _, opcode, _ = struct.unpack_from('<ihh', data, position)
        if opcode == 0:
            camera_positions.append(struct.unpack_from('<3f', data, position+8))
    minimum_y = min(point[1] for point in required) - water[0][3]
    maximum_y = max(point[1] for point in required) - water[0][3]
    for x, y, z in camera_positions:
        for dx, dy, camera_distance in slopes:
            projected_x = x+192+(water[0][4]-z+camera_distance)*dx-water[0][2]
            minimum_x = min(minimum_x, projected_x)
            maximum_x = max(maximum_x, projected_x)
            projected_y = y+224+(water[0][4]-z+camera_distance)*dy
            minimum_y = min(minimum_y, projected_y-water[0][3])
            maximum_y = max(maximum_y, projected_y-water[0][3])
    added_tiles = []
    while left > minimum_x:
        left -= width
        tile = list(tiles[0]); tile[4] = left
        added_tiles.append(struct.pack('<4h5f', *tile))
    while right < maximum_x:
        tile = list(tiles[-1]); tile[4] = right
        added_tiles.append(struct.pack('<4h5f', *tile))
        right += width
    added = b''.join(added_tiles)
    result = bytearray(data[:tile_end] + added + data[tile_end:])
    if added:
        struct.pack_into('<h', result, 2, faces + len(added_tiles))
        for i, object_offset in enumerate(objects):
            if object_offset >= tile_end:
                struct.pack_into('<I', result, 1168+i*4, object_offset+len(added))
        # Retain the authored loose culling margin around the enlarged mesh.
        struct.pack_into('<f', result, objects[9]+4, left-32)
        struct.pack_into('<f', result, objects[9]+16, right-left+64)
    instances_offset += len(added)
    script_offset += len(added)
    offset += len(added)
    step = min(b[3]-a[3] for a, b in zip(water, water[1:]) if b[3] > a[3])
    extra = []
    y = water[0][3]
    while y > minimum_y+water[0][3]:
        y -= step
        extra.append((*water[0][:3], y, water[0][4]))
    y = water[-1][3]
    while y+step < maximum_y+water[0][3]:
        y += step
        extra.append((*water[-1][:3], y, water[-1][4]))
    added = b''.join(struct.pack('<hh3f', *item) for item in extra)
    result[offset:offset] = added
    struct.pack_into('<ii', result, 4, instances_offset, script_offset+len(added))
    return bytes(result)
