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

#include <curl/curl.h>
#include <json-glib/json-glib.h>
#include <pthread.h>

#define MOFLOW_SAMPLE_URL "http://eth0.org.uk/m25.json"
#define MOFLOW_GETURL_SPEC "http://eth0.org.uk/cgi-bin/motorway.pl?m_id=%d&direction=%d&min_level=%d&compact=1"
#define MOFLOW_LIST_URL "http://eth0.org.uk/motorway?get_list=1"
#define MOFLOW_STATIC_LIST_URL "http://eth0.org.uk/motorwaylist.json"
#define MOFLOW_POST_URL "http://eth0.org.uk/cgi-bin/motorway.pl"
#define MOFLOW_USER_AGENT "libmoflow"
#define MF_DIRECTION_BOTH 0
#define MF_DIRECTION_LEFT 1
#define MF_DIRECTION_RIGHT 2

typedef struct
{
  int motorway_id;
  int min_level;
  int direction;
  gchar *serialized;
  gchar *url;
} moflow_request;

typedef struct
{
  JsonParser *parser;
  moflow_request* activereq;
} moflow_client;

typedef struct
{
  gchar *roadName;
  gchar *leftDirection;
  gchar *rightDirection;
  GList *junctionNames;
  GList *junctionIds;
  GList *leftJunctions;
  GList *rightJunctions;
} motorway;

typedef struct
{
  GList *matrixRows;
  GList *matrixMsgs;
  GList *incidents;
  GList *roadworks;
  GList *cameras;
  GList *weather;
  gchar *junctionName;
  gchar *junctionId;
  gint avgSpeed;
  gint status;
  gint index;
} junction;

typedef struct
{
  gchar *description;
  gchar *delay;
} motorway_event;

typedef struct
{
  gchar *title;
  gchar *location;
} motorway_weather;

moflow_client *init_moflow();
moflow_request *moflow_init_request();

pthread_t moflow_update_async(moflow_client *client, motorway *old, void(*callback)(int*, gboolean, gpointer*), gpointer *data);
int junctions_update(GList **old, GList **new);
gboolean moflow_update(moflow_client *client, int *res, motorway *old);
void moflow_update_async_thread(void *arg);

motorway *moflow_get(moflow_client *client, int *res, moflow_request *request);
pthread_t moflow_getactive_async(moflow_client *client, void(*callback)(motorway*, int*, gpointer*), gpointer *data);
motorway *moflow_getactive(moflow_client *client, int *res);
pthread_t moflow_get_async(moflow_client *client, moflow_request *request, void(*callback)(motorway*, int*, gpointer*), gpointer *data);
void moflow_get_async_thread(void *arg);

void moflow_serialize_request(moflow_request *request);
gchar *moflow_req_generate_url(moflow_request *request);

GHashTable *motorway_list(moflow_client *client, int *res);
pthread_t motorway_list_async(moflow_client *client, void(*callback)(GHashTable*, int*, gpointer*), gpointer *data);
void motorway_list_async_thread(void *arg);

void motorway_list_free(GHashTable *hash);
void moflow_free(moflow_client *moflow);
void junction_free(junction *junction);
void junction_list_free(GList *list);
void motorway_weather_free(motorway_weather* weather);
void motorway_event_free(motorway_event *event);
motorway_event *motorway_event_copy(motorway_event *src);
void motorway_free(motorway *motorway);
void moflow_request_free(moflow_request *request);
void g_list_free_and_contents(GList *list);
void motorway_event_list_free(GList *list);
gchar *moflow_easy_strerror(int res);

struct motorway_request_async
{
  moflow_client *client;
  moflow_request *request;
  void *data;
  void (*callback)(motorway*, int*, gpointer*);
};

struct motorway_list_request_async
{
  moflow_client *client;
  void *data;
  void (*callback)(GHashTable*, int*, gpointer*);
};

struct motorway_update_async
{
  gpointer *data;
  motorway *old;
  moflow_client *client;
  void(*callback)(int*, gboolean, gpointer*);
};
