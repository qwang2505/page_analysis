#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include "gtest/gtest.h"
#include "python2.6/Python.h"

#include "body_extractor.h"

using namespace std;

class PythonWrapper
{
public:
    PythonWrapper() :
        _initialized(false)
    {
    }

    ~PythonWrapper()
    {
        if (this->_initialized)
        {
            for (map<const char*, PyObject*>::iterator iter = this->_modules.begin(); iter != this->_modules.end(); ++iter)
            {
                Py_DECREF(iter->second);
            }

            Py_Finalize();
        }
    }

    bool init()
    {
        assert(!this->_initialized);

        Py_Initialize();
        if (!Py_IsInitialized())
        {
            cout << "python init failed" << endl;
            return false;
        }

        PyRun_SimpleString("import sys");
        PyRun_SimpleString("sys.path.append('./')");


        this->_initialized = true;
        return true;
    }

    bool import_module(const char* module_name)
    {
        PyObject* module = PyImport_ImportModule(module_name);
        if (!module)
        {
            cout << "import module failed" << endl;
            return false;
        }

        PyObject* dict = PyModule_GetDict(module);
        if (!dict)
        {
            cout << "get module dict failed" << endl;
            return false;
        }

        this->_modules[module_name] = dict;
        return true;
    }

    bool print_dict(PyObject* dict)
    {
        if (!PyDict_Check(dict))
        {
            cout << "invalid dict" << endl;
            return false;
        }

        PyObject* keys = PyDict_Keys(dict);
        for (int i = 0; i < PyList_GET_SIZE(keys); ++i)
        {
            PyObject* k = PyList_GET_ITEM(keys, i);
            char* value = PyString_AsString(k);
            Py_DECREF(k);
            cout << value << endl;
        }

        Py_DECREF(keys);
        return true;
    }

    const char* invoke(const char* module_name, const char* func_name, PyObject* param)
    {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyObject* func = PyDict_GetItemString(this->_modules[module_name], func_name);
        if (!func)
        {
            return "";
        }

        PyObject* result = PyObject_CallFunctionObjArgs(func, param, NULL);
        
        PyGILState_Release(gstate);
        //Py_DECREF(func);
        return PyString_AsString(result);
    }

    bool invoke_string(const char* module_name, const char* func_name, const string& text, PyObject*& result)
    {
        PyObject* func = PyDict_GetItemString(this->_modules[module_name], func_name);
        if (!func)
        {
            return false;
        }

        PyObject* param = PyString_FromStringAndSize(text.data(), text.size());

        result = PyObject_CallFunctionObjArgs(func, param, NULL);

        //Py_DECREF(func);
        //Py_DECREF(param);
        return true;
    }

    PyObject* invoke_object(PyObject* obj, const char* func_name)
    {
        return PyObject_CallMethod(obj, (char*)(func_name), NULL);
    }

    const char* get_attr_string(PyObject* obj, const char* attr_name)
    {
        PyGILState_STATE gstate = PyGILState_Ensure();
        PyObject* attr = PyObject_GetAttrString(obj, attr_name);
        if (attr == NULL)
        {
            return "";
        }

        char* attr_value = PyString_AsString(attr);
        PyGILState_Release(gstate);
        //Py_DECREF(attr);
        return attr_value;
    }

public:
    bool _initialized;
    map<const char*, PyObject*> _modules;
};

PythonWrapper python;

PyObject* tag_type = NULL;

const char* get_dom_attr(PyObject* node, const char* attr_name)
{
    PyObject* attr = PyObject_GetAttrString(node, "attrib");
    if (attr == NULL)
    {
        return "";
    }

    PyObject* result = PyObject_CallMethod(attr, (char*)"get", (char*)"ss", attr_name, "");
    if (result == NULL)
    {
        return "";
    }
    else
    {
        return PyString_AsString(result);
    }
}

// build dom tree from python lxml dom tree.
DomNode* build_dom_tree(DomNode* parent, PyObject* node, map<DomNode*, PyObject*>& relation)
{
    const char* tag = python.get_attr_string(node, "tag");
    const char* inner_text = python.get_attr_string(node, "text");
    const char* inner_tail = python.get_attr_string(node, "tail");
    string text;
    if (inner_text != NULL)
    {
        text.append(inner_text);
    }

    if (inner_tail != NULL)
    {
        if (parent != NULL)
        {
            parent->append_text(inner_tail);
        }
    }

    DomNode* dom_node = new DomNode(tag, text);
    relation[dom_node] = node;

    //PyObject* parent = python.invoke_object(node, "getparent");
    FILE* fp = fopen("object.txt", "w+");
    PyObject_Print(node, fp, Py_PRINT_RAW);
    fclose(fp);
    PyObject* children = python.invoke_object(node, "getchildren");
    if (children != NULL)
    {
        int count = PyList_GET_SIZE(children);
        for (int i = 0; i < count; ++i)
        {
            PyObject* child = PyList_GET_ITEM(children, i);
            if (PyObject_IsInstance(child, tag_type) != 1)
            {
                continue;
            }

            DomNode* child_node = build_dom_tree(dom_node, child, relation);
            dom_node->append_child(child_node);
        }
    }

    dom_node->add_attribute("class", get_dom_attr(node, "class"));
    dom_node->add_attribute("id", get_dom_attr(node, "id"));

    return dom_node;
}

void print_dom(DomNode* node, stringstream& text)
{
    const char* parent_tag = node->get_parent() != NULL ? node->get_parent()->get_tag() : "NULL";
    text << node->get_tag() << "," << parent_tag << "," << node->get_text() << "," << (int)node->get_children()->size() << "," << node->get_attribute("class") << "," << node->get_attribute("id") << endl;
    for (size_t i = 0; i < node->get_children()->size(); ++i)
    {
        print_dom((*node->get_children())[i], text);
    }
}

const char* module = "lxml.html";


map<DomNode*, PyObject*> dom_relation;

void node_dropped(DomNode* node)
{
    map<DomNode*, PyObject*>::iterator iter = dom_relation.find(node);
    if (iter != dom_relation.end())
    {
        PyObject* py_node = iter->second;
        python.invoke_object(py_node, "drop_tree");
    }
}

void read_file(const char* file_name, stringstream& output)
{
    ifstream file(file_name);
    if (file.is_open())
    {
        string line;
        while (file.good())
        {
            getline(file, line);
            output << line << '\n';
        }

        file.close();
    }
}

void write_file(const char* file_name, const char* body_str)
{
    ofstream file(file_name);
    if (file.is_open())
    {
        file << body_str;
        file.close();
    }
}

void test_file(const string& html)
{
    PyObject* py_dom = NULL;
    if (!python.invoke_string(module, "fromstring", html, py_dom))
    {
        EXPECT_TRUE(false);
    }

    // build dom tree from python lxml dom tree.
    DomNode* dom = build_dom_tree(NULL, py_dom, dom_relation);
    // get html text of dom tree by call python lxml tostring method.
    const char* body_str = python.invoke(module, "tostring", dom_relation[dom]);
    // write.
    write_file("news.ori.html", body_str);

    // init body extractor
    BodyExtractor extractor;
    // init.
    EXPECT_TRUE(extractor.init("../body_extractor.ini"));

    // extract dom
    DomNode* body = extractor.extract(dom);
    EXPECT_TRUE(body != NULL);
    if (body != NULL)
    {
        cout << "body result " << body->get_tag() << " " << body->get_attribute("class") << " " << body->get_attribute("id") << endl;
        FILE* fp = fopen("object.txt", "w+");
        PyObject_Print(py_dom, fp, Py_PRINT_RAW);
        fclose(fp);
        const char* body_str = python.invoke(module, "tostring", dom_relation[body]);
        write_file("news.out.html", body_str);
    }

    stringstream text;
    print_dom(dom, text);
    dom_relation.clear();
    delete dom;
}

TEST(BodyExtractor, main)
{
    EXPECT_TRUE(python.init());
    EXPECT_TRUE(python.import_module(module));
    //EXPECT_TRUE(python.print_dict(python._modules[module]));

    DomNode::register_node_dropped(node_dropped);
    tag_type = PyDict_GetItemString(python._modules["lxml.html"], "HtmlElement");


    //EXPECT_TRUE(python.print_dict(python._modules[module]));

    //const char* html = "<html><a class='aa'>xyz</a>abc<div>hello, world.</div><th/><div><p id='ad_wrapper'>xyz</p><div id='body'>xxxxxxxxxxxxxxxxxxxxxxxxxxx,y,yyyyyyyyyyyyyyyyyyyyyyyyzzzzzzzzzzzzzzzzzzzzzzzzzzz</div></div></html>";
    //test_file(html);

    //const char* html = "<div><a><b>xxx</b>yyy<c>zzz</c>aaa</a></div>";
    //test_file(html);

    stringstream text;
    read_file("sina.html", text);
    test_file(text.str());

}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
