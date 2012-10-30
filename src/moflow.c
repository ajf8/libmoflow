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
#include <pthread.h>

#include <curl/curl.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <json-glib/json-glib.h>

#include "moflow.h"
#include "http.h"
#include "json.h"

moflow_client *init_moflow()
{
  moflow_client *client = malloc(sizeof(moflow_client));
  curl_global_init(CURL_GLOBAL_ALL);
  moflow_json_init(client);
  return client;
}

motorway_event *motorway_event_copy(motorway_event *a)
{
  motorway_event *newEvent = malloc(sizeof(motorway_event));
  newEvent->delay = g_strdup(a->delay);
  newEvent->description = g_strdup(a->description);
  return newEvent;
}

int meventcmp(motorway_event *a, motorway_event *b)
{
  int r;
  if ((r = g_strcmp0(a->description, b->description)) != 0)
    return r;
  
  return g_strcmp0(a->delay, b->delay);
}

gboolean contains_event(GList *list, motorway_event *event)
{
  for (GList *li = list; li != NULL; li = g_list_next(li))
    if (meventcmp(event, (motorway_event*)li->data) == 0)
      return TRUE;

  return FALSE;
}

int events_update(GList **old, GList *new)
{
  int updates = 0;
  for (GList *li = new; li != NULL; li = g_list_next(li))
  {
    if (contains_event(*old, (motorway_event*)li->data) == FALSE)
    {
      *old = g_list_append(*old, motorway_event_copy((motorway_event*)li->data));
      updates++;
    }
  }
  for (GList *li = *old; li != NULL; li = g_list_next(li))
  {
    if (contains_event(new, (motorway_event*)li->data) == FALSE)
    {
      motorway_event_free((motorway_event*)li->data);
      *old = g_list_remove(*old, li->data);
      updates++;
    }
  }
  return updates;
}

int string_list_update(GList **old, GList *new)
{
  int updates = 0;
  int i = 0;
  int oldCount = g_list_length(*old);
  for (GList *li = new; li != NULL; li = g_list_next(li))
  {
    if (i >= oldCount)
    {
      *old = g_list_append(*old, g_strdup((gchar*)li->data));
      updates++;
    }
    else
    {
      GList *oldNode = g_list_nth(*old, i);
      if (g_strcmp0((gchar*)oldNode->data, (gchar*)li->data) != 0)
      {
        g_free(oldNode->data);
        oldNode->data = g_strdup((gchar*)li->data);
        updates++;
      }
    }
    i++;
  }

  while (g_list_length(*old) > g_list_length(new))
  {
    GList *excessNode = g_list_last(*old);
    g_free(excessNode->data);
    *old = g_list_delete_link(*old, excessNode);
    updates++;
  }
  
  return updates;
}

int junctions_update(GList **old, GList **new)
{
  if (*new == NULL && *old == NULL)
  {
    return 0;
  }
  else if (*old == NULL && *new != NULL)
  {
    *old = *new;
    *new = NULL;
    return 0;
  }
  else if (*new == NULL && *old != NULL)
  {
    junction_list_free(*old);
    *old = NULL;
    return 0;
  }
  
  int updates = 0;
  int i = 0;
  for (GList *li = *old; li != NULL; li = g_list_next(li))
  {
    junction *jo = (junction*)li->data;
    junction *jn = (junction*)g_list_nth_data(*new, i);
    
    if (jo->avgSpeed != jn->avgSpeed)
    {
      jo->avgSpeed = jn->avgSpeed;
      updates++;
    }
    if (jo->status != jn->status)
    {
      jo->status = jn->status;
      updates++;
    }
    
    updates += events_update(&jo->incidents, jn->incidents);
    updates += events_update(&jo->roadworks, jn->roadworks);
    updates += string_list_update(&jo->matrixMsgs, jn->matrixMsgs);
    
    i++;
  }
  
  return updates;
}

pthread_t moflow_update_async(moflow_client *client, motorway *old, void(*callback)(int*, gboolean, gpointer*), gpointer *data)
{
  pthread_t tid;
  struct motorway_update_async *async_req = malloc(sizeof(struct motorway_update_async));
  async_req->client = client;
  async_req->data = data;
  async_req->callback = callback;
  async_req->old = old;
  pthread_create(&tid, NULL, moflow_update_async_thread, (void*)async_req);
  return tid;
}

gboolean moflow_update(moflow_client *client, int *res, motorway *old)
{
  motorway *new  = moflow_getactive(client, res);
  if (new == NULL)
    return FALSE;

  int update = FALSE;
  if (junctions_update(&old->rightJunctions, &new->rightJunctions) > 0)
    update = TRUE;
  
  if (junctions_update(&old->leftJunctions, &new->leftJunctions) > 0)
    update = TRUE;
  
  motorway_free(new);
  return update;
}

gchar *moflow_easy_strerror(int res)
{
  if (res >= 0)
    return g_strdup_printf("%s (%d)", curl_easy_strerror(res), res);
  else if (res <= -100)
    return g_strdup_printf("HTTP code %d", -(res+100));
  else if (res == -1)
    return g_strdup("JSON parsing error");
  else
    return g_strdup_printf("unknown error (%d)", res);
}

void moflow_update_async_thread(void *arg)
{
  struct motorway_update_async *async_req = (struct motorway_update_async*)arg;
  int res = 0;
  (async_req->callback)(&res, moflow_update(async_req->client, &res, async_req->old), async_req->data);
  g_free(async_req);
}

void moflow_get_async_thread(void *arg)
{
  struct motorway_request_async *async_req = (struct motorway_request_async*)arg;
  moflow_request *req = async_req->request;

  int res = 0;
  motorway *result = moflow_get(async_req->client, &res, req);
  (async_req->callback)(result, &res, async_req->data);
  g_free(async_req);
}

pthread_t moflow_getactive_async(moflow_client *client, void(*callback)(motorway*, int*, gpointer*), gpointer *data)
{
  return moflow_get_async(client, client->activereq, callback, data);
}

pthread_t moflow_get_async(moflow_client *client, moflow_request *request, void(*callback)(motorway*, int*, gpointer*), gpointer *data)
{
  pthread_t tid;
  struct motorway_request_async *async_req = malloc(sizeof(struct motorway_request_async));
  async_req->client = client;
  async_req->data = data;
  async_req->callback = callback;
  async_req->request = request;
  pthread_create(&tid, NULL, moflow_get_async_thread, (void*)async_req);
  return tid;
}

motorway *moflow_getactive(moflow_client *client, int *res)
{
  return moflow_get(client, res, client->activereq);
}

motorway *moflow_get(moflow_client *client, int *res, moflow_request *request)
{
  moflow_curl_client *http = http_request_motorway(client, request);
  motorway *mway = NULL;
  
  if (http->res == CURLE_OK)
    mway = decode_response(client->parser, http);
  
  *res = http->res;

  moflow_curl_cleanup(http);
  return mway;
}

void moflow_serialize_request(moflow_request *request)
{
  moflow_json_serialize_request(request);
}

moflow_request *moflow_init_request()
{
  moflow_request *request = malloc(sizeof(moflow_request));
  request->direction = MF_DIRECTION_BOTH;
  request->min_level = 0;
  request->serialized = NULL;
  request->url = NULL;
  return request;
}

gchar *moflow_req_generate_url(moflow_request *request)
{
  char *sample;
  if ((sample = getenv("MOFLOW_USE_SAMPLE")) != NULL)
  {
    return request->url = g_strdup(sample);
  }
  else
  {
    return request->url = g_strdup_printf(MOFLOW_GETURL_SPEC, request->motorway_id, request->direction, request->min_level);
  }
}

GHashTable *motorway_list(moflow_client *client, int *res)
{
  moflow_curl_client *http = http_request_generic(client, getenv("MOFLOW_NO_STATIC_LIST") == NULL ? MOFLOW_STATIC_LIST_URL : MOFLOW_LIST_URL);
  GHashTable *table = NULL;
  
  if (http->res == CURLE_OK)
    table = decode_motorways_list(client->parser, http);
  
  moflow_curl_cleanup(http);
  *res = http->res;

  return table;
}

void motorway_list_async_thread(void *arg)
{
  struct motorway_list_request_async *async_req = (struct motorway_list_request_async*)arg;
  int res;
  GHashTable *table = motorway_list(async_req->client, &res);
  (async_req->callback)(table, &res, async_req->data);
  g_free(async_req);
}

pthread_t motorway_list_async(moflow_client *client, void(*callback)(GHashTable*, int*, gpointer*), gpointer *data)
{
  pthread_t tid;
  struct motorway_list_request_async *async_req = malloc(sizeof(struct motorway_list_request_async));
  async_req->client = client;
  async_req->callback = callback;
  async_req->data = data;
  pthread_create(&tid, NULL, motorway_list_async_thread, (void*)async_req);
  return tid;
}

void g_list_free_and_contents(GList *list)
{
  if (list == NULL)
    return;
  
  for (GList *li = list; li != NULL; li = g_list_next(li))
    g_free(li->data);

  g_list_free(list);
}

void junction_list_free(GList *list)
{
  for (GList *li = list; li != NULL; li = g_list_next(li))
    junction_free((junction*)li->data);

  g_list_free(list);
}

void motorway_list_free(GHashTable *table)
{
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, table);

  while (g_hash_table_iter_next (&iter, &key, &value))
  {
    g_free(key);
    g_free(value);
  }

  g_hash_table_destroy(table);
}

void junction_free(junction *junction)
{
  if (junction == NULL)
    return;

  if (junction->matrixRows != NULL)
  {
    for (GList *li = junction->matrixRows; li != NULL; li = g_list_next(li))
      g_list_free_and_contents(li->data);
    
    g_list_free(junction->matrixRows);
  }

  g_list_free_and_contents(junction->matrixMsgs);
  g_list_free_and_contents(junction->cameras);
  motorway_event_list_free(junction->roadworks);
  motorway_event_list_free(junction->incidents);

  if (junction->weather != NULL)
  {
    for (GList *li = junction->weather; li != NULL; li = g_list_next(li))
      motorway_weather_free((motorway_weather*)li->data);

    g_list_free(junction->weather);
  }

  g_free(junction);
}

void motorway_event_list_free(GList *list)
{
  if (list != NULL)
  {
    for (GList *li = list; li != NULL; li = g_list_next(li))
      motorway_event_free((motorway_event*)li->data);

    g_list_free(list);
  }
}

void motorway_weather_free(motorway_weather* weather)
{
  g_free(weather->title);
  g_free(weather->location);
  g_free(weather);
}

void motorway_event_free(motorway_event *event)
{
  g_free(event->description);
  g_free(event->delay);
  g_free(event);
}

void motorway_free(motorway *motorway)
{
  g_free(motorway->roadName);
  g_list_free_and_contents(motorway->junctionNames);
  g_list_free_and_contents(motorway->junctionIds);
  g_free(motorway->rightDirection);
  g_free(motorway->leftDirection);
  junction_list_free(motorway->rightJunctions);
  junction_list_free(motorway->leftJunctions);
  g_free(motorway);
}

void moflow_request_free(moflow_request *request)
{
  if (request->url != NULL)
    g_free(request->url);

  if (request->serialized != NULL)
    g_free(request->serialized);

  g_free(request);
}

void moflow_free(moflow_client *client)
{
  curl_global_cleanup();
  g_object_unref(client->parser);
  g_free(client);
}
