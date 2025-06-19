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

def main():
    # Create assets/models directory if it doesn't exist
    models_dir = "../assets/models"
    os.makedirs(models_dir, exist_ok=True)
    
    print("Generating test glTF files for CorePulse...")
    
    # Generate cube
    cube_gltf, cube_buffer = generate_cube_gltf()
    
    # Write glTF JSON file
    with open(f"{models_dir}/cube.gltf", 'w') as f:
        json.dump(cube_gltf, f, indent=2)
    
    # Write binary buffer file
    with open(f"{models_dir}/cube.bin", 'wb') as f:
        f.write(cube_buffer)
    
    print(f"Generated test glTF files:")
    print(f"  {models_dir}/cube.gltf - Simple cube model")
    print(f"  {models_dir}/cube.bin - Binary vertex/index data")
    print(f"Buffer size: {len(cube_buffer)} bytes")
    print(f"Vertices: 24, Indices: 36")

if __name__ == "__main__":
    main()