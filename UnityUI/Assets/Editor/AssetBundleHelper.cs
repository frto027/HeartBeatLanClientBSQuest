using UnityEditor;
using System.IO;
using System.Diagnostics;
using System;

public class CreateAssetBundles
{
    [MenuItem("Assets/Build AssetBundles")]
    static void BuildAllAssetBundles()
    {
        try
        {
            string assetBundleDirectory = "Assets/AssetBundles";
            if (!Directory.Exists(assetBundleDirectory))
                Directory.CreateDirectory(assetBundleDirectory);

            BuildPipeline.BuildAssetBundles(assetBundleDirectory,
                                            BuildAssetBundleOptions.None,
                                            BuildTarget.StandaloneWindows64);
        }
        catch (Exception e)
        {
            Debug.WriteLine(e.ToString());
        }

        try
        {
            string assetBundleDirectory = "Assets/AssetBundlesAndroid";
            if (!Directory.Exists(assetBundleDirectory))
                Directory.CreateDirectory(assetBundleDirectory);

            BuildPipeline.BuildAssetBundles(assetBundleDirectory,
                                            BuildAssetBundleOptions.None,
                                            BuildTarget.Android);
        }
        catch (Exception e)
        {
            Debug.WriteLine(e.ToString());
        }


    }
}
