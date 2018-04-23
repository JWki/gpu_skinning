
bl_info = { 
    "name": "GT Mesh Exporter",
    "author": "Jascha Wedowski",
    "blender": (2, 79, 0), 
    "description": "Export .gtmesh files", 
    "category": "Import-Export"}


import bpy
from bpy_extras.io_utils import (
    ExportHelper,
    orientation_helper_factory,
    axis_conversion,
    )

GTMeshOrientationHelper = orientation_helper_factory("GTMeshOrientationHelper", axis_forward='Y', axis_up='Z')

class Vertex(object):
    __slots__="id", "position", "normal", "uvs", "blendWeights", "blendIndices"
    def __init__(self, id, position, normal, uvs, blendWeights, blendIndices):
        self.id = id
        self.position = position
        self.normal = normal
        self.uvs = uvs
        self.blendWeights = blendWeights
        self.blendIndices = blendIndices
    def get_tuple(self):
        position = tuple(self.position)
        normal = tuple(self.normal)
        uvs = tuple(self.uvs)
        blendWeights = tuple(self.blendWeights)
        blendIndices = tuple(self.blendIndices)
        return (position, normal, uvs, blendWeights, blendIndices)

class Face(object):
    __slots__="vertices"
    def __init__(self, vertices):
        self.vertices = vertices
    

def save(operator, context, filepath="", use_selection=True, global_matrix=None):
    
    file = open(filepath, 'wb')
    
    import bpy
    import mathutils
    import struct
    import bmesh
    from bpy_extras.io_utils import create_derived_objects, free_derived_objects

    if global_matrix is None:
        global_matrix = mathutils.Matrix()
    
    mesh_objects = [] 

    scene = context.scene
    if use_selection:
        objects = (ob for ob in scene.objects if ob.is_visible(scene) and ob.select)
    else:
        objects = (ob for ob in scene.objects if ob.is_visible(scene))

    for ob in objects:
        # get derived objects
        free, derived = create_derived_objects(scene, ob)

        if derived is None:
            continue

        for ob_derived, mat in derived:
            if ob.type not in {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META'}:
                continue

            try:
                data = ob_derived.to_mesh(scene, True, 'PREVIEW')
            except:
                data = None
            
            if data:
                matrix = global_matrix * mat
                data.transform(matrix)
                mesh_objects.append((ob_derived, data, matrix))

        if free:
            free_derived_objects(ob)

    # write number of meshes
    file.write(struct.pack('<I', len(mesh_objects)))

    id = 0
    for ob, blender_mesh, matrix in mesh_objects:

        # convert all faces to triangles and extract UVs
        bm = bmesh.new()
        bm.from_mesh(blender_mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method=0, ngon_method=0)
        bm.verts.index_update()

        bm.to_mesh(blender_mesh)
        bm.free()

        blender_mesh.calc_normals()             # calculate normals
        blender_mesh.update(calc_tessface=True)

        # extract faces
        faces = []
        for i, face in enumerate(blender_mesh.tessfaces):
            faceVertices = []
            for n, vertIdx in enumerate(face.vertices):
                position = (blender_mesh.vertices[vertIdx].co.x, blender_mesh.vertices[vertIdx].co.y, blender_mesh.vertices[vertIdx].co.z)
                normal = (blender_mesh.vertices[vertIdx].normal.x, blender_mesh.vertices[vertIdx].normal.y, blender_mesh.vertices[vertIdx].normal.z)
                tex = blender_mesh.tessface_uv_textures[0]
                uvs = (tex.data[i].uv[n][0], tex.data[i].uv[n][1])
                blendWeights = [1.0, 0.0, 0.0, 0.0]
                blendIndices = [0, 0, 0, 0]
                
                faceVertices.append(Vertex(vertIdx, position, normal, uvs, blendWeights, blendIndices))
            faces.append(Face(faceVertices))
        
        vertices = []
        indices = []

        indexMap = {}
        for f in faces:
            ind = []
            for v in f.vertices:
                vTuple = v.get_tuple()
                if vTuple in indexMap:
                    ind.append(indexMap[vTuple])
                else:
                    ind.append(len(vertices))
                    indexMap[vTuple] = len(vertices)
                    vertices.append(v)
            indices.append(ind[0])
            indices.append(ind[1])
            indices.append(ind[2])
        
        # write submesh id
        file.write(struct.pack('<I', id))
        # write number of vertices
        file.write(struct.pack('<I', len(vertices)))
        # write interleaved vertex data as (position)(normal)(uvs)
        for i, v in enumerate(vertices):
            # position
            file.write(struct.pack('<f', v.position[0]))
            file.write(struct.pack('<f', v.position[1]))
            file.write(struct.pack('<f', v.position[2]))
            # normal
            file.write(struct.pack('<f', v.normal[0]))
            file.write(struct.pack('<f', v.normal[1]))
            file.write(struct.pack('<f', v.normal[2]))
            # uvs
            file.write(struct.pack('<f', v.uvs[0]))
            file.write(struct.pack('<f', v.uvs[1]))
            # blend weights
            file.write(struct.pack('<f', v.blendWeights[0]))
            file.write(struct.pack('<f', v.blendWeights[1]))
            file.write(struct.pack('<f', v.blendWeights[2]))
            file.write(struct.pack('<f', v.blendWeights[3]))
            # blend indices
            file.write(struct.pack('<I', v.blendIndices[0]))
            file.write(struct.pack('<I', v.blendIndices[1]))
            file.write(struct.pack('<I', v.blendIndices[2]))
            file.write(struct.pack('<I', v.blendIndices[3]))
            
        # write indices
        file.write(struct.pack('<I', len(indices)))
        for i in indices:
            file.write(struct.pack('<I', i))
        id = id + 1
        
    file.close()
    return {'FINISHED'}

class ExportGTMesh(bpy.types.Operator, ExportHelper, GTMeshOrientationHelper):
    """Export GT Mesh Format"""
    bl_idname = "export_scene.gtmesh"
    bl_label = 'Export .gtmesh'

    filename_ext = ".gtmesh"
    
    def execute(self, context):
        keywords = self.as_keywords(ignore=("axis_forward",
                                            "axis_up",
                                            "filter_glob",
                                            "check_existing"
                                            ))

        global_matrix = axis_conversion(from_forward=self.axis_forward,
                                        from_up=self.axis_up,
                                        ).to_4x4()
        keywords["global_matrix"] = global_matrix
        return save(self, context, **keywords)

def menu_func_export(self, context):
    self.layout.operator(ExportGTMesh.bl_idname, text="GT Mesh (.gtmesh)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export)

def unregister():
    bpy.types.INFO_MT_file_export.remove(menu_func_export)
    bpy.utils.unregister_module(__name__)

    
#if __name__ == "__main__":
    register()