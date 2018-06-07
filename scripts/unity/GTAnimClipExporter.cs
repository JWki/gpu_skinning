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
    public string name;
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
        // first pass: stitch curves so we get a union of keyframes
        foreach (var binding in AnimationUtility.GetCurveBindings(clip))
        {
            var unityCurve = AnimationUtility.GetEditorCurve(clip, binding);
            var idx = GetBoneIndexForPath(renderer.bones, binding.path);
            if (idx == -1)
            {
                // skip bones that don't exist duh
                Debug.Log("Can't find bone with path " + binding.path + ", skipping.");
                continue;
            }
            var curve = exportClip.curves.Find(cv => cv.boneIndex == idx);
            if (curve == null)
            {
                exportClip.curves.Add(new BoneCurve());
                curve = exportClip.curves.Last();
            }
            curve.boneIndex = idx;
            foreach (var key in unityCurve.keys)
            {
                var keyframe = curve.keys.Find(frame => Mathf.Approximately(frame.time, key.time));
                if (keyframe == null)
                {
                    int idx0;
                    for (idx0 = 0; idx0 < curve.keys.Count; ++idx0)
                    {
                        if (curve.keys[idx0].time > key.time) break;
                    }
                    keyframe = new Keyframe();
                    keyframe.time = key.time;
                    if (curve.keys.Count <= idx0)
                    {
                        curve.keys.Add(keyframe);
                    }
                    else
                    {   
                        curve.keys.Insert(idx0, keyframe);
                    }
                }
            }

        }
        // second pass: fill in values, effectively resampling in cases where there has been stitching happening (i.e. keyframes have been inserted for properties that didn't have a keyframe there)
        foreach (var binding in AnimationUtility.GetCurveBindings(clip))
        {
            var unityCurve = AnimationUtility.GetEditorCurve(clip, binding);
            var idx = GetBoneIndexForPath(renderer.bones, binding.path);
            if (idx == -1)
            {
                // skip bones that don't exist duh
                Debug.Log("Can't find bone with path " + binding.path + ", skipping.");
                continue;
            }
            var curve = exportClip.curves.Find(cv => cv.boneIndex == idx);
            if (curve == null)
            {
                Debug.Log("Can't find curve for bone with path " + binding.path + ", this is no good!");
                return null;
            }
            curve.boneIndex = idx;
            for (int i = 0; i < unityCurve.keys.Count(); ++i)
            {
                var key = unityCurve.keys[i];
                var keyframeIdx = curve.keys.FindIndex(frame => Mathf.Approximately(frame.time, key.time));
                int prevKeyframeIdx = -1;
                var lastKey = key;
                if(i != 0)
                {
                    lastKey = unityCurve.keys[i - 1];
                    prevKeyframeIdx = curve.keys.FindIndex(frame => Mathf.Approximately(frame.time, lastKey.time));
                    if(prevKeyframeIdx < 0)
                    {
                        Debug.Log("Can't find keyframe, this is no good!");
                        return null;
                    }
                }
                if (keyframeIdx < 0)
                {
                    Debug.Log("Can't find keyframe, this is no good!");
                    return null;
                }
                var keyframe = curve.keys[keyframeIdx];
                /*
                    property names are:
                        m_LocalRotation.xyzw
                        m_LocalPosition.xyz
                */
                switch (binding.propertyName)
                {
                    case "m_LocalRotation.x":
                        keyframe.orientation.x = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].orientation.x = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    case "m_LocalRotation.y":
                        keyframe.orientation.y = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].orientation.y = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    case "m_LocalRotation.z":
                        keyframe.orientation.z = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].orientation.z = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    case "m_LocalRotation.w":
                        keyframe.orientation.w = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].orientation.w = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    case "m_LocalPosition.x":
                        keyframe.position.x = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].position.x = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    case "m_LocalPosition.y":
                        keyframe.position.y = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].position.y = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    case "m_LocalPosition.z":
                        keyframe.position.z = key.value;
                        if (prevKeyframeIdx != -1)
                        {
                            for (int j = prevKeyframeIdx + 1; j < keyframeIdx; ++j)
                            {
                                curve.keys[j].position.z = Mathf.Lerp(lastKey.value, key.value, (float)(j - prevKeyframeIdx) / (float)(keyframeIdx - prevKeyframeIdx));
                            }
                        }
                        break;
                    default:
                        Debug.Log("Unsupported curve key " + binding.propertyName);
                        break;
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

                if(exportClip == null)
                {
                    Debug.Log("Export failed, no file has been written.");
                    return;
                }
                using(BinaryWriter writer = new BinaryWriter(File.Open(fileName, FileMode.Create)))
                {
                    // magic number and version
			        writer.Write((UInt32)0xdeadbeef);
			        writer.Write((UInt32)1);

                    // clip name
                    string name = exportClip.name;
                    byte[] arr = System.Text.Encoding.UTF8.GetBytes(name);
                    writer.Write((UInt32)arr.Length);
                    for (int b = 0; b < arr.Length; ++b)
                    {
                        writer.Write(arr[b]);
                    }

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
                        Debug.Log("Exported curve for bone #" + curve.boneIndex + " with " + curve.keys.Count + " keyframes.");
                    }

                    Debug.Log("Exported anim '" + name + " (" + exportClip.curves.Count + " curves)");
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