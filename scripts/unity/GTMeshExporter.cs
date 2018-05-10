// obj exporter taken from http://wiki.unity3d.com/index.php?title=ExportOBJ
// to play with the export API

using UnityEngine;
using UnityEngine.Rendering;
using UnityEditor;
using System;
using System.Collections;
using System.IO;
using System.Text;

/*
	GTMesh Format:
	Contains geometry data for use in gtech rendering system.
	Format description:
	
	magic string: 		"gtmeshformat"	(strlen("gtmeshformat") bytes)
	version number:		1				(UInt32)
	number of vertices:					(Uint32)	
	for each vertex:
		position						(float3)
		normal							(float3)
		uv0								(float2)		//	optional, all 0 if not present
		uv1								(float2)		// 	optional, all 0 if not present
		tangents						(float4)		// 	optional, all 0 if not present
		boneWeights						(float4)		// 	optional, all 0 if not present
		boneIndices[4]					(UInt32 * 4)	// 	optional, all 0 if not present				
	index format:		2/4				(Uint32)		// for uint16 / uint32 indices
	number of submeshes/materials		(UInt32)
	for each submesh:
		number of indices 				(UInt32)
		for each index:
			vertex index				(UInt16/UInt32)
 */
 
public class GTMeshExporter : ScriptableObject
{
	[MenuItem ("File/Export/GTMesh")]
	static void Export()
	{
		if (Selection.gameObjects.Length == 0)
		{
			Debug.Log("No game object selected; nothing has been exported.");
			return;
		}
 
		Mesh mesh = null;
		var meshFilter = Selection.gameObjects[0].GetComponent<MeshFilter>();
		if(!meshFilter)
		{
			var skinnedMesh = Selection.gameObjects[0].GetComponent<SkinnedMeshRenderer>();
			mesh = skinnedMesh.sharedMesh;
		} 
		else 
		{
			mesh = meshFilter.mesh;
		}
		if(!mesh)
		{
			Debug.Log("Selected game object doesn't contain a mesh; nothing has been exported.");
			return;
		}

		Vector3[] positions = mesh.vertices;
		Vector3[] normals = mesh.normals;

		Vector2[] uv0 = mesh.uv;
		Vector2[] uv1 = mesh.uv2;
		if(uv0 == null || uv0.Length == 0) uv0 = new Vector2[mesh.vertexCount];
		if(uv1 == null || uv1.Length == 0) uv1 = new Vector2[mesh.vertexCount];		

		Vector4[] tangents = mesh.tangents;
		if(tangents == null || tangents.Length == 0) tangents = new Vector4[mesh.vertexCount];

		Color32[] colors = mesh.colors32;
		if(colors == null || colors.Length == 0) colors = new Color32[mesh.vertexCount];

		BoneWeight[] boneWeights = mesh.boneWeights;
		if(boneWeights == null || boneWeights.Length == 0) boneWeights = new BoneWeight[mesh.vertexCount];

		// @NOTE 32 bit indices are not supported in Unity 5.x
		//IndexFormat indexFormat = mesh.indexFormat;

		int numVertices = mesh.vertexCount;
		int numSubmeshes = mesh.subMeshCount;
		Debug.Log("Exporting mesh with " + numSubmeshes + " submeshes and " + numVertices + " vertices...");

		string meshName = Selection.gameObjects[0].name;
		string fileName = EditorUtility.SaveFilePanel("Export .gtmesh file", "", meshName, "gtmesh");

		using (BinaryWriter writer = new BinaryWriter(File.Open(fileName, FileMode.Create))) 
		{
			// magic number and version
			writer.Write((UInt32)0xdeadbeef);
			writer.Write((UInt32)1);

			// number of vertices
			writer.Write((UInt32)numVertices);

			// write vertices
			for(int i = 0; i < numVertices; ++i)
			{
				var pos = positions[i];
				writer.Write(pos.x);
				writer.Write(pos.y);
				writer.Write(pos.z);
				
				var normal = normals[i];
				writer.Write(normal.x);
				writer.Write(normal.y);
				writer.Write(normal.z);

				var uv = uv0[i];
				writer.Write(uv.x);
				writer.Write(uv.y);
				uv = uv1[i];
				writer.Write(uv.x);
				writer.Write(uv.y);

				var tangent = tangents[i];
				writer.Write(tangent.x);
				writer.Write(tangent.y);
				writer.Write(tangent.z);
				writer.Write(tangent.w);

				var boneWeight = boneWeights[i];
				writer.Write(boneWeight.weight0);
				writer.Write(boneWeight.weight1);
				writer.Write(boneWeight.weight2);
				writer.Write(boneWeight.weight3);
				writer.Write((UInt32)boneWeight.boneIndex0);
				writer.Write((UInt32)boneWeight.boneIndex1);
				writer.Write((UInt32)boneWeight.boneIndex2);
				writer.Write((UInt32)boneWeight.boneIndex3);	
			}

			// submeshes
			int indexSize = 2;	//indexFormat == IndexFormat.UInt16 ? 2 : 4;
			writer.Write((UInt32)indexSize); 
			writer.Write((UInt32)numSubmeshes);
			for(int i = 0; i < numSubmeshes; ++i) 
			{
				int[] indices = mesh.GetTriangles(i);
				writer.Write((UInt32)indices.Length);
				for(int idx = 0; idx < indices.Length; ++idx)
				{
					if(indexSize == 2)
						writer.Write((UInt16)indices[idx]);
					else
						writer.Write((UInt32)indices[idx]);
				}
			}
		}

		Debug.Log("Exported Mesh: " + fileName);
	}

}