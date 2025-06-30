#!/usr/bin/env python3
"""
Generate simple test glTF files for validating the CorePulse glTF loader.
"""

import json
import struct
import os

def generate_cube_gltf():
    """Generate a simple cube glTF file"""
    
    # Cube vertices (positions, normals, texture coordinates)
    vertices = [
        # Front face
        [-1, -1,  1,  0,  0,  1,  0, 0],  # 0
        [ 1, -1,  1,  0,  0,  1,  1, 0],  # 1
        [ 1,  1,  1,  0,  0,  1,  1, 1],  # 2
        [-1,  1,  1,  0,  0,  1,  0, 1],  # 3
        
        # Back face
        [-1, -1, -1,  0,  0, -1,  1, 0],  # 4
        [-1,  1, -1,  0,  0, -1,  1, 1],  # 5
        [ 1,  1, -1,  0,  0, -1,  0, 1],  # 6
        [ 1, -1, -1,  0,  0, -1,  0, 0],  # 7
        
        # Top face
        [-1,  1, -1,  0,  1,  0,  0, 1],  # 8
        [-1,  1,  1,  0,  1,  0,  0, 0],  # 9
        [ 1,  1,  1,  0,  1,  0,  1, 0],  # 10
        [ 1,  1, -1,  0,  1,  0,  1, 1],  # 11
        
        # Bottom face
        [-1, -1, -1,  0, -1,  0,  1, 1],  # 12
        [ 1, -1, -1,  0, -1,  0,  0, 1],  # 13
        [ 1, -1,  1,  0, -1,  0,  0, 0],  # 14
        [-1, -1,  1,  0, -1,  0,  1, 0],  # 15
        
        # Right face
        [ 1, -1, -1,  1,  0,  0,  1, 0],  # 16
        [ 1,  1, -1,  1,  0,  0,  1, 1],  # 17
        [ 1,  1,  1,  1,  0,  0,  0, 1],  # 18
        [ 1, -1,  1,  1,  0,  0,  0, 0],  # 19
        
        # Left face
        [-1, -1, -1, -1,  0,  0,  0, 0],  # 20
        [-1, -1,  1, -1,  0,  0,  1, 0],  # 21
        [-1,  1,  1, -1,  0,  0,  1, 1],  # 22
        [-1,  1, -1, -1,  0,  0,  0, 1],  # 23
    ]
    
    # Cube indices (triangles)
    indices = [
        # Front face
        0, 1, 2,   2, 3, 0,
        # Back face
        4, 5, 6,   6, 7, 4,
        # Top face
        8, 9, 10,  10, 11, 8,
        # Bottom face
        12, 13, 14, 14, 15, 12,
        # Right face
        16, 17, 18, 18, 19, 16,
        # Left face
        20, 21, 22, 22, 23, 20,
    ]
    
    # Create binary buffer data
    buffer_data = b''
    
    # Add vertex data
    vertex_data_start = len(buffer_data)
    for vertex in vertices:
        for component in vertex:
            buffer_data += struct.pack('<f', component)  # Little-endian float
    
    # Add index data (align to 4-byte boundary)
    while len(buffer_data) % 4 != 0:
        buffer_data += b'\x00'
    
    index_data_start = len(buffer_data)
    for index in indices:
        buffer_data += struct.pack('<H', index)  # Little-endian unsigned short
    
    # Align to 4-byte boundary
    while len(buffer_data) % 4 != 0:
        buffer_data += b'\x00'
    
    buffer_length = len(buffer_data)
    
    # Create the glTF JSON
    gltf = {
        "asset": {
            "version": "2.0",
            "generator": "CorePulse Test Generator"
        },
        "scene": 0,
        "scenes": [
            {
                "name": "Test Scene",
                "nodes": [0]
            }
        ],
        "nodes": [
            {
                "name": "Cube",
                "mesh": 0,
                "translation": [0, 0, 0],
                "rotation": [0, 0, 0, 1],
                "scale": [1, 1, 1]
            }
        ],
        "meshes": [
            {
                "name": "Cube Mesh",
                "primitives": [
                    {
                        "attributes": {
                            "POSITION": 0,
                            "NORMAL": 1,
                            "TEXCOORD_0": 2
                        },
                        "indices": 3,
                        "material": 0
                    }
                ]
            }
        ],
        "materials": [
            {
                "name": "Default Material",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.8, 0.2, 0.2, 1.0],
                    "metallicFactor": 0.1,
                    "roughnessFactor": 0.8
                }
            }
        ],
        "accessors": [
            {
                "name": "Position Accessor",
                "bufferView": 0,
                "byteOffset": 0,
                "componentType": 5126,  # FLOAT
                "count": 24,
                "type": "VEC3",
                "min": [-1, -1, -1],
                "max": [1, 1, 1]
            },
            {
                "name": "Normal Accessor",
                "bufferView": 0,
                "byteOffset": 12,  # 3 * sizeof(float)
                "componentType": 5126,  # FLOAT
                "count": 24,
                "type": "VEC3"
            },
            {
                "name": "TexCoord Accessor",
                "bufferView": 0,
                "byteOffset": 24,  # 6 * sizeof(float)
                "componentType": 5126,  # FLOAT
                "count": 24,
                "type": "VEC2"
            },
            {
                "name": "Index Accessor",
                "bufferView": 1,
                "byteOffset": 0,
                "componentType": 5123,  # UNSIGNED_SHORT
                "count": 36,
                "type": "SCALAR"
            }
        ],
        "bufferViews": [
            {
                "name": "Vertex Buffer View",
                "buffer": 0,
                "byteOffset": vertex_data_start,
                "byteLength": 24 * 8 * 4,  # 24 vertices * 8 components * 4 bytes
                "byteStride": 32,  # 8 components * 4 bytes
                "target": 34962  # ARRAY_BUFFER
            },
            {
                "name": "Index Buffer View",
                "buffer": 0,
                "byteOffset": index_data_start,
                "byteLength": 36 * 2,  # 36 indices * 2 bytes
                "target": 34963  # ELEMENT_ARRAY_BUFFER
            }
        ],
        "buffers": [
            {
                "name": "Cube Buffer",
                "uri": "cube.bin",
                "byteLength": buffer_length
            }
        ]
    }
    
    return gltf, buffer_data

def generate_simple_mech_gltf(name, color, scale_factor=1.0):
    """Generate a simple mech-like model using basic shapes"""
    
    # Simple mech structure: main body (cube) + legs (smaller cubes)
    vertices = []
    indices = []
    vertex_offset = 0
    
    def add_cube_at_position(pos, scale, vertices, indices, vertex_offset):
        """Add a cube at a specific position and scale"""
        base_vertices = [
            # Front face
            [-1, -1,  1,  0,  0,  1,  0, 0],
            [ 1, -1,  1,  0,  0,  1,  1, 0],
            [ 1,  1,  1,  0,  0,  1,  1, 1],
            [-1,  1,  1,  0,  0,  1,  0, 1],
            # Back face
            [-1, -1, -1,  0,  0, -1,  1, 0],
            [-1,  1, -1,  0,  0, -1,  1, 1],
            [ 1,  1, -1,  0,  0, -1,  0, 1],
            [ 1, -1, -1,  0,  0, -1,  0, 0],
            # Top face
            [-1,  1, -1,  0,  1,  0,  0, 1],
            [-1,  1,  1,  0,  1,  0,  0, 0],
            [ 1,  1,  1,  0,  1,  0,  1, 0],
            [ 1,  1, -1,  0,  1,  0,  1, 1],
            # Bottom face
            [-1, -1, -1,  0, -1,  0,  1, 1],
            [ 1, -1, -1,  0, -1,  0,  0, 1],
            [ 1, -1,  1,  0, -1,  0,  0, 0],
            [-1, -1,  1,  0, -1,  0,  1, 0],
            # Right face
            [ 1, -1, -1,  1,  0,  0,  1, 0],
            [ 1,  1, -1,  1,  0,  0,  1, 1],
            [ 1,  1,  1,  1,  0,  0,  0, 1],
            [ 1, -1,  1,  1,  0,  0,  0, 0],
            # Left face
            [-1, -1, -1, -1,  0,  0,  0, 0],
            [-1, -1,  1, -1,  0,  0,  1, 0],
            [-1,  1,  1, -1,  0,  0,  1, 1],
            [-1,  1, -1, -1,  0,  0,  0, 1],
        ]
        
        # Transform vertices
        for vertex in base_vertices:
            new_vertex = [
                vertex[0] * scale[0] + pos[0],  # X position
                vertex[1] * scale[1] + pos[1],  # Y position  
                vertex[2] * scale[2] + pos[2],  # Z position
                vertex[3], vertex[4], vertex[5],  # Normal
                vertex[6], vertex[7]  # UV
            ]
            vertices.append(new_vertex)
        
        # Add indices with offset
        cube_indices = [
            0, 1, 2,   2, 3, 0,    4, 5, 6,   6, 7, 4,
            8, 9, 10,  10, 11, 8,  12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20,
        ]
        
        for idx in cube_indices:
            indices.append(idx + vertex_offset)
        
        return vertex_offset + 24
    
    # Main body (torso)
    vertex_offset = add_cube_at_position([0, 2, 0], [1.5, 2, 1], vertices, indices, vertex_offset)
    
    # Left leg
    vertex_offset = add_cube_at_position([-0.8, -1, 0], [0.6, 2, 0.6], vertices, indices, vertex_offset)
    
    # Right leg  
    vertex_offset = add_cube_at_position([0.8, -1, 0], [0.6, 2, 0.6], vertices, indices, vertex_offset)
    
    # Head/Cockpit
    vertex_offset = add_cube_at_position([0, 4.5, 0.3], [0.8, 0.8, 0.8], vertices, indices, vertex_offset)
    
    # Scale entire model
    if scale_factor != 1.0:
        for vertex in vertices:
            vertex[0] *= scale_factor
            vertex[1] *= scale_factor  
            vertex[2] *= scale_factor
    
    # Create binary buffer data
    buffer_data = b''
    
    # Add vertex data
    vertex_data_start = len(buffer_data)
    for vertex in vertices:
        for component in vertex:
            buffer_data += struct.pack('<f', component)
    
    # Add index data (align to 4-byte boundary)
    while len(buffer_data) % 4 != 0:
        buffer_data += b'\x00'
    
    index_data_start = len(buffer_data)
    for index in indices:
        buffer_data += struct.pack('<H', index)
    
    # Align to 4-byte boundary
    while len(buffer_data) % 4 != 0:
        buffer_data += b'\x00'
    
    buffer_length = len(buffer_data)
    vertex_count = len(vertices)
    index_count = len(indices)
    
    # Calculate bounds
    positions = [[v[0], v[1], v[2]] for v in vertices]
    min_pos = [min(p[i] for p in positions) for i in range(3)]
    max_pos = [max(p[i] for p in positions) for i in range(3)]
    
    # Create mech-specific extensions
    hardpoints_extension = {
        "hardpoints": [
            {
                "id": f"{name.lower().replace(' ', '_')}_arm_energy_1",
                "name": "Left Arm Energy Hardpoint",
                "type": "ENERGY",
                "size": "MEDIUM",
                "position": [-1.5 * scale_factor, 2.5 * scale_factor, 0.5 * scale_factor],
                "orientation": [0, 0, 0],
                "max_tonnage": 5.0 if scale_factor > 1.0 else 3.0,
                "critical_slots": 2,
                "attachment_node": "left_arm_mount"
            },
            {
                "id": f"{name.lower().replace(' ', '_')}_arm_ballistic_1",
                "name": "Right Arm Ballistic Hardpoint",
                "type": "BALLISTIC",
                "size": "LARGE" if scale_factor > 1.2 else "MEDIUM",
                "position": [1.5 * scale_factor, 2.5 * scale_factor, 0.5 * scale_factor],
                "orientation": [0, 0, 0],
                "max_tonnage": 8.0 if scale_factor > 1.0 else 5.0,
                "critical_slots": 3,
                "attachment_node": "right_arm_mount"
            }
        ]
    }
    
    damage_zones_extension = {
        "zones": [
            {
                "id": f"{name.lower().replace(' ', '_')}_head",
                "name": "Head",
                "type": "HEAD",
                "max_armor": 9.0,
                "max_internal": 3.0,
                "bounds_min": [-0.8 * scale_factor, 4.0 * scale_factor, -0.8 * scale_factor],
                "bounds_max": [0.8 * scale_factor, 5.2 * scale_factor, 0.8 * scale_factor],
                "total_slots": 6,
                "destruction_effects": ["cockpit_breach", "sensor_damage"]
            },
            {
                "id": f"{name.lower().replace(' ', '_')}_center_torso",
                "name": "Center Torso",
                "type": "CENTER_TORSO",
                "max_armor": 20.0 * scale_factor,
                "max_internal": 15.0 * scale_factor,
                "bounds_min": [-1.5 * scale_factor, 1.0 * scale_factor, -1.0 * scale_factor],
                "bounds_max": [1.5 * scale_factor, 3.0 * scale_factor, 1.0 * scale_factor],
                "total_slots": 12,
                "destruction_effects": ["engine_shutdown", "mech_destruction"]
            },
            {
                "id": f"{name.lower().replace(' ', '_')}_left_arm",
                "name": "Left Arm",
                "type": "LEFT_ARM",
                "max_armor": 12.0 * scale_factor,
                "max_internal": 8.0 * scale_factor,
                "bounds_min": [-2.2 * scale_factor, 1.5 * scale_factor, -0.6 * scale_factor],
                "bounds_max": [-0.8 * scale_factor, 3.5 * scale_factor, 0.6 * scale_factor],
                "total_slots": 8,
                "destruction_effects": ["weapon_loss", "actuator_damage"]
            },
            {
                "id": f"{name.lower().replace(' ', '_')}_right_arm",
                "name": "Right Arm", 
                "type": "RIGHT_ARM",
                "max_armor": 12.0 * scale_factor,
                "max_internal": 8.0 * scale_factor,
                "bounds_min": [0.8 * scale_factor, 1.5 * scale_factor, -0.6 * scale_factor],
                "bounds_max": [2.2 * scale_factor, 3.5 * scale_factor, 0.6 * scale_factor],
                "total_slots": 8,
                "destruction_effects": ["weapon_loss", "actuator_damage"]
            }
        ]
    }

    # Create the glTF JSON
    gltf = {
        "asset": {
            "version": "2.0",
            "generator": "CorePulse Mech Generator"
        },
        "scene": 0,
        "scenes": [
            {
                "name": f"{name} Scene",
                "nodes": [0]
            }
        ],
        "nodes": [
            {
                "name": name,
                "mesh": 0,
                "translation": [0, 0, 0],
                "rotation": [0, 0, 0, 1],
                "scale": [1, 1, 1]
            }
        ],
        "meshes": [
            {
                "name": f"{name} Mesh",
                "primitives": [
                    {
                        "attributes": {
                            "POSITION": 0,
                            "NORMAL": 1,
                            "TEXCOORD_0": 2
                        },
                        "indices": 3,
                        "material": 0
                    }
                ]
            }
        ],
        "materials": [
            {
                "name": f"{name} Material",
                "pbrMetallicRoughness": {
                    "baseColorFactor": color,
                    "metallicFactor": 0.3,
                    "roughnessFactor": 0.7
                }
            }
        ],
        "accessors": [
            {
                "name": "Position Accessor",
                "bufferView": 0,
                "byteOffset": 0,
                "componentType": 5126,  # FLOAT
                "count": vertex_count,
                "type": "VEC3",
                "min": min_pos,
                "max": max_pos
            },
            {
                "name": "Normal Accessor", 
                "bufferView": 0,
                "byteOffset": 12,
                "componentType": 5126,  # FLOAT
                "count": vertex_count,
                "type": "VEC3"
            },
            {
                "name": "TexCoord Accessor",
                "bufferView": 0,
                "byteOffset": 24,
                "componentType": 5126,  # FLOAT
                "count": vertex_count,
                "type": "VEC2"
            },
            {
                "name": "Index Accessor",
                "bufferView": 1,
                "byteOffset": 0,
                "componentType": 5123,  # UNSIGNED_SHORT
                "count": index_count,
                "type": "SCALAR"
            }
        ],
        "bufferViews": [
            {
                "name": "Vertex Buffer View",
                "buffer": 0,
                "byteOffset": vertex_data_start,
                "byteLength": vertex_count * 8 * 4,
                "byteStride": 32,
                "target": 34962  # ARRAY_BUFFER
            },
            {
                "name": "Index Buffer View", 
                "buffer": 0,
                "byteOffset": index_data_start,
                "byteLength": index_count * 2,
                "target": 34963  # ELEMENT_ARRAY_BUFFER
            }
        ],
        "buffers": [
            {
                "name": f"{name} Buffer",
                "uri": f"{name.lower().replace(' ', '_')}.bin",
                "byteLength": buffer_length
            }
        ],
        "extensions": {
            "CP_walker_hardpoints": hardpoints_extension,
            "CP_damage_zones": damage_zones_extension
        },
        "extensionsUsed": ["CP_walker_hardpoints", "CP_damage_zones"]
    }
    
    print(f"DEBUG: About to return glTF for {name} with keys: {list(gltf.keys())}")
    print(f"DEBUG: Extensions present: {'extensions' in gltf}")
    
    return gltf, buffer_data

def main():
    # Create assets/models directory structure
    models_dir = "../assets/models"
    walkers_dir = f"{models_dir}/walkers"
    weapons_dir = f"{models_dir}/weapons"
    
    os.makedirs(models_dir, exist_ok=True)
    os.makedirs(walkers_dir, exist_ok=True)
    os.makedirs(weapons_dir, exist_ok=True)
    
    print("Generating test glTF files for CorePulse...")
    
    # Generate basic cube (for testing)
    cube_gltf, cube_buffer = generate_cube_gltf()
    
    with open(f"{models_dir}/cube.gltf", 'w') as f:
        json.dump(cube_gltf, f, indent=2)
    with open(f"{models_dir}/cube.bin", 'wb') as f:
        f.write(cube_buffer)
    
    # Generate test mech models
    mech_configs = [
        ("Light Mech", [0.2, 0.8, 0.2, 1.0], 0.8),   # Green, smaller
        ("Medium Mech", [0.2, 0.2, 0.8, 1.0], 1.0),  # Blue, normal size
        ("Heavy Mech", [0.8, 0.2, 0.2, 1.0], 1.3),   # Red, larger
    ]
    
    for name, color, scale in mech_configs:
        print(f"Generating {name}...")
        mech_gltf, mech_buffer = generate_simple_mech_gltf(name, color, scale)
        
        filename = name.lower().replace(' ', '_')
        
        # Debug: Write to a temporary file first
        temp_file = f"/tmp/{filename}_debug.gltf"
        with open(temp_file, 'w') as f:
            json.dump(mech_gltf, f, indent=2)
        
        # Check the temp file
        with open(temp_file, 'r') as f:
            temp_content = f.read()
            has_extensions = "extensions" in temp_content
            print(f"Temp file for {filename} has extensions: {has_extensions}")
        
        with open(f"{walkers_dir}/{filename}.gltf", 'w') as f:
            # Debug: Print the keys in the glTF object before writing
            print(f"glTF keys for {filename}: {list(mech_gltf.keys())}")
            json.dump(mech_gltf, f, indent=2)
        with open(f"{walkers_dir}/{filename}.bin", 'wb') as f:
            f.write(mech_buffer)
        
        # Debug: Check if extensions are in the generated JSON
        print(f"Extensions in {filename}: {'extensions' in mech_gltf}")
        if 'extensions' in mech_gltf:
            print(f"  Hardpoints: {'CP_walker_hardpoints' in mech_gltf['extensions']}")
            print(f"  Damage zones: {'CP_damage_zones' in mech_gltf['extensions']}")
            if 'CP_walker_hardpoints' in mech_gltf['extensions']:
                print(f"  Hardpoint count: {len(mech_gltf['extensions']['CP_walker_hardpoints']['hardpoints'])}")
            if 'CP_damage_zones' in mech_gltf['extensions']:
                print(f"  Damage zone count: {len(mech_gltf['extensions']['CP_damage_zones']['zones'])}")
    
    # Generate simple weapon models
    weapon_gltf, weapon_buffer = generate_cube_gltf()
    # Modify for weapon (elongated cube)
    weapon_gltf["nodes"][0]["name"] = "Laser Cannon"
    weapon_gltf["nodes"][0]["scale"] = [3.0, 0.3, 0.3]  # Long and thin
    weapon_gltf["materials"][0]["name"] = "Weapon Material"
    weapon_gltf["materials"][0]["pbrMetallicRoughness"]["baseColorFactor"] = [0.6, 0.6, 0.6, 1.0]
    weapon_gltf["materials"][0]["pbrMetallicRoughness"]["metallicFactor"] = 0.8
    weapon_gltf["buffers"][0]["uri"] = "laser_cannon.bin"
    
    with open(f"{weapons_dir}/laser_cannon.gltf", 'w') as f:
        json.dump(weapon_gltf, f, indent=2)
    with open(f"{weapons_dir}/laser_cannon.bin", 'wb') as f:
        f.write(weapon_buffer)
    
    print(f"\nGenerated test glTF files:")
    print(f"  {models_dir}/cube.gltf - Basic cube")
    print(f"  {walkers_dir}/light_mech.gltf - Light mech")
    print(f"  {walkers_dir}/medium_mech.gltf - Medium mech")  
    print(f"  {walkers_dir}/heavy_mech.gltf - Heavy mech")
    print(f"  {weapons_dir}/laser_cannon.gltf - Weapon model")
    print(f"\nAssets ready for CorePulse AssetManager testing!")

if __name__ == "__main__":
    main()