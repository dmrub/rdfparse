/*
 * redland.cpp
 *
 *  Created on: Nov 25, 2015
 *      Author: Dmitri Rubinstein
 */
#include "redland.hpp"

#include <cassert> /* for assert() */
#include <cstddef> /* for std::size_t */

/**
 * Code from https://github.com/datagraph/librdf/blob/master/src/rdf%2B%2B/raptor.cc
 */
namespace Redland
{

/**
 * @see http://librdf.org/raptor/api/raptor2-section-iostream.html#raptor-iostream-write-byte-func
 */
static int std_iostream_write_byte(void* const user_data, const int byte)
{
    std::ostream* const stream = reinterpret_cast<std::ostream*>(user_data);
    assert(stream != nullptr);
    try
    {
        stream->put(static_cast<char>(byte));
        return 1; /* success */
    }
    catch (const std::ios_base::failure& error)
    {
        return 0; /* failure */
    }
}

/**
 * http://librdf.org/raptor/api/raptor2-section-iostream.html#raptor-iostream-write-bytes-func
 */
static int std_iostream_write_bytes(void* const user_data, const void* const data, const std::size_t size,
                                    const std::size_t nmemb)
{
    std::ostream* const stream = reinterpret_cast<std::ostream*>(user_data);
    assert(stream != nullptr);
    assert(data != nullptr);
    try
    {
        const std::size_t byte_count = nmemb * size;
        stream->write(reinterpret_cast<const char*>(data), byte_count);
        return static_cast<int>(byte_count); /* success */
    }
    catch (const std::ios_base::failure& error)
    {
        return 0; /* failure */
    }
}

/**
 * @see http://librdf.org/raptor/api/raptor2-section-iostream.html#raptor-iostream-read-bytes-func
 */
static int std_iostream_read_bytes(void* const user_data, void* const data_, const std::size_t size,
                                   const std::size_t nmemb)
{

    std::istream* const stream = reinterpret_cast<std::istream*>(user_data);
    assert(stream != nullptr);
    char* data = reinterpret_cast<char*>(data_);
    assert(data != nullptr);
    try
    {
        if (stream->eof())
            return 0;
        std::size_t count = 0;
        while (count < nmemb)
        {
            stream->read(data + (count * size), size);
            if (stream->fail())
                break;
            count++;
        }
        return static_cast<int>(count);
    }
    catch (const std::ios_base::failure& error)
    {
        return -1; /* failure */
    }
}

/**
 * @see http://librdf.org/raptor/api/raptor2-section-iostream.html#raptor-iostream-read-eof-func
 */
static int std_iostream_read_eof(void* const user_data)
{
    std::istream* const stream = reinterpret_cast<std::istream*>(user_data);
    assert(stream != nullptr);
    try
    {
        return stream->eof() ? 1 : 0;
    }
    catch (const std::ios_base::failure& error)
    {
        return 1; /* failure: always indicate EOF */
    }
}

/**
 * @see http://librdf.org/raptor/api/raptor2-section-iostream.html#raptor-iostream-handler
 */
static const raptor_iostream_handler std_iostream_handler = {
/* .version     = */2,
/* .init        = */nullptr,
/* .finish      = */nullptr,
/* .write_byte  = */std_iostream_write_byte,
/* .write_bytes = */std_iostream_write_bytes,
/* .write_end   = */nullptr,
/* .read_bytes  = */std_iostream_read_bytes,
/* .read_eof    = */std_iostream_read_eof, };

raptor_iostream*
raptor_new_iostream_from_std_istream(raptor_world* world, std::istream* stream)
{
    return raptor_new_iostream_from_handler(world, stream, &std_iostream_handler);
}

raptor_iostream*
raptor_new_iostream_to_std_ostream(raptor_world* world, std::ostream* stream)
{
    return raptor_new_iostream_from_handler(world, stream, &std_iostream_handler);
}

} // namespace Redland
