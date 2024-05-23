#include <iostream>
#include <json/json.h>
#include <string>
#include <fstream>
using namespace std;
using namespace Json;
/*
[
        12,
        12.34,
        true,
        "tom",
        ["jack", "ace", "robin"],
    { "sex":"man", "girlfriend" : "lucy" }
]
*/


void readJson()
{
    ifstream ifs("text.json");
    Reader rd;
    Value root;
    rd.parse(ifs, root);



    if (root.isArray())
    {
        for (int i = 0; i < root.size(); i++)
        {
            Value item = root[i];
            // 判断item中存储的数据的类型然后打印出来
            if (item.isString())
            {
                cout << item.asString() << ", ";
            }
            else if (item.isInt())
            {
                cout << item.asInt() << ", ";
            }
            else if (item.isBool())
            {
                cout << item.asBool() << ", ";
            }
            else if (item.isDouble())
            {
                cout << item.asDouble() << ", ";
            }
            else if (item.isArray())
            {
                for (int j = 0; j < item.size(); ++j)
                {
                    cout << item[j].asString() << ", ";
                }
            }
            else if (item.isObject())
            {
                // 对象
                // 得到所有的key
                Value::Members keys = item.getMemberNames();
                for (int k = 0; k < keys.size(); ++k)
                {
                    cout << keys.at(k) << ":" << item[keys[k]] << ", ";
                }
            }

        }
        cout << endl;
    }
}


void writeJson()
{
    //创建json中的Value对象,其实是一个json数组
    Value root;

    //将数据添加到Value对象中
    root.append(12);
    root.append(12.34);
    root.append(true);
    root.append("tom");

    Value subArray;
    subArray.append("jack");
    subArray.append("ace");
    subArray.append("robin");
    root.append(subArray);

    Value obj;
    obj["sex"] = "man";
    obj["girlfriend"] = "lucy";
    root.append(obj);

    //将root中的数据序列化
    //有格式的
    string json1=root.toStyledString();

    //无格式的
    FastWriter f;
    string json2 = f.write(root);
    
    //write->ofstream
    //read->ifstream

    //将数据写入磁盘文件
    ofstream ofs("text.json");
    ofs << json1 << endl;
    ofs.close();
}


int main()
{

    writeJson();
    readJson();
}
