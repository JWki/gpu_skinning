using UnityEngine;
using UnityEngine.Rendering;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Linq;

/*
    GTAnimClip Format
    Contains animation curve data for individual animation clips
    Format description:

    magic number:       0xdeadbeef  (UInt32)
    version number:     1           (UInt32)

    clip name length                (UInt32)
    clip name as str                (char * len)    // utf-8 encoded
    number of affected bones        (UInt32)
    for each bone:
        index                       (UInt32)
        number of keyframes         (UInt32)
        for each keyframe:
            time                    (float)
            position                (float3)
            orientation             (float4)        // quaternion
*/

public class Keyframe
{
    public float time;
    public Vector3 position;
    public Quaternion orientation;
}

public class BoneCurve
{
    public int boneIndex;
    public List<Keyframe> keys = new List<Keyframe>();
}

public class GTAnimClip 
{
    string name;
    public List<BoneCurve> curves = new List<BoneCurve>();
}

public class GTAnimClipExporter : EditorWindow
{
    private AnimationClip   clip;
    private GameObject      gameObject;    
    private SkinnedMeshRenderer renderer;
    private GTAnimClip exportClip;

    [MenuItem("File/Export/GTAnimClip")]
    static void Init()
    {
        var window = GetWindow(typeof(GTAnimClipExporter)) as GTAnimClipExporter;
        window.gameObject = Selection.gameObjects[0];
    }

    private int GetBoneIndexForPath(Transform[] bones, string path)
    {
        string[] splitPath = path.Split('/');
        string boneName = splitPath[splitPath.Length - 1];
        return Array.FindIndex(bones, bone => bone.gameObject.name == boneName);
    }

    public GTAnimClip BuildAnimClip()
    {
        GTAnimClip exportClip = new GTAnimClip();
        exportClip.name = clip.name;
        foreach(var binding in AnimationUtility.GetCurveBindings(clip))
        {
            var unityCurve = AnimationUtility.GetEditorCurve(clip, binding);
            var idx = GetBoneIndexForPath(renderer.bones, binding.path);
            if(idx == -1) 
            {
                // skip bones that don't exist duh
                Debug.Log("Can't find bone with path " + binding.path + ", skipping.");
                continue;
            }
            var curve = exportClip.curves.Find(cv => cv.boneIndex == idx);
            if(curve == null)
            {
                exportClip.curves.Add(new BoneCurve());
                curve = exportClip.curves.Last();
                curve.boneIndex = idx;
                foreach(var key in unityCurve.keys)
                {
                    var keyframe = curve.keys.Find(frame => Mathf.Approximately(frame.time, key.time));
                    if(keyframe == null)
                    {
                        int idx0;
                        for(idx0 = 0; idx0 < curve.keys.Count; ++idx0) {
                            if(curve.keys[idx0].time > key.time) break;
                        }
                        keyframe = new Keyframe();
                        keyframe.time = key.time;
                        if(curve.keys.Count == 0) 
                        {
                            curve.keys.Add(keyframe);
                        } 
                        else 
                        {   // initialize with pose from previous keyframe
                            keyframe.position = curve.keys[idx0].position;
                            keyframe.orientation = curve.keys[idx0].orientation;
                            curve.keys.Insert(idx0, keyframe);
                        }
                    }
                    /*
                        property names are:
                            m_LocalRotation.xyzw
                            m_LocalPosition.xyz
                     */
                    switch(binding.propertyName)
                    {
                        case "m_LocalRotation.x":
                            keyframe.orientation.x = key.value;
                            break;
                        case "m_LocalRotation.y":
                            keyframe.orientation.y = key.value;
                            break;
                        case "m_LocalRotation.z":
                            keyframe.orientation.z = key.value;
                            break;
                        case "m_LocalRotation.w":
                            keyframe.orientation.w = key.value;
                            break;
                        case "m_LocalPosition.x":
                            keyframe.position.x = key.value;
                            break;
                        case "m_LocalPosition.y":
                            keyframe.position.y = key.value;
                            break;
                        case "m_LocalPosition.z":
                            keyframe.position.z = key.value;
                            break;
                        default:
                            Debug.Log("Unsupported curve key " + binding.propertyName);
                            break;
                    }
                }
            }
        }
        return exportClip;
    }


    public void OnGUI()
    {
        clip = EditorGUILayout.ObjectField("Clip", clip, typeof(AnimationClip), false) as AnimationClip;
        gameObject = EditorGUILayout.ObjectField("Object", gameObject, typeof(GameObject), false) as GameObject;
        renderer = gameObject.GetComponent<SkinnedMeshRenderer>();

        if(GUILayout.Button("Export"))
        {
            if(clip != null && gameObject != null && renderer != null)
            {
                string fileName = EditorUtility.SaveFilePanel("Export .gtanimclip file", "", gameObject.name, "gtanimclip");
                exportClip = BuildAnimClip();

                using(BinaryWriter writer = new BinaryWriter(File.Open(fileName, FileMode.Create)))
                {
                    // magic number and version
			        writer.Write((UInt32)0xdeadbeef);
			        writer.Write((UInt32)1);

                    // clip name
                    string name = exportClip.name;
                    byte[] arr = System.Text.Encoding.UTF8.GetBytes(name);
                    writer.Write((UInt32)arr.Length);

                    writer.Write((UInt32)exportClip.curves.Count);

                    foreach(var curve in exportClip.curves)
                    {
                        writer.Write((UInt32)curve.boneIndex);
                        writer.Write((UInt32)curve.keys.Count);

                        foreach(var key in curve.keys)
                        {
                            writer.Write(key.time);

                            writer.Write(key.position.x);
                            writer.Write(key.position.y);
                            writer.Write(key.position.z);

                            writer.Write(key.orientation.x);
                            writer.Write(key.orientation.y);
                            writer.Write(key.orientation.z);
                            writer.Write(key.orientation.w);
                        }
                    }
                }
            }
        }

        if(exportClip != null)
        {
            EditorGUILayout.LabelField("Clip: " + clip.name);
            EditorGUILayout.LabelField("Num affected bones: " + exportClip.curves.Count);
            for(int i = 0; i < exportClip.curves.Count; ++i)
            {
                EditorGUILayout.LabelField("Bone #" + exportClip.curves[i].boneIndex + " : " + exportClip.curves[i].keys.Count + " keyframes");
            }
        }
    }
} 

// for debugging
public class ClipInfo : EditorWindow
{
    private AnimationClip clip;
    private Vector2 scrollPos;

    [MenuItem("Window/Clip Info")]
    static void Init()
    {
        GetWindow(typeof(ClipInfo));
    }

    public void OnGUI()
    {
        clip = EditorGUILayout.ObjectField("Clip", clip, typeof(AnimationClip), false) as AnimationClip;

        EditorGUILayout.LabelField("Curves:");
        if (clip != null)
        {
            scrollPos = EditorGUILayout.BeginScrollView(scrollPos);
            foreach (var binding in AnimationUtility.GetCurveBindings(clip))
            {
                AnimationCurve curve = AnimationUtility.GetEditorCurve(clip, binding);
                EditorGUILayout.LabelField(binding.path + "/" + binding.propertyName + ", Keys: " + curve.keys.Length);
            }
            EditorGUILayout.EndScrollView();
        }
    }
}