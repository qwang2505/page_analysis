#ifndef _DOM_TREE_H_
#define _DOM_TREE_H_

#include <vector>
#include <string>
#include <map>
#include <cstdio>

class DomNode;

class DomTreeVisitor
{
public:
    virtual bool preprocess(DomNode* node)
    {
        return true;
    }

    virtual bool visit(DomNode* node)
    {
        return true;
    }

    virtual bool postprocess(DomNode* node)
    {
        return true;
    }

    virtual ~DomTreeVisitor()
    {
    }
};

class DomNode
{
public:
    DomNode(const std::string& tag, const std::string& text) :
        m_tag(tag), m_text(text), m_parent(NULL), m_children()
    {
    }

    ~DomNode()
    {
        for (size_t i = 0; i < m_children.size(); ++i)
        {
            delete m_children[i];
            m_children[i] = NULL;
        }
    }

    void append_child(DomNode* child)
    {
        this->m_children.push_back(child);
        child->m_parent = this;
    }

    void append_text(const std::string& text)
    {
        this->m_text.append(text);
    }

    void add_attribute(const char* name, const char* value)
    {
        this->m_attributes[std::string(name)] = std::string(value);
    }

    DomNode* get_parent() const
    {
        return this->m_parent;
    }

    const std::vector<DomNode*>* get_children() const
    {
        return &this->m_children;
    }

    const char* get_text() const
    {
        return this->m_text.c_str();
    }

    const std::string& get_text_string() const
    {
        return this->m_text;
    }

    const char* get_tag() const
    {
        return this->m_tag.c_str();
    }

    void set_tag(const std::string& tag_name)
    {
        this->m_tag = tag_name;
    }

    const char* get_attribute(const char* name) const;
    
    bool has_extra(int key) const
    {
        return this->m_extras.find(key) != this->m_extras.end();
    }


    void set_extra(int key, double value)
    {
        this->m_extras[key] = value;
    }

    const std::map<int, double>& get_extras() const
    {
        return this->m_extras;
    }

    static void register_node_dropped(void (*func)(DomNode*))
    {
        DomNode::node_dropped = func;
    }

    void print_node() const;

public:
    void find_tags(const char* tag_name, std::vector<DomNode*>& results);
    void find_tags(const std::vector<std::string>& tag_names, std::vector<DomNode*>& results);
    static bool drop_node(DomNode* node);
    bool preorder_traverse(DomTreeVisitor& visitor);
    bool postorder_traverse(DomTreeVisitor& visitor);
    double get_extra(int key) const;
    bool get_extra(int key, double& result) const;
    double get_extra_default(int key, double default_value) const;

protected:
    std::string m_tag;
    std::string m_text;
    DomNode* m_parent;
    std::vector<DomNode*> m_children;
    std::map<std::string, std::string> m_attributes;
    std::map<int, double> m_extras;

    static void (*node_dropped)(DomNode*);
};

#endif
