using System.Collections;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using Unity.VisualScripting;
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
        var bundle = AssetBundle.LoadFromFile("Assets\\AssetBundles\\defaultwidget");
        foreach ( var s in bundle.GetAllAssetNames())
        {
            //Debug.Log(s);
            var asset = bundle.LoadAsset<GameObject>(s);
            var info = asset.transform.Find("info");
            Debug.Log(info);
            if (info != null)
            {
                for(int i = 0; i < info.childCount; i++)
                {
                    var child = info.GetChild(i);
                    var comma = child.name.IndexOf(":");

                    if(comma > 0)
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

        void FindTag(Transform transform)
        {
            if(transform.tag == "heartrate")
            {
                var tm = transform.GetComponent<TMP_Text>();
                if (tm)
                    heartRateTexts.Add(tm);
            }
            for(int i = 0;i < transform.childCount; i++)
            {
                FindTag(transform.GetChild(i));
            }
        }

        //target = Instantiate(targetPrefab, transform);
        animator = target.GetComponent<Animator>();
        FindTag(target.transform);
    }

    void DataCome(int heart)
    {
        int age = 25;
        float heartzone = heart * 10 / (220 - age);

        foreach (var tm in heartRateTexts)
            tm.text = "" + heart;
        if (animator)
        {
            animator.SetTrigger("datacome");
            animator.SetFloat("heartzone", heartzone);
            animator.SetInteger("heartrate", heart);
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
