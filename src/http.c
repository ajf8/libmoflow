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
#include <string.h>
#include <curl/curl.h>

#include "moflow.h"
#include "http.h"

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t realsize = size * nmemb;
  moflow_curl_client *mem = (moflow_curl_client *) data;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory) {
    memcpy(& (mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
  }

  return realsize;
}

moflow_curl_client *moflow_curl_init(moflow_client *client)
{
  CURL *curl = curl_easy_init();

  if (curl == NULL)
    return NULL;

  moflow_curl_client *rbuf = malloc(sizeof(moflow_curl_client));
  rbuf->memory = NULL;
  rbuf->size = 0;
  rbuf->curl = curl;
  rbuf->res = 0;
  rbuf->httpCode = 0;

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)rbuf);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, MOFLOW_USER_AGENT);
  curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

  return rbuf;
}

void moflow_curl_cleanup(moflow_curl_client *http)
{
  if (http->memory != NULL)
    g_free(http->memory);

  if (http->curl != NULL)
    curl_easy_cleanup(http->curl);

  g_free(http);
}

void moflow_curl_easy_perform(moflow_curl_client *http)
{
  CURL *curl = http->curl;
  http->res = curl_easy_perform(curl);
  curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http->httpCode);
  if (http->res == CURLE_OK && http->httpCode != 200)
  {
    http->res = (-(http->httpCode))-100;
  }
}

moflow_curl_client *http_request_generic(moflow_client *client, gchar *url)
{
  moflow_curl_client *http = moflow_curl_init(client);

  CURL *curl = http->curl;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

  moflow_curl_easy_perform(http);

  return http;
}

moflow_curl_client *http_request_motorway(moflow_client *client, moflow_request *request)
{
  moflow_curl_client *http = moflow_curl_init(client);
  CURL *curl = http->curl;

  if (request->url != NULL)
  {
    curl_easy_setopt(curl, CURLOPT_URL, request->url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
  }
  else
  {
    curl_easy_setopt(curl, CURLOPT_URL, MOFLOW_POST_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->serialized);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
  }

  moflow_curl_easy_perform(http);

  return http;
}
