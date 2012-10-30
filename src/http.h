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

typedef struct
{
  char *memory;
  size_t size;
  CURL *curl;
  CURLcode res;
  long httpCode;
} moflow_curl_client;

void moflow_curl_easy_perform(moflow_curl_client *http);
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
moflow_curl_client *http_request_motorway(moflow_client *client, moflow_request *request);
moflow_curl_client *http_request_generic(moflow_client *client, gchar *url);
moflow_curl_client *moflow_curl_init(moflow_client *client);
void moflow_curl_cleanup(moflow_curl_client *http);
