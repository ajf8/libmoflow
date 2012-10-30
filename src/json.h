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

void moflow_json_init(moflow_client *moflow);
motorway *decode_response(JsonParser *parser, moflow_curl_client *http);
GList *decode_junctions(JsonArray *input, GList *junctionNames, GList *junctionIds);
junction *decode_junction(JsonObject *input, GList *junctionNames, GList *junctionIds, gint index);
GList *decode_events(JsonArray *input);
motorway_event *decode_event(JsonObject *input);
GList *decode_matrixRows(JsonArray *input);
GList *json_string_array_to_glist(JsonArray *array);
GList *json_int_array_to_glist(JsonArray *array);
GList *decode_weather(JsonArray *weather);
void moflow_json_serialize_request(moflow_request *request);
void moflow_parser_clear(JsonParser *parser);
GHashTable *decode_motorways_list(JsonParser *parser, moflow_curl_client *http);
