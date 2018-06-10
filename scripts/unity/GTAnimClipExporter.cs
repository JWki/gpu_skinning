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
    public Quaternion rotation;
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
    private AnimationClip clip;
    private GameObject gameObject;
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

    public AnimationCurve GetCurve(AnimationClip clip, int boneIdx, string propertyName, List<AnimationCurve> curves)
    {
        foreach (var binding in AnimationUtility.GetCurveBindings(clip))
        {
            if (boneIdx == GetBoneIndexForPath(renderer.bones, binding.path) && binding.propertyName.Equals(propertyName))
            {
                var curve = AnimationUtility.GetEditorCurve(clip, binding);
                curves.Add(curve);
                return curve;
            }
        }
        return null;
    }

    public GTAnimClip BuildAnimClip()
    {
        GTAnimClip exportClip = new GTAnimClip();
        exportClip.name = clip.name;

        // collect all paths
        HashSet<string> bonePaths = new HashSet<string>();
        foreach (var binding in AnimationUtility.GetCurveBindings(clip))
        {
            bonePaths.Add(binding.path);
        }

        foreach (var path in bonePaths)
        {
            // get all curves we're interested in:
            // translation and rotation curves
            var idx = GetBoneIndexForPath(renderer.bones, path);
            if (idx == -1)
            {
                // skip bones that don't exist duh
                Debug.Log("Can't find bone with path " + path + ", skipping.");
                continue;

            }

            var boneCurve = new BoneCurve();
            boneCurve.boneIndex = idx;
            exportClip.curves.Add(boneCurve);

            List<AnimationCurve> curves = new List<AnimationCurve>();
            var translationCurveX = GetCurve(clip, idx, "m_LocalPosition.x", curves);
            var translationCurveY = GetCurve(clip, idx, "m_LocalPosition.y", curves);
            var translationCurveZ = GetCurve(clip, idx, "m_LocalPosition.z", curves);

            var rotationCurveX = GetCurve(clip, idx, "m_LocalRotation.x", curves);
            var rotationCurveY = GetCurve(clip, idx, "m_LocalRotation.y", curves);
            var rotationCurveZ = GetCurve(clip, idx, "m_LocalRotation.z", curves);
            var rotationCurveW = GetCurve(clip, idx, "m_LocalRotation.w", curves);

            float duration = 0.0f;
            foreach (var curve in curves)
            {
                curve.postWrapMode = WrapMode.ClampForever;
                var len = curve.length;
                var lastKey = curve.keys[len - 1];
                if (duration < lastKey.time) { duration = lastKey.time; }
            }
            // sample all curves at 30hz, create one keyframe for each sample
            float frequency = 1.0f / 60.0f;
            float progress = 0.0f;
            while (progress < duration)
            {
                Keyframe key = new Keyframe();
                key.time = progress;

                key.position.x = translationCurveX == null ? 0.0f : translationCurveX.Evaluate(progress);
                key.position.y = translationCurveY == null ? 0.0f : translationCurveY.Evaluate(progress);
                key.position.z = translationCurveZ == null ? 0.0f : translationCurveZ.Evaluate(progress);
                key.rotation.x = rotationCurveX == null ? 0.0f : rotationCurveX.Evaluate(progress);
                key.rotation.y = rotationCurveY == null ? 0.0f : rotationCurveY.Evaluate(progress);
                key.rotation.z = rotationCurveZ == null ? 0.0f : rotationCurveZ.Evaluate(progress);
                key.rotation.w = rotationCurveW == null ? 0.0f : rotationCurveW.Evaluate(progress);

                boneCurve.keys.Add(key);

                progress += frequency;
            }
            if (progress != duration)
            {   // @TODO

            }

        }

        return exportClip;
    }


    public void OnGUI()
    {
        clip = EditorGUILayout.ObjectField("Clip", clip, typeof(AnimationClip), false) as AnimationClip;
        gameObject = EditorGUILayout.ObjectField("Object", gameObject, typeof(GameObject), false) as GameObject;
        renderer = gameObject.GetComponent<SkinnedMeshRenderer>();

        if (GUILayout.Button("Export"))
        {
            if (clip != null && gameObject != null && renderer != null)
            {
                string fileName = EditorUtility.SaveFilePanel("Export .gtanimclip file", "", gameObject.name, "gtanimclip");
                exportClip = BuildAnimClip();

                if (exportClip == null)
                {
                    Debug.Log("Export failed, no file has been written.");
                    return;
                }
                using (BinaryWriter writer = new BinaryWriter(File.Open(fileName, FileMode.Create)))
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

                    {   // curves
                        writer.Write((UInt32)exportClip.curves.Count);
                        foreach (var curve in exportClip.curves)
                        {
                            writer.Write((UInt32)curve.boneIndex);
                            writer.Write((UInt32)curve.keys.Count);

                            foreach (var key in curve.keys)
                            {
                                writer.Write(key.time);

                                writer.Write(key.position.x);
                                writer.Write(key.position.y);
                                writer.Write(key.position.z);

                                writer.Write(key.rotation.x);
                                writer.Write(key.rotation.y);
                                writer.Write(key.rotation.z);
                                writer.Write(key.rotation.w);
                            }
                            Debug.Log("Exported curve for bone #" + curve.boneIndex + " with " + curve.keys.Count + " keyframes.");
                        }
                    }

                    Debug.Log("Exported anim '" + name + " (" + exportClip.curves.Count + " curves)");
                }
            }
        }

        if (exportClip != null)
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