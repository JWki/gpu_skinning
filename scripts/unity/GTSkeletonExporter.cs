
using UnityEngine;
using UnityEngine.Rendering;
using UnityEditor;
using System;
using System.Collections;
using System.IO;
using System.Text;

/*
    GTSkeleton Format
    Contains skeleton data for use in gtech rendering system
    Format description:

    magic number:   0xdeadbeef  (UInt32)
    version number: 1           (UInt32)

    number of bones             (UInt32)
    for each bone:
        length of name          (UInt32)
        name in utf-8           (char * len)
        bindpose                (float4x4)      // @NOTE bindpose is stored in object space
        parent index            (Int32)         // @NOTE: if parent index is -1, this bone has no parent  
*/


public class GTSkeletonExporter : ScriptableObject
{
    [MenuItem("File/Export/GTSkeleton")]
    static void Export()
    {
        if(Selection.gameObjects.Length == 0)
        {
            Debug.Log("No game object selected; nothing has been exported.");
            return;
        }

        // mesh for bindposes
        var skinnedMesh = Selection.gameObjects[0].GetComponent<SkinnedMeshRenderer>();
        if(!skinnedMesh)
        {
            Debug.Log("Selected game object doesn't have a skinned mesh renderer; nothing has been exported.");
        }
        var mesh = skinnedMesh.sharedMesh;
		if(!mesh)
		{
			Debug.Log("Selected game object doesn't contain a mesh; nothing has been exported.");
			return;
		}

        Matrix4x4[] invBindposes = mesh.bindposes;
        Matrix4x4[] bindposes = new Matrix4x4[invBindposes.Length];
        for(int i = 0; i < invBindposes.Length; ++i) 
        {
            bindposes[i] = invBindposes[i].inverse;
        }
        Transform[] bones = skinnedMesh.bones;
        int numBones = bones.Length;

        string meshName = Selection.gameObjects[0].name;
		string fileName = EditorUtility.SaveFilePanel("Export .gtskel file", "", meshName, "gtskel");

        using(BinaryWriter writer = new BinaryWriter(File.Open(fileName, FileMode.Create)))
        {
            // magic number and version
			writer.Write((UInt32)0xdeadbeef);
			writer.Write((UInt32)1);

            // number of bones
            writer.Write((UInt32)numBones);

            // write bones
            for(int i = 0; i < numBones; ++i)
            {
                // get name:
                string name = bones[i].gameObject.name;
                byte[] arr = System.Text.Encoding.UTF8.GetBytes(name);
                writer.Write((UInt32)arr.Length);
                for(int b = 0; b < arr.Length; ++b) 
                {
                    writer.Write(arr[b]);
                }
                // bindpose
                for(int j = 0; j < 16; ++j)
                {
                    writer.Write(bindposes[i][j]);
                }
                if(bones[i].parent == null)
                {
                    writer.Write((Int32)(-1));
                }
                else 
                {
                    int parentIndex = -1;
                    for(int k = 0; k < numBones; ++k)
                    {
                        if(bones[k] == bones[i].parent)
                        {
                            parentIndex = k;
                            break;
                        }
                    }
                    if(parentIndex == -1) Debug.Log("Incomplete hierarchy!");
                    writer.Write((Int32)parentIndex);
                }
            }
        }
        Debug.Log("Exported Skeleton: " + fileName);

    }
}