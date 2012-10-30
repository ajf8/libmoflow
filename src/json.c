// libmoflow
// Copyright (C) 2010 Alan F
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <stdlib.h>

#include <glib-object.h>
#include <glib.h>
#include <json-glib/json-glib.h>

#include "moflow.h"
#include "http.h"
#include "json.h"

void moflow_parser_clear(JsonParser *parser)
{
  JsonNode *root = json_parser_get_root(parser);
  g_object_unref(root);
}

void moflow_json_init(moflow_client *moflow)
{
  moflow->parser = json_parser_new();
}

void moflow_json_serialize_request(moflow_request *request)
{
  JsonGenerator *gen = json_generator_new();
  JsonNode *rootNode = json_node_new(JSON_NODE_OBJECT);
  JsonObject *rootObj = json_object_new();

  json_object_set_int_member(rootObj, "motorway_id", request->motorway_id);
  json_object_set_int_member(rootObj, "direction", request->direction);
  json_object_set_int_member(rootObj, "min_level", request->min_level);

  json_node_set_object(rootNode, rootObj);
  json_generator_set_root(gen, rootNode);

  request->serialized = json_generator_to_data(gen, NULL);

  g_object_unref(gen);
}

GHashTable *decode_motorways_list(JsonParser *parser, moflow_curl_client *http)
{
  if (json_parser_load_from_data(parser, http->memory, http->size, NULL) != TRUE)
  {
    http->res = -1;
    return NULL;
  }

  JsonObject *json = json_node_get_object(json_parser_get_root(parser));
  GList *keys = json_object_get_members(json);

  GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);

  for (GList *li = keys; li != NULL; li = g_list_next(li))
  {
    JsonNode *node = json_object_get_member(json, li->data);
    int *value = malloc(sizeof(int));
    *value = json_node_get_int(node);
    g_hash_table_insert(table, g_strdup(li->data), value );
  }

  g_list_free(keys);

  return table;
}

motorway *decode_response(JsonParser *parser, moflow_curl_client *http)
{
  if (json_parser_load_from_data(parser, http->memory, http->size, NULL) != TRUE)
  {
    http->res = -1;
    return NULL;
  }

  JsonObject *json = json_node_get_object(json_parser_get_root(parser));

  motorway *mway = malloc(sizeof(motorway));

  mway->roadName = g_strdup( json_object_get_string_member(json, "roadName") );
  mway->leftDirection = g_strdup( json_object_get_string_member(json, "leftTitle") );
  mway->rightDirection = g_strdup( json_object_get_string_member(json, "rightTitle") );
  mway->junctionNames = json_string_array_to_glist( json_object_get_array_member(json, "junctionNames") );
  mway->junctionIds = json_string_array_to_glist( json_object_get_array_member(json, "junctionIds") );
  mway->leftJunctions = decode_junctions( json_object_get_array_member(json, "leftJunctions"), mway->junctionNames, mway->junctionIds );
  mway->rightJunctions = decode_junctions( json_object_get_array_member(json, "rightJunctions"), mway->junctionNames, mway->junctionIds );
  
  return mway;
}

void moflow_clear_parser(JsonParser *parser)
{
  JsonNode *node = json_parser_get_root(parser);
  g_free(node);
}

GList *decode_junctions(JsonArray *input, GList *junctionNames, GList *junctionIds)
{
  GList *output = NULL;

  for (int i = 0; i < json_array_get_length(input); i++)
  {
    output = g_list_append(output, decode_junction( json_array_get_object_element(input, i), junctionNames, junctionIds, i ) );
  }

  return output;
}

junction *decode_junction(JsonObject *input, GList *junctionNames, GList *junctionIds, gint index)
{
  junction *output = malloc(sizeof(junction));
  
  output->status = json_object_get_int_member(input, "status");
  output->cameras = json_int_array_to_glist( json_object_get_array_member(input, "cameras") );
  output->matrixMsgs = json_string_array_to_glist ( json_object_get_array_member(input, "matrixMsgs") );
  output->matrixRows = decode_matrixRows( json_object_get_array_member(input, "matrixRows") );
  output->avgSpeed = json_object_get_int_member(input, "averageSpeed");
  output->roadworks = decode_events( json_object_get_array_member(input, "roadworks" ) );
  output->incidents = decode_events( json_object_get_array_member(input, "incidents" ) );
  output->weather = decode_weather( json_object_get_array_member(input, "weather" ) );
  output->junctionName = (gchar*)g_list_nth_data(junctionNames, index);
  output->junctionId = (gchar*)g_list_nth_data(junctionIds, index);
  output->index = index;

  return output;
}

GList *decode_events(JsonArray *input)
{
  GList *output = NULL;

  for (int i = 0; i < json_array_get_length(input); i++)
  {
    output = g_list_append(output, decode_event( json_array_get_object_element(input, i) ) );
  }

  return output;
}

motorway_event *decode_event(JsonObject *input)
{
  motorway_event *event = malloc( sizeof(motorway_event) );
  event->description = g_strdup( json_object_get_string_member(input, "description") );
  event->delay = g_strdup( json_object_get_string_member(input, "delay") );
  return event;
}

GList *decode_matrixRows(JsonArray *input)
{
  GList *output = NULL;

  for (int i = 0; i < json_array_get_length(input); i++)
  {
    JsonArray *innerArray = json_array_get_array_element(input, i);
    output = g_list_append(output, json_string_array_to_glist(innerArray) );
  }

  return output;
}

GList *json_string_array_to_glist(JsonArray *array)
{
  GList *output = NULL;

  for (int i = 0; i < json_array_get_length(array); i++)
  {
    output = g_list_append(output, g_strdup( json_array_get_string_element(array, i) ) );
  }

  return output;
}

GList *json_int_array_to_glist(JsonArray *array)
{
  GList *output = NULL;

  for (int i = 0; i < json_array_get_length(array); i++)
  {
    int *value = malloc(sizeof(int));
    *value = json_array_get_int_element(array, i);
    output = g_list_append(output, value);
  }

  return output;
}

GList *decode_weather(JsonArray *array)
{
  GList *output = NULL;

  for (int i = 0; i < json_array_get_length(array); i++)
  {
    JsonObject *input = json_array_get_object_element(array, i);
    motorway_weather *weatherItem = malloc( sizeof(motorway_weather) );
    weatherItem->title = g_strdup( json_object_get_string_member(input, "title") );
    weatherItem->location = g_strdup( json_object_get_string_member(input, "location") );
    output = g_list_append(output, weatherItem);
  }

  return output;
}
