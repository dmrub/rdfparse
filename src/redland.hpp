/*
 * redland.hpp
 *
 *  Created on: Feb 16, 2015
 *      Author: Dmitri Rubinstein
 */

#ifndef RDW_HPP_INCLUDED
#define RDW_HPP_INCLUDED

#include <redland.h>
#include <utility>
#include <exception>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <unordered_set>
#include <istream>
#include <ostream>
#include <iterator>

// Macros from Boost C++ Libraries

//
// Helper macro RDW_STRINGIZE:
// Converts the parameter X to a string after macro replacement
// on X has been performed.
//
#define RDW_STRINGIZE(X) RDW_DO_STRINGIZE(X)
#define RDW_DO_STRINGIZE(X) #X

//
// Helper macro RDW_JOIN:
// The following piece of macro magic joins the two
// arguments together, even when one of the arguments is
// itself a macro (see 16.3.1 in C++ standard).  The key
// is other macro expansion of macro arguments does not
// occur in RDW_DO_JOIN2 but does in RDW_DO_JOIN.
//
#define RDW_JOIN( X, Y ) RDW_DO_JOIN( X, Y )
#define RDW_DO_JOIN( X, Y ) RDW_DO_JOIN2(X,Y)
#define RDW_DO_JOIN2( X, Y ) X##Y

namespace Redland
{

class Exception : public std::exception
{
public:

    explicit Exception(const std::string &arg) : msg(arg) { }

    virtual ~Exception() throw() { }

    /** Returns a C-style character string describing the general cause of
     *  the current error (the same string passed to the ctor).  */
    virtual const char* what() const throw() { return msg.c_str(); }

private:
    std::string msg;
};

#define RDW_THROW_EXCEPTION(ExcClass, message)                  \
do {                                                            \
   std::ostringstream msgs;                                     \
   msgs << RDW_STRINGIZE(ExcClass) " exception occurred ("      \
        << __FILE__ ":" RDW_STRINGIZE(__LINE__) "): "           \
        << message;                                             \
   throw ExcClass(msgs.str());                                  \
} while(false)


class AllocException : public Exception
{
public:
    explicit AllocException(const std::string &arg) : Exception(arg) { }
};

template <typename T>
class CObjWrapper
{
public:
    typedef T c_type;

    T * c_obj() const { return c_obj_; }

    T * release()
    {
        T *tmp = c_obj_;
        c_obj_ = NULL;
        return tmp;
    }

    bool is_valid() const
    {
        return c_obj_ != 0;
    }

protected:
    T * c_obj_;
    CObjWrapper(T *c_obj) : c_obj_(c_obj) { }
    CObjWrapper(CObjWrapper && other) : c_obj_(other.release()) { }

    CObjWrapper & operator=(CObjWrapper && other)
    {
        c_obj_ = other.release();
        return *this;
    }

};

class World : public CObjWrapper<librdf_world>
{
public:

    World()
        : CObjWrapper(librdf_new_world())
    {
        if (!c_obj_)
            throw AllocException("librdf_new_world");
        librdf_world_open(c_obj_);
    }

    World(const World &) = delete;

    World(World && other)
        : CObjWrapper(std::move(other))
    {
    }

    World & operator=(World && other)
    {
        return static_cast<World&>(CObjWrapper::operator=(std::move(other)));
    }

    World & operator=(const World & other) = delete;

    ~World()
    {
        librdf_free_world(c_obj_);
    }

};

class Namespaces
{
public:
    Namespaces() { }

    void add_prefix(const std::string& prefix, const std::string& uri)
    {
        prefixToUriMap_[prefix] = uri;
    }

    std::string expand(const std::string uri) const
    {
        std::string::size_type i = uri.find(":");
        if (i == std::string::npos)
            return uri;
        std::string prefix = uri.substr(0, i);
        std::map<std::string, std::string>::const_iterator it = prefixToUriMap_.find(prefix);
        if (it == prefixToUriMap_.end())
            return uri;
        return it->second + uri.substr(i+1);
    }

    void register_with_serializer(const World &world, librdf_serializer *ser) const
    {
        librdf_uri *uri;
        for (std::map<std::string, std::string>::const_iterator it = prefixToUriMap_.begin();
            it != prefixToUriMap_.end(); ++it)
        {
            uri = librdf_new_uri(world.c_obj(), (const unsigned char*)it->second.c_str());
            librdf_serializer_set_namespace(ser, uri, it->first.c_str());
            librdf_free_uri(uri);
        }
    }

private:
    std::map<std::string, std::string> prefixToUriMap_;
};

class Uri : public CObjWrapper<librdf_uri>
{
public:

    Uri()
        : CObjWrapper(0)
    { }

    Uri(const World &world, const unsigned char *uri_string)
        : CObjWrapper(librdf_new_uri(world.c_obj(), uri_string))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_uri");
    }

    Uri(const World &world, const unsigned char *uri_string, size_t length)
        : CObjWrapper(librdf_new_uri2(world.c_obj(), uri_string, length))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_uri2");
    }

    Uri(const World &world, const char *uri_string)
        : Uri(world, (const unsigned char *)uri_string)
    {
    }

    Uri(const World &world, const std::string &uri_string)
        : Uri(world, uri_string.c_str())
    {
    }

    Uri(const World &world, const char *uri_string, size_t length)
        : Uri(world, (const unsigned char *)uri_string, length)
    {
    }

    Uri(const Uri &other)
        : CObjWrapper(librdf_new_uri_from_uri(other.c_obj()))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_uri_from_uri");
    }

    Uri(Uri && other)
        : CObjWrapper(std::move(other))
    {
    }

    Uri & operator=(Uri && other)
    {
        librdf_free_uri(c_obj_);
        c_obj_ = 0;
        return static_cast<Uri&>(CObjWrapper::operator=(std::move(other)));
    }

    Uri & operator=(const Uri & other)
    {
        if (this != &other)
        {
            librdf_free_uri(c_obj_);
            c_obj_ = librdf_new_uri_from_uri(other.c_obj());
        }
        return *this;
    }

    ~Uri()
    {
        librdf_free_uri(c_obj_);
    }

    const char * to_string() const
    {
        return reinterpret_cast<const char *>(c_obj_ ? librdf_uri_to_string(c_obj_) : 0);
    }

    bool operator==(const Uri &uri) const
    {
        return librdf_uri_equals(c_obj_, uri.c_obj()) != 0;
    }
};


class Storage : public CObjWrapper<librdf_storage>
{
public:
    Storage(const World &world,
            const char *storage_name,
            const char *name,
            const char *options_string)
        : CObjWrapper(librdf_new_storage(world.c_obj(), storage_name, name, options_string))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_storage");
    }

    Storage(const Storage &other)
        : CObjWrapper(librdf_new_storage_from_storage(other.c_obj()))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_storage_from_storage");
    }

    Storage(Storage && other)
        : CObjWrapper(std::move(other))
    {
    }

    Storage & operator=(Storage && other)
    {
        librdf_free_storage(c_obj_);
        c_obj_ = 0;
        return static_cast<Storage&>(CObjWrapper::operator=(std::move(other)));
    }

    Storage & operator=(const Storage & other)
    {
        if (this != &other)
        {
            librdf_free_storage(c_obj_);
            c_obj_ = librdf_new_storage_from_storage(other.c_obj());
        }
        return *this;
    }

    ~Storage()
    {
        librdf_free_storage(c_obj_);
    }
};

struct blank_node_t { };

class Node : public CObjWrapper<librdf_node>
{
public:

    Node()
        : CObjWrapper(0)
    { }

    Node(librdf_node *node)
        : CObjWrapper(node)
    { }

    Node(const World &world)
        : CObjWrapper(librdf_new_node(world.c_obj()))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_node");
    }

    Node(const Node &other)
        : CObjWrapper(other.is_valid() ? librdf_new_node_from_node(other.c_obj()) : 0)
    {
        if (other.is_valid() && !c_obj_)
            throw AllocException("librdf_new_node_from_node");
    }

    Node(Node && other)
        : CObjWrapper(std::move(other))
    {
    }

    Node & operator=(Node && other)
    {
        librdf_free_node(c_obj_);
        c_obj_ = 0;
        return static_cast<Node&>(CObjWrapper::operator=(std::move(other)));
    }

    Node & operator=(const Node & other)
    {
        if (this != &other)
        {
            librdf_free_node(c_obj_);
            c_obj_ = other.is_valid() ? librdf_new_node_from_node(other.c_obj()) : 0;
        }
        return *this;
    }

    Node(const World &world, const char *uri_string)
        : Node(world, (const unsigned char*)uri_string)
    { }

    Node(const World &world, const std::string &uri_string)
        : Node(world, (const unsigned char*)uri_string.c_str())
    { }

    Node(const World &world, const unsigned char *uri_string)
        : CObjWrapper(librdf_new_node_from_uri_string(world.c_obj(), uri_string))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_node_from_uri_string");
    }

    Node(const World &world, const unsigned char *value, const Uri &datatype_uri)
        : CObjWrapper(librdf_new_node_from_typed_literal(world.c_obj(), value, NULL, datatype_uri.c_obj()))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_node_from_typed_literal");
    }

    Node(const World &world, const char *value, const Uri &datatype_uri)
        : Node(world, (const unsigned char *)value, datatype_uri)
    {
    }

    Node(const World &world, const std::string &value, const Uri &datatype_uri)
        : Node(world, (const unsigned char *)value.c_str(), datatype_uri)
    {
    }

    Node(const World &world,
         const unsigned char *string,
         const char *xml_language,
         int is_wf_xml)
        : CObjWrapper(librdf_new_node_from_literal(world.c_obj(), string, xml_language, is_wf_xml))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_node_from_literal");
    }

    Node(const World &world,
         const char *string,
         const char *xml_language,
         bool is_wf_xml)
        : Node(world, (const unsigned char *)string, xml_language, is_wf_xml ? 1 : 0)
    {
    }

    Node(const World &world, const unsigned char *identifier, blank_node_t)
        : CObjWrapper(librdf_new_node_from_blank_identifier(world.c_obj(), identifier))
    { }

    Node(const World &world, const char *identifier, blank_node_t tag)
        : Node(world, (const unsigned char *)identifier, tag)
    { }

    Node(const World &world, blank_node_t tag)
        : Node(world, (const unsigned char *)NULL, tag)
    { }

    ~Node()
    {
        librdf_free_node(c_obj_);
    }

    bool operator==(const Node &other) const
    {
        return librdf_node_equals(c_obj(), other.c_obj()) == 0;
    }

    bool is_blank() const { return librdf_node_is_blank(c_obj_); }

    bool is_literal() const { return librdf_node_is_literal(c_obj_); }

    std::string get_uri_as_string() const
    {
        const char *s = reinterpret_cast<const char *>(librdf_uri_as_string(librdf_node_get_uri(c_obj())));
        return s ? s : "";
    }

    std::string get_literal_value() const
    {
        const char *s = reinterpret_cast<const char *>(librdf_node_get_literal_value(c_obj()));
        return s ? s : "";
    }

    std::string get_blank_identifier() const
    {
        const char *s = reinterpret_cast<const char *>(librdf_node_get_blank_identifier(c_obj()));
        return s ? s : "";
    }

    std::string to_string() const
    {
        return is_blank() ? get_blank_identifier() : (is_literal() ? get_literal_value() : get_uri_as_string());
    }

    static Node make_blank_node(const World &world)
    {
        return Node(world, blank_node_t());
    }

    static Node make_literal_node(const World &world,
                             const char *string,
                             const char *xml_language,
                             bool is_wf_xml)
    {
        return Node(world, string, xml_language, is_wf_xml);
    }

    static Node make_literal_node(const World &world, const char *string)
    {
        return make_literal_node(world, string, 0, false);
    }

    static Node make_typed_literal_node(const World &world, const char *value, const Uri &datatype_uri)
    {
        return Node(world, value, datatype_uri);
    }

    static Node make_typed_literal_node(const World &world, const std::string &value, const Uri &datatype_uri)
    {
        return Node(world, value, datatype_uri);
    }

    static Node make_value_node(const World &world, double value)
    {
        Redland::Uri xsd_type(world, "http://www.w3.org/2001/XMLSchema#double");
        return make_typed_literal_node(world, std::to_string(value), xsd_type);
    }

    static Node make_value_node(const World &world, float value)
    {
        Redland::Uri xsd_type(world, "http://www.w3.org/2001/XMLSchema#float");
        return make_typed_literal_node(world, std::to_string(value), xsd_type);
    }

    static Node make_value_node(const World &world, const std::string &value)
    {
        Redland::Uri xsd_type(world, "http://www.w3.org/2001/XMLSchema#string");
        return make_typed_literal_node(world, value, xsd_type);
    }

    static Node make_uri_node(const World &world, const char *uri_string)
    {
        return Node(world, uri_string);
    }

    static Node make_uri_node(const World &world, const std::string &uri_string)
    {
        return Node(world, uri_string);
    }

};

struct shallow_copy_t { };

class Statement : public CObjWrapper<librdf_statement>
{
public:

    Statement()
        : CObjWrapper(0)
    { }

    Statement(librdf_statement *statement)
        : CObjWrapper(statement)
    { }

    Statement(const World &world)
        : CObjWrapper(librdf_new_statement(world.c_obj()))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_statement");
    }

    Statement(const Statement &other)
        : CObjWrapper(other.is_valid() ? librdf_new_statement_from_statement(other.c_obj()) : 0)
    {
        if (other.is_valid() && !c_obj_)
            throw AllocException("librdf_new_statement_from_statement");
    }

    Statement(const Statement &other, shallow_copy_t)
        : CObjWrapper(other.is_valid() ? librdf_new_statement_from_statement2(other.c_obj()) : 0)
    {
        if (other.is_valid() && !c_obj_)
            throw AllocException("librdf_new_statement_from_statement2");
    }

    Statement(Statement && other)
        : CObjWrapper(std::move(other))
    {
    }

    Statement & operator=(Statement && other)
    {
        librdf_free_statement(c_obj_);
        c_obj_ = 0;
        return static_cast<Statement&>(CObjWrapper::operator=(std::move(other)));
    }

    Statement & operator=(const Statement & other)
    {
        if (this != &other)
        {
            librdf_free_statement(c_obj_);
            c_obj_ = other.is_valid() ? librdf_new_statement_from_statement(other.c_obj()) : 0;
        }
        return *this;
    }

    Statement(const World &world, Node subject, Node predicate, Node object)
        : CObjWrapper(librdf_new_statement_from_nodes(world.c_obj(), subject.release(), predicate.release(), object.release()))
    {
        if (!c_obj_)
            throw AllocException("librdf_new_statement_from_nodes");
    }

    ~Statement()
    {
        librdf_free_statement(c_obj_);
    }

    Node get_subject() const
    {
        if (is_valid())
        {
            if (librdf_node *n = librdf_statement_get_subject(c_obj_))
                return Node(librdf_new_node_from_node(n));
        }
        return Node();
    }

    Node get_predicate() const
    {
        if (is_valid())
        {
            if (librdf_node *n = librdf_statement_get_predicate(c_obj_))
                return Node(librdf_new_node_from_node(n));
        }
        return Node();
    }

    Node get_object() const
    {
        if (is_valid())
        {
            if (librdf_node *n = librdf_statement_get_object(c_obj_))
                return Node(librdf_new_node_from_node(n));
        }
        return Node();
    }

    void set_subject(Node node)
    {
        librdf_statement_set_subject(c_obj_, node.release());
    }

    void set_predicate(Node node)
    {
        librdf_statement_set_predicate(c_obj_, node.release());
    }

    void set_object(Node node)
    {
        librdf_statement_set_object(c_obj_, node.release());
    }

};

/**
 * Iterator - Iterate a sequence of objects across some other object.
 * http://librdf.org/docs/api/redland-iterator.html
 */
class Iterator : public CObjWrapper<librdf_iterator>
{
public:

    Iterator()
        : CObjWrapper(0)
    { }

    Iterator(librdf_iterator *iterator)
        : CObjWrapper(iterator)
    { }

    Iterator(const World &world)
        : CObjWrapper(librdf_new_empty_iterator(world.c_obj()))
    {
        if (!is_valid())
            throw AllocException("librdf_new_empty_iterator");
    }

    Iterator(const Iterator &other) = delete;

    Iterator(Iterator && other)
        : CObjWrapper(std::move(other))
    {
    }

    ~Iterator()
    {
        librdf_free_iterator(c_obj_);
    }

    Iterator & operator=(Iterator && other)
    {
        librdf_free_iterator(c_obj_);
        c_obj_ = 0;
        return static_cast<Iterator&>(CObjWrapper::operator=(std::move(other)));
    }

    Iterator & operator=(const Iterator & other) = delete;

    bool is_end() const
    {
        return librdf_iterator_end(c_obj_) != 0;
    }

    bool next()
    {
        return librdf_iterator_next(c_obj_) != 0;
    }

    void * get_object() const
    {
        return librdf_iterator_get_object(c_obj_);
    }

    Node get_context() const
    {
        librdf_node *context = (librdf_node*)librdf_iterator_get_context(c_obj_);
        if (context)
        {
            context = librdf_new_node_from_node(context);
        }
        return Node(context);
    }

    void * get_key() const
    {
        return librdf_iterator_get_key(c_obj_);
    }

    void * get_value() const
    {
        return librdf_iterator_get_value(c_obj_);
    }

    bool add_map(librdf_iterator_map_handler map_function,
                 librdf_iterator_map_free_context_handler free_context,
                 void *map_context)
    {
        return librdf_iterator_add_map(c_obj_, map_function, free_context, map_context) == 0;
    }
};


/**
 * Stream of triples (#librdf_statement). - Sequence of RDF Triples.
 * http://librdf.org/docs/api/redland-stream.html
 */
class Stream : public CObjWrapper<librdf_stream>
{
public:

    Stream()
        : CObjWrapper(0)
    { }

    Stream(librdf_stream *stream)
        : CObjWrapper(stream)
    { }

    Stream(const World &world)
        : CObjWrapper(librdf_new_empty_stream(world.c_obj()))
    {
        if (!is_valid())
            throw AllocException("librdf_new_empty_stream");
    }

    Stream(const Stream &other) = delete;

    Stream(Stream && other)
        : CObjWrapper(std::move(other))
    {
    }

    ~Stream()
    {
        librdf_free_stream(c_obj_);
    }

    Stream & operator=(Stream && other)
    {
        librdf_free_stream(c_obj_);
        c_obj_ = 0;
        return static_cast<Stream&>(CObjWrapper::operator=(std::move(other)));
    }

    Stream & operator=(const Stream & other) = delete;

    bool is_end() const
    {
        return librdf_stream_end(c_obj_) != 0;
    }

    bool next()
    {
        return librdf_stream_next(c_obj_) != 0;
    }

    Statement get_object() const
    {
        librdf_statement *stmt = librdf_stream_get_object(c_obj_);
        if (stmt)
        {
            stmt = librdf_new_statement_from_statement(stmt);
        }
        return Statement(stmt);
    }

    Node get_context() const
    {
        librdf_node *context = librdf_stream_get_context2(c_obj_);
        if (context)
        {
            context = librdf_new_node_from_node(context);
        }
        return Node(context);
    }

    bool add_map(librdf_stream_map_handler map_function,
                 librdf_stream_map_free_context_handler free_context,
                 void *map_context)
    {
        return librdf_stream_add_map(c_obj_, map_function, free_context, map_context) == 0;
    }

    void write(raptor_iostream *iostr)
    {
        librdf_stream_write(c_obj_, iostr);
    }

    template <class OutputIt, class Size>
    OutputIt copy_n(OutputIt first, Size count)
    {
        Size i = 0;
        while (!is_end() && (i < count))
        {
            if (librdf_statement * stmt = librdf_stream_get_object(c_obj_))
                *first++ = std::move(Statement(librdf_new_statement_from_statement(stmt)));
            next();
            i++;
        }
        return first;
    }

    template <class OutputIt>
    OutputIt copy(OutputIt first)
    {
        while (!is_end())
        {
            if (librdf_statement * stmt = librdf_stream_get_object(c_obj_))
                *first++ = std::move(Statement(librdf_new_statement_from_statement(stmt)));
            next();
        }
        return first;
    }

    /**
     * Create Stream class from statement container. Container reference must be valid
     * as long as returned stream is used.
     */
    template <class T>
    static Redland::Stream create_from(const T &statements, const Redland::World &world)
    {
        StreamContext<T> *sctx = new StreamContext<T>(statements);
        return Stream(
            librdf_new_stream(world.c_obj(), sctx,
            StreamContext<T>::is_end,
            StreamContext<T>::next,
            StreamContext<T>::get,
            StreamContext<T>::finished));
    }

private:
    template <class T>
    struct StreamContext
    {
        typename T::const_iterator current;
        typename T::const_iterator end;

        StreamContext(const T &container)
            : current(container.begin())
            , end(container.end()) { }

        static int is_end(void *ctx)
        {
            StreamContext *sctx = (StreamContext *)ctx;
            return sctx->current == sctx->end ? 1 : 0;
        }

        static int next(void *ctx)
        {
            StreamContext *sctx = (StreamContext *)ctx;
            if (sctx->current == sctx->end)
                return 1;
            ++sctx->current;
            return 0;
        }

        static void * get(void *ctx, int flags)
        {
            StreamContext *sctx = (StreamContext *)ctx;
            switch(flags) {
              case LIBRDF_STREAM_GET_METHOD_GET_OBJECT:
                return sctx->current->c_obj();
              case LIBRDF_STREAM_GET_METHOD_GET_CONTEXT:
                return NULL;
              default:
                return NULL;
            }
        }

        static void finished(void *ctx)
        {
            StreamContext *sctx = (StreamContext *)ctx;
            delete sctx;
        }
    };
};


class Model : public CObjWrapper<librdf_model>
{
public:

    Model(const World &world, const Storage &storage, const char *options_string)
        : CObjWrapper(librdf_new_model(world.c_obj(), storage.c_obj(), options_string))
        , world_(&world)
    { }

    Model(Model && other)
        : CObjWrapper(std::move(other))
        , world_(other.world_)
    {
    }

    const World & get_world() const { return *world_; }

    Model & operator=(Model && other)
    {
        librdf_free_model(c_obj_);
        c_obj_ = 0;
        world_ = other.world_;
        return static_cast<Model&>(CObjWrapper::operator=(std::move(other)));
    }

    Model & operator=(const Model & other)
    {
        if (this != &other)
        {
            librdf_free_model(c_obj_);
            c_obj_ = other.is_valid() ? librdf_new_model_from_model(other.c_obj()) : 0;
            world_ = other.world_;
        }
        return *this;
    }

    ~Model()
    {
        librdf_free_model(c_obj_);
    }

    bool add_statement(const Statement &statement)
    {
        return librdf_model_add_statement(c_obj_, statement.c_obj()) == 0;
    }

    bool add_statement(const Node &context, const Statement &statement)
    {
        return librdf_model_context_add_statement(c_obj_, context.c_obj(), statement.c_obj()) == 0;
    }

    template <class N1, class N2, class N3>
    bool add_statement(const World &world, N1 &&subject, N2 &&predicate, N3 &&object)
    {
        return add_statement(Statement(world,
            std::forward<N1>(subject),
            std::forward<N2>(predicate),
            std::forward<N3>(object)));
    }

    template <class N1, class N2, class N3, class N4>
    bool add_statement(const World &world, N1 &&context, N2 &&subject, N3 &&predicate, N4 &&object)
    {
        return add_statement(std::forward<N1>(context),
            Statement(world,
            std::forward<N2>(subject),
            std::forward<N3>(predicate),
            std::forward<N4>(object)));
    }

    bool remove_statement(const Statement &statement)
    {
        return librdf_model_remove_statement(c_obj_, statement.c_obj()) == 0;
    }

    bool remove_statement(const Node &context, const Statement &statement)
    {
        return librdf_model_context_remove_statement(c_obj_, context.c_obj(), statement.c_obj()) == 0;
    }

    Stream get_context_as_stream(const Node &context) const
    {
        return Stream(librdf_model_context_as_stream(c_obj_, context.c_obj()));
    }

    bool remove_context_statements(const Node &context)
    {
        return librdf_model_context_remove_statements(c_obj_, context.c_obj()) == 0;
    }

    void remove_all_statements()
    {
        librdf_stream *sr = librdf_model_as_stream(c_obj());
        if (!librdf_stream_end(sr))
        {
            librdf_statement * stmt = librdf_stream_get_object(sr);
            if (stmt)
            {
                librdf_model_remove_statement(c_obj(), stmt);
            }
        }
        librdf_free_stream(sr);
    }

    bool contains_context(const Node &context) const
    {
        return librdf_model_contains_context(c_obj_, context.c_obj()) != 0;
    }

    bool supports_contexts() const
    {
        return librdf_model_supports_contexts(c_obj_) != 0;
    }

    bool sync() const
    {
        return librdf_model_sync(c_obj_) == 0;
    }

    Stream as_stream() const
    {
        return Stream(librdf_model_as_stream(c_obj_));
    }

    bool has_statement(const Statement &statement) const
    {
        librdf_stream *sr = librdf_model_find_statements(c_obj(), statement.c_obj());
        bool found = !librdf_stream_end(sr);
        librdf_free_stream(sr);
        return found;
    }

    template <class OutputIt, class Size>
    OutputIt find_statements(OutputIt first, Size count, const Statement &statement) const
    {
        if (librdf_stream *sr = librdf_model_find_statements(c_obj_, statement.c_obj()))
        {
            return Stream(sr).copy_n(first, count);
        }
        return first;
    }

    template <class OutputIt>
    OutputIt find_statements(OutputIt first, const Statement &statement) const
    {
        if (librdf_stream *sr = librdf_model_find_statements(c_obj_, statement.c_obj()))
        {
            return Stream(sr).copy(first);
        }
        return first;
    }

    Statement find_statement(const Statement &statement) const
    {
        Statement stmt;
        if (librdf_stream *sr = librdf_model_find_statements(c_obj(), statement.c_obj()))
        {
            Stream stream(sr);
            if (!stream.is_end())
                stmt = std::move(stream.get_object());
        }
        return stmt;
    }

    Statement find_statement(const World &world, Node subject, Node predicate, Node object)
    {
        return find_statement(Statement(world, subject, predicate, object));
    }

    std::vector<Statement> find_statements(const Statement &statement) const
    {
        std::vector<Statement> result;
        find_statements(std::back_inserter<std::vector<Statement> >(result), statement);
        return result;
    }

    std::vector<Statement> find_statements(const World &world, Node subject, Node predicate, Node object) const
    {
        return find_statements(Statement(world, subject, predicate, object));
    }

    Stream find_statements_as_stream(const Statement &statement) const
    {
        return Stream(librdf_model_find_statements(c_obj(), statement.c_obj()));
    }

    Stream find_statements_in_context(const Statement &statement, const Node &context_node)
    {
        return Stream(librdf_model_find_statements_in_context(c_obj_, statement.c_obj(), context_node.c_obj()));
    }

private:
    const World * world_;
};

raptor_iostream* raptor_new_iostream_from_std_istream(
  raptor_world* world,
  std::istream* stream);

raptor_iostream* raptor_new_iostream_to_std_ostream(
  raptor_world* world,
  std::ostream* stream);

/**
 * Serializers - RDF serializers from triples to syntax.
 * http://librdf.org/docs/api/redland-serializer.html
 */
class Serializer : public CObjWrapper<librdf_serializer>
{
public:

    Serializer()
        : CObjWrapper(0)
    { }

    Serializer(librdf_serializer *serializer)
        : CObjWrapper(serializer)
    { }

    Serializer(const World &world,
               const char *name = 0,
               const char *mime_type = 0,
               const Uri &type_uri = Uri())
        : CObjWrapper(librdf_new_serializer(world.c_obj(), name, mime_type, type_uri.c_obj()))
    {
        if (!is_valid())
            throw AllocException("librdf_new_serializer");
    }

    Serializer(const World &world,
               const Namespaces &namespaces,
               const char *name = 0,
               const char *mime_type = 0,
               const Uri &type_uri = Uri())
        : Serializer(world, name, mime_type, type_uri)
    {
        namespaces.register_with_serializer(world, c_obj_);
    }

    void register_namespaces(const World &world, const Namespaces &namespaces)
    {
        namespaces.register_with_serializer(world, c_obj_);
    }


    Serializer(const Serializer &other) = delete;

    Serializer(Serializer && other)
        : CObjWrapper(std::move(other))
    {
    }

    ~Serializer()
    {
        librdf_free_serializer(c_obj_);
    }

    Serializer & operator=(Serializer && other)
    {
        librdf_free_serializer(c_obj_);
        c_obj_ = 0;
        return static_cast<Serializer&>(CObjWrapper::operator=(std::move(other)));
    }

    Serializer & operator=(const Serializer & other) = delete;

    static bool check_name(const World &world, const char *name)
    {
        return librdf_serializer_check_name(world.c_obj(), name) != 0;
    }

    bool serialize_model(FILE *handle, const Uri &base_uri, const Model &model)
    {
        return librdf_serializer_serialize_model_to_file_handle(c_obj_, handle, base_uri.c_obj(), model.c_obj()) == 0;
    }

    bool serialize_model(FILE *handle, const Model &model)
    {
        return librdf_serializer_serialize_model_to_file_handle(c_obj_, handle, 0, model.c_obj()) == 0;
    }

    bool serialize_model_to_file(const char *file_name, const Uri &base_uri, const Model &model)
    {
        return librdf_serializer_serialize_model_to_file(c_obj_, file_name, base_uri.c_obj(), model.c_obj()) == 0;
    }

    bool serialize_model_to_file(const char *file_name, const Model &model)
    {
        return librdf_serializer_serialize_model_to_file(c_obj_, file_name, 0, model.c_obj()) == 0;
    }

    bool serialize_model(std::string &dest, const Uri &base_uri, const Model &model)
    {
        unsigned char *str = librdf_serializer_serialize_model_to_string(c_obj_, base_uri.c_obj(), model.c_obj());
        if (!str)
            return false;
        dest.assign(reinterpret_cast<char*>(str));
        librdf_free_memory(str);
        return true;
    }

    bool serialize_model(std::string &dest, const Model &model)
    {
        return serialize_model(dest, Uri(), model);
    }

    bool serialize_model(raptor_iostream *iostr, const Uri &base_uri, const Model &model)
    {
        return librdf_serializer_serialize_model_to_iostream(c_obj_, base_uri.c_obj(), model.c_obj(), iostr) == 0;
    }

    bool serialize_model(std::ostream &out, const Uri &base_uri, const Model &model)
    {
        raptor_world *rw = librdf_world_get_raptor(model.get_world().c_obj());
        if (!rw)
            return false;
        raptor_iostream *iostr = raptor_new_iostream_to_std_ostream(rw, &out);
        if (!iostr)
            return false;
        bool result = serialize_model(iostr, base_uri, model);
        raptor_free_iostream(iostr);
        return result;
    }

    // Stream serialization

    bool serialize_stream(FILE *handle, const Uri &base_uri, const Stream &stream)
    {
        return librdf_serializer_serialize_stream_to_file_handle(c_obj_, handle, base_uri.c_obj(), stream.c_obj()) == 0;
    }

    bool serialize_stream_to_file(const char *file_name, const Uri &base_uri, const Stream &stream)
    {
        return librdf_serializer_serialize_stream_to_file(c_obj_, file_name, base_uri.c_obj(), stream.c_obj()) == 0;
    }

    bool serialize_stream_to_file(const char *file_name, const Stream &stream)
    {
        return librdf_serializer_serialize_stream_to_file(c_obj_, file_name, 0, stream.c_obj()) == 0;
    }

    bool serialize_stream(std::string &dest, const Uri &base_uri, const Stream &stream)
    {
        unsigned char *str = librdf_serializer_serialize_stream_to_string(c_obj_, base_uri.c_obj(), stream.c_obj());
        if (!str)
            return false;
        dest.assign(reinterpret_cast<char*>(str));
        librdf_free_memory(str);
        return true;
    }

    bool serialize_stream(std::string &dest, const Stream &stream)
    {
        return serialize_stream(dest, Uri(), stream);
    }

    bool serialize_stream(raptor_iostream *iostr, const Uri &base_uri, const Stream &stream)
    {
        return librdf_serializer_serialize_stream_to_iostream(c_obj_, base_uri.c_obj(), stream.c_obj(), iostr) == 0;
    }

    bool serialize_stream(std::ostream &out, const Uri &base_uri, const Stream &stream, const World &world)
    {
        raptor_world *rw = librdf_world_get_raptor(world.c_obj());
        if (!rw)
            return false;
        raptor_iostream *iostr = raptor_new_iostream_to_std_ostream(rw, &out);
        if (!iostr)
            return false;
        bool result = serialize_stream(iostr, base_uri, stream);
        raptor_free_iostream(iostr);
        return result;
    }

};

/**
 * Parser - RDF parsers from syntax to triples.
 * http://librdf.org/docs/api/redland-parser.html
 */
class Parser : public CObjWrapper<librdf_parser>
{
public:

    Parser()
        : CObjWrapper(0)
    { }

    Parser(librdf_parser *parser)
        : CObjWrapper(parser)
    { }

    Parser(const World &world,
           const char *name = 0,
           const char *mime_type = 0,
           const Uri &type_uri = Uri())
        : CObjWrapper(librdf_new_parser(world.c_obj(), name, mime_type, type_uri.c_obj()))
    {
        if (!is_valid())
            throw AllocException("librdf_new_parser");
    }

    Parser(const Parser &other) = delete;

    Parser(Parser && other)
        : CObjWrapper(std::move(other))
    {
    }

    ~Parser()
    {
        librdf_free_parser(c_obj_);
    }

    Parser & operator=(Parser && other)
    {
        librdf_free_parser(c_obj_);
        c_obj_ = 0;
        return static_cast<Parser&>(CObjWrapper::operator=(std::move(other)));
    }

    Parser & operator=(const Parser & other) = delete;

    static bool check_name(const World &world, const char *name)
    {
        return librdf_parser_check_name(world.c_obj(), name) != 0;
    }

    Stream parse_as_stream(const Uri &uri, const Uri &base_uri)
    {
        return Stream(librdf_parser_parse_as_stream(c_obj_, uri.c_obj(), base_uri.c_obj()));
    }

    bool parse_into_model(const Uri &uri, const Uri &base_uri, const Model &model)
    {
        return librdf_parser_parse_into_model(c_obj_, uri.c_obj(), base_uri.c_obj(), model.c_obj()) == 0;
    }

    Stream parse_as_stream(FILE *handle, bool close_fh, const Uri &base_uri)
    {
        return Stream(librdf_parser_parse_file_handle_as_stream(c_obj_, handle, close_fh ? 1 : 0, base_uri.c_obj()));
    }

    bool parse_into_model(FILE *handle, bool close_fh, const Uri &base_uri, const Model &model)
    {
        return librdf_parser_parse_file_handle_into_model(c_obj_, handle, close_fh ? 1 : 0, base_uri.c_obj(), model.c_obj()) == 0;
    }

    Stream parse_as_stream(const char *str, const Uri &base_uri)
    {
        return Stream(librdf_parser_parse_string_as_stream(c_obj_, reinterpret_cast<const unsigned char *>(str), base_uri.c_obj()));
    }

    Stream parse_as_stream(const std::string &str, const Uri &base_uri)
    {
        return parse_as_stream(str.c_str(), base_uri);
    }

    bool parse_into_model(const char *str, const Uri &base_uri, const Model &model)
    {
        return librdf_parser_parse_string_into_model(
            c_obj_, reinterpret_cast<const unsigned char *>(str), base_uri.c_obj(), model.c_obj()) == 0;
    }

    bool parse_into_model(const std::string &str, const Uri &base_uri, const Model &model)
    {
        return parse_into_model(str.c_str(), base_uri, model);
    }

    Stream parse_as_stream(const char *str, size_t length, const Uri &base_uri)
    {
        return Stream(librdf_parser_parse_counted_string_as_stream(
            c_obj_, reinterpret_cast<const unsigned char *>(str), length, base_uri.c_obj()));
    }

    bool parse_into_model(const char *str, size_t length, const Uri &base_uri, const Model &model)
    {
        return librdf_parser_parse_counted_string_into_model(
            c_obj_, reinterpret_cast<const unsigned char *>(str), length, base_uri.c_obj(), model.c_obj()) == 0;
    }

    Stream parse_as_stream(raptor_iostream *iostr, const Uri &base_uri)
    {
        return Stream(librdf_parser_parse_iostream_as_stream(c_obj_, iostr, base_uri.c_obj()));
    }

    Stream parse_as_stream(std::istream &in, const Uri &base_uri, const World &world)
    {
        Stream stream;
        if (raptor_world *rw = librdf_world_get_raptor(world.c_obj()))
        {
            if (raptor_iostream *iostr = raptor_new_iostream_from_std_istream(rw, &in))
            {
                stream = std::move(parse_as_stream(iostr, base_uri));
                raptor_free_iostream(iostr);
            }
        }
        return stream;
    }

    bool parse_into_model(raptor_iostream *iostr, const Uri &base_uri, const Model &model)
    {
        return librdf_parser_parse_iostream_into_model(c_obj_, iostr, base_uri.c_obj(), model.c_obj()) == 0;
    }

    bool parse_into_model(std::istream &in, const Uri &base_uri, const Model &model)
    {
        raptor_world *rw = librdf_world_get_raptor(model.get_world().c_obj());
        if (!rw)
            return false;
        raptor_iostream *iostr = raptor_new_iostream_from_std_istream(rw, &in);
        if (!iostr)
            return false;

        bool result = parse_into_model(iostr, base_uri, model);
        raptor_free_iostream(iostr);
        return result;
    }

    Node get_feature(const Uri &feature)
    {
        return Node(librdf_parser_get_feature(c_obj_, feature.c_obj()));
    }

    bool set_feature(const Uri &feature, const Node &value)
    {
        return librdf_parser_set_feature(c_obj_, feature.c_obj(), value.c_obj()) == 0;
    }
};

inline bool serialize_rdf(FILE *fd, const World &world, const Model &model, Namespaces &namespaces, const char *format_name = "turtle")
{
    librdf_serializer *ser = librdf_new_serializer(world.c_obj(), format_name, NULL, NULL);

    if (!ser)
    {
        std::string errmsg = std::string("Could not load ")+(format_name ? format_name : "<empty>")+" serializer";
        fprintf(stderr, errmsg.c_str());
        return false;
    }

    namespaces.register_with_serializer(world, ser);

    int result = librdf_serializer_serialize_model_to_file_handle(ser, fd, NULL, model.c_obj());

    librdf_free_serializer(ser);

    return result == 0;
}

inline bool serialize_rdf(const char *filename, const World &world, const Model &model, Namespaces &namespaces, const char *format_name = "turtle")
{
    FILE *fd = fopen(filename, "wb");

    bool result = serialize_rdf(fd, world, model, namespaces, format_name);

    fclose(fd);

    return result;
}

inline bool serialize_rdf_to_string(std::string &dest, const World &world, const Model &model, Namespaces &namespaces, const char *format_name = "turtle")
{
    librdf_serializer *ser = librdf_new_serializer(world.c_obj(), format_name, NULL, NULL);

    namespaces.register_with_serializer(world, ser);

    if (!ser)
    {
        std::string errmsg = std::string("Could not load ")+(format_name ? format_name : "<empty>")+" serializer";
        fprintf(stderr, errmsg.c_str());
        return false;
    }

    unsigned char *str = librdf_serializer_serialize_model_to_string(ser, NULL, model.c_obj());
    if (!str)
    {
        librdf_free_serializer(ser);
        return false;
    }
    dest.assign(reinterpret_cast<char*>(str));

    librdf_free_memory(str);
    librdf_free_serializer(ser);

    return true;
}

inline bool serialize_turtle(const char *filename, const World &world, const Model &model, Namespaces &namespaces)
{
    return serialize_rdf(filename, world, model, namespaces, "turtle");
}

inline bool parse_rdf(const char *filename, const char *base_uri, const World &world, const Model &model, const char *format_name = "turtle")
{
    librdf_parser *par = librdf_new_parser(world.c_obj(), "turtle", NULL, NULL);

    if (!par)
    {
        fprintf(stderr, "Could not load turtle parser");
        return false;
    }
    FILE *fd = fopen(filename, "rb");

    int result;

    if (base_uri)
    {
        Uri uri(world, base_uri);
        result = librdf_parser_parse_file_handle_into_model(par, fd, 1, uri.c_obj(), model.c_obj());
    }
    else
    {
        result = librdf_parser_parse_file_handle_into_model(par, fd, 1, NULL, model.c_obj());
    }

    librdf_free_parser(par);

    return result == 0;
}

inline bool parse_turtle(const char *filename, const char *base_uri, const World &world, const Model &model)
{
    return parse_rdf(filename, base_uri, world, model, "turtle");
}

inline bool parse_rdf_from_string(const char *str, const char *base_uri, const World &world, const Model &model, const char *format_name = "turtle")
{
    librdf_parser *par = librdf_new_parser(world.c_obj(), "turtle", NULL, NULL);

    if (!par)
    {
        fprintf(stderr, "Could not load turtle parser");
        return false;
    }

    int result;

    if (base_uri)
    {
        Uri uri(world, base_uri);
        result = librdf_parser_parse_string_into_model(par, (const unsigned char *)str, uri.c_obj(), model.c_obj());
    }
    else
    {
        result = librdf_parser_parse_string_into_model(par, (const unsigned char *)str, NULL, model.c_obj());
    }

    librdf_free_parser(par);

    return result == 0;
}

inline bool parse_turtle_from_string(const char *str, const char *base_uri, const World &world, const Model &model)
{
    return parse_rdf_from_string(str, base_uri, world, model, "turtle");
}

/**
 * Computes fix point of all blank nodes reachable from passed node
 */
template <class OutputIt>
OutputIt add_reachable_blank_nodes(OutputIt first, const Redland::Node &node, std::unordered_set<std::string> &added_names, const Redland::Model &model)
{
    std::string nid =
        node.is_blank() ? ("b:" + node.get_blank_identifier()) :
        (node.is_literal() ? ("l:" + node.get_literal_value()) : ("u:" + node.get_uri_as_string()));
    if (added_names.find(nid) != added_names.end())
        return first;
    added_names.insert(nid);

    Redland::Statement stmt(model.get_world(), node, Redland::Node(), Redland::Node());
    auto stmtsResult = model.find_statements(stmt);
    for (auto it = std::begin(stmtsResult), et = std::end(stmtsResult); it != et; ++it)
    {
        auto object = (*it).get_object();
        *first++ = std::move(*it);

        if (object.is_blank())
            first = add_reachable_blank_nodes(first, object, added_names, model);
    }
    return first;
}

/**
 * Compute directly reachable statements from passed node,
 * including statements containing intermediate blank nodes.
 */
template <class Container>
Container get_reachable_statements(const Redland::Node &node, const Redland::Model &model)
{
    Container result;
    std::unordered_set<std::string> added_names;
    add_reachable_blank_nodes(std::back_insert_iterator<Container>(result), node, added_names, model);
    return result;
}


} // namespace Redland

namespace Raptor
{

using Redland::CObjWrapper;
using Redland::AllocException;

class World : public CObjWrapper<raptor_world>
{
public:

    World()
        : CObjWrapper(raptor_new_world())
    {
        if (!c_obj_)
            throw AllocException("raptor_new_world");
        raptor_world_open(c_obj_);
    }

    World(const World &) = delete;

    World(World && other)
        : CObjWrapper(std::move(other))
    {
    }

    World & operator=(World && other)
    {
        raptor_free_world(c_obj_);
        c_obj_ = 0;
        return static_cast<World&>(CObjWrapper::operator=(std::move(other)));
    }

    World & operator=(const World & other) = delete;

    ~World()
    {
        raptor_free_world(c_obj_);
    }

};



} // namespace Raptor

#endif /* RDW_HPP_INCLUDED */
