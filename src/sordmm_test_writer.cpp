/*
 * pose_sord.cpp
 *
 *  Created on: Feb 20, 2015
 *      Author: Dmitri Rubinstein
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <iostream>

#include "sord/sordmm.hpp"
#include "serd/serd.h"
#include "Profiler.h"

#define RDF(x) "http://www.w3.org/1999/02/22-rdf-syntax-ns#" x
#define SPATIAL(x) "http://vocab.arvida.de/2014/03/spatial/vocab#" x
#define TRACKING(x) "http://vocab.arvida.de/2014/03/tracking/vocab#" x
#define MATHS(x) "http://vocab.arvida.de/2014/03/maths/vocab#" x
#define VOM(x) "http://vocab.arvida.de/2014/03/vom/vocab#" x
#define MEA(x) "http://vocab.arvida.de/2014/03/mea/vocab#" x
#define XSD(x) "http://www.w3.org/2001/XMLSchema#" x

inline Sord::Node double_node(Sord::World &world, double d)
{
    const SerdNode val = serd_node_new_decimal(d, 7);
    const SerdNode type = serd_node_from_string(
        SERD_URI, (const uint8_t *)SORD_NS_XSD "double");

    return Sord::Node(
        world,
        sord_node_from_serd_node(
            world.c_obj(), world.prefixes().c_obj(), &val, &type, NULL),
        false);
}

int main(int argc, char *argv[])
{
    using namespace Sord;

    int num = 1;

    if (argc > 1)
    {
        num = atoi(argv[1]);
    }

    World world;

    world.add_prefix("rdf", RDF(""));
    world.add_prefix("maths", MATHS(""));
    world.add_prefix("spatial", SPATIAL(""));
    world.add_prefix("tracking", TRACKING(""));
    world.add_prefix("vom", VOM(""));
    world.add_prefix("mea", MEA(""));
    world.add_prefix("xsd", XSD(""));

    Model model(world, "http://test.arvida.de/");

    std::cout << "Producing " << num << " poses" << std::endl;

    MIDDLEWARENEWSBRIEF_PROFILER_TIME_TYPE start, finish, elapsed;

    start = MIDDLEWARENEWSBRIEF_PROFILER_GET_TIME;

    for (int i = 0; i < num; ++i)
    {
        const std::string uuid_url = "http://test.arvida.de/UUID" + std::to_string(i);

        model.add_statement(
            URI(world, uuid_url),
            URI(world, RDF("type")),
            URI(world, SPATIAL("SpatialRelationship")));

        {
            Node n1 = Node::blank_id(world);

            model.add_statement(
                URI(world, uuid_url),
                URI(world, SPATIAL("sourceCoordinateSystem")),
                n1);

            model.add_statement(
                n1,
                URI(world, RDF("type")),
                URI(world, MATHS("LeftHandedCartesianCoordinateSystem3D")));
        }

        {
            Node n1 = Node::blank_id(world);

            model.add_statement(
                URI(world, uuid_url),
                URI(world, SPATIAL("targetCoordinateSystem")),
                n1);

            model.add_statement(
                n1,
                URI(world, RDF("type")),
                URI(world, MATHS("RightHandedCartesianCoordinateSystem2D")));
        }

        URI xsd_double(world, XSD("double"));

        {

            // translation
            Node n1 = Node::blank_id(world);
            Node n2 = Node::blank_id(world);

            model.add_statement(
                URI(world, uuid_url),
                URI(world, SPATIAL("translation")),
                n1);

            model.add_statement(
                n1,
                URI(world, RDF("type")),
                URI(world, SPATIAL("Translation3D")));

            model.add_statement(
                n1,
                URI(world, VOM("quantityValue")),
                n2);

            model.add_statement(
                n2,
                URI(world, RDF("type")),
                URI(world, MATHS("Vector3D")));

            model.add_statement(
                n2,
                URI(world, MATHS("x")),
                double_node(world, 1));

            model.add_statement(
                n2,
                URI(world, MATHS("y")),
                double_node(world, 2));

            model.add_statement(
                n2,
                URI(world, MATHS("z")),
                double_node(world, 3));
        }

        {
            // rotation

            Node n1 = Node::blank_id(world);
            Node n2 = Node::blank_id(world);

            model.add_statement(
                URI(world, uuid_url),
                URI(world, SPATIAL("rotation")),
                n1);

            model.add_statement(
                n1,
                URI(world, RDF("type")),
                URI(world, SPATIAL("Rotation3D")));

            model.add_statement(
                n1,
                URI(world, VOM("quantityValue")),
                n2);

            model.add_statement(
                n2,
                URI(world, RDF("type")),
                URI(world, MATHS("Quaternion")));

            model.add_statement(
                n2,
                URI(world, RDF("type")),
                URI(world, MATHS("Vector4D")));

            model.add_statement(
                n2,
                URI(world, MATHS("x")),
                double_node(world, 1));

            model.add_statement(n2,
                                URI(world, MATHS("y")),
                                double_node(world, 1));

            model.add_statement(
                n2,
                URI(world, MATHS("z")),
                double_node(world, 1));

            model.add_statement(
                n2,
                URI(world, MATHS("w")),
                double_node(world, 1));
        }
    }

    finish = MIDDLEWARENEWSBRIEF_PROFILER_GET_TIME;

    elapsed = MIDDLEWARENEWSBRIEF_PROFILER_DIFF(finish, start);
    printf("\nElapsed %s for model construction: %i (%f per pose)\n\n",
           MIDDLEWARENEWSBRIEF_PROFILER_TIME_UNITS,
           elapsed, double(elapsed) / num);

    std::cout << "Writing poses to file pose_sordmm.ttl" << std::endl;

    start = MIDDLEWARENEWSBRIEF_PROFILER_GET_TIME;

    model.write_to_file("pose_sordmm.ttl", SERD_TURTLE,
                        (SerdStyle)(SERD_STYLE_ABBREVIATED | SERD_STYLE_CURIED | SERD_STYLE_RESOLVED));

    finish = MIDDLEWARENEWSBRIEF_PROFILER_GET_TIME;

    elapsed = MIDDLEWARENEWSBRIEF_PROFILER_DIFF(finish, start);
    printf("\nElapsed %s for model output: %i (%f per pose)\n",
           MIDDLEWARENEWSBRIEF_PROFILER_TIME_UNITS,
           elapsed, double(elapsed) / num);

    /* keep gcc -Wall happy */
    return (0);
}
