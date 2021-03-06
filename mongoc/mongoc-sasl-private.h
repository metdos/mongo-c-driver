/*
 * Copyright 2013 10gen Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef MONGOC_SASL_PRIVATE_H
#define MONGOC_SASL_PRIVATE_H


#include <bson.h>
#include <sasl/sasl.h>
#include <sasl/saslutil.h>


BSON_BEGIN_DECLS


typedef struct _mongoc_sasl_t mongoc_sasl_t;


struct _mongoc_sasl_t
{
   sasl_callback_t  callbacks [4];
   sasl_conn_t     *conn;
   bson_bool_t      done;
   int              step;
   char            *mechanism;
   char            *user;
   char            *pass;
   char            *service_name;
   char            *service_host;
   sasl_interact_t *interact;
};


void
_mongoc_sasl_init (mongoc_sasl_t *sasl)
   BSON_GNUC_INTERNAL;

void
_mongoc_sasl_set_pass (mongoc_sasl_t *sasl,
                       const char    *pass)
   BSON_GNUC_INTERNAL;

void
_mongoc_sasl_set_user (mongoc_sasl_t *sasl,
                       const char    *user)
   BSON_GNUC_INTERNAL;

void
_mongoc_sasl_set_mechanism (mongoc_sasl_t *sasl,
                            const char    *mechanism)
   BSON_GNUC_INTERNAL;

void
_mongoc_sasl_set_service_name (mongoc_sasl_t *sasl,
                               const char    *service_name)
   BSON_GNUC_INTERNAL;

void
_mongoc_sasl_set_service_host (mongoc_sasl_t *sasl,
                               const char    *service_host)
   BSON_GNUC_INTERNAL;

void
_mongoc_sasl_destroy (mongoc_sasl_t *sasl)
   BSON_GNUC_INTERNAL;

bson_bool_t
_mongoc_sasl_step (mongoc_sasl_t      *sasl,
                   const bson_uint8_t *inbuf,
                   bson_uint32_t       inbuflen,
                   bson_uint8_t       *outbuf,
                   bson_uint32_t       outbufmax,
                   bson_uint32_t      *outbuflen,
                   bson_error_t       *error)
   BSON_GNUC_INTERNAL;


BSON_END_DECLS


#endif /* MONGOC_SASL_PRIVATE_H */
