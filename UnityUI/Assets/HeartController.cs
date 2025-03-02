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

    Animator animator;

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
                            Debug.Log(key + ": " + value);
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
            for(int i = 0;i < transform.childCount; i++)
            {
                FindTag(transform.GetChild(i));
            }
        }

        animator = target.GetComponent<Animator>();
        FindTag(target.transform);
    }

    void DataCome(int heart)
    {
        int age = 25;
        float maxheart = 220 - age;
        float precent = heart / maxheart;

        foreach (var tm in heartRateTexts)
            tm.text = "" + heart;
        if (animator)
        {
            animator.SetFloat("heartpercent", precent);
            animator.SetInteger("heartrate", heart);
            animator.SetTrigger("datacome");
        }
    }

    int count = 0;

    // Update is called once per frame
    void FixedUpdate()
    {
        if(count++ % 70 == 0)
        {
            DataCome(Random.Range(1, 180));
        }
    }
}
