#include "dom_tree.h"

#include <iostream>
#include <assert.h>
#include <algorithm>

#include "utils.h"

using namespace std;

void (*DomNode::node_dropped)(DomNode*) = NULL;

const char* DomNode::get_attribute(const char* name) const
{
    map<string, string>::const_iterator iter = this->m_attributes.find(string(name));
    if (iter != this->m_attributes.end())
    {
        return iter->second.c_str();
    }
    else
    {
        return NULL;
    }
}

void DomNode::find_tags(const char* tag_name, std::vector<DomNode*>& results)
{
    if (this->m_tag.compare(tag_name) == 0)
    {
        results.push_back(this);
    }

    for (size_t i = 0; i < this->m_children.size(); ++i)
    {
        this->m_children[i]->find_tags(tag_name, results);
    }
}

void DomNode::find_tags(const vector<string>& tag_names, vector<DomNode*>& results)
{
    if (match_list(this->m_tag.c_str(), tag_names, 1) != -1)
    {
        results.push_back(this);
    }

    for (size_t i = 0; i < this->m_children.size(); ++i)
    {
        this->m_children[i]->find_tags(tag_names, results);
    }
}

bool DomNode::drop_node(DomNode* node)
{
    assert(node != NULL);

    if (DomNode::node_dropped != NULL)
    {
        DomNode::node_dropped(node);
    }

    cout << "before dropping" << node->m_parent->m_children.size() << endl;

    if (node->m_parent == NULL)
    {
        return false;
    }
    else
    {
        for (vector<DomNode*>::iterator iter = node->m_parent->m_children.begin(); iter != node->m_parent->m_children.end(); ++iter)
        {
            if (*iter == node)
            {
                node->m_parent->m_children.erase(iter);
                break;
            }
        }

        delete node;
        return true;
    }
}

bool DomNode::preorder_traverse(DomTreeVisitor& visitor)
{
    bool success = visitor.preprocess(this);
    if (!success)
    {
        return false;
    }

    success = visitor.visit(this);
    if (!success)
    {
        return false;
    }

    for (size_t i = 0; i < this->m_children.size(); ++i)
    {
        bool kept = this->m_children[i]->preorder_traverse(visitor);
        if (!kept)
        {
            --i;
        }
    }

    visitor.postprocess(this);
    return true;
}

bool DomNode::postorder_traverse(DomTreeVisitor& visitor)
{
    cout << "visiting " << this->m_tag.c_str() << " " << this->m_children.size() << " " << this->get_attribute("class") << " " << this->get_attribute("id") << endl;
    bool success = visitor.preprocess(this);
    if (!success)
    {
        return false;
    }

    for (size_t i = 0; i < this->m_children.size(); ++i)
    {
        bool kept = this->m_children[i]->postorder_traverse(visitor);
        if (!kept)
        {
            --i;
        }
    }

    success = visitor.visit(this);
    if (!success)
    {
        return false;
    }

    visitor.postprocess(this);
    return true;
}

double DomNode::get_extra(int key) const
{
    double result;
    bool success = this->get_extra(key, result);
    if (success)
    {
        return result;
    }
    else
    {
        cout <<"extra " << this->m_tag << key << endl;
        assert(false);
    }
}

bool DomNode::get_extra(int key, double& result) const
{
    std::map<int, double>::const_iterator iter = this->m_extras.find(key);
    if (iter != this->m_extras.end())
    {
        result = iter->second;
        return true;
    }
    else
    {
        return false;
    }
}

double DomNode::get_extra_default(int key, double default_value) const
{
    double result;
    bool success = this->get_extra(key, result);
    if (success)
    {
        return result;
    }
    else
    {
        return default_value;
    }
}

void DomNode::print_node() const
{
    for (std::map<int, double>::const_iterator iter = this->m_extras.begin(); iter != this->m_extras.end(); ++iter)
    {
        cout << this->m_tag.c_str() << " " << iter->first << " " << iter->second << endl;
    }
}
