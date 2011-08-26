/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "geocache.h"
#include <libxml/tree.h>
#include <apr_tables.h>
#include <apr_strings.h>

/**
 * \private \memberof geocache_source_wms
 * \sa geocache_source::render_metatile()
 */
void _geocache_source_wms_render_metatile(geocache_context *ctx, geocache_metatile *tile) {
   geocache_source_wms *wms = (geocache_source_wms*)tile->tile.tileset->source;
   apr_table_t *params = apr_table_clone(ctx->pool,wms->wms_default_params);
   apr_table_setn(params,"BBOX",apr_psprintf(ctx->pool,"%f,%f,%f,%f",
         tile->bbox[0],tile->bbox[1],tile->bbox[2],tile->bbox[3]));
   apr_table_setn(params,"WIDTH",apr_psprintf(ctx->pool,"%d",tile->tile.sx));
   apr_table_setn(params,"HEIGHT",apr_psprintf(ctx->pool,"%d",tile->tile.sy));
   apr_table_setn(params,"FORMAT","image/png");
   apr_table_setn(params,"SRS",tile->tile.tileset->srs);
   
   apr_table_overlap(params,wms->wms_params,0);
        
   tile->tile.data = geocache_buffer_create(30000,ctx->pool);
   geocache_http_request_url_with_params(ctx,wms->url,params,tile->tile.data);
   GC_CHECK_ERROR(ctx);
   
   if(!geocache_imageio_is_valid_format(ctx,tile->tile.data)) {
      char *returned_data = apr_pstrndup(ctx->pool,(char*)tile->tile.data->buf,tile->tile.data->size);
      ctx->set_error(ctx, GEOCACHE_SOURCE_WMS_ERROR, "wms request for tileset %s: %d %d %d returned an unsupported format:\n%s",
            tile->tile.tileset->name, tile->tile.x, tile->tile.y, tile->tile.z, returned_data);
   }
}

/**
 * \private \memberof geocache_source_wms
 * \sa geocache_source::configuration_parse()
 */
void _geocache_source_wms_configuration_parse(geocache_context *ctx, xmlNode *xml, geocache_source *source) {
   xmlNode *cur_node;
   geocache_source_wms *src = (geocache_source_wms*)source;
   for(cur_node = xml->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "url")) {
         char* value = (char*)xmlNodeGetContent(cur_node);
         src->url = value;
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "wmsparams")) {
         xmlNode *param_node;
         for(param_node = cur_node->children; param_node; param_node = param_node->next) {
            char *key,*value;
            if(param_node->type != XML_ELEMENT_NODE) continue;
            value = (char*)xmlNodeGetContent(param_node);
            key = apr_pstrdup(ctx->pool, (char*)param_node->name);
            apr_table_setn(src->wms_params, key, value);
         }
      }
   }
}

/**
 * \private \memberof geocache_source_wms
 * \sa geocache_source::configuration_check()
 */
void _geocache_source_wms_configuration_check(geocache_context *ctx, geocache_source *source) {
   geocache_source_wms *src = (geocache_source_wms*)source;
   /* check all required parameters are configured */
   if(!strlen(src->url)) {
      ctx->set_error(ctx, GEOCACHE_SOURCE_WMS_ERROR, "wms source %s has no url",source->name);
   }
   if(!apr_table_get(src->wms_params,"LAYERS")) {
      ctx->set_error(ctx, GEOCACHE_SOURCE_WMS_ERROR, "wms source %s has no LAYERS", source->name);
   }
}

geocache_source* geocache_source_wms_create(geocache_context *ctx) {
   geocache_source_wms *source = apr_pcalloc(ctx->pool, sizeof(geocache_source_wms));
   if(!source) {
      ctx->set_error(ctx, GEOCACHE_ALLOC_ERROR, "failed to allocate wms source");
      return NULL;
   }
   geocache_source_init(ctx, &(source->source));
   source->source.type = GEOCACHE_SOURCE_WMS;
   source->source.supports_metatiling = 1;
   source->source.render_metatile = _geocache_source_wms_render_metatile;
   source->source.configuration_check = _geocache_source_wms_configuration_check;
   source->source.configuration_parse = _geocache_source_wms_configuration_parse;
   source->wms_default_params = apr_table_make(ctx->pool,4);;
   source->wms_params = apr_table_make(ctx->pool,4);
   apr_table_add(source->wms_default_params,"VERSION","1.1.1");
   apr_table_add(source->wms_default_params,"REQUEST","GetMap");
   apr_table_add(source->wms_default_params,"SERVICE","WMS");
   apr_table_add(source->wms_default_params,"STYLES","");
   return (geocache_source*)source;
}


