using System.Collections;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;

//this is an example controller, it shows you how the mod uses the targetPrefab, which is in your asset bundle

public class HeartController : MonoBehaviour
{
    public GameObject targetPrefab;

    GameObject target;

    List<Animator> animators = new List<Animator>();

    List<TMP_Text> heartRateTexts = new List<TMP_Text>();



    // Start is called before the first frame update
    void Start()
    {
        if(targetPrefab != null)
        {
            target = Instantiate(targetPrefab, transform);
        }
        else
        {
            var bundle = AssetBundle.LoadFromFile("Assets\\AssetBundles\\defaultwidget");
            foreach (var s in bundle.GetAllAssetNames())
            {
                //Debug.Log(s);
                var asset = bundle.LoadAsset<GameObject>(s);
                var info = asset.transform.Find("info");
                Debug.Log(info);
                if (info != null)
                {
                    for (int i = 0; i < info.childCount; i++)
                    {
                        var child = info.GetChild(i);
                        var comma = child.name.IndexOf(":");

                        if (comma > 0)
                        {
                            var key = child.name.Substring(0, comma);
                            var value = child.name.Substring(comma + 1);
                            Debug.Log(key + ": " + value); // the mod will see all key value pair, and do something with these property if possible
                        }
                    }
                }
                target = Instantiate(asset, transform);
                break;
            }
        }

        void FindTag(Transform transform)
        {
            if(transform.name == "auto:heartrate")
            {
                var tm = transform.GetComponent<TMP_Text>();
                if (tm)
                    heartRateTexts.Add(tm);
                
                var font = tm.font;
                Debug.Log(tm.font);
                Debug.Log(font.material);
                Debug.Log(font.material.shader);
                var shader = tm.material.shader;
                Debug.Log(shader);
            }
            if(transform.GetComponent<TMP_Text>() != null)
            {
                //the mod will fix the font for this component
            }

            var anmt = transform.GetComponent<Animator>();
            if (anmt)
            {
                animators.Add(anmt);
            }
            for(int i = 0;i < transform.childCount; i++)
            {
                FindTag(transform.GetChild(i));
            }
        }

        FindTag(target.transform);
    }

    void DataCome(int heart)
    {
        int age = 25;
        float maxheart = 220 - age;
        float precent = heart / maxheart;

        foreach (var tm in heartRateTexts)
            tm.text = "" + heart;
        foreach (var anmt in animators)
        {
            anmt.SetFloat("heartpercent", precent);
            anmt.SetInteger("heartrate", heart);
            anmt.SetTrigger("datacome");

            anmt.SetInteger("hr_1", heart % 10);
            anmt.SetInteger("hr_10", (heart / 10) % 10);
            anmt.SetInteger("hr_100", (heart / 100) % 10);
        }
    }

    int count = 0;

    // Update is called once per frame
    void FixedUpdate()
    {
        if(count++ % 70 == 0)
        {
            DataCome(130 + count / 50);
        }
    }
}
