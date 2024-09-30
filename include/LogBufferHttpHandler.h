#pragma once

#include "Core.h"
#include "LogBuffer.h"
#include "esp_err.h"
#include <esp_http_server.h>

static esp_err_t post_logBuffer_handler(httpd_req_t *req)
{
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_type(req, "application/json");

  uint16_t lastId = 0;
  if (req->content_len > 0)
  {
    char *body = (char *)malloc(sizeof(char) * req->content_len + 1);

    if (httpd_req_recv(req, body, req->content_len + 1) <= 0)
    {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive");
      httpd_resp_sendstr_chunk(req, NULL);
      return ESP_FAIL;
    }

    lastId = atoi(body);
    free(body);
  }

  LogLine *line = nullptr;
  if (lastId == 0)
    line = log_readFirst();
  else
    line = log_readNext(lastId);

  while (line != nullptr)
  {
    uint16_t id = line->GetId();

    LogLine *nextLine = log_readNext(id);
    if (nextLine == nullptr)
    {
      const char *format = "%d";
      int len = snprintf(nullptr, 0, format, id);
      char *idString = (char *)malloc(sizeof(char) * len + 1);
      snprintf(idString, len + 1, format, id);
      httpd_resp_sendstr_chunk(req, idString);
      free(idString);
      httpd_resp_sendstr_chunk(req, "|");
    }

    char *remStr = RemovingString(line->GetData());
    httpd_resp_sendstr_chunk(req, remStr);
    free(remStr);

    delete line;
    line = nextLine;

    if (line != nullptr)
      httpd_resp_sendstr_chunk(req, "\r");
  }

  httpd_resp_sendstr_chunk(req, NULL);

  return ESP_OK;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

const static char *apilog = "/api/log*";

static const httpd_uri_t http_server_post_logBuffer_request = {
    .uri = apilog,
    .method = HTTP_POST,
    .handler = post_logBuffer_handler};

#pragma GCC diagnostic pop

static esp_err_t register_logBuffer_handler(httpd_handle_t handle)
{
  return httpd_register_uri_handler(handle, &http_server_post_logBuffer_request);
}