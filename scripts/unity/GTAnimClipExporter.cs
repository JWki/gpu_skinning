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

public class TranslationKeyframe
{
    public float time;
    public Vector3 value;
}

public class RotationKeyframe
{
    public float time;
    public Quaternion value;
}

public class TranslationCurve
{
    public int boneIndex;
    public List<TranslationKeyframe> keys = new List<TranslationKeyframe>();
}

public class RotationCurve
{
    public int boneIndex;
    public List<RotationKeyframe> keys = new List<RotationKeyframe>();
}

public class GTAnimClip 
{
    public string name;
    public List<TranslationCurve> translationCurves = new List<TranslationCurve>();
    public List<RotationCurve> rotationCurves = new List<RotationCurve>();
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
            // translation
            if(binding.propertyName.Contains("m_LocalPosition"))
            {
                var curve = exportClip.translationCurves.Find(cv => cv.boneIndex == idx);
                if (curve == null)
                {
                    exportClip.translationCurves.Add(new TranslationCurve());
                    curve = exportClip.translationCurves.Last();
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
                        keyframe = new TranslationKeyframe();
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
                    if (binding.propertyName.Contains(".x"))
                    {
                        keyframe.value.x = key.value;
                    }
                    else if (binding.propertyName.Contains(".y"))
                    {
                        keyframe.value.y = key.value;
                    }
                    else if (binding.propertyName.Contains(".z"))
                    {
                        keyframe.value.z = key.value;
                    }
                }
            } else if (binding.propertyName.Contains("m_LocalRotation"))
            {
                var curve = exportClip.rotationCurves.Find(cv => cv.boneIndex == idx);
                if (curve == null)
                {
                    exportClip.rotationCurves.Add(new RotationCurve());
                    curve = exportClip.rotationCurves.Last();
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
                        keyframe = new RotationKeyframe();
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
                    if (binding.propertyName.Contains(".x"))
                    {
                        keyframe.value.x = key.value;
                    }
                    else if (binding.propertyName.Contains(".y"))
                    {
                        keyframe.value.y = key.value;
                    }
                    else if (binding.propertyName.Contains(".z"))
                    {
                        keyframe.value.z = key.value;
                    }
                    else if (binding.propertyName.Contains(".w"))
                    {
                        keyframe.value.w = key.value;
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

                    { // translation curves
                        writer.Write((UInt32)exportClip.translationCurves.Count);
                        foreach (var curve in exportClip.translationCurves)
                        {
                            writer.Write((UInt32)curve.boneIndex);
                            writer.Write((UInt32)curve.keys.Count);

                            foreach (var key in curve.keys)
                            {
                                writer.Write(key.time);

                                writer.Write(key.value.x);
                                writer.Write(key.value.y);
                                writer.Write(key.value.z);
                            }
                            Debug.Log("Exported translation curve for bone #" + curve.boneIndex + " with " + curve.keys.Count + " keyframes.");
                        }
                    }
                    { // rotation curves
                        writer.Write((UInt32)exportClip.rotationCurves.Count);
                        foreach (var curve in exportClip.rotationCurves)
                        {
                            writer.Write((UInt32)curve.boneIndex);
                            writer.Write((UInt32)curve.keys.Count);

                            foreach (var key in curve.keys)
                            {
                                writer.Write(key.time);

                                writer.Write(key.value.x);
                                writer.Write(key.value.y);
                                writer.Write(key.value.z);
                                writer.Write(key.value.w);
                            }
                            Debug.Log("Exported rotation curve for bone #" + curve.boneIndex + " with " + curve.keys.Count + " keyframes.");
                        }
                    }

                    Debug.Log("Exported anim '" + name + " (" + exportClip.translationCurves.Count + " translation curves, " + exportClip.rotationCurves.Count +  ")");
                }
            }
        }

        if(exportClip != null)
        {
            //EditorGUILayout.LabelField("Clip: " + clip.name);
            //EditorGUILayout.LabelField("Num affected bones: " + exportClip.curves.Count);
            //for(int i = 0; i < exportClip.curves.Count; ++i)
            //{
            //    EditorGUILayout.LabelField("Bone #" + exportClip.curves[i].boneIndex + " : " + exportClip.curves[i].keys.Count + " keyframes");
            //}
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