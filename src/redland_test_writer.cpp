/*
 * pose_redland_writer.cpp
 *
 *  Created on: Apr 15, 2015
 *      Author: Dmitri Rubinstein
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <iostream>

#define REDLAND_LIB
#include "redland.hpp"
#include "Profiler.h"

#define RDF(x) "http://www.w3.org/1999/02/22-rdf-syntax-ns#" x
#define SPATIAL(x) "http://vocab.arvida.de/2014/03/spatial/vocab#" x
#define TRACKING(x) "http://vocab.arvida.de/2014/03/tracking/vocab#" x
#define MATHS(x) "http://vocab.arvida.de/2014/03/maths/vocab#" x
#define VOM(x) "http://vocab.arvida.de/2014/03/vom/vocab#" x
#define MEA(x) "http://vocab.arvida.de/2014/03/mea/vocab#" x
#define XSD(x) "http://www.w3.org/2001/XMLSchema#" x

#define NEW_URI_NODE(world, uri_str) librdf_new_node_from_uri_string(world, (const unsigned char *)uri_str)
#define NEW_LITERAL_NODE(world, literal_str) librdf_new_node_from_literal(world, (const unsigned char *)literal_str, NULL, 0)
#define NEW_BLANK_NODE(world) librdf_new_node_from_blank_identifier(world, NULL)

inline Redland::Node double_node(const Redland::World &world, double value)
{
    Redland::Uri xsd_double(world, "http://www.w3.org/2001/XMLSchema#double");
    return Redland::Node::make_typed_literal_node(world, std::to_string(value), xsd_double);
}

int main(int argc, char *argv[])
{
    using namespace Redland;

    int num = 1;

    if (argc > 1)
    {
        num = atoi(argv[1]);
    }

    World world;
    Namespaces namespaces;

    namespaces.add_prefix("rdf", RDF(""));
    namespaces.add_prefix("maths", MATHS(""));
    namespaces.add_prefix("spatial", SPATIAL(""));
    namespaces.add_prefix("tracking", TRACKING(""));
    namespaces.add_prefix("vom", VOM(""));
    namespaces.add_prefix("mea", MEA(""));
    namespaces.add_prefix("xsd", XSD(""));

    // storage=librdf_new_storage(world, "hashes", NULL,  "hash-type='memory'")
    // storage=librdf_new_storage(world, "hashes", "test", "hash-type='bdb',dir='.'")
    Storage storage(world, "hashes", 0, "hash-type='memory'");
    Model model(world, storage, 0);

    std::cout << "Producing " << num << " poses" << std::endl;

    MIDDLEWARENEWSBRIEF_PROFILER_TIME_TYPE start, finish, elapsed;

    start = MIDDLEWARENEWSBRIEF_PROFILER_GET_TIME;

    for (int i = 0; i < num; ++i)
    {
        const std::string uuid_url = "http://test.arvida.de/UUID" + std::to_string(i);

        model.add_statement(
            world,
            Redland::Node::make_uri_node(world, uuid_url),
            Redland::Node::make_uri_node(world, RDF("type")),
            Redland::Node::make_uri_node(world, SPATIAL("SpatialRelationship")));

        {
            Node n1 = Redland::Node::make_blank_node(world);

            model.add_statement(
                world,
                Redland::Node::make_uri_node(world, uuid_url),
                Redland::Node::make_uri_node(world, SPATIAL("sourceCoordinateSystem")),
                n1);

            model.add_statement(
                world,
                n1,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, MATHS("LeftHandedCartesianCoordinateSystem3D")));
        }

        {
            Node n1 = Redland::Node::make_blank_node(world);

            model.add_statement(
                world,
                Redland::Node::make_uri_node(world, uuid_url),
                Redland::Node::make_uri_node(world, SPATIAL("targetCoordinateSystem")),
                n1);

            model.add_statement(
                world,
                n1,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, MATHS("RightHandedCartesianCoordinateSystem2D")));
        }

        Redland::Uri xsd_double(world, XSD("double"));

        {

            // translation
            Node n1 = Redland::Node::make_blank_node(world);
            Node n2 = Redland::Node::make_blank_node(world);

            model.add_statement(
                world,
                Redland::Node::make_uri_node(world, uuid_url),
                Redland::Node::make_uri_node(world, SPATIAL("translation")),
                n1);

            model.add_statement(
                world,
                n1,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, SPATIAL("Translation3D")));

            model.add_statement(
                world,
                n1,
                Redland::Node::make_uri_node(world, VOM("quantityValue")),
                n2);

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, MATHS("Vector3D")));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("x")),
                double_node(world, 1));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("y")),
                double_node(world, 2));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("z")),
                double_node(world, 3));
        }

        {
            // rotation

            Node n1 = Redland::Node::make_blank_node(world);
            Node n2 = Redland::Node::make_blank_node(world);

            model.add_statement(
                world,
                Redland::Node::make_uri_node(world, uuid_url),
                Redland::Node::make_uri_node(world, SPATIAL("rotation")),
                n1);

            model.add_statement(
                world,
                n1,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, SPATIAL("Rotation3D")));

            model.add_statement(
                world,
                n1,
                Redland::Node::make_uri_node(world, VOM("quantityValue")),
                n2);

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, MATHS("Quaternion")));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, RDF("type")),
                Redland::Node::make_uri_node(world, MATHS("Vector4D")));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("x")),
                double_node(world, 1));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("y")),
                double_node(world, 1));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("z")),
                double_node(world, 1));

            model.add_statement(
                world,
                n2,
                Redland::Node::make_uri_node(world, MATHS("w")),
                double_node(world, 1));
        }
    }

    finish = MIDDLEWARENEWSBRIEF_PROFILER_GET_TIME;

    elapsed = MIDDLEWARENEWSBRIEF_PROFILER_DIFF(finish, start);
    printf("\nElapsed %s for model construction: %i (%f per pose)\n\n",
           MIDDLEWARENEWSBRIEF_PROFILER_TIME_UNITS,
           elapsed, double(elapsed) / num);

    std::cout << "Writing poses to pose_redland.ttl" << std::endl;

    /* serialize */
    {
        librdf_serializer *ser = librdf_new_serializer(world.c_obj(), "turtle", NULL, NULL);

        namespaces.register_with_serializer(world, ser);

        if (!ser)
        {
            fprintf(stderr, "Could not load turtle serializer");
        }
        FILE *fd = fopen("pose_redland.ttl", "wb");

        librdf_serializer_serialize_model_to_file_handle(ser, fd, NULL, model.c_obj());

        fclose(fd);

        librdf_free_serializer(ser);
    }

    /* free everything */

#ifdef LIBRDF_MEMORY_DEBUG
    librdf_memory_report(stderr);
#endif

    /* keep gcc -Wall happy */
    return (0);
}
