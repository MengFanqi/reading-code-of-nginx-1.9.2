
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#if (NGX_HTTP_CACHE)
static ngx_int_t ngx_http_upstream_cache(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_cache_get(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_http_file_cache_t **cache);
static ngx_int_t ngx_http_upstream_cache_send(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_cache_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_cache_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_cache_etag(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
#endif

static void ngx_http_upstream_init_request(ngx_http_request_t *r);
static void ngx_http_upstream_resolve_handler(ngx_resolver_ctx_t *ctx);
static void ngx_http_upstream_rd_check_broken_connection(ngx_http_request_t *r);
static void ngx_http_upstream_wr_check_broken_connection(ngx_http_request_t *r);
static void ngx_http_upstream_check_broken_connection(ngx_http_request_t *r,
    ngx_event_t *ev);
static void ngx_http_upstream_connect(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_reinit(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_send_request(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_uint_t do_write);
static ngx_int_t ngx_http_upstream_send_request_body(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_uint_t do_write);
static void ngx_http_upstream_send_request_handler(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_read_request_handler(ngx_http_request_t *r);
static void ngx_http_upstream_process_header(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_test_next(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_intercept_errors(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static ngx_int_t ngx_http_upstream_test_connect(ngx_connection_t *c);
static ngx_int_t ngx_http_upstream_process_headers(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_process_body_in_memory(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_send_response(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_upgrade(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_upgraded_read_downstream(ngx_http_request_t *r);
static void ngx_http_upstream_upgraded_write_downstream(ngx_http_request_t *r);
static void ngx_http_upstream_upgraded_read_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_upgraded_write_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_process_upgraded(ngx_http_request_t *r,
    ngx_uint_t from_upstream, ngx_uint_t do_write);
static void
    ngx_http_upstream_process_non_buffered_downstream(ngx_http_request_t *r);
static void
    ngx_http_upstream_process_non_buffered_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void
    ngx_http_upstream_process_non_buffered_request(ngx_http_request_t *r,
    ngx_uint_t do_write);
static ngx_int_t ngx_http_upstream_non_buffered_filter_init(void *data);
static ngx_int_t ngx_http_upstream_non_buffered_filter(void *data,
    ssize_t bytes);
static void ngx_http_upstream_process_downstream(ngx_http_request_t *r);
static void ngx_http_upstream_process_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_process_request(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_store(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_dummy_handler(ngx_http_request_t *r,
    ngx_http_upstream_t *u);
static void ngx_http_upstream_next(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_uint_t ft_type);
static void ngx_http_upstream_cleanup(void *data);
static void ngx_http_upstream_finalize_request(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_int_t rc);

static ngx_int_t ngx_http_upstream_process_header_line(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_content_length(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_last_modified(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_set_cookie(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t
    ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_ignore_header_line(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_expires(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_accel_expires(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_limit_rate(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_buffering(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_charset(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_connection(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t
    ngx_http_upstream_process_transfer_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_process_vary(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_header_line(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t
    ngx_http_upstream_copy_multi_header_lines(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_content_type(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_last_modified(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_rewrite_location(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_rewrite_refresh(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_rewrite_set_cookie(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
static ngx_int_t ngx_http_upstream_copy_allow_ranges(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);

#if (NGX_HTTP_GZIP)
static ngx_int_t ngx_http_upstream_copy_content_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset);
#endif

static ngx_int_t ngx_http_upstream_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_upstream_addr_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_status_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_response_time_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_upstream_response_length_variable(
    ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data);

static char *ngx_http_upstream(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy);
static char *ngx_http_upstream_server(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);

static ngx_addr_t *ngx_http_upstream_get_local(ngx_http_request_t *r,
    ngx_http_upstream_local_t *local);

static void *ngx_http_upstream_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_upstream_init_main_conf(ngx_conf_t *cf, void *conf);

#if (NGX_HTTP_SSL)
static void ngx_http_upstream_ssl_init_connection(ngx_http_request_t *,
    ngx_http_upstream_t *u, ngx_connection_t *c);
static void ngx_http_upstream_ssl_handshake(ngx_connection_t *c);
static ngx_int_t ngx_http_upstream_ssl_name(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_connection_t *c);
#endif

//脥篓鹿媒ngx_http_upstream_init_main_conf掳脩脣霉脫脨ngx_http_upstream_headers_in鲁脡脭卤脳枚hash脭脣脣茫拢卢路脜脠毛ngx_http_upstream_main_conf_t碌脛headers_in_hash脰脨
//脮芒脨漏鲁脡脭卤脳卯脰脮禄谩赂鲁脰碌赂酶ngx_http_request_t->upstream->headers_in
ngx_http_upstream_header_t  ngx_http_upstream_headers_in[] = { //潞贸露脣脫娄麓冒碌脛脥路虏驴脨脨脝楼脜盲脮芒脌茂脙忙碌脛脳脰露脦潞贸拢卢脳卯脰脮脫脡ngx_http_upstream_headers_in_t脌茂脙忙碌脛鲁脡脭卤脰赂脧貌
//赂脙脢媒脳茅脡煤脨搂碌脴路陆录没ngx_http_proxy_process_header拢卢脥篓鹿媒handler(脠莽ngx_http_upstream_copy_header_line)掳脩潞贸露脣脥路虏驴脨脨碌脛脧脿鹿脴脨脜脧垄赂鲁脰碌赂酶ngx_http_request_t->upstream->headers_in脧脿鹿脴鲁脡脭卤
    { ngx_string("Status"),
                 ngx_http_upstream_process_header_line, //脥篓鹿媒赂脙handler潞炉脢媒掳脩麓脫潞贸露脣路镁脦帽脝梅陆芒脦枚碌陆碌脛脥路虏驴脨脨脳脰露脦赂鲁脰碌赂酶ngx_http_upstream_headers_in_t->status
                 offsetof(ngx_http_upstream_headers_in_t, status),
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("Content-Type"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, content_type),
                 ngx_http_upstream_copy_content_type, 0, 1 },

    { ngx_string("Content-Length"),
                 ngx_http_upstream_process_content_length, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

    { ngx_string("Date"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, date),
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, date), 0 },

    { ngx_string("Last-Modified"),
                 ngx_http_upstream_process_last_modified, 0,
                 ngx_http_upstream_copy_last_modified, 0, 0 },

    { ngx_string("ETag"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, etag),
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, etag), 0 },

    { ngx_string("Server"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, server),
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, server), 0 },

    { ngx_string("WWW-Authenticate"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, www_authenticate),
                 ngx_http_upstream_copy_header_line, 0, 0 },

//脰禄脫脨脭脷脜盲脰脙脕脣location /uri {mytest;}潞贸拢卢HTTP驴貌录脺虏脜禄谩脭脷脛鲁赂枚脟毛脟贸脝楼脜盲脕脣/uri潞贸碌梅脫脙脣眉麓娄脌铆脟毛脟贸
    { ngx_string("Location"),  //潞贸露脣脫娄麓冒脮芒赂枚拢卢卤铆脢戮脨猫脪陋脰脴露篓脧貌
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, location),
                 ngx_http_upstream_rewrite_location, 0, 0 }, //ngx_http_upstream_process_headers脰脨脰麓脨脨

    { ngx_string("Refresh"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_rewrite_refresh, 0, 0 },

    { ngx_string("Set-Cookie"),
                 ngx_http_upstream_process_set_cookie,
                 offsetof(ngx_http_upstream_headers_in_t, cookies),
                 ngx_http_upstream_rewrite_set_cookie, 0, 1 },

    { ngx_string("Content-Disposition"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_copy_header_line, 0, 1 },

    { ngx_string("Cache-Control"),
                 ngx_http_upstream_process_cache_control, 0,
                 ngx_http_upstream_copy_multi_header_lines,
                 offsetof(ngx_http_headers_out_t, cache_control), 1 },

    { ngx_string("Expires"),
                 ngx_http_upstream_process_expires, 0,
                 ngx_http_upstream_copy_header_line,
                 offsetof(ngx_http_headers_out_t, expires), 1 },

    { ngx_string("Accept-Ranges"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, accept_ranges),
                 ngx_http_upstream_copy_allow_ranges,
                 offsetof(ngx_http_headers_out_t, accept_ranges), 1 },

    { ngx_string("Connection"),
                 ngx_http_upstream_process_connection, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

    { ngx_string("Keep-Alive"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

    { ngx_string("Vary"), //nginx脭脷禄潞麓忙鹿媒鲁脤脰脨虏禄禄谩麓娄脌铆"Vary"脥路拢卢脦陋脕脣脠路卤拢脪禄脨漏脣陆脫脨脢媒戮脻虏禄卤禄脣霉脫脨碌脛脫脙禄搂驴麓碌陆拢卢
                 ngx_http_upstream_process_vary, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Powered-By"),
                 ngx_http_upstream_ignore_header_line, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Expires"),
                 ngx_http_upstream_process_accel_expires, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Redirect"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, x_accel_redirect),
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Limit-Rate"),
                 ngx_http_upstream_process_limit_rate, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Buffering"),
                 ngx_http_upstream_process_buffering, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("X-Accel-Charset"),
                 ngx_http_upstream_process_charset, 0,
                 ngx_http_upstream_copy_header_line, 0, 0 },

    { ngx_string("Transfer-Encoding"),
                 ngx_http_upstream_process_transfer_encoding, 0,
                 ngx_http_upstream_ignore_header_line, 0, 0 },

#if (NGX_HTTP_GZIP)
    { ngx_string("Content-Encoding"),
                 ngx_http_upstream_process_header_line,
                 offsetof(ngx_http_upstream_headers_in_t, content_encoding),
                 ngx_http_upstream_copy_content_encoding, 0, 0 },
#endif

    { ngx_null_string, NULL, 0, NULL, 0, 0 }
};

/*
鹿脴脫脷nginx upstream碌脛录赂脰脰脜盲脰脙路陆脢陆
路垄卤铆脫脷2011 脛锚 06 脭脗 16 脠脮脫脡edwin 
脝陆脢卤脪禄脰卤脪脌脌碌脫虏录镁脌麓脳梅load blance拢卢脳卯陆眉脩脨戮驴Nginx脌麓脳枚赂潞脭脴脡猫卤赂拢卢录脟脗录脧脗upstream碌脛录赂脰脰脜盲脰脙路陆脢陆隆拢

碌脷脪禄脰脰拢潞脗脰脩炉

upstream test{
    server 192.168.0.1:3000;
    server 192.168.0.1:3001;
}碌脷露镁脰脰拢潞脠篓脰脴

upstream test{
    server 192.168.0.1 weight=2;
    server 192.168.0.2 weight=3;
}脮芒脰脰脛拢脢陆驴脡陆芒戮枚路镁脦帽脝梅脨脭脛脺虏禄碌脠碌脛脟茅驴枚脧脗脗脰脩炉卤脠脗脢碌脛碌梅脜盲

碌脷脠媒脰脰拢潞ip_hash

upstream test{
    ip_hash;
    server 192.168.0.1;
    server 192.168.0.2;
}脮芒脰脰脛拢脢陆禄谩赂霉戮脻脌麓脭麓IP潞脥潞贸露脣脜盲脰脙脌麓脳枚hash路脰脜盲拢卢脠路卤拢鹿脤露篓IP脰禄路脙脦脢脪禄赂枚潞贸露脣

碌脷脣脛脰脰拢潞fair

脨猫脪陋掳虏脳掳Upstream Fair Balancer Module

upstream test{
    server 192.168.0.1;
    server 192.168.0.2;
    fair;
}脮芒脰脰脛拢脢陆禄谩赂霉戮脻潞贸露脣路镁脦帽碌脛脧矛脫娄脢卤录盲脌麓路脰脜盲拢卢脧矛脫娄脢卤录盲露脤碌脛潞贸露脣脫脜脧脠路脰脜盲

碌脷脦氓脰脰拢潞脳脭露篓脪氓hash

脨猫脪陋掳虏脳掳Upstream Hash Module

upstream test{
    server 192.168.0.1;
    server 192.168.0.2;
    hash $request_uri;
}脮芒脰脰脛拢脢陆驴脡脪脭赂霉戮脻赂酶露篓碌脛脳脰路没麓庐陆酶脨脨Hash路脰脜盲

戮脽脤氓脫娄脫脙拢潞

server{
    listen 80;
    server_name .test.com;
    charset utf-8;
    
    location / {
        proxy_pass http://test/;
    } 
}麓脣脥芒upstream脙驴赂枚潞贸露脣碌脛驴脡脡猫脰脙虏脦脢媒脦陋拢潞

1.down: 卤铆脢戮麓脣脤篓server脭脻脢卤虏禄虏脦脫毛赂潞脭脴隆拢
2.weight: 脛卢脠脧脦陋1拢卢weight脭陆麓贸拢卢赂潞脭脴碌脛脠篓脰脴戮脥脭陆麓贸隆拢
3.max_fails: 脭脢脨铆脟毛脟贸脢搂掳脺碌脛麓脦脢媒脛卢脠脧脦陋1.碌卤鲁卢鹿媒脳卯麓贸麓脦脢媒脢卤拢卢路碌禄脴proxy_next_upstream脛拢驴茅露篓脪氓碌脛麓铆脦贸隆拢
4.fail_timeout: max_fails麓脦脢搂掳脺潞贸拢卢脭脻脥拢碌脛脢卤录盲隆拢
5.backup: 脝盲脣眉脣霉脫脨碌脛路脟backup禄煤脝梅down禄貌脮脽脙娄碌脛脢卤潞貌拢卢脟毛脟贸backup禄煤脝梅拢卢脫娄录卤麓毛脢漏隆拢
*/
static ngx_command_t  ngx_http_upstream_commands[] = {
/*
脫茂路篓拢潞upstream name { ... } 
脛卢脠脧脰碌拢潞none 
脢鹿脫脙脳脰露脦拢潞http 
脮芒赂枚脳脰露脦脡猫脰脙脪禄脠潞路镁脦帽脝梅拢卢驴脡脪脭陆芦脮芒赂枚脳脰露脦路脜脭脷proxy_pass潞脥fastcgi_pass脰赂脕卯脰脨脳梅脦陋脪禄赂枚碌楼露脌碌脛脢碌脤氓拢卢脣眉脙脟驴脡脪脭驴脡脪脭脢脟录脿脤媒虏禄脥卢露脣驴脷碌脛路镁脦帽脝梅拢卢
虏垄脟脪脪虏驴脡脪脭脢脟脥卢脢卤录脿脤媒TCP潞脥Unix socket碌脛路镁脦帽脝梅隆拢
路镁脦帽脝梅驴脡脪脭脰赂露篓虏禄脥卢碌脛脠篓脰脴拢卢脛卢脠脧脦陋1隆拢
脢戮脌媒脜盲脰脙

upstream backend {
  server backend1.example.com weight=5;
  server 127.0.0.1:8080       max_fails=3  fail_timeout=30s;
  server unix:/tmp/backend3;

  server backup1.example.com:8080 backup; 
}脟毛脟贸陆芦掳麓脮脮脗脰脩炉碌脛路陆脢陆路脰路垄碌陆潞贸露脣路镁脦帽脝梅拢卢碌芦脥卢脢卤脪虏禄谩驴录脗脟脠篓脰脴隆拢
脭脷脡脧脙忙碌脛脌媒脳脫脰脨脠莽鹿没脙驴麓脦路垄脡煤7赂枚脟毛脟贸拢卢5赂枚脟毛脟贸陆芦卤禄路垄脣脥碌陆backend1.example.com拢卢脝盲脣没脕陆脤篓陆芦路脰卤冒碌脙碌陆脪禄赂枚脟毛脟贸拢卢脠莽鹿没脫脨脪禄脤篓路镁脦帽脝梅虏禄驴脡脫脙拢卢脛脟脙麓
脟毛脟贸陆芦卤禄脳陋路垄碌陆脧脗脪禄脤篓路镁脦帽脝梅拢卢脰卤碌陆脣霉脫脨碌脛路镁脦帽脝梅录矛虏茅露录脥篓鹿媒隆拢脠莽鹿没脣霉脫脨碌脛路镁脦帽脝梅露录脦脼路篓脥篓鹿媒录矛虏茅拢卢脛脟脙麓陆芦路碌禄脴赂酶驴脥禄搂露脣脳卯潞贸脪禄脤篓鹿陇脳梅碌脛路镁脦帽脝梅虏煤脡煤碌脛陆谩鹿没隆拢

max_fails=number

  脡猫脰脙脭脷fail_timeout虏脦脢媒脡猫脰脙碌脛脢卤录盲脛脷脳卯麓贸脢搂掳脺麓脦脢媒拢卢脠莽鹿没脭脷脮芒赂枚脢卤录盲脛脷拢卢脣霉脫脨脮毛露脭赂脙路镁脦帽脝梅碌脛脟毛脟贸
  露录脢搂掳脺脕脣拢卢脛脟脙麓脠脧脦陋赂脙路镁脦帽脝梅禄谩卤禄脠脧脦陋脢脟脥拢禄煤脕脣拢卢脥拢禄煤脢卤录盲脢脟fail_timeout脡猫脰脙碌脛脢卤录盲隆拢脛卢脠脧脟茅驴枚脧脗拢卢
  虏禄鲁脡鹿娄脕卢陆脫脢媒卤禄脡猫脰脙脦陋1隆拢卤禄脡猫脰脙脦陋脕茫脭貌卤铆脢戮虏禄陆酶脨脨脕麓陆脫脢媒脥鲁录脝隆拢脛脟脨漏脕卢陆脫卤禄脠脧脦陋脢脟虏禄鲁脡鹿娄碌脛驴脡脪脭脥篓鹿媒
  proxy_next_upstream, fastcgi_next_upstream拢卢潞脥memcached_next_upstream脰赂脕卯脜盲脰脙隆拢http_404
  脳麓脤卢虏禄禄谩卤禄脠脧脦陋脢脟虏禄鲁脡鹿娄碌脛鲁垄脢脭隆拢

fail_time=time
  脡猫脰脙 露脿鲁陇脢卤录盲脛脷脢搂掳脺麓脦脢媒麓茂碌陆脳卯麓贸脢搂掳脺麓脦脢媒禄谩卤禄脠脧脦陋路镁脦帽脝梅脥拢禄煤脕脣路镁脦帽脝梅禄谩卤禄脠脧脦陋脥拢禄煤碌脛脢卤录盲鲁陇露脠 脛卢脠脧脟茅驴枚脧脗拢卢鲁卢脢卤脢卤录盲卤禄脡猫脰脙脦陋10S
*/
    { ngx_string("upstream"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_TAKE1,
      ngx_http_upstream,
      0,
      0,
      NULL },

    /*
    脫茂路篓拢潞server name [parameters];
    脜盲脰脙驴茅拢潞upstream
    server脜盲脰脙脧卯脰赂露篓脕脣脪禄脤篓脡脧脫脦路镁脦帽脝梅碌脛脙没脳脰拢卢脮芒赂枚脙没脳脰驴脡脪脭脢脟脫貌脙没隆垄IP碌脴脰路露脣驴脷隆垄UNIX戮盲卤煤碌脠拢卢脭脷脝盲潞贸禄鹿驴脡脪脭赂煤脧脗脕脨虏脦脢媒隆拢
    weight=number拢潞脡猫脰脙脧貌脮芒脤篓脡脧脫脦路镁脦帽脝梅脳陋路垄碌脛脠篓脰脴拢卢脛卢脠脧脦陋1隆拢 weigth虏脦脢媒卤铆脢戮脠篓脰碌拢卢脠篓脰碌脭陆赂脽卤禄路脰脜盲碌陆碌脛录赂脗脢脭陆麓贸 
    max_fails=number拢潞赂脙脩隆脧卯脫毛fail_timeout脜盲潞脧脢鹿脫脙拢卢脰赂脭脷fail_timeout脢卤录盲露脦脛脷拢卢脠莽鹿没脧貌碌卤脟掳碌脛脡脧脫脦路镁脦帽脝梅脳陋路垄脢搂掳脺麓脦脢媒鲁卢鹿媒number拢卢脭貌脠脧脦陋脭脷碌卤脟掳碌脛fail_timeout脢卤录盲露脦脛脷脮芒脤篓脡脧脫脦路镁脦帽脝梅虏禄驴脡脫脙隆拢max_fails脛卢脠脧脦陋1拢卢脠莽鹿没脡猫脰脙脦陋0拢卢脭貌卤铆脢戮虏禄录矛虏茅脢搂掳脺麓脦脢媒隆拢
    fail_timeout=time拢潞fail_timeout卤铆脢戮赂脙脢卤录盲露脦脛脷脳陋路垄脢搂掳脺露脿脡脵麓脦潞贸戮脥脠脧脦陋脡脧脫脦路镁脦帽脝梅脭脻脢卤虏禄驴脡脫脙拢卢脫脙脫脷脫脜禄炉路麓脧貌麓煤脌铆鹿娄脛脺隆拢脣眉脫毛脧貌脡脧脫脦路镁脦帽脝梅陆篓脕垄脕卢陆脫碌脛鲁卢脢卤脢卤录盲隆垄露脕脠隆脡脧脫脦路镁脦帽脝梅碌脛脧矛脫娄鲁卢脢卤脢卤录盲碌脠脥锚脠芦脦脼鹿脴隆拢fail_timeout脛卢脠脧脦陋10脙毛隆拢
    down拢潞卤铆脢戮脣霉脭脷碌脛脡脧脫脦路镁脦帽脝梅脫脌戮脙脧脗脧脽拢卢脰禄脭脷脢鹿脫脙ip_hash脜盲脰脙脧卯脢卤虏脜脫脨脫脙隆拢
    backup拢潞脭脷脢鹿脫脙ip_hash脜盲脰脙脧卯脢卤脣眉脢脟脦脼脨搂碌脛隆拢脣眉卤铆脢戮脣霉脭脷碌脛脡脧脫脦路镁脦帽脝梅脰禄脢脟卤赂路脻路镁脦帽脝梅拢卢脰禄脫脨脭脷脣霉脫脨碌脛路脟卤赂路脻脡脧脫脦路镁脦帽脝梅露录脢搂脨搂潞贸拢卢虏脜禄谩脧貌脣霉脭脷碌脛脡脧脫脦路镁脦帽脝梅脳陋路垄脟毛脟贸隆拢
    脌媒脠莽拢潞
    upstream  backend  {
      server   backend1.example.com    weight=5;
      server   127.0.0.1:8080          max_fails=3  fail_timeout=30s;
      server   unix:/tmp/backend3;
    }

    
    脫茂路篓拢潞server name [parameters] 
    脛卢脠脧脰碌拢潞none 
    脢鹿脫脙脳脰露脦拢潞upstream 
    脰赂露篓潞贸露脣路镁脦帽脝梅碌脛脙没鲁脝潞脥脪禄脨漏虏脦脢媒拢卢驴脡脪脭脢鹿脫脙脫貌脙没拢卢IP拢卢露脣驴脷拢卢禄貌脮脽unix socket隆拢脠莽鹿没脰赂露篓脦陋脫貌脙没拢卢脭貌脢脳脧脠陆芦脝盲陆芒脦枚脦陋IP隆拢
    隆陇weight = NUMBER - 脡猫脰脙路镁脦帽脝梅脠篓脰脴拢卢脛卢脠脧脦陋1隆拢
    隆陇max_fails = NUMBER - 脭脷脪禄露篓脢卤录盲脛脷拢篓脮芒赂枚脢卤录盲脭脷fail_timeout虏脦脢媒脰脨脡猫脰脙拢漏录矛虏茅脮芒赂枚路镁脦帽脝梅脢脟路帽驴脡脫脙脢卤虏煤脡煤碌脛脳卯露脿脢搂掳脺脟毛脟贸脢媒拢卢脛卢脠脧脦陋1拢卢陆芦脝盲脡猫脰脙脦陋0驴脡脪脭鹿脴卤脮录矛虏茅拢卢脮芒脨漏麓铆脦贸脭脷proxy_next_upstream禄貌fastcgi_next_upstream拢篓404麓铆脦贸虏禄禄谩脢鹿max_fails脭枚录脫拢漏脰脨露篓脪氓隆拢
    隆陇fail_timeout = TIME - 脭脷脮芒赂枚脢卤录盲脛脷虏煤脡煤脕脣max_fails脣霉脡猫脰脙麓贸脨隆碌脛脢搂掳脺鲁垄脢脭脕卢陆脫脟毛脟贸潞贸脮芒赂枚路镁脦帽脝梅驴脡脛脺虏禄驴脡脫脙拢卢脥卢脩霉脣眉脰赂露篓脕脣路镁脦帽脝梅虏禄驴脡脫脙碌脛脢卤录盲拢篓脭脷脧脗脪禄麓脦鲁垄脢脭脕卢陆脫脟毛脟贸路垄脝冒脰庐脟掳拢漏拢卢脛卢脠脧脦陋10脙毛拢卢fail_timeout脫毛脟掳露脣脧矛脫娄脢卤录盲脙禄脫脨脰卤陆脫鹿脴脧碌拢卢虏禄鹿媒驴脡脪脭脢鹿脫脙proxy_connect_timeout潞脥proxy_read_timeout脌麓驴脴脰脝隆拢
    隆陇down - 卤锚录脟路镁脦帽脝梅麓娄脫脷脌毛脧脽脳麓脤卢拢卢脥篓鲁拢潞脥ip_hash脪禄脝冒脢鹿脫脙隆拢
    隆陇backup - (0.6.7禄貌赂眉赂脽)脠莽鹿没脣霉脫脨碌脛路脟卤赂路脻路镁脦帽脝梅露录氓麓禄煤禄貌路卤脙娄拢卢脭貌脢鹿脫脙卤戮路镁脦帽脝梅拢篓脦脼路篓潞脥ip_hash脰赂脕卯麓卯脜盲脢鹿脫脙拢漏隆拢
    脢戮脌媒脜盲脰脙
    
    upstream  backend  {
      server   backend1.example.com    weight=5;
      server   127.0.0.1:8080          max_fails=3  fail_timeout=30s;
      server   unix:/tmp/backend3;
    }脳垄脪芒拢潞脠莽鹿没脛茫脰禄脢鹿脫脙脪禄脤篓脡脧脫脦路镁脦帽脝梅拢卢nginx陆芦脡猫脰脙脪禄赂枚脛脷脰脙卤盲脕驴脦陋1拢卢录麓max_fails潞脥fail_timeout虏脦脢媒虏禄禄谩卤禄麓娄脌铆隆拢
    陆谩鹿没拢潞脠莽鹿没nginx虏禄脛脺脕卢陆脫碌陆脡脧脫脦拢卢脟毛脟贸陆芦露陋脢搂隆拢
    陆芒戮枚拢潞脢鹿脫脙露脿脤篓脡脧脫脦路镁脦帽脝梅隆拢
    */
    { ngx_string("server"),
      NGX_HTTP_UPS_CONF|NGX_CONF_1MORE,
      ngx_http_upstream_server,
      NGX_HTTP_SRV_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_module_ctx = {
    ngx_http_upstream_add_variables,       /* preconfiguration */
    NULL,                                  /* postconfiguration */

    ngx_http_upstream_create_main_conf,    /* create main configuration */
    ngx_http_upstream_init_main_conf,      /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

/*
赂潞脭脴戮霉潞芒脧脿鹿脴脜盲脰脙:
upstream
server
ip_hash:赂霉戮脻驴脥禄搂露脣碌脛IP脌麓脳枚hash,虏禄鹿媒脠莽鹿没squid -- nginx -- server(s)脭貌拢卢ip脫脌脭露脢脟squid路镁脦帽脝梅ip,脪貌麓脣虏禄鹿脺脫脙,脨猫脪陋ngx_http_realip_module禄貌脮脽碌脷脠媒路陆脛拢驴茅
keepalive:脜盲脰脙碌陆潞贸露脣碌脛脳卯麓贸脕卢陆脫脢媒拢卢卤拢鲁脰鲁陇脕卢陆脫拢卢虏禄卤脴脦陋脙驴脪禄赂枚驴脥禄搂露脣露录脰脴脨脗陆篓脕垄nginx碌陆潞贸露脣PHP碌脠路镁脦帽脝梅碌脛脕卢陆脫拢卢脨猫脪陋卤拢鲁脰潞脥潞贸露脣
    鲁陇脕卢陆脫拢卢脌媒脠莽fastcgi:fastcgi_keep_conn on;       proxy:  proxy_http_version 1.1;  proxy_set_header Connection "";
least_conn:赂霉戮脻脝盲脠篓脰脴脰碌拢卢陆芦脟毛脟贸路垄脣脥碌陆禄卯脭戮脕卢陆脫脢媒脳卯脡脵碌脛脛脟脤篓路镁脦帽脝梅
hash:驴脡脪脭掳麓脮脮uri  ip 碌脠虏脦脢媒陆酶脨脨脳枚hash

虏脦驴录http://tengine.taobao.org/nginx_docs/cn/docs/http/ngx_http_upstream_module.html#ip_hash
*/


/*
Nginx虏禄陆枚陆枚驴脡脪脭脫脙脳枚Web路镁脦帽脝梅隆拢upstream禄煤脰脝脝盲脢碌脢脟脫脡ngx_http_upstream_module脛拢驴茅脢碌脧脰碌脛拢卢脣眉脢脟脪禄赂枚HTTP脛拢驴茅拢卢脢鹿脫脙upstream禄煤脰脝脢卤驴脥
禄搂露脣碌脛脟毛脟贸卤脴脨毛禄霉脫脷HTTP隆拢

录脠脠禄upstream脢脟脫脙脫脷路脙脦脢隆掳脡脧脫脦隆卤路镁脦帽脝梅碌脛拢卢脛脟脙麓拢卢Nginx脨猫脪陋路脙脦脢脢虏脙麓脌脿脨脥碌脛隆掳脡脧脫脦隆卤路镁脦帽脝梅脛脴拢驴脢脟Apache隆垄Tomcat脮芒脩霉碌脛Web路镁脦帽脝梅拢卢禄鹿
脢脟memcached隆垄cassandra脮芒脩霉碌脛Key-Value麓忙麓垄脧碌脥鲁拢卢脫脰禄貌脢脟mongoDB隆垄MySQL脮芒脩霉碌脛脢媒戮脻驴芒拢驴脮芒戮脥脡忙录掳upstream禄煤脰脝碌脛路露脦搂脕脣隆拢禄霉脫脷脢脗录镁脟媒露炉
录脺鹿鹿碌脛upstream禄煤脰脝脣霉脪陋路脙脦脢碌脛戮脥脢脟脣霉脫脨脰搂鲁脰TCP碌脛脡脧脫脦路镁脦帽脝梅隆拢脪貌麓脣拢卢录脠脫脨ngx_http_proxy_module脛拢驴茅禄霉脫脷upstream禄煤脰脝脢碌脧脰脕脣HTTP碌脛路麓脧貌
麓煤脌铆鹿娄脛脺拢卢脪虏脫脨脌脿脣脝ngx_http_memcached_module碌脛脛拢驴茅禄霉脫脷upstream禄煤脰脝脢鹿碌脙脟毛脟贸驴脡脪脭路脙脦脢memcached路镁脦帽脝梅隆拢

碌卤nginx陆脫脢脮碌陆脪禄赂枚脕卢陆脫潞贸拢卢露脕脠隆脥锚驴脥禄搂露脣路垄脣脥鲁枚脌麓碌脛Header拢卢脠禄潞贸戮脥禄谩陆酶脨脨赂梅赂枚麓娄脌铆鹿媒鲁脤碌脛碌梅脫脙隆拢脰庐潞贸戮脥脢脟upstream路垄禄脫脳梅脫脙碌脛脢卤潞貌脕脣拢卢
upstream脭脷驴脥禄搂露脣赂煤潞贸露脣卤脠脠莽FCGI/PHP脰庐录盲拢卢陆脫脢脮驴脥禄搂露脣碌脛HTTP body拢卢路垄脣脥赂酶FCGI拢卢脠禄潞贸陆脫脢脮FCGI碌脛陆谩鹿没拢卢路垄脣脥赂酶驴脥禄搂露脣隆拢脳梅脦陋脪禄赂枚脟脜脕潞碌脛脳梅脫脙隆拢
脥卢脢卤拢卢upstream脦陋脕脣鲁盲路脰脧脭脢戮脝盲脕茅禄卯脨脭拢卢脰脕脫脷潞贸露脣戮脽脤氓脢脟脢虏脙麓脨颅脪茅拢卢脢虏脙麓脧碌脥鲁脣没露录虏禄care拢卢脦脪脰禄脢碌脧脰脰梅脤氓碌脛驴貌录脺拢卢戮脽脤氓碌陆FCGI脨颅脪茅碌脛路垄脣脥拢卢陆脫脢脮拢卢
陆芒脦枚拢卢脮芒脨漏露录陆禄赂酶潞贸脙忙碌脛虏氓录镁脌麓麓娄脌铆拢卢卤脠脠莽脫脨fastcgi,memcached,proxy碌脠虏氓录镁

http://chenzhenianqing.cn/articles/category/%e5%90%84%e7%a7%8dserver/nginx
upstream潞脥FastCGI memcached  uwsgi  scgi proxy碌脛鹿脴脧碌虏脦驴录:http://chenzhenianqing.cn/articles/category/%e5%90%84%e7%a7%8dserver/nginx
*/
ngx_module_t  ngx_http_upstream_module = { //赂脙脛拢驴茅脢脟路脙脦脢脡脧脫脦路镁脦帽脝梅脧脿鹿脴脛拢驴茅碌脛禄霉麓隆(脌媒脠莽 FastCGI memcached  uwsgi  scgi proxy露录禄谩脫脙碌陆upstream脛拢驴茅  ngx_http_proxy_module  ngx_http_memcached_module)
    NGX_MODULE_V1,
    &ngx_http_upstream_module_ctx,            /* module context */
    ngx_http_upstream_commands,               /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_variable_t  ngx_http_upstream_vars[] = {

    { ngx_string("upstream_addr"), NULL,
      ngx_http_upstream_addr_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 }, //脟掳露脣路镁脦帽脝梅麓娄脌铆脟毛脟贸碌脛路镁脦帽脝梅碌脴脰路

    { ngx_string("upstream_status"), NULL,
      ngx_http_upstream_status_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 }, //脟掳露脣路镁脦帽脝梅碌脛脧矛脫娄脳麓脤卢隆拢

    { ngx_string("upstream_connect_time"), NULL,
      ngx_http_upstream_response_time_variable, 2,
      NGX_HTTP_VAR_NOCACHEABLE, 0 }, 

    { ngx_string("upstream_header_time"), NULL,
      ngx_http_upstream_response_time_variable, 1,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_response_time"), NULL,
      ngx_http_upstream_response_time_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },//脟掳露脣路镁脦帽脝梅碌脛脫娄麓冒脢卤录盲拢卢戮芦脠路碌陆潞脕脙毛拢卢虏禄脥卢碌脛脫娄麓冒脪脭露潞潞脜潞脥脙掳潞脜路脰驴陋隆拢

    { ngx_string("upstream_response_length"), NULL,
      ngx_http_upstream_response_length_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

#if (NGX_HTTP_CACHE)

    { ngx_string("upstream_cache_status"), NULL,
      ngx_http_upstream_cache_status, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_cache_last_modified"), NULL,
      ngx_http_upstream_cache_last_modified, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

    { ngx_string("upstream_cache_etag"), NULL,
      ngx_http_upstream_cache_etag, 0,
      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },

#endif

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_http_upstream_next_t  ngx_http_upstream_next_errors[] = {
    { 500, NGX_HTTP_UPSTREAM_FT_HTTP_500 },
    { 502, NGX_HTTP_UPSTREAM_FT_HTTP_502 },
    { 503, NGX_HTTP_UPSTREAM_FT_HTTP_503 },
    { 504, NGX_HTTP_UPSTREAM_FT_HTTP_504 },
    { 403, NGX_HTTP_UPSTREAM_FT_HTTP_403 },
    { 404, NGX_HTTP_UPSTREAM_FT_HTTP_404 },
    { 0, 0 }
};


ngx_conf_bitmask_t  ngx_http_upstream_cache_method_mask[] = {
   { ngx_string("GET"),  NGX_HTTP_GET},
   { ngx_string("HEAD"), NGX_HTTP_HEAD },
   { ngx_string("POST"), NGX_HTTP_POST },
   { ngx_null_string, 0 }
};


ngx_conf_bitmask_t  ngx_http_upstream_ignore_headers_masks[] = {
    { ngx_string("X-Accel-Redirect"), NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT },
    { ngx_string("X-Accel-Expires"), NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES },
    { ngx_string("X-Accel-Limit-Rate"), NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE },
    { ngx_string("X-Accel-Buffering"), NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING },
    { ngx_string("X-Accel-Charset"), NGX_HTTP_UPSTREAM_IGN_XA_CHARSET },
    { ngx_string("Expires"), NGX_HTTP_UPSTREAM_IGN_EXPIRES },
    { ngx_string("Cache-Control"), NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL },
    { ngx_string("Set-Cookie"), NGX_HTTP_UPSTREAM_IGN_SET_COOKIE },
    { ngx_string("Vary"), NGX_HTTP_UPSTREAM_IGN_VARY },
    { ngx_null_string, 0 }
};


//ngx_http_upstream_create麓麓陆篓ngx_http_upstream_t拢卢脳脢脭麓禄脴脢脮脫脙ngx_http_upstream_finalize_request
//upstream脳脢脭麓禄脴脢脮脭脷ngx_http_upstream_finalize_request   ngx_http_XXX_handler(ngx_http_proxy_handler)脰脨脰麓脨脨
ngx_int_t
ngx_http_upstream_create(ngx_http_request_t *r)//麓麓陆篓脪禄赂枚ngx_http_upstream_t陆谩鹿鹿拢卢路脜碌陆r->upstream脌茂脙忙脠楼隆拢
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

    if (u && u->cleanup) {
        r->main->count++;
        ngx_http_upstream_cleanup(r);
    }

    u = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_t));
    if (u == NULL) {
        return NGX_ERROR;
    }

    r->upstream = u;

    u->peer.log = r->connection->log;
    u->peer.log_error = NGX_ERROR_ERR;

#if (NGX_HTTP_CACHE)
    r->cache = NULL;
#endif

    u->headers_in.content_length_n = -1;
    u->headers_in.last_modified_time = -1;

    return NGX_OK;
}

/*
    1)碌梅脫脙ngx_http_up stream_init路陆路篓脝么露炉upstream隆拢
    2) upstream脛拢驴茅禄谩脠楼录矛虏茅脦脛录镁禄潞麓忙拢卢脠莽鹿没禄潞麓忙脰脨脪脩戮颅脫脨潞脧脢脢碌脛脧矛脫娄掳眉拢卢脭貌禄谩脰卤陆脫路碌禄脴禄潞麓忙拢篓碌卤脠禄卤脴脨毛脢脟脭脷脢鹿脫脙路麓脧貌麓煤脌铆脦脛录镁禄潞麓忙碌脛脟掳脤谩脧脗拢漏隆拢
    脦陋脕脣脠脙露脕脮脽路陆卤茫碌脴脌铆陆芒upstream禄煤脰脝拢卢卤戮脮脗陆芦虏禄脭脵脤谩录掳脦脛录镁禄潞麓忙隆拢
    3)禄脴碌梅mytest脛拢驴茅脪脩戮颅脢碌脧脰碌脛create_request禄脴碌梅路陆路篓隆拢
    4) mytest脛拢驴茅脥篓鹿媒脡猫脰脙r->upstream->request_bufs脪脩戮颅戮枚露篓潞脙路垄脣脥脢虏脙麓脩霉碌脛脟毛脟贸碌陆脡脧脫脦路镁脦帽脝梅隆拢
    5) upstream脛拢驴茅陆芦禄谩录矛虏茅resolved鲁脡脭卤拢卢脠莽鹿没脫脨resolved鲁脡脭卤碌脛禄掳拢卢戮脥赂霉戮脻脣眉脡猫脰脙潞脙脡脧脫脦路镁脦帽脝梅碌脛碌脴脰路r->upstream->peer鲁脡脭卤隆拢
    6)脫脙脦脼脳猫脠没碌脛TCP脤脳陆脫脳脰陆篓脕垄脕卢陆脫隆拢
    7)脦脼脗脹脕卢陆脫脢脟路帽陆篓脕垄鲁脡鹿娄拢卢赂潞脭冒陆篓脕垄脕卢陆脫碌脛connect路陆路篓露录禄谩脕垄驴脤路碌禄脴隆拢
*/
//ngx_http_upstream_init路陆路篓陆芦禄谩赂霉戮脻ngx_http_upstream_conf_t脰脨碌脛鲁脡脭卤鲁玫脢录禄炉upstream拢卢脥卢脢卤禄谩驴陋脢录脕卢陆脫脡脧脫脦路镁脦帽脝梅拢卢脪脭麓脣脮鹿驴陋脮没赂枚upstream麓娄脌铆脕梅鲁脤
void ngx_http_upstream_init(ngx_http_request_t *r) 
//脭脷露脕脠隆脥锚盲炉脌脌脝梅路垄脣脥脌麓碌脛脟毛脟贸脥路虏驴脳脰露脦潞贸拢卢禄谩脥篓鹿媒proxy fastcgi碌脠脛拢驴茅露脕脠隆掳眉脤氓拢卢露脕脠隆脥锚潞贸脰麓脨脨赂脙潞炉脢媒拢卢脌媒脠莽ngx_http_read_client_request_body(r, ngx_http_upstream_init);
{
    ngx_connection_t     *c;

    c = r->connection;//碌脙碌陆驴脥禄搂露脣脕卢陆脫陆谩鹿鹿隆拢

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http init upstream, client timer: %d", c->read->timer_set);

#if (NGX_HTTP_V2)
    if (r->stream) {
        ngx_http_upstream_init_request(r);
        return;
    }
#endif

    /*
        脢脳脧脠录矛虏茅脟毛脟贸露脭脫娄脫脷驴脥禄搂露脣碌脛脕卢陆脫拢卢脮芒赂枚脕卢陆脫脡脧碌脛露脕脢脗录镁脠莽鹿没脭脷露篓脢卤脝梅脰脨拢卢脪虏戮脥脢脟脣碌拢卢露脕脢脗录镁碌脛timer_ set卤锚脰戮脦禄脦陋1拢卢脛脟脙麓碌梅脫脙ngx_del_timer
    路陆路篓掳脩脮芒赂枚露脕脢脗录镁麓脫露篓脢卤脝梅脰脨脪脝鲁媒隆拢脦陋脢虏脙麓脪陋脳枚脮芒录镁脢脗脛脴拢驴脪貌脦陋脪禄碌漏脝么露炉upstream禄煤脰脝拢卢戮脥虏禄脫娄赂脙露脭驴脥禄搂露脣碌脛露脕虏脵脳梅麓酶脫脨鲁卢脢卤脢卤录盲碌脛麓娄脌铆(鲁卢脢卤禄谩鹿脴卤脮驴脥禄搂露脣脕卢陆脫)拢卢
    脟毛脟贸碌脛脰梅脪陋麓楼路垄脢脗录镁陆芦脪脭脫毛脡脧脫脦路镁脦帽脝梅碌脛脕卢陆脫脦陋脰梅隆拢
     */
    if (c->read->timer_set) {
        ngx_del_timer(c->read, NGX_FUNC_LINE);
    }

    if (ngx_event_flags & NGX_USE_CLEAR_EVENT) { //脠莽鹿没epoll脢鹿脫脙卤脽脭碌麓楼路垄

/*
2025/04/24 05:31:47[             ngx_http_upstream_init,   654]  [debug] 15507#15507: *1 <   ngx_http_upstream_init,   653> epoll NGX_WRITE_EVENT(et) read add
2025/04/24 05:31:47[                ngx_epoll_add_event,  1400]  [debug] 15507#15507: *1 epoll modify read and write event: fd:11 op:3 ev:80002005
025/04/24 05:31:47[           ngx_epoll_process_events,  1624]  [debug] 15507#15507: begin to epoll_wait, epoll timer: 60000 
2025/04/24 05:31:47[           ngx_epoll_process_events,  1709]  [debug] 15507#15507: epoll: fd:11 epoll-out(ev:0004) d:B26A00E8
脢碌录脢脡脧脢脟脥篓鹿媒ngx_http_upstream_init脰脨碌脛mod epoll_ctl脤铆录脫露脕脨麓脢脗录镁麓楼路垄碌脛拢卢碌卤卤戮麓脦脩颅禄路脥脣禄脴碌陆ngx_worker_process_cycle ..->ngx_epoll_process_events
碌脛脢卤潞貌拢卢戮脥禄谩麓楼路垄epoll_out,麓脫露酶脰麓脨脨ngx_http_upstream_wr_check_broken_connection
*/
        //脮芒脌茂脢碌录脢脡脧脢脟麓楼路垄脰麓脨脨ngx_http_upstream_check_broken_connection
        if (!c->write->active) {//脪陋脭枚录脫驴脡脨麓脢脗录镁脥篓脰陋拢卢脦陋脡露?脪貌脦陋麓媒禄谩驴脡脛脺戮脥脛脺脨麓脕脣,驴脡脛脺禄谩脳陋路垄脡脧脫脦路镁脦帽脝梅碌脛脛脷脠脻赂酶盲炉脌脌脝梅碌脠驴脥禄搂露脣
            //脢碌录脢脡脧脢脟录矛虏茅潞脥驴脥禄搂露脣碌脛脕卢陆脫脢脟路帽脪矛鲁拢脕脣
            char tmpbuf[256];
            
            snprintf(tmpbuf, sizeof(tmpbuf), "<%25s, %5d> epoll NGX_WRITE_EVENT(et) write add", NGX_FUNC_LINE);
            ngx_log_debug0(NGX_LOG_DEBUG_EVENT, c->log, 0, tmpbuf);
            if (ngx_add_event(c->write, NGX_WRITE_EVENT, NGX_CLEAR_EVENT)
                == NGX_ERROR)
            {
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
        }
    }

    ngx_http_upstream_init_request(r);
}

//ngx_http_upstream_init碌梅脫脙脮芒脌茂拢卢麓脣脢卤驴脥禄搂露脣路垄脣脥碌脛脢媒戮脻露录脪脩戮颅陆脫脢脮脥锚卤脧脕脣隆拢
/*
1. 碌梅脫脙create_request麓麓陆篓fcgi禄貌脮脽proxy碌脛脢媒戮脻陆谩鹿鹿隆拢
2. 碌梅脫脙ngx_http_upstream_connect脕卢陆脫脧脗脫脦路镁脦帽脝梅隆拢
*/ 
static void
ngx_http_upstream_init_request(ngx_http_request_t *r)
{
    ngx_str_t                      *host;
    ngx_uint_t                      i;
    ngx_resolver_ctx_t             *ctx, temp;
    ngx_http_cleanup_t             *cln;
    ngx_http_upstream_t            *u;
    ngx_http_core_loc_conf_t       *clcf;
    ngx_http_upstream_srv_conf_t   *uscf, **uscfp;
    ngx_http_upstream_main_conf_t  *umcf;
    

    if (r->aio) {
        return;
    }

    u = r->upstream;//ngx_http_upstream_create脌茂脙忙脡猫脰脙碌脛  ngx_http_XXX_handler(ngx_http_proxy_handler)脰脨脰麓脨脨

#if (NGX_HTTP_CACHE)

    if (u->conf->cache) {
        ngx_int_t  rc;

        int cache = u->conf->cache;
        rc = ngx_http_upstream_cache(r, u);
        ngx_log_debugall(r->connection->log, 0, "ngx http cache, conf->cache:%d, rc:%d", cache, rc);
        
        if (rc == NGX_BUSY) {
            r->write_event_handler = ngx_http_upstream_init_request;
            return;
        }

        r->write_event_handler = ngx_http_request_empty_handler;

        if (rc == NGX_DONE) {
            return;
        }

        if (rc == NGX_ERROR) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        if (rc != NGX_DECLINED) {
            ngx_http_finalize_request(r, rc);
            return;
        }
    }

#endif

    u->store = u->conf->store;

    /*
    脡猫脰脙Nginx脫毛脧脗脫脦驴脥禄搂露脣脰庐录盲TCP脕卢陆脫碌脛录矛虏茅路陆路篓
    脢碌录脢脡脧拢卢脮芒脕陆赂枚路陆路篓露录禄谩脥篓鹿媒ngx_http_upstream_check_broken_connection路陆路篓录矛虏茅Nginx脫毛脧脗脫脦碌脛脕卢陆脫脢脟路帽脮媒鲁拢拢卢脠莽鹿没鲁枚脧脰麓铆脦贸拢卢戮脥禄谩脕垄录麓脰脮脰鹿脕卢陆脫隆拢
     */
/*
2025/04/24 05:31:47[             ngx_http_upstream_init,   654]  [debug] 15507#15507: *1 <   ngx_http_upstream_init,   653> epoll NGX_WRITE_EVENT(et) read add
2025/04/24 05:31:47[                ngx_epoll_add_event,  1400]  [debug] 15507#15507: *1 epoll modify read and write event: fd:11 op:3 ev:80002005
2025/04/24 05:31:47[           ngx_epoll_process_events,  1624]  [debug] 15507#15507: begin to epoll_wait, epoll timer: 60000 
2025/04/24 05:31:47[           ngx_epoll_process_events,  1709]  [debug] 15507#15507: epoll: fd:11 epoll-out(ev:0004) d:B26A00E8
脢碌录脢脡脧脢脟脥篓鹿媒ngx_http_upstream_init脰脨碌脛mod epoll_ctl脤铆录脫露脕脨麓脢脗录镁麓楼路垄碌脛拢卢碌卤卤戮麓脦脩颅禄路脥脣禄脴碌陆ngx_worker_process_cycle ..->ngx_epoll_process_events
碌脛脢卤潞貌拢卢戮脥禄谩麓楼路垄epoll_out,麓脫露酶脰麓脨脨ngx_http_upstream_wr_check_broken_connection
*/
    if (!u->store && !r->post_action && !u->conf->ignore_client_abort) {
        //脳垄脪芒脮芒脢卤潞貌碌脛r禄鹿脢脟驴脥禄搂露脣碌脛脕卢陆脫拢卢脫毛脡脧脫脦路镁脦帽脝梅碌脛脕卢陆脫r禄鹿脙禄脫脨陆篓脕垄
        r->read_event_handler = ngx_http_upstream_rd_check_broken_connection; //脡猫脰脙禄脴碌梅脨猫脪陋录矛虏芒脕卢陆脫脢脟路帽脫脨脦脢脤芒隆拢
        r->write_event_handler = ngx_http_upstream_wr_check_broken_connection;
    }
    

    //脫脨陆脫脢脮碌陆驴脥禄搂露脣掳眉脤氓拢卢脭貌掳脩掳眉脤氓陆谩鹿鹿赂鲁脰碌赂酶u->request_bufs拢卢脭脷潞贸脙忙碌脛if (u->create_request(r) != NGX_OK) {禄谩脫脙碌陆
    if (r->request_body) {//驴脥禄搂露脣路垄脣脥鹿媒脌麓碌脛POST脢媒戮脻麓忙路脜脭脷麓脣,ngx_http_read_client_request_body路脜碌脛
        u->request_bufs = r->request_body->bufs; //录脟脗录驴脥禄搂露脣路垄脣脥碌脛脢媒戮脻拢卢脧脗脙忙脭脷create_request碌脛脢卤潞貌驴陆卤麓碌陆路垄脣脥禄潞鲁氓脕麓陆脫卤铆脌茂脙忙碌脛隆拢
    }

    /*
     碌梅脫脙脟毛脟贸脰脨ngx_http_upstream_t陆谩鹿鹿脤氓脌茂脫脡脛鲁赂枚HTTP脛拢驴茅脢碌脧脰碌脛create_request路陆路篓拢卢鹿鹿脭矛路垄脥霉脡脧脫脦路镁脦帽脝梅碌脛脟毛脟贸
     拢篓脟毛脟贸脰脨碌脛脛脷脠脻脢脟脡猫脰脙碌陆request_bufs禄潞鲁氓脟酶脕麓卤铆脰脨碌脛拢漏隆拢脠莽鹿没create_request路陆路篓脙禄脫脨路碌禄脴NGX_OK拢卢脭貌upstream陆谩脢酶

     脠莽鹿没脢脟FCGI隆拢脧脗脙忙脳茅陆篓潞脙FCGI碌脛赂梅脰脰脥路虏驴拢卢掳眉脌篓脟毛脟贸驴陋脢录脥路拢卢脟毛脟贸虏脦脢媒脥路拢卢脟毛脟贸STDIN脥路隆拢麓忙路脜脭脷u->request_bufs脕麓陆脫卤铆脌茂脙忙隆拢
	脠莽鹿没脢脟Proxy脛拢驴茅拢卢ngx_http_proxy_create_request脳茅录镁路麓脧貌麓煤脌铆碌脛脥路虏驴脡露碌脛,路脜碌陆u->request_bufs脌茂脙忙
	FastCGI memcached  uwsgi  scgi proxy露录禄谩脫脙碌陆upstream脛拢驴茅
     */
    if (u->create_request(r) != NGX_OK) { //ngx_http_XXX_create_request   ngx_http_proxy_create_request碌脠
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    
    /* 禄帽脠隆ngx_http_upstream_t陆谩鹿鹿脰脨脰梅露炉脕卢陆脫陆谩鹿鹿peer碌脛local卤戮碌脴碌脴脰路脨脜脧垄 */
    u->peer.local = ngx_http_upstream_get_local(r, u->conf->local);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
    
    /* 鲁玫脢录禄炉ngx_http_upstream_t陆谩鹿鹿脰脨鲁脡脭卤output脧貌脧脗脫脦路垄脣脥脧矛脫娄碌脛路陆脢陆 */
    u->output.alignment = clcf->directio_alignment; //
    u->output.pool = r->pool;
    u->output.bufs.num = 1;
    u->output.bufs.size = clcf->client_body_buffer_size;

    if (u->output.output_filter == NULL) {
        //脡猫脰脙鹿媒脗脣脛拢驴茅碌脛驴陋脢录鹿媒脗脣潞炉脢媒脦陋writer隆拢脪虏戮脥脢脟output_filter隆拢脭脷ngx_output_chain卤禄碌梅脫脙脪脩陆酶脨脨脢媒戮脻碌脛鹿媒脗脣
        u->output.output_filter = ngx_chain_writer; 
        u->output.filter_ctx = &u->writer; //虏脦驴录ngx_chain_writer拢卢脌茂脙忙禄谩陆芦脢盲鲁枚buf脪禄赂枚赂枚脕卢陆脫碌陆脮芒脌茂隆拢
    }

    u->writer.pool = r->pool;
    
    /* 脤铆录脫脫脙脫脷卤铆脢戮脡脧脫脦脧矛脫娄碌脛脳麓脤卢拢卢脌媒脠莽拢潞麓铆脦贸卤脿脗毛隆垄掳眉脤氓鲁陇露脠碌脠 */
    if (r->upstream_states == NULL) {//脢媒脳茅upstream_states拢卢卤拢脕么upstream碌脛脳麓脤卢脨脜脧垄隆拢

        r->upstream_states = ngx_array_create(r->pool, 1,
                                            sizeof(ngx_http_upstream_state_t));
        if (r->upstream_states == NULL) {
            ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

    } else {

        u->state = ngx_array_push(r->upstream_states);
        if (u->state == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        ngx_memzero(u->state, sizeof(ngx_http_upstream_state_t));
    }

    cln = ngx_http_cleanup_add(r, 0);//禄路脨脦脕麓卤铆拢卢脡锚脟毛脪禄赂枚脨脗碌脛脭陋脣脴隆拢
    if (cln == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    cln->handler = ngx_http_upstream_cleanup; //碌卤脟毛脟贸陆谩脢酶脢卤拢卢脪禄露篓禄谩碌梅脫脙ngx_http_upstream_cleanup路陆路篓
    cln->data = r;//脰赂脧貌脣霉脰赂碌脛脟毛脟贸陆谩鹿鹿脤氓隆拢
    u->cleanup = &cln->handler;

    /*
    http://www.pagefault.info/?p=251
    脠禄潞贸戮脥脢脟脮芒赂枚潞炉脢媒脳卯潞脣脨脛碌脛麓娄脌铆虏驴路脰拢卢脛脟戮脥脢脟赂霉戮脻upstream碌脛脌脿脨脥脌麓陆酶脨脨虏禄脥卢碌脛虏脵脳梅拢卢脮芒脌茂碌脛upstream戮脥脢脟脦脪脙脟脥篓鹿媒XXX_pass麓芦碌脻陆酶脌麓碌脛脰碌拢卢
    脮芒脌茂碌脛upstream脫脨驴脡脛脺脧脗脙忙录赂脰脰脟茅驴枚隆拢 
    Ngx_http_fastcgi_module.c (src\http\modules):    { ngx_string("fastcgi_pass"),
    Ngx_http_memcached_module.c (src\http\modules):    { ngx_string("memcached_pass"),
    Ngx_http_proxy_module.c (src\http\modules):    { ngx_string("proxy_pass"),
    Ngx_http_scgi_module.c (src\http\modules):    { ngx_string("scgi_pass"),
    Ngx_http_uwsgi_module.c (src\http\modules):    { ngx_string("uwsgi_pass"),
    Ngx_stream_proxy_module.c (src\stream):    { ngx_string("proxy_pass"),
    1 XXX_pass脰脨虏禄掳眉潞卢卤盲脕驴隆拢
    2 XXX_pass麓芦碌脻碌脛脰碌掳眉潞卢脕脣脪禄赂枚卤盲脕驴($驴陋脢录).脮芒脰脰脟茅驴枚脪虏戮脥脢脟脣碌upstream碌脛url脢脟露炉脤卢卤盲禄炉碌脛拢卢脪貌麓脣脨猫脪陋脙驴麓脦露录陆芒脦枚脪禄卤茅.
    露酶碌脷露镁脰脰脟茅驴枚脫脰路脰脦陋2脰脰拢卢脪禄脰脰脢脟脭脷陆酶脠毛upstream脰庐脟掳拢卢脪虏戮脥脢脟 upstream脛拢驴茅碌脛handler脰庐脰脨脪脩戮颅卤禄resolve碌脛碌脴脰路(脟毛驴麓ngx_http_XXX_eval潞炉脢媒)拢卢
    脪禄脰脰脢脟脙禄脫脨卤禄resolve拢卢麓脣脢卤戮脥脨猫脪陋upstream脛拢驴茅脌麓陆酶脨脨resolve隆拢陆脫脧脗脌麓碌脛麓煤脗毛戮脥脢脟麓娄脌铆脮芒虏驴路脰碌脛露芦脦梅隆拢
    */
    if (u->resolved == NULL) {//脡脧脫脦碌脛IP碌脴脰路脢脟路帽卤禄陆芒脦枚鹿媒拢卢ngx_http_fastcgi_handler碌梅脫脙ngx_http_fastcgi_eval禄谩陆芒脦枚隆拢 脦陋NULL脣碌脙梅脙禄脫脨陆芒脦枚鹿媒拢卢脪虏戮脥脢脟fastcgi_pas xxx脰脨碌脛xxx虏脦脢媒脙禄脫脨卤盲脕驴
        uscf = u->conf->upstream; //upstream赂鲁脰碌脭脷ngx_http_fastcgi_pass
    } else { //fastcgi_pass xxx碌脛xxx脰脨脫脨卤盲脕驴拢卢脣碌脙梅潞贸露脣路镁脦帽脝梅脢脟禄谩赂霉戮脻脟毛脟贸露炉脤卢卤盲禄炉碌脛拢卢虏脦驴录ngx_http_fastcgi_handler

#if (NGX_HTTP_SSL)
        u->ssl_name = u->resolved->host;
#endif
    //ngx_http_fastcgi_handler 禄谩碌梅脫脙 ngx_http_fastcgi_eval潞炉脢媒拢卢陆酶脨脨fastcgi_pass 潞贸脙忙碌脛URL碌脛录貌脦枚拢卢陆芒脦枚鲁枚unix脫貌拢卢禄貌脮脽socket.
        // 脠莽鹿没脪脩戮颅脢脟ip碌脴脰路赂帽脢陆脕脣拢卢戮脥虏禄脨猫脪陋脭脵陆酶脨脨陆芒脦枚
        if (u->resolved->sockaddr) {//脠莽鹿没碌脴脰路脪脩戮颅卤禄resolve鹿媒脕脣拢卢脦脪IP碌脴脰路拢卢麓脣脢卤麓麓陆篓round robin peer戮脥脨脨

            if (ngx_http_upstream_create_round_robin_peer(r, u->resolved)
                != NGX_OK)
            {
                ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }

            ngx_http_upstream_connect(r, u);//脕卢陆脫 

            return;
        }

        //脧脗脙忙驴陋脢录虏茅脮脪脫貌脙没拢卢脪貌脦陋fcgi_pass潞贸脙忙虏禄脢脟ip:port拢卢露酶脢脟url拢禄
        host = &u->resolved->host;//禄帽脠隆host脨脜脧垄隆拢 
        // 陆脫脧脗脌麓戮脥脪陋驴陋脢录虏茅脮脪脫貌脙没

        umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);

        uscfp = umcf->upstreams.elts;

        for (i = 0; i < umcf->upstreams.nelts; i++) {//卤茅脌煤脣霉脫脨碌脛脡脧脫脦脛拢驴茅拢卢赂霉戮脻脝盲host陆酶脨脨虏茅脮脪拢卢脮脪碌陆host,port脧脿脥卢碌脛隆拢

            uscf = uscfp[i];//脮脪脪禄赂枚IP脪禄脩霉碌脛脡脧脕梅脛拢驴茅

            if (uscf->host.len == host->len
                && ((uscf->port == 0 && u->resolved->no_port)
                     || uscf->port == u->resolved->port)
                && ngx_strncasecmp(uscf->host.data, host->data, host->len) == 0)
            {
                goto found;//脮芒赂枚host脮媒潞脙脧脿碌脠
            }
        }

        if (u->resolved->port == 0) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "no port in upstream \"%V\"", host);
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        //脙禄掳矛路篓脕脣拢卢url虏禄脭脷upstreams脢媒脳茅脌茂脙忙拢卢脪虏戮脥脢脟虏禄脢脟脦脪脙脟脜盲脰脙碌脛拢卢脛脟脙麓鲁玫脢录禄炉脫貌脙没陆芒脦枚脝梅
        temp.name = *host;
        
        // 鲁玫脢录禄炉脫貌脙没陆芒脦枚脝梅
        ctx = ngx_resolve_start(clcf->resolver, &temp);//陆酶脨脨脫貌脙没陆芒脦枚拢卢麓酶禄潞麓忙碌脛隆拢脡锚脟毛脧脿鹿脴碌脛陆谩鹿鹿拢卢路碌禄脴脡脧脧脗脦脛碌脴脰路隆拢
        if (ctx == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        if (ctx == NGX_NO_RESOLVER) {//脦脼路篓陆酶脨脨脫貌脙没陆芒脦枚隆拢 
            // 路碌禄脴NGX_NO_RESOLVER卤铆脢戮脦脼路篓陆酶脨脨脫貌脙没陆芒脦枚
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "no resolver defined to resolve %V", host);

            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
            return;
        }
        
        // 脡猫脰脙脨猫脪陋陆芒脦枚碌脛脫貌脙没碌脛脌脿脨脥脫毛脨脜脧垄
        ctx->name = *host;
        ctx->handler = ngx_http_upstream_resolve_handler;//脡猫脰脙脫貌脙没陆芒脦枚脥锚鲁脡潞贸碌脛禄脴碌梅潞炉脢媒隆拢
        ctx->data = r;
        ctx->timeout = clcf->resolver_timeout;

        u->resolved->ctx = ctx;

        //驴陋脢录脫貌脙没陆芒脦枚拢卢脙禄脫脨脥锚鲁脡脪虏禄谩路碌禄脴碌脛隆拢
        if (ngx_resolve_name(ctx) != NGX_OK) {
            u->resolved->ctx = NULL;
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        return;
        // 脫貌脙没禄鹿脙禄脫脨陆芒脦枚脥锚鲁脡拢卢脭貌脰卤陆脫路碌禄脴
    }

found:

    if (uscf == NULL) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      "no upstream configuration");
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

#if (NGX_HTTP_SSL)
    u->ssl_name = uscf->host;
#endif

    if (uscf->peer.init(r, uscf) != NGX_OK) {//ngx_http_upstream_init_round_XX_peer(ngx_http_upstream_init_round_robin_peer)
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    u->peer.start_time = ngx_current_msec;

    if (u->conf->next_upstream_tries
        && u->peer.tries > u->conf->next_upstream_tries)
    {
        u->peer.tries = u->conf->next_upstream_tries;
    }

    ngx_http_upstream_connect(r, u);//碌梅脫脙ngx_http_upstream_connect路陆路篓脧貌脡脧脫脦路镁脦帽脝梅路垄脝冒脕卢陆脫
}


#if (NGX_HTTP_CACHE)
/*ngx_http_upstream_init_request->ngx_http_upstream_cache 驴脥禄搂露脣禄帽脠隆禄潞麓忙 潞贸露脣脫娄麓冒禄脴脌麓脢媒戮脻潞贸脭脷ngx_http_upstream_send_response->ngx_http_file_cache_create
脰脨麓麓陆篓脕脵脢卤脦脛录镁拢卢脠禄潞贸脭脷ngx_event_pipe_write_chain_to_temp_file掳脩露脕脠隆碌脛潞贸露脣脢媒戮脻脨麓脠毛脕脵脢卤脦脛录镁拢卢脳卯潞贸脭脷
ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update脰脨掳脩脕脵脢卤脦脛录镁脛脷脠脻rename(脧脿碌卤脫脷mv)碌陆proxy_cache_path脰赂露篓
碌脛cache脛驴脗录脧脗脙忙
*/
static ngx_int_t
ngx_http_upstream_cache(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t               rc;
    ngx_http_cache_t       *c; 
    ngx_http_file_cache_t  *cache;

    c = r->cache;

    if (c == NULL) { /* 脠莽鹿没禄鹿脦麓赂酶碌卤脟掳脟毛脟贸路脰脜盲禄潞麓忙脧脿鹿脴陆谩鹿鹿脤氓( ngx_http_cache_t ) 脢卤拢卢麓麓陆篓麓脣脌脿脨脥脳脰露脦( r->cache ) 虏垄鲁玫脢录禄炉拢潞 */
        //脌媒脠莽proxy |fastcgi _cache_methods  POST脡猫脰脙脰碌禄潞麓忙POST脟毛脟贸拢卢碌芦脢脟驴脥禄搂露脣脟毛脟贸路陆路篓脢脟GET拢卢脭貌脰卤陆脫路碌禄脴
        if (!(r->method & u->conf->cache_methods)) {
            return NGX_DECLINED;
        }

        rc = ngx_http_upstream_cache_get(r, u, &cache);

        if (rc != NGX_OK) {
            return rc;
        }

        if (r->method & NGX_HTTP_HEAD) {
            u->method = ngx_http_core_get_method;
        }

        if (ngx_http_file_cache_new(r) != NGX_OK) {
            return NGX_ERROR;
        }

        if (u->create_key(r) != NGX_OK) {////陆芒脦枚xx_cache_key adfaxx 虏脦脢媒脰碌碌陆r->cache->keys
            return NGX_ERROR;
        }

        /* TODO: add keys */

        ngx_http_file_cache_create_key(r); /* 脡煤鲁脡 md5sum(key) 潞脥 crc32(key)虏垄录脝脣茫 `c->header_start` 脰碌 */

        if (r->cache->header_start + 256 >= u->conf->buffer_size) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "%V_buffer_size %uz is not enough for cache key, "
                          "it should be increased to at least %uz",
                          &u->conf->module, u->conf->buffer_size,
                          ngx_align(r->cache->header_start + 256, 1024));

            r->cache = NULL;
            return NGX_DECLINED;
        }

        u->cacheable = 1;/* 脛卢脠脧脣霉脫脨脟毛脟贸碌脛脧矛脫娄陆谩鹿没露录脢脟驴脡卤禄禄潞麓忙碌脛 */

        c = r->cache;

        
        /* 潞贸脨酶禄谩陆酶脨脨碌梅脮没 */
        c->body_start = u->conf->buffer_size; //xxx_buffer_size(fastcgi_buffer_size proxy_buffer_size memcached_buffer_size)
        c->min_uses = u->conf->cache_min_uses; //Proxy_cache_min_uses number 脛卢脠脧脦陋1拢卢碌卤驴脥禄搂露脣路垄脣脥脧脿脥卢脟毛脟贸麓茂碌陆鹿忙露篓麓脦脢媒潞贸拢卢nginx虏脜露脭脧矛脫娄脢媒戮脻陆酶脨脨禄潞麓忙拢禄
        c->file_cache = cache;

        /*
          赂霉戮脻脜盲脰脙脦脛录镁脰脨 ( fastcgi_cache_bypass ) 禄潞麓忙脠脝鹿媒脤玫录镁潞脥脟毛脟贸脨脜脧垄拢卢脜脨露脧脢脟路帽脫娄赂脙 
          录脤脨酶鲁垄脢脭脢鹿脫脙禄潞麓忙脢媒戮脻脧矛脫娄赂脙脟毛脟贸拢潞 
          */
        switch (ngx_http_test_predicates(r, u->conf->cache_bypass)) {//脜脨露脧脢脟路帽脫娄赂脙鲁氓禄潞麓忙脰脨脠隆拢卢禄鹿脢脟麓脫潞贸露脣路镁脦帽脝梅脠隆

        case NGX_ERROR:
            return NGX_ERROR;

        case NGX_DECLINED:
            u->cache_status = NGX_HTTP_CACHE_BYPASS;
            return NGX_DECLINED;

        default: /* NGX_OK */ //脫娄赂脙麓脫潞贸露脣路镁脦帽脝梅脰脴脨脗禄帽脠隆
            break;
        }

        c->lock = u->conf->cache_lock;
        c->lock_timeout = u->conf->cache_lock_timeout;
        c->lock_age = u->conf->cache_lock_age;

        u->cache_status = NGX_HTTP_CACHE_MISS;
    }

    rc = ngx_http_file_cache_open(r);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http upstream cache: %i", rc);

    switch (rc) {

    case NGX_HTTP_CACHE_UPDATING:
        
        if (u->conf->cache_use_stale & NGX_HTTP_UPSTREAM_FT_UPDATING) {
            //脠莽鹿没脡猫脰脙脕脣fastcgi_cache_use_stale updating拢卢卤铆脢戮脣碌脣盲脠禄赂脙禄潞麓忙脦脛录镁脢搂脨搂脕脣拢卢脪脩戮颅脫脨脝盲脣没驴脥禄搂露脣脟毛脟贸脭脷禄帽脠隆潞贸露脣脢媒戮脻拢卢碌芦脢脟脧脰脭脷禄鹿脙禄脫脨禄帽脠隆脥锚脮没拢卢
            //脮芒脢卤潞貌戮脥驴脡脪脭掳脩脪脭脟掳鹿媒脝脷碌脛禄潞麓忙路垄脣脥赂酶碌卤脟掳脟毛脟贸碌脛驴脥禄搂露脣
            u->cache_status = rc;
            rc = NGX_OK;

        } else {
            rc = NGX_HTTP_CACHE_STALE;
        }

        break;

    case NGX_OK: //禄潞麓忙脮媒鲁拢脙眉脰脨
        u->cache_status = NGX_HTTP_CACHE_HIT;
    }

    switch (rc) {

    case NGX_OK:

        rc = ngx_http_upstream_cache_send(r, u);

        if (rc != NGX_HTTP_UPSTREAM_INVALID_HEADER) {
            return rc;
        }

        break;

    case NGX_HTTP_CACHE_STALE: //卤铆脢戮禄潞麓忙鹿媒脝脷拢卢录没脡脧脙忙碌脛ngx_http_file_cache_open->ngx_http_file_cache_read

        c->valid_sec = 0;
        u->buffer.start = NULL;
        u->cache_status = NGX_HTTP_CACHE_EXPIRED;

        break;

    //脠莽鹿没路碌禄脴脮芒赂枚拢卢禄谩掳脩cached脰脙0拢卢路碌禄脴鲁枚脠楼潞贸脰禄脫脨麓脫潞贸露脣麓脫脨脗禄帽脠隆脢媒戮脻
    case NGX_DECLINED: //卤铆脢戮禄潞麓忙脦脛录镁麓忙脭脷拢卢禄帽脠隆禄潞麓忙脦脛录镁脰脨脟掳脙忙碌脛脥路虏驴虏驴路脰录矛虏茅脫脨脦脢脤芒拢卢脙禄脫脨脥篓鹿媒录矛虏茅隆拢禄貌脮脽禄潞麓忙脦脛录镁虏禄麓忙脭脷(碌脷脪禄麓脦脟毛脟贸赂脙uri禄貌脮脽脙禄脫脨麓茂碌陆驴陋脢录禄潞麓忙碌脛脟毛脟贸麓脦脢媒)

        if ((size_t) (u->buffer.end - u->buffer.start) < u->conf->buffer_size) {
            u->buffer.start = NULL;

        } else {
            u->buffer.pos = u->buffer.start + c->header_start;
            u->buffer.last = u->buffer.pos;
        }

        break;

    case NGX_HTTP_CACHE_SCARCE: //脙禄脫脨麓茂碌陆脟毛脟贸麓脦脢媒拢卢脰禄脫脨麓茂碌陆脟毛脟贸麓脦脢媒虏脜禄谩禄潞麓忙

        u->cacheable = 0;//脮芒脌茂脰脙0拢卢戮脥脢脟脣碌脠莽鹿没脜盲脰脙5麓脦驴陋脢录禄潞麓忙拢卢脭貌脟掳脙忙4麓脦露录虏禄禄谩禄潞麓忙拢卢掳脩cacheable脰脙0戮脥虏禄禄谩禄潞麓忙脕脣

        break;

    case NGX_AGAIN:

        return NGX_BUSY;

    case NGX_ERROR:

        return NGX_ERROR;

    default:

        /* cached NGX_HTTP_BAD_GATEWAY, NGX_HTTP_GATEWAY_TIME_OUT, etc. */

        u->cache_status = NGX_HTTP_CACHE_HIT;

        return rc;
    }

    r->cached = 0;

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_upstream_cache_get(ngx_http_request_t *r, ngx_http_upstream_t *u,
    ngx_http_file_cache_t **cache)
{
    ngx_str_t               *name, val;
    ngx_uint_t               i;
    ngx_http_file_cache_t  **caches;

    if (u->conf->cache_zone) { 
    //禄帽脠隆proxy_cache脡猫脰脙碌脛鹿虏脧铆脛脷麓忙驴茅脙没拢卢脰卤陆脫路碌禄脴u->conf->cache_zone->data(脮芒赂枚脢脟脭脷proxy_cache_path fastcgi_cache_path脡猫脰脙碌脛)拢卢脪貌麓脣卤脴脨毛脥卢脢卤脡猫脰脙
    //proxy_cache潞脥proxy_cache_path
        *cache = u->conf->cache_zone->data;
        ngx_log_debugall(r->connection->log, 0, "ngx http upstream cache get use keys_zone:%V", &u->conf->cache_zone->shm.name);
        return NGX_OK;
    }

    //脣碌脙梅proxy_cache xxx$ss脰脨麓酶脫脨虏脦脢媒
    if (ngx_http_complex_value(r, u->conf->cache_value, &val) != NGX_OK) {
        return NGX_ERROR;
    }

    if (val.len == 0
        || (val.len == 3 && ngx_strncmp(val.data, "off", 3) == 0))
    {
        return NGX_DECLINED;
    }

    caches = u->caches->elts; //脭脷proxy_cache_path脡猫脰脙碌脛zone_key脰脨虏茅脮脪脫脨脙禄脫脨露脭脫娄碌脛鹿虏脧铆脛脷麓忙脙没//keys_zone=fcgi:10m脰脨碌脛fcgi

    for (i = 0; i < u->caches->nelts; i++) {//脭脷u->caches脰脨虏茅脮脪proxy_cache禄貌脮脽fastcgi_cache xxx$ss陆芒脦枚鲁枚碌脛xxx$ss脳脰路没麓庐拢卢脢脟路帽脫脨脧脿脥卢碌脛
        name = &caches[i]->shm_zone->shm.name;

        if (name->len == val.len
            && ngx_strncmp(name->data, val.data, val.len) == 0)
        {
            *cache = caches[i];
            return NGX_OK;
        }
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                  "cache \"%V\" not found", &val);

    return NGX_ERROR;
}


static ngx_int_t
ngx_http_upstream_cache_send(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t          rc;
    ngx_http_cache_t  *c;

    r->cached = 1;
    c = r->cache;

    /*
    root@root:/var/yyz# cat cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f   路芒掳眉鹿媒鲁脤录没ngx_http_file_cache_set_header
     3hwhdBw
     KEY: /test2.php
     
     X-Powered-By: PHP/5.2.13
     Content-type: text/html
    //body_start戮脥脢脟脡脧脙忙脮芒脪禄露脦脛脷麓忙脛脷脠脻鲁陇露脠
    
     <Html> 
     <title>file update</title>
     <body> 
     <form method="post" action="" enctype="multipart/form-data">
     <input type="file" name="file" /> 
     <input type="submit" value="submit" /> 
     </form> 
     </body> 
     </html>
    
     脳垄脪芒碌脷脠媒脨脨脛脛脌茂脝盲脢碌脫脨8脳脰陆脷碌脛fastcgi卤铆脢戮脥路虏驴陆谩鹿鹿ngx_http_fastcgi_header_t拢卢脥篓鹿媒vi cache_xxx/f/27/46492fbf0d9d35d3753c66851e81627f驴脡脪脭驴麓鲁枚
    
     offset    0  1  2  3   4  5  6  7   8  9  a  b   c  d  e  f  0123456789abcdef
    00000000 <03>00 00 00  ab 53 83 56  ff ff ff ff  2b 02 82 56  ....芦S.V+..V
    00000010  64 42 77 17  00 00 91 00  ce 00 00 00  00 00 00 00  dBw...........
    00000020  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
    00000030  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
    00000040  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
    00000050  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
    00000060  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
    00000070  00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
    00000080  0a 4b 45 59  3a 20 2f 74  65 73 74 32  2e 70 68 70  .KEY: /test2.php
    00000090  0a 01 06 00  01 01 0c 04  00 58 2d 50  6f 77 65 72  .........X-Power
    000000a0  65 64 2d 42  79 3a 20 50  48 50 2f 35  2e 32 2e 31  ed-By: PHP/5.2.1
    000000b0  33 0d 0a 43  6f 6e 74 65  6e 74 2d 74  79 70 65 3a  3..Content-type:
    000000c0  20 74 65 78  74 2f 68 74  6d 6c 0d 0a  0d 0a 3c 48   text/html....<H
    000000d0  74 6d 6c 3e  20 0d 0a 3c  74 69 74 6c  65 3e 66 69  tml> ..<title>fi
    000000e0  6c 65 20 75  70 64 61 74  65 3c 2f 74  69 74 6c 65  le update</title
    000000f0  3e 0d 0a 3c  62 6f 64 79  3e 20 0d 0a  3c 66 6f 72  >..<body> ..<for
    
     offset    0  1  2  3   4  5  6  7   8  9  a  b   c  d  e  f  0123456789abcdef
    00000100  6d 20 6d 65  74 68 6f 64  3d 22 70 6f  73 74 22 20  m method="post"
    00000110  61 63 74 69  6f 6e 3d 22  22 20 65 6e  63 74 79 70  action="" enctyp
    00000120  65 3d 22 6d  75 6c 74 69  70 61 72 74  2f 66 6f 72  e="multipart/for
    00000130  6d 2d 64 61  74 61 22 3e  0d 0a 3c 69  6e 70 75 74  m-data">..<input
    00000140  20 74 79 70  65 3d 22 66  69 6c 65 22  20 6e 61 6d   type="file" nam
    00000150  65 3d 22 66  69 6c 65 22  20 2f 3e 20  0d 0a 3c 69  e="file" /> ..<i
    00000160  6e 70 75 74  20 74 31 31  31 31 31 31  31 31 31 31  nput t1111111111
    00000170  31 31 31 31  31 31 31 31  31 31 31 31  31 31 31 31  1111111111111111
    00000180  31 31 31 31  31 31 31 31  31 31 31 31  31 31 31 31  1111111111111111
    00000190  31 31 31 31  31 31 31 31  31 31 31 31  31 31 31 31  1111111111111111
    000001a0  31 31 31 31  31 31 31 31  31 31 31 31  31 79 70 65  1111111111111ype
    000001b0  3d 22 73 75  62 6d 69 74  22 20 76 61  6c 75 65 3d  ="submit" value=
    000001c0  22 73 75 62  6d 69 74 22  20 2f 3e 20  0d 0a 3c 2f  "submit" /> ..</
    000001d0  66 6f 72 6d  3e 20 0d 0a  3c 2f 62 6f  64 79 3e 20  form> ..</body>
    000001e0  0d 0a 3c 2f  68 74 6d 6c  3e 20 0d 0a               ..</html> ..
    
    
    header_start: [ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key脰脨碌脛KEY]["\n"] 脪虏戮脥脢脟脡脧脙忙碌脛碌脷脪禄脨脨潞脥碌脷露镁脨脨
    body_start: [ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key脰脨碌脛KEY]["\n"][header]脪虏戮脥脢脟脡脧脙忙碌脛碌脷脪禄碌陆碌脷脦氓脨脨脛脷脠脻
    脪貌麓脣:body_start = header_start + [header]虏驴路脰(脌媒脠莽fastcgi路碌禄脴碌脛脥路虏驴脨脨卤锚脢露虏驴路脰)
         */ 

    if (c->header_start == c->body_start) {
        r->http_version = NGX_HTTP_VERSION_9;
        return ngx_http_cache_send(r);
    }

    /* TODO: cache stack */
    //ngx_http_file_cache_open->ngx_http_file_cache_read脰脨c->buf->last脰赂脧貌脕脣露脕脠隆碌陆碌脛脢媒戮脻碌脛脛漏脦虏
    u->buffer = *c->buf;
//脰赂脧貌[ngx_http_file_cache_header_t]["\nKEY: "][fastcgi_cache_key脰脨碌脛KEY]["\n"][header]脰脨碌脛[header]驴陋脢录麓娄拢卢脪虏戮脥脢脟脟掳脙忙碌脛"X-Powered-By: PHP/5.2.13"
    u->buffer.pos += c->header_start; //脰赂脧貌潞贸露脣路碌禄脴鹿媒脌麓碌脛脢媒戮脻驴陋脢录麓娄(潞贸露脣路碌禄脴碌脛脭颅脢录脥路虏驴脨脨+脥酶脪鲁掳眉脤氓脢媒戮脻)

    ngx_memzero(&u->headers_in, sizeof(ngx_http_upstream_headers_in_t));
    u->headers_in.content_length_n = -1;
    u->headers_in.last_modified_time = -1;

    if (ngx_list_init(&u->headers_in.headers, r->pool, 8,
                      sizeof(ngx_table_elt_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    rc = u->process_header(r); //掳脩潞贸露脣路碌禄脴鹿媒脌麓碌脛脥路虏驴脨脨脨脜脧垄鲁枚脠楼fastcgi脥路虏驴8脳脰陆脷脪脭脥芒碌脛脢媒戮脻虏驴路脰碌陆

    if (rc == NGX_OK) {
        //掳脩潞贸露脣脥路虏驴脨脨脰脨碌脛脧脿鹿脴脢媒戮脻陆芒脦枚碌陆u->headers_in脰脨
        if (ngx_http_upstream_process_headers(r, u) != NGX_OK) {
            return NGX_DONE;
        }

        return ngx_http_cache_send(r);
    }

    if (rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    /* rc == NGX_HTTP_UPSTREAM_INVALID_HEADER */

    /* TODO: delete file */

    return rc;
}

#endif


static void
ngx_http_upstream_resolve_handler(ngx_resolver_ctx_t *ctx)
{
    ngx_connection_t              *c;
    ngx_http_request_t            *r;
    ngx_http_upstream_t           *u;
    ngx_http_upstream_resolved_t  *ur;

    r = ctx->data;
    c = r->connection;

    u = r->upstream;
    ur = u->resolved;

    ngx_http_set_log_request(c->log, r);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream resolve: \"%V?%V\"", &r->uri, &r->args);

    if (ctx->state) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "%V could not be resolved (%i: %s)",
                      &ctx->name, ctx->state,
                      ngx_resolver_strerror(ctx->state));

        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
        goto failed;
    }

    ur->naddrs = ctx->naddrs;
    ur->addrs = ctx->addrs;

#if (NGX_DEBUG)
    {
    u_char      text[NGX_SOCKADDR_STRLEN];
    ngx_str_t   addr;
    ngx_uint_t  i;

    addr.data = text;

    for (i = 0; i < ctx->naddrs; i++) {
        addr.len = ngx_sock_ntop(ur->addrs[i].sockaddr, ur->addrs[i].socklen,
                                 text, NGX_SOCKADDR_STRLEN, 0);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "name was resolved to %V", &addr);
    }
    }
#endif

    if (ngx_http_upstream_create_round_robin_peer(r, ur) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        goto failed;
    }

    ngx_resolve_name_done(ctx);
    ur->ctx = NULL;

    u->peer.start_time = ngx_current_msec;

    if (u->conf->next_upstream_tries
        && u->peer.tries > u->conf->next_upstream_tries)
    {
        u->peer.tries = u->conf->next_upstream_tries;
    }

    ngx_http_upstream_connect(r, u);

failed:

    ngx_http_run_posted_requests(c);
}

//驴脥禄搂露脣脢脗录镁麓娄脌铆handler脪禄掳茫(write(read)->handler)脪禄掳茫脦陋ngx_http_request_handler拢卢 潞脥潞贸露脣碌脛handler脪禄掳茫(write(read)->handler)脪禄掳茫脦陋ngx_http_upstream_handler拢卢 潞脥潞贸露脣碌脛
//潞脥潞贸露脣路镁脦帽脝梅碌脛露脕脨麓脢脗录镁麓楼路垄潞贸脳脽碌陆脮芒脌茂
static void
ngx_http_upstream_handler(ngx_event_t *ev)
{
    ngx_connection_t     *c;
    ngx_http_request_t   *r;
    ngx_http_upstream_t  *u;
    int writef = ev->write;
    
    c = ev->data;
    r = c->data; 

    u = r->upstream;
    c = r->connection;

    ngx_http_set_log_request(c->log, r);

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, c->log, 0, "http upstream request(ev->write:%d): \"%V?%V\"", writef, &r->uri, &r->args);

    //碌卤ev脦陋ngx_connection_t->write 脛卢脠脧write脦陋1拢禄碌卤ev脦陋ngx_connection_t->read 脛卢脠脧write脦陋0
    if (ev->write) { //脣碌脙梅脢脟c->write脢脗录镁
        u->write_event_handler(r, u);//ngx_http_upstream_send_request_handler

    } else { //脣碌脙梅脢脟c->read脢脗录镁
        u->read_event_handler(r, u); //ngx_http_upstream_process_header ngx_http_upstream_process_non_buffered_upstream
        
    }

    ngx_http_run_posted_requests(c);
}

/*
2025/04/24 05:31:47[             ngx_http_upstream_init,   654]  [debug] 15507#15507: *1 <   ngx_http_upstream_init,   653> epoll NGX_WRITE_EVENT(et) read add
2025/04/24 05:31:47[                ngx_epoll_add_event,  1400]  [debug] 15507#15507: *1 epoll modify read and write event: fd:11 op:3 ev:80002005
025/04/24 05:31:47[           ngx_epoll_process_events,  1624]  [debug] 15507#15507: begin to epoll_wait, epoll timer: 60000 
2025/04/24 05:31:47[           ngx_epoll_process_events,  1709]  [debug] 15507#15507: epoll: fd:11 epoll-out(ev:0004) d:B26A00E8
脢碌录脢脡脧脢脟脥篓鹿媒ngx_http_upstream_init脰脨碌脛mod epoll_ctl脤铆录脫露脕脨麓脢脗录镁麓楼路垄碌脛
*/
static void
ngx_http_upstream_rd_check_broken_connection(ngx_http_request_t *r)
{
    ngx_http_upstream_check_broken_connection(r, r->connection->read);
}

/*
2025/04/24 05:31:47[             ngx_http_upstream_init,   654]  [debug] 15507#15507: *1 <   ngx_http_upstream_init,   653> epoll NGX_WRITE_EVENT(et) read add
2025/04/24 05:31:47[                ngx_epoll_add_event,  1400]  [debug] 15507#15507: *1 epoll modify read and write event: fd:11 op:3 ev:80002005
025/04/24 05:31:47[           ngx_epoll_process_events,  1624]  [debug] 15507#15507: begin to epoll_wait, epoll timer: 60000 
2025/04/24 05:31:47[           ngx_epoll_process_events,  1709]  [debug] 15507#15507: epoll: fd:11 epoll-out(ev:0004) d:B26A00E8
脢碌录脢脡脧脢脟脥篓鹿媒ngx_http_upstream_init脰脨碌脛mod epoll_ctl脤铆录脫露脕脨麓脢脗录镁麓楼路垄碌脛
*/
static void
ngx_http_upstream_wr_check_broken_connection(ngx_http_request_t *r)
{
    ngx_http_upstream_check_broken_connection(r, r->connection->write);
}

/*
2025/04/24 05:31:47[             ngx_http_upstream_init,   654]  [debug] 15507#15507: *1 <   ngx_http_upstream_init,   653> epoll NGX_WRITE_EVENT(et) read add
2025/04/24 05:31:47[                ngx_epoll_add_event,  1400]  [debug] 15507#15507: *1 epoll modify read and write event: fd:11 op:3 ev:80002005
025/04/24 05:31:47[           ngx_epoll_process_events,  1624]  [debug] 15507#15507: begin to epoll_wait, epoll timer: 60000 
2025/04/24 05:31:47[           ngx_epoll_process_events,  1709]  [debug] 15507#15507: epoll: fd:11 epoll-out(ev:0004) d:B26A00E8
脢碌录脢脡脧脢脟脥篓鹿媒ngx_http_upstream_init脰脨碌脛mod epoll_ctl脤铆录脫露脕脨麓脢脗录镁麓楼路垄碌脛拢卢碌卤卤戮麓脦脩颅禄路脥脣禄脴碌陆ngx_worker_process_cycle ..->ngx_epoll_process_events
碌脛脢卤潞貌拢卢戮脥禄谩麓楼路垄epoll_out,麓脫露酶脰麓脨脨ngx_http_upstream_wr_check_broken_connection
*/
static void
ngx_http_upstream_check_broken_connection(ngx_http_request_t *r,
    ngx_event_t *ev)
{
    int                  n;
    char                 buf[1];
    ngx_err_t            err;
    ngx_int_t            event;
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ev->log, 0,
                   "http upstream check client, write event:%d, \"%V\"",
                   ev->write, &r->uri);

    c = r->connection;
    u = r->upstream;

    if (c->error) {
        if ((ngx_event_flags & NGX_USE_LEVEL_EVENT) && ev->active) {

            event = ev->write ? NGX_WRITE_EVENT : NGX_READ_EVENT;

            if (ngx_del_event(ev, event, 0) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }
        }

        if (!u->cacheable) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_CLIENT_CLOSED_REQUEST);
        }

        return;
    }

#if (NGX_HTTP_V2)
    if (r->stream) {
        return;
    }
#endif

#if (NGX_HAVE_KQUEUE)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT) {

        if (!ev->pending_eof) {
            return;
        }

        ev->eof = 1;
        c->error = 1;

        if (ev->kq_errno) {
            ev->error = 1;
        }

        if (!u->cacheable && u->peer.connection) {
            ngx_log_error(NGX_LOG_INFO, ev->log, ev->kq_errno,
                          "kevent() reported that client prematurely closed "
                          "connection, so upstream connection is closed too");
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_CLIENT_CLOSED_REQUEST);
            return;
        }

        ngx_log_error(NGX_LOG_INFO, ev->log, ev->kq_errno,
                      "kevent() reported that client prematurely closed "
                      "connection");

        if (u->peer.connection == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_CLIENT_CLOSED_REQUEST);
        }

        return;
    }

#endif

#if (NGX_HAVE_EPOLLRDHUP)

    if ((ngx_event_flags & NGX_USE_EPOLL_EVENT) && ev->pending_eof) {
        socklen_t  len;

        ev->eof = 1;
        c->error = 1;

        err = 0;
        len = sizeof(ngx_err_t);

        /*
         * BSDs and Linux return 0 and set a pending error in err
         * Solaris returns -1 and sets errno
         */

        if (getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (void *) &err, &len)
            == -1)
        {
            err = ngx_socket_errno;
        }

        if (err) {
            ev->error = 1;
        }

        if (!u->cacheable && u->peer.connection) {
            ngx_log_error(NGX_LOG_INFO, ev->log, err,
                        "epoll_wait() reported that client prematurely closed "
                        "connection, so upstream connection is closed too");
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_CLIENT_CLOSED_REQUEST);
            return;
        }

        ngx_log_error(NGX_LOG_INFO, ev->log, err,
                      "epoll_wait() reported that client prematurely closed "
                      "connection");

        if (u->peer.connection == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_CLIENT_CLOSED_REQUEST);
        }

        return;
    }

#endif

    n = recv(c->fd, buf, 1, MSG_PEEK);

    err = ngx_socket_errno;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, ev->log, err,
                   "http upstream recv() size: %d, fd:%d", n, c->fd);

    if (ev->write && (n >= 0 || err == NGX_EAGAIN)) {
        return;
    }

    if ((ngx_event_flags & NGX_USE_LEVEL_EVENT) && ev->active) {

        event = ev->write ? NGX_WRITE_EVENT : NGX_READ_EVENT;

        if (ngx_del_event(ev, event, 0) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    if (n > 0) {
        return;
    }

    if (n == -1) {
        if (err == NGX_EAGAIN) {
            return;
        }

        ev->error = 1;

    } else { /* n == 0 */
        err = 0;
    }

    ev->eof = 1;
    c->error = 1;

    if (!u->cacheable && u->peer.connection) {
        ngx_log_error(NGX_LOG_INFO, ev->log, err,
                      "client prematurely closed connection, "
                      "so upstream connection is closed too");
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_CLIENT_CLOSED_REQUEST);
        return;
    }

    ngx_log_error(NGX_LOG_INFO, ev->log, err,
                  "client prematurely closed connection");

    if (u->peer.connection == NULL) {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_CLIENT_CLOSED_REQUEST);
    }
}

/*
upstream禄煤脰脝脫毛脡脧脫脦路镁脦帽脝梅脢脟脥篓鹿媒TCP陆篓脕垄脕卢陆脫碌脛拢卢脰脷脣霉脰脺脰陋拢卢陆篓脕垄TCP脕卢陆脫脨猫脪陋脠媒麓脦脦脮脢脰拢卢露酶脠媒麓脦脦脮脢脰脧没潞脛碌脛脢卤录盲脢脟虏禄驴脡驴脴碌脛隆拢脦陋脕脣卤拢脰陇陆篓脕垄TCP
脕卢陆脫脮芒赂枚虏脵脳梅虏禄禄谩脳猫脠没陆酶鲁脤拢卢Nginx脢鹿脫脙脦脼脳猫脠没碌脛脤脳陆脫脳脰脌麓脕卢陆脫脡脧脫脦路镁脦帽脝梅隆拢碌梅脫脙碌脛ngx_http_upstream_connect路陆路篓戮脥脢脟脫脙脌麓脕卢陆脫脡脧脫脦路镁脦帽脝梅碌脛拢卢
脫脡脫脷脢鹿脫脙脕脣路脟脳猫脠没碌脛脤脳陆脫脳脰拢卢碌卤路陆路篓路碌禄脴脢卤脫毛脡脧脫脦脰庐录盲碌脛TCP脕卢陆脫脦麓卤脴禄谩鲁脡鹿娄陆篓脕垄拢卢驴脡脛脺禄鹿脨猫脪陋碌脠麓媒脡脧脫脦路镁脦帽脝梅路碌禄脴TCP碌脛SYN/ACK掳眉隆拢脪貌麓脣拢卢
ngx_http_upstream_connect路陆路篓脰梅脪陋赂潞脭冒路垄脝冒陆篓脕垄脕卢陆脫脮芒赂枚露炉脳梅拢卢脠莽鹿没脮芒赂枚路陆路篓脙禄脫脨脕垄驴脤路碌禄脴鲁脡鹿娄拢卢脛脟脙麓脨猫脪陋脭脷epoll脰脨录脿驴脴脮芒赂枚脤脳陆脫脳脰拢卢碌卤
脣眉鲁枚脧脰驴脡脨麓脢脗录镁脢卤拢卢戮脥脣碌脙梅脕卢陆脫脪脩戮颅陆篓脕垄鲁脡鹿娄脕脣隆拢

//碌梅脫脙socket,connect脕卢陆脫脪禄赂枚潞贸露脣碌脛peer,脠禄潞贸脡猫脰脙露脕脨麓脢脗录镁禄脴碌梅潞炉脢媒拢卢陆酶脠毛路垄脣脥脢媒戮脻碌脛ngx_http_upstream_send_request脌茂脙忙
//脮芒脌茂赂潞脭冒脕卢陆脫潞贸露脣路镁脦帽拢卢脠禄潞贸脡猫脰脙赂梅赂枚露脕脨麓脢脗录镁禄脴碌梅隆拢脳卯潞贸脠莽鹿没脕卢陆脫陆篓脕垄鲁脡鹿娄拢卢禄谩碌梅脫脙ngx_http_upstream_send_request陆酶脨脨脢媒戮脻路垄脣脥隆拢
*/
static void
ngx_http_upstream_connect(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    r->connection->log->action = "connecting to upstream";

    if (u->state && u->state->response_time) {
        u->state->response_time = ngx_current_msec - u->state->response_time;
    }

    u->state = ngx_array_push(r->upstream_states);
    if (u->state == NULL) {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_memzero(u->state, sizeof(ngx_http_upstream_state_t));

    u->state->response_time = ngx_current_msec;
    u->state->connect_time = (ngx_msec_t) -1;
    u->state->header_time = (ngx_msec_t) -1;
    //鲁玫脢录赂鲁脰碌录没ngx_http_upstream_connect->ngx_event_connect_peer(&u->peer);
    //驴脡脪脭驴麓鲁枚脫脨露脿脡脵赂枚驴脥禄搂露脣脕卢陆脫拢卢nginx戮脥脪陋脫毛php路镁脦帽脝梅陆篓脕垄露脿脡脵赂枚脕卢陆脫拢卢脦陋脢虏脙麓nginx潞脥php路镁脦帽脝梅虏禄脰禄陆篓脕垄脪禄赂枚脕卢陆脫脛脴????????????????
    rc = ngx_event_connect_peer(&u->peer); //陆篓脕垄脪禄赂枚TCP脤脳陆脫脳脰拢卢脥卢脢卤拢卢脮芒赂枚脤脳陆脫脳脰脨猫脪陋脡猫脰脙脦陋路脟脳猫脠没脛拢脢陆隆拢

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http upstream connect: %i", rc);

    if (rc == NGX_ERROR) {//
    //脠么 rc = NGX_ERROR拢卢卤铆脢戮路垄脝冒脕卢陆脫脢搂掳脺拢卢脭貌碌梅脫脙ngx_http_upstream_finalize_request 路陆路篓鹿脴卤脮脕卢陆脫脟毛脟贸拢卢虏垄 return 麓脫碌卤脟掳潞炉脢媒路碌禄脴拢禄
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    u->state->peer = u->peer.name;

    if (rc == NGX_BUSY) {
    //脠么 rc = NGX_BUSY拢卢卤铆脢戮碌卤脟掳脡脧脫脦路镁脦帽脝梅麓娄脫脷虏禄禄卯脭戮脳麓脤卢拢卢脭貌碌梅脫脙 ngx_http_upstream_next 路陆路篓赂霉戮脻麓芦脠毛碌脛虏脦脢媒鲁垄脢脭脰脴脨脗路垄脝冒脕卢陆脫脟毛脟贸拢卢虏垄 return 麓脫碌卤脟掳潞炉脢媒路碌禄脴拢禄
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "no live upstreams");
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_NOLIVE);
        return;
    }

    if (rc == NGX_DECLINED) {
    //脠么 rc = NGX_DECLINED拢卢卤铆脢戮碌卤脟掳脡脧脫脦路镁脦帽脝梅赂潞脭脴鹿媒脰脴拢卢脭貌碌梅脫脙 ngx_http_upstream_next 路陆路篓鲁垄脢脭脫毛脝盲脣没脡脧脫脦路镁脦帽脝梅陆篓脕垄脕卢陆脫拢卢虏垄 return 麓脫碌卤脟掳潞炉脢媒路碌禄脴拢禄
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    /* rc == NGX_OK || rc == NGX_AGAIN || rc == NGX_DONE */

    c = u->peer.connection;

    c->data = r;
/*
脡猫脰脙脡脧脫脦脕卢陆脫 ngx_connection_t 陆谩鹿鹿脤氓碌脛露脕脢脗录镁隆垄脨麓脢脗录镁碌脛禄脴碌梅路陆路篓 handler 露录脦陋 ngx_http_upstream_handler拢卢脡猫脰脙 ngx_http_upstream_t 
陆谩鹿鹿脤氓碌脛脨麓脢脗录镁 write_event_handler 碌脛禄脴碌梅脦陋 ngx_http_upstream_send_request_handler拢卢露脕脢脗录镁 read_event_handler 碌脛禄脴碌梅路陆路篓脦陋 
ngx_http_upstream_process_header拢禄
*/
    c->write->handler = ngx_http_upstream_handler; 
    c->read->handler = ngx_http_upstream_handler;

    //脮芒脪禄虏陆脰猫脢碌录脢脡脧戮枚露篓脕脣脧貌脡脧脫脦路镁脦帽脝梅路垄脣脥脟毛脟贸碌脛路陆路篓脢脟ngx_http_upstream_send_request_handler.
//脫脡脨麓脢脗录镁(脨麓脢媒戮脻禄貌脮脽驴脥禄搂露脣脕卢陆脫路碌禄脴鲁脡鹿娄)麓楼路垄c->write->handler = ngx_http_upstream_handler;脠禄潞贸脭脷ngx_http_upstream_handler脰脨脰麓脨脨ngx_http_upstream_send_request_handler
    u->write_event_handler = ngx_http_upstream_send_request_handler; //脠莽鹿没ngx_event_connect_peer路碌禄脴NGX_AGAIN脪虏脥篓鹿媒赂脙潞炉脢媒麓楼路垄脕卢陆脫鲁脡鹿娄
//脡猫脰脙upstream禄煤脰脝碌脛read_event_handler路陆路篓脦陋ngx_http_upstream_process_header拢卢脪虏戮脥脢脟脫脡ngx_http_upstream_process_header路陆路篓陆脫脢脮脡脧脫脦路镁脦帽脝梅碌脛脧矛脫娄隆拢
    u->read_event_handler = ngx_http_upstream_process_header;

    c->sendfile &= r->connection->sendfile;
    u->output.sendfile = c->sendfile;

    if (c->pool == NULL) {

        /* we need separate pool here to be able to cache SSL connections */

        c->pool = ngx_create_pool(128, r->connection->log);
        if (c->pool == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    c->log = r->connection->log;
    c->pool->log = c->log;
    c->read->log = c->log;
    c->write->log = c->log;

    /* init or reinit the ngx_output_chain() and ngx_chain_writer() contexts */

    u->writer.out = NULL;
    u->writer.last = &u->writer.out;
    u->writer.connection = c;
    u->writer.limit = 0;

    if (u->request_sent) {
        if (ngx_http_upstream_reinit(r, u) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    if (r->request_body
        && r->request_body->buf
        && r->request_body->temp_file
        && r == r->main) 
    //驴脥禄搂露脣掳眉脤氓麓忙脠毛脕脣脕脵脢卤脦脛录镁潞贸拢卢脭貌脢鹿脫脙r->request_body->bufs脕麓卤铆脰脨碌脛ngx_buf_t陆谩鹿鹿碌脛file_pos潞脥file_last脰赂脧貌拢卢脣霉脪脭r->request_body->buf驴脡脪脭录脤脨酶露脕脠隆掳眉脤氓
    {
        /*
         * the r->request_body->buf can be reused for one request only,
         * the subrequests should allocate their own temporary bufs
         */

        u->output.free = ngx_alloc_chain_link(r->pool);
        if (u->output.free == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        u->output.free->buf = r->request_body->buf;
        u->output.free->next = NULL;
        u->output.allocated = 1;

        r->request_body->buf->pos = r->request_body->buf->start;
        r->request_body->buf->last = r->request_body->buf->start;
        r->request_body->buf->tag = u->output.tag;
    }

    u->request_sent = 0;
    
    if (rc == NGX_AGAIN) { //脮芒脌茂碌脛露篓脢卤脝梅脭脷ngx_http_upstream_send_request禄谩脡戮鲁媒
            /*
            2025/04/24 02:54:29[             ngx_event_connect_peer,    32]  [debug] 14867#14867: *1 socket 12
2025/04/24 02:54:29[           ngx_epoll_add_connection,  1486]  [debug] 14867#14867: *1 epoll add connection: fd:12 ev:80002005
2025/04/24 02:54:29[             ngx_event_connect_peer,   125]  [debug] 14867#14867: *1 connect to 127.0.0.1:3666, fd:12 #2
2025/04/24 02:54:29[          ngx_http_upstream_connect,  1549]  [debug] 14867#14867: *1 http upstream connect: -2 //路碌禄脴NGX_AGAIN
2025/04/24 02:54:29[                ngx_event_add_timer,    88]  [debug] 14867#14867: *1 <ngx_http_upstream_connect,  1665>  event timer add: 12: 60000:1677807811 //脮芒脌茂脤铆录脫
2025/04/24 02:54:29[          ngx_http_finalize_request,  2526]  [debug] 14867#14867: *1 http finalize request: -4, "/test.php?" a:1, c:2
2025/04/24 02:54:29[             ngx_http_close_request,  3789]  [debug] 14867#14867: *1 http request count:2 blk:0
2025/04/24 02:54:29[           ngx_worker_process_cycle,  1110]  [debug] 14867#14867: worker(14867) cycle again
2025/04/24 02:54:29[           ngx_trylock_accept_mutex,   405]  [debug] 14867#14867: accept mutex locked
2025/04/24 02:54:29[           ngx_epoll_process_events,  1614]  [debug] 14867#14867: begin to epoll_wait, epoll timer: 60000 
2025/04/24 02:54:29[           ngx_epoll_process_events,  1699]  [debug] 14867#14867: epoll: fd:11 epoll-out(ev:0004) d:B27440E8
2025/04/24 02:54:29[           ngx_epoll_process_events,  1772]  [debug] 14867#14867: *1 post event AEB44068
2025/04/24 02:54:29[           ngx_epoll_process_events,  1699]  [debug] 14867#14867: epoll: fd:12 epoll-out(ev:0004) d:B2744158
2025/04/24 02:54:29[           ngx_epoll_process_events,  1772]  [debug] 14867#14867: *1 post event AEB44098
2025/04/24 02:54:29[      ngx_process_events_and_timers,   371]  [debug] 14867#14867: epoll_wait timer range(delta): 2
2025/04/24 02:54:29[           ngx_event_process_posted,    65]  [debug] 14867#14867: posted event AEB44068
2025/04/24 02:54:29[           ngx_event_process_posted,    67]  [debug] 14867#14867: *1 delete posted event AEB44068
2025/04/24 02:54:29[           ngx_http_request_handler,  2400]  [debug] 14867#14867: *1 http run request: "/test.php?"
2025/04/24 02:54:29[ngx_http_upstream_check_broken_connection,  1335]  [debug] 14867#14867: *1 http upstream check client, write event:1, "/test.php"
2025/04/24 02:54:29[ngx_http_upstream_check_broken_connection,  1458]  [debug] 14867#14867: *1 http upstream recv(): -1 (11: Resource temporarily unavailable)
2025/04/24 02:54:29[           ngx_event_process_posted,    65]  [debug] 14867#14867: posted event AEB44098
2025/04/24 02:54:29[           ngx_event_process_posted,    67]  [debug] 14867#14867: *1 delete posted event AEB44098
2025/04/24 02:54:29[          ngx_http_upstream_handler,  1295]  [debug] 14867#14867: *1 http upstream request: "/test.php?"
2025/04/24 02:54:29[ngx_http_upstream_send_request_handler,  2210]  [debug] 14867#14867: *1 http upstream send request handler
2025/04/24 02:54:29[     ngx_http_upstream_send_request,  2007]  [debug] 14867#14867: *1 http upstream send request
2025/04/24 02:54:29[ngx_http_upstream_send_request_body,  2095]  [debug] 14867#14867: *1 http upstream send request body
2025/04/24 02:54:29[                   ngx_chain_writer,   690]  [debug] 14867#14867: *1 chain writer buf fl:0 s:968
2025/04/24 02:54:29[                   ngx_chain_writer,   704]  [debug] 14867#14867: *1 chain writer in: 080EC838
2025/04/24 02:54:29[                         ngx_writev,   192]  [debug] 14867#14867: *1 writev: 968 of 968
2025/04/24 02:54:29[                   ngx_chain_writer,   740]  [debug] 14867#14867: *1 chain writer out: 00000000
2025/04/24 02:54:29[                ngx_event_del_timer,    39]  [debug] 14867#14867: *1 <ngx_http_upstream_send_request,  2052>  event timer del: 12: 1677807811//脮芒脌茂脡戮鲁媒
2025/04/24 02:54:29[                ngx_event_add_timer,    88]  [debug] 14867#14867: *1 <ngx_http_upstream_send_request,  2075>  event timer add: 12: 60000:1677807813
           */
        /*
          脠么 rc = NGX_AGAIN拢卢卤铆脢戮碌卤脟掳脪脩戮颅路垄脝冒脕卢陆脫拢卢碌芦脢脟脙禄脫脨脢脮碌陆脡脧脫脦路镁脦帽脝梅碌脛脠路脠脧脫娄麓冒卤篓脦脛拢卢录麓脡脧脫脦脕卢陆脫碌脛脨麓脢脗录镁虏禄驴脡脨麓拢卢脭貌脨猫碌梅脫脙 ngx_add_timer 
          路陆路篓陆芦脡脧脫脦脕卢陆脫碌脛脨麓脢脗录镁脤铆录脫碌陆露篓脢卤脝梅脰脨拢卢鹿脺脌铆鲁卢脢卤脠路脠脧脫娄麓冒拢禄
            
          脮芒脪禄虏陆麓娄脌铆路脟脳猫脠没碌脛脕卢陆脫脡脨脦麓鲁脡鹿娄陆篓脕垄脢卤碌脛露炉脳梅隆拢脢碌录脢脡脧拢卢脭脷ngx_event_connect_peer脰脨拢卢脤脳陆脫脳脰脪脩戮颅录脫脠毛碌陆epoll脰脨录脿驴脴脕脣拢卢脪貌麓脣拢卢
          脮芒脪禄虏陆陆芦碌梅脫脙ngx_add_timer路陆路篓掳脩脨麓脢脗录镁脤铆录脫碌陆露篓脢卤脝梅脰脨拢卢鲁卢脢卤脢卤录盲脦陋ngx_http_upstream_conf_t陆谩鹿鹿脤氓脰脨碌脛connect_timeout
          鲁脡脭卤拢卢脮芒脢脟脭脷脡猫脰脙陆篓脕垄TCP脕卢陆脫碌脛鲁卢脢卤脢卤录盲隆拢
          */ //脮芒脌茂碌脛露篓脢卤脝梅脭脷ngx_http_upstream_send_request禄谩脡戮鲁媒
        ngx_add_timer(c->write, u->conf->connect_timeout, NGX_FUNC_LINE);
        return; //麓贸虏驴路脰脟茅驴枚脥篓鹿媒脮芒脌茂路碌禄脴拢卢脠禄潞贸脥篓鹿媒ngx_http_upstream_send_request_handler脌麓脰麓脨脨epoll write脢脗录镁
    }

    
//脠么 rc = NGX_OK拢卢卤铆脢戮鲁脡鹿娄陆篓脕垄脕卢陆脫拢卢脭貌碌梅脫脙 ngx_http_upsream_send_request 路陆路篓脧貌脡脧脫脦路镁脦帽脝梅路垄脣脥脟毛脟贸拢禄
#if (NGX_HTTP_SSL)

    if (u->ssl && c->ssl == NULL) {
        ngx_http_upstream_ssl_init_connection(r, u, c);
        return;
    }

#endif

    //脠莽麓么脪脩戮颅鲁脡鹿娄陆篓脕垄脕卢陆脫拢卢脭貌碌梅脫脙ngx_http_upstream_send_request路陆路篓脧貌脡脧脫脦路镁脦帽脝梅路垄脣脥脟毛脟贸
    ngx_http_upstream_send_request(r, u, 1);
}


#if (NGX_HTTP_SSL)

static void
ngx_http_upstream_ssl_init_connection(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_connection_t *c)
{
    int                        tcp_nodelay;
    ngx_int_t                  rc;
    ngx_http_core_loc_conf_t  *clcf;

    if (ngx_http_upstream_test_connect(c) != NGX_OK) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    if (ngx_ssl_create_connection(u->conf->ssl, c,
                                  NGX_SSL_BUFFER|NGX_SSL_CLIENT)
        != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    c->sendfile = 0;
    u->output.sendfile = 0;

    if (u->conf->ssl_server_name || u->conf->ssl_verify) {
        if (ngx_http_upstream_ssl_name(r, u, c) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }
    }

    if (u->conf->ssl_session_reuse) {
        if (u->peer.set_session(&u->peer, u->peer.data) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        /* abbreviated SSL handshake may interact badly with Nagle */

        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        if (clcf->tcp_nodelay && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            tcp_nodelay = 1;

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                           (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }
    }

    r->connection->log->action = "SSL handshaking to upstream";

    rc = ngx_ssl_handshake(c);

    if (rc == NGX_AGAIN) {

        if (!c->write->timer_set) {
            ngx_add_timer(c->write, u->conf->connect_timeout, NGX_FUNC_LINE);
        }

        c->ssl->handler = ngx_http_upstream_ssl_handshake;
        return;
    }

    ngx_http_upstream_ssl_handshake(c);
}


static void
ngx_http_upstream_ssl_handshake(ngx_connection_t *c)
{
    long                  rc;
    ngx_http_request_t   *r;
    ngx_http_upstream_t  *u;

    r = c->data;
    u = r->upstream;

    ngx_http_set_log_request(c->log, r);

    if (c->ssl->handshaked) {

        if (u->conf->ssl_verify) {
            rc = SSL_get_verify_result(c->ssl->connection);

            if (rc != X509_V_OK) {
                ngx_log_error(NGX_LOG_ERR, c->log, 0,
                              "upstream SSL certificate verify error: (%l:%s)",
                              rc, X509_verify_cert_error_string(rc));
                goto failed;
            }

            if (ngx_ssl_check_host(c, &u->ssl_name) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, c->log, 0,
                              "upstream SSL certificate does not match \"%V\"",
                              &u->ssl_name);
                goto failed;
            }
        }

        if (u->conf->ssl_session_reuse) {
            u->peer.save_session(&u->peer, u->peer.data);
        }

        c->write->handler = ngx_http_upstream_handler;
        c->read->handler = ngx_http_upstream_handler;

        c = r->connection;

        ngx_http_upstream_send_request(r, u, 1);

        ngx_http_run_posted_requests(c);
        return;
    }

failed:

    c = r->connection;

    ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);

    ngx_http_run_posted_requests(c);
}


static ngx_int_t
ngx_http_upstream_ssl_name(ngx_http_request_t *r, ngx_http_upstream_t *u,
    ngx_connection_t *c)
{
    u_char     *p, *last;
    ngx_str_t   name;

    if (u->conf->ssl_name) {
        if (ngx_http_complex_value(r, u->conf->ssl_name, &name) != NGX_OK) {
            return NGX_ERROR;
        }

    } else {
        name = u->ssl_name;
    }

    if (name.len == 0) {
        goto done;
    }

    /*
     * ssl name here may contain port, notably if derived from $proxy_host
     * or $http_host; we have to strip it
     */

    p = name.data;
    last = name.data + name.len;

    if (*p == '[') {
        p = ngx_strlchr(p, last, ']');

        if (p == NULL) {
            p = name.data;
        }
    }

    p = ngx_strlchr(p, last, ':');

    if (p != NULL) {
        name.len = p - name.data;
    }

    if (!u->conf->ssl_server_name) {
        goto done;
    }

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME

    /* as per RFC 6066, literal IPv4 and IPv6 addresses are not permitted */

    if (name.len == 0 || *name.data == '[') {
        goto done;
    }

    if (ngx_inet_addr(name.data, name.len) != INADDR_NONE) {
        goto done;
    }

    /*
     * SSL_set_tlsext_host_name() needs a null-terminated string,
     * hence we explicitly null-terminate name here
     */

    p = ngx_pnalloc(r->pool, name.len + 1);
    if (p == NULL) {
        return NGX_ERROR;
    }

    (void) ngx_cpystrn(p, name.data, name.len + 1);

    name.data = p;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "upstream SSL server name: \"%s\"", name.data);

    if (SSL_set_tlsext_host_name(c->ssl->connection, name.data) == 0) {
        ngx_ssl_error(NGX_LOG_ERR, r->connection->log, 0,
                      "SSL_set_tlsext_host_name(\"%s\") failed", name.data);
        return NGX_ERROR;
    }

#endif

done:

    u->ssl_name = name;

    return NGX_OK;
}

#endif


static ngx_int_t
ngx_http_upstream_reinit(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    off_t         file_pos;
    ngx_chain_t  *cl;

    if (u->reinit_request(r) != NGX_OK) {
        return NGX_ERROR;
    }

    u->keepalive = 0;
    u->upgrade = 0;

    ngx_memzero(&u->headers_in, sizeof(ngx_http_upstream_headers_in_t));
    u->headers_in.content_length_n = -1;
    u->headers_in.last_modified_time = -1;

    if (ngx_list_init(&u->headers_in.headers, r->pool, 8,
                      sizeof(ngx_table_elt_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    /* reinit the request chain */

    file_pos = 0;

    for (cl = u->request_bufs; cl; cl = cl->next) {
        cl->buf->pos = cl->buf->start;

        /* there is at most one file */

        if (cl->buf->in_file) {
            cl->buf->file_pos = file_pos;
            file_pos = cl->buf->file_last;
        }
    }

    /* reinit the subrequest's ngx_output_chain() context */

    if (r->request_body && r->request_body->temp_file
        && r != r->main && u->output.buf)
    {
        u->output.free = ngx_alloc_chain_link(r->pool);
        if (u->output.free == NULL) {
            return NGX_ERROR;
        }

        u->output.free->buf = u->output.buf;
        u->output.free->next = NULL;

        u->output.buf->pos = u->output.buf->start;
        u->output.buf->last = u->output.buf->start;
    }

    u->output.buf = NULL;
    u->output.in = NULL;
    u->output.busy = NULL;

    /* reinit u->buffer */

    u->buffer.pos = u->buffer.start;

#if (NGX_HTTP_CACHE)

    if (r->cache) {
        u->buffer.pos += r->cache->header_start;
    }

#endif

    u->buffer.last = u->buffer.pos;

    return NGX_OK;
}


static void
ngx_http_upstream_send_request(ngx_http_request_t *r, ngx_http_upstream_t *u,
    ngx_uint_t do_write) //脧貌脡脧脫脦路镁脦帽脝梅路垄脣脥脟毛脟贸   碌卤脪禄麓脦路垄脣脥虏禄脥锚拢卢脥篓鹿媒ngx_http_upstream_send_request_handler脭脵麓脦麓楼路垄路垄脣脥
{
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = u->peer.connection; //脧貌脡脧脫脦路镁脦帽脝梅碌脛脕卢陆脫脨脜脧垄

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream send request");

    if (u->state->connect_time == (ngx_msec_t) -1) {
        u->state->connect_time = ngx_current_msec - u->state->response_time;
    }

    //脥篓鹿媒getsockopt虏芒脢脭脫毛脡脧脫脦路镁脦帽脝梅碌脛tcp脕卢陆脫脢脟路帽脪矛鲁拢
    if (!u->request_sent && ngx_http_upstream_test_connect(c) != NGX_OK) { //虏芒脢脭脕卢陆脫脢搂掳脺
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);//脠莽鹿没虏芒脢脭脢搂掳脺拢卢碌梅脫脙ngx_http_upstream_next潞炉脢媒拢卢脮芒赂枚潞炉脢媒驴脡脛脺脭脵麓脦碌梅脫脙peer.get碌梅脫脙卤冒碌脛脕卢陆脫隆拢
        return;
    }

    c->log->action = "sending request to upstream";

    rc = ngx_http_upstream_send_request_body(r, u, do_write);

    if (rc == NGX_ERROR) {
        /*  脠么路碌禄脴脰碌rc=NGX_ERROR拢卢卤铆脢戮碌卤脟掳脕卢陆脫脡脧鲁枚麓铆拢卢 陆芦麓铆脦贸脨脜脧垄麓芦碌脻赂酶ngx_http_upstream_next路陆路篓拢卢 赂脙路陆路篓赂霉戮脻麓铆脦贸脨脜脧垄戮枚露篓
        脢脟路帽脰脴脨脗脧貌脡脧脫脦脝盲脣没路镁脦帽脝梅路垄脝冒脕卢陆脫拢禄 虏垄return麓脫碌卤脟掳潞炉脢媒路碌禄脴拢禄 */
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        ngx_http_upstream_finalize_request(r, u, rc);
        return;
    }

    /* 
         脠么路碌禄脴脰碌rc = NGX_AGAIN拢卢卤铆脢戮脟毛脟贸脢媒戮脻虏垄脦麓脥锚脠芦路垄脣脥拢卢 录麓脫脨脢拢脫脿碌脛脟毛脟贸脢媒戮脻卤拢麓忙脭脷output脰脨拢卢碌芦麓脣脢卤拢卢脨麓脢脗录镁脪脩戮颅虏禄驴脡脨麓拢卢 
         脭貌碌梅脫脙ngx_add_timer路陆路篓掳脩碌卤脟掳脕卢陆脫脡脧碌脛脨麓脢脗录镁脤铆录脫碌陆露篓脢卤脝梅禄煤脰脝拢卢 虏垄碌梅脫脙ngx_handle_write_event路陆路篓陆芦脨麓脢脗录镁脳垄虏谩碌陆epoll脢脗录镁禄煤脰脝脰脨拢禄 
     */ //脥篓鹿媒ngx_http_upstream_read_request_handler陆酶脨脨脭脵麓脦epoll write
    if (rc == NGX_AGAIN) {//脨颅脪茅脮禄禄潞鲁氓脟酶脪脩脗煤拢卢脨猫脪陋碌脠麓媒路垄脣脥脢媒戮脻鲁枚脠楼潞贸鲁枚路垄epoll驴脡脨麓拢卢麓脫露酶录脤脨酶write
        if (!c->write->ready) {  
        //脮芒脌茂录脫露篓脢卤脝梅碌脛脭颅脪貌脢脟拢卢脌媒脠莽脦脪掳脩脢媒戮脻脠脫碌陆脨颅脪茅脮禄脕脣拢卢虏垄脟脪脨颅脪茅脮禄脪脩戮颅脗煤脕脣拢卢碌芦脢脟露脭路陆戮脥脢脟虏禄陆脫脢脺脢媒戮脻拢卢脭矛鲁脡脢媒戮脻脪禄脰卤脭脷脨颅脪茅脮禄禄潞麓忙脰脨
        //脪貌麓脣脰禄脪陋脢媒戮脻路垄脣脥鲁枚脠楼拢卢戮脥禄谩麓楼路垄epoll录脤脨酶脨麓拢卢麓脫露酶脭脷脧脗脙忙脕陆脨脨脡戮鲁媒脨麓鲁卢脢卤露篓脢卤脝梅
            ngx_add_timer(c->write, u->conf->send_timeout, NGX_FUNC_LINE); 
            //脠莽鹿没鲁卢脢卤禄谩脰麓脨脨ngx_http_upstream_send_request_handler拢卢脮芒脌茂脙忙露脭脨麓鲁卢脢卤陆酶脨脨麓娄脌铆

        } else if (c->write->timer_set) { //脌媒脠莽ngx_http_upstream_send_request_body路垄脣脥脕脣脠媒麓脦路碌禄脴NGX_AGAIN,脛脟脙麓碌脷露镁麓脦戮脥脨猫脪陋掳脩碌脷脪禄麓脦脡脧脙忙碌脛鲁卢脢卤露篓脢卤脝梅鹿脴脕脣拢卢卤铆脢戮路垄脣脥脮媒鲁拢
            ngx_del_timer(c->write, NGX_FUNC_LINE);
        }

        //脭脷脕卢陆脫潞贸露脣路镁脦帽脝梅conncet脟掳拢卢脫脨脡猫脰脙ngx_add_conn拢卢脌茂脙忙脪脩戮颅陆芦fd脤铆录脫碌陆脕脣露脕脨麓脢脗录镁脰脨拢卢脪貌麓脣脮芒脌茂脢碌录脢脡脧脰禄脢脟录貌碌楼脰麓脨脨脧脗ngx_send_lowat
        if (ngx_handle_write_event(c->write, u->conf->send_lowat, NGX_FUNC_LINE) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        return;
    }

    /* rc == NGX_OK */ 
    //脧貌潞贸露脣碌脛脢媒戮脻路垄脣脥脥锚卤脧

    //碌卤路垄脥霉潞贸露脣路镁脦帽脝梅碌脛脢媒戮脻掳眉鹿媒麓贸拢卢脨猫脪陋路脰露脿麓脦路垄脣脥碌脛脢卤潞貌拢卢脭脷脡脧脙忙碌脛if (rc == NGX_AGAIN)脰脨禄谩脤铆录脫露篓脢卤脝梅脌麓麓楼路垄路垄脣脥拢卢脠莽鹿没脨颅脪茅脮禄脪禄脰卤虏禄路垄脣脥脢媒戮脻鲁枚脠楼
    //戮脥禄谩鲁卢脢卤拢卢脠莽鹿没脢媒戮脻脳卯脰脮脠芦虏驴路垄脣脥鲁枚脠楼脭貌脨猫脪陋脦陋脳卯潞贸脪禄麓脦time_write脤铆录脫脡戮鲁媒虏脵脳梅隆拢

    //脠莽鹿没路垄脥霉潞贸露脣碌脛脢媒戮脻鲁陇露脠潞贸脨隆拢卢脭貌脪禄掳茫虏禄禄谩脭脵脡脧脙脜脤铆录脫露篓脢卤脝梅拢卢脮芒脌茂碌脛timer_set驴脧露篓脦陋0拢卢脣霉脪脭脠莽鹿没掳脦碌么潞贸露脣脥酶脧脽拢卢脥篓鹿媒ngx_http_upstream_test_connect
    //脢脟脜脨露脧虏禄鲁枚潞贸露脣路镁脦帽脝梅碌么脧脽碌脛拢卢脡脧脙忙碌脛ngx_http_upstream_send_request_body禄鹿脢脟禄谩路碌禄脴鲁脡鹿娄碌脛拢卢脣霉脪脭脮芒脌茂脫脨赂枚bug
    if (c->write->timer_set) { //脮芒脌茂碌脛露篓脢卤脝梅脢脟ngx_http_upstream_connect脰脨connect路碌禄脴NGX_AGAIN碌脛脢卤潞貌脤铆录脫碌脛露篓脢卤脝梅
        /*
2025/04/24 02:54:29[             ngx_event_connect_peer,    32]  [debug] 14867#14867: *1 socket 12
2025/04/24 02:54:29[           ngx_epoll_add_connection,  1486]  [debug] 14867#14867: *1 epoll add connection: fd:12 ev:80002005
2025/04/24 02:54:29[             ngx_event_connect_peer,   125]  [debug] 14867#14867: *1 connect to 127.0.0.1:3666, fd:12 #2
2025/04/24 02:54:29[          ngx_http_upstream_connect,  1549]  [debug] 14867#14867: *1 http upstream connect: -2 //路碌禄脴NGX_AGAIN
2025/04/24 02:54:29[                ngx_event_add_timer,    88]  [debug] 14867#14867: *1 <ngx_http_upstream_connect,  1665>  event timer add: 12: 60000:1677807811 //脮芒脌茂脤铆录脫
2025/04/24 02:54:29[          ngx_http_finalize_request,  2526]  [debug] 14867#14867: *1 http finalize request: -4, "/test.php?" a:1, c:2
2025/04/24 02:54:29[             ngx_http_close_request,  3789]  [debug] 14867#14867: *1 http request count:2 blk:0
2025/04/24 02:54:29[           ngx_worker_process_cycle,  1110]  [debug] 14867#14867: worker(14867) cycle again
2025/04/24 02:54:29[           ngx_trylock_accept_mutex,   405]  [debug] 14867#14867: accept mutex locked
2025/04/24 02:54:29[           ngx_epoll_process_events,  1614]  [debug] 14867#14867: begin to epoll_wait, epoll timer: 60000 
2025/04/24 02:54:29[           ngx_epoll_process_events,  1699]  [debug] 14867#14867: epoll: fd:11 epoll-out(ev:0004) d:B27440E8
2025/04/24 02:54:29[           ngx_epoll_process_events,  1772]  [debug] 14867#14867: *1 post event AEB44068
2025/04/24 02:54:29[           ngx_epoll_process_events,  1699]  [debug] 14867#14867: epoll: fd:12 epoll-out(ev:0004) d:B2744158
2025/04/24 02:54:29[           ngx_epoll_process_events,  1772]  [debug] 14867#14867: *1 post event AEB44098
2025/04/24 02:54:29[      ngx_process_events_and_timers,   371]  [debug] 14867#14867: epoll_wait timer range(delta): 2
2025/04/24 02:54:29[           ngx_event_process_posted,    65]  [debug] 14867#14867: posted event AEB44068
2025/04/24 02:54:29[           ngx_event_process_posted,    67]  [debug] 14867#14867: *1 delete posted event AEB44068
2025/04/24 02:54:29[           ngx_http_request_handler,  2400]  [debug] 14867#14867: *1 http run request: "/test.php?"
2025/04/24 02:54:29[ngx_http_upstream_check_broken_connection,  1335]  [debug] 14867#14867: *1 http upstream check client, write event:1, "/test.php"
2025/04/24 02:54:29[ngx_http_upstream_check_broken_connection,  1458]  [debug] 14867#14867: *1 http upstream recv(): -1 (11: Resource temporarily unavailable)
2025/04/24 02:54:29[           ngx_event_process_posted,    65]  [debug] 14867#14867: posted event AEB44098
2025/04/24 02:54:29[           ngx_event_process_posted,    67]  [debug] 14867#14867: *1 delete posted event AEB44098
2025/04/24 02:54:29[          ngx_http_upstream_handler,  1295]  [debug] 14867#14867: *1 http upstream request: "/test.php?"
2025/04/24 02:54:29[ngx_http_upstream_send_request_handler,  2210]  [debug] 14867#14867: *1 http upstream send request handler
2025/04/24 02:54:29[     ngx_http_upstream_send_request,  2007]  [debug] 14867#14867: *1 http upstream send request
2025/04/24 02:54:29[ngx_http_upstream_send_request_body,  2095]  [debug] 14867#14867: *1 http upstream send request body
2025/04/24 02:54:29[                   ngx_chain_writer,   690]  [debug] 14867#14867: *1 chain writer buf fl:0 s:968
2025/04/24 02:54:29[                   ngx_chain_writer,   704]  [debug] 14867#14867: *1 chain writer in: 080EC838
2025/04/24 02:54:29[                         ngx_writev,   192]  [debug] 14867#14867: *1 writev: 968 of 968
2025/04/24 02:54:29[                   ngx_chain_writer,   740]  [debug] 14867#14867: *1 chain writer out: 00000000
2025/04/24 02:54:29[                ngx_event_del_timer,    39]  [debug] 14867#14867: *1 <ngx_http_upstream_send_request,  2052>  event timer del: 12: 1677807811//脮芒脌茂脡戮鲁媒
2025/04/24 02:54:29[                ngx_event_add_timer,    88]  [debug] 14867#14867: *1 <ngx_http_upstream_send_request,  2075>  event timer add: 12: 60000:1677807813
           */
        ngx_del_timer(c->write, NGX_FUNC_LINE);
    }

    /* 脠么路碌禄脴脰碌 rc = NGX_OK拢卢卤铆脢戮脪脩戮颅路垄脣脥脥锚脠芦虏驴脟毛脟贸脢媒戮脻拢卢 脳录卤赂陆脫脢脮脌麓脳脭脡脧脫脦路镁脦帽脝梅碌脛脧矛脫娄卤篓脦脛拢卢脭貌脰麓脨脨脪脭脧脗鲁脤脨貌拢禄  */ 
    if (c->tcp_nopush == NGX_TCP_NOPUSH_SET) {
        if (ngx_tcp_push(c->fd) == NGX_ERROR) {
            ngx_log_error(NGX_LOG_CRIT, c->log, ngx_socket_errno,
                          ngx_tcp_push_n " failed");
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        c->tcp_nopush = NGX_TCP_NOPUSH_UNSET;
    }

    u->write_event_handler = ngx_http_upstream_dummy_handler; //脢媒戮脻脪脩戮颅脭脷脟掳脙忙脠芦虏驴路垄脥霉潞贸露脣路镁脦帽脝梅脕脣拢卢脣霉脪脭虏禄脨猫脪陋脭脵脳枚脨麓麓娄脌铆

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "send out chain data to uppeer server OK");
    //脭脷脕卢陆脫潞贸露脣路镁脦帽脝梅conncet脟掳拢卢脫脨脡猫脰脙ngx_add_conn拢卢脌茂脙忙脪脩戮颅陆芦fd脤铆录脫碌陆脕脣露脕脨麓脢脗录镁脰脨拢卢脪貌麓脣脮芒脌茂脢碌录脢脡脧脰禄脢脟录貌碌楼脰麓脨脨脧脗ngx_send_lowat
    if (ngx_handle_write_event(c->write, 0, NGX_FUNC_LINE) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    //脮芒禄脴脢媒戮脻脪脩戮颅路垄脣脥脕脣拢卢驴脡脪脭脳录卤赂陆脫脢脮脕脣拢卢脡猫脰脙陆脫脢脮潞贸露脣脫娄麓冒碌脛鲁卢脢卤露篓脢卤脝梅隆拢 
    /* 
        赂脙露篓脢卤脝梅脭脷脢脮碌陆潞贸露脣脫娄麓冒脢媒戮脻潞贸脡戮鲁媒拢卢录没ngx_event_pipe 
        if (rev->timer_set) {
            ngx_del_timer(rev, NGX_FUNC_LINE);
        }
     */
    ngx_add_timer(c->read, u->conf->read_timeout, NGX_FUNC_LINE); //脠莽鹿没鲁卢脢卤脭脷赂脙潞炉脢媒录矛虏芒ngx_http_upstream_process_header

    if (c->read->ready) {
        ngx_http_upstream_process_header(r, u);
        return;
    }
}

//脧貌潞贸露脣路垄脣脥脟毛脟贸碌脛碌梅脫脙鹿媒鲁脤ngx_http_upstream_send_request_body->ngx_output_chain->ngx_chain_writer
static ngx_int_t
ngx_http_upstream_send_request_body(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_uint_t do_write)
{
    int                        tcp_nodelay;
    ngx_int_t                  rc;
    ngx_chain_t               *out, *cl, *ln;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http upstream send request body");

    if (!r->request_body_no_buffering) {

       /* buffered request body */

       if (!u->request_sent) {
           u->request_sent = 1;
           out = u->request_bufs; //脠莽鹿没脢脟fastcgi脮芒脌茂脙忙脦陋脢碌录脢路垄脥霉潞贸露脣碌脛脢媒戮脻(掳眉脌篓fastcgi赂帽脢陆脥路虏驴+驴脥禄搂露脣掳眉脤氓碌脠)

       } else {
           out = NULL;
       }

       return ngx_output_chain(&u->output, out);
    }

    if (!u->request_sent) {
        u->request_sent = 1;
        out = u->request_bufs;

        if (r->request_body->bufs) {
            for (cl = out; cl->next; cl = out->next) { /* void */ }
            cl->next = r->request_body->bufs;
            r->request_body->bufs = NULL;
        }

        c = u->peer.connection;
        clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

        if (clcf->tcp_nodelay && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            tcp_nodelay = 1;

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                           (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                return NGX_ERROR;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }

        r->read_event_handler = ngx_http_upstream_read_request_handler;

    } else {
        out = NULL;
    }

    for ( ;; ) {

        if (do_write) {
            rc = ngx_output_chain(&u->output, out);

            if (rc == NGX_ERROR) {
                return NGX_ERROR;
            }

            while (out) {
                ln = out;
                out = out->next;
                ngx_free_chain(r->pool, ln);
            }

            if (rc == NGX_OK && !r->reading_body) {
                break;
            }
        }

        if (r->reading_body) {
            /* read client request body */

            rc = ngx_http_read_unbuffered_request_body(r);

            if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
                return rc;
            }

            out = r->request_body->bufs;
            r->request_body->bufs = NULL;
        }

        /* stop if there is nothing to send */

        if (out == NULL) {
            rc = NGX_AGAIN;
            break;
        }

        do_write = 1;
    }

    if (!r->reading_body) {
        if (!u->store && !r->post_action && !u->conf->ignore_client_abort) {
            r->read_event_handler =
                                  ngx_http_upstream_rd_check_broken_connection;
        }
    }

    return rc;
}

//ngx_http_upstream_send_request_handler脫脙禄搂脧貌潞贸露脣路垄脣脥掳眉脤氓脢卤拢卢脪禄麓脦路垄脣脥脙禄脥锚脥锚鲁脡拢卢脭脵麓脦鲁枚路垄epoll write碌脛脢卤潞貌碌梅脫脙
static void
ngx_http_upstream_send_request_handler(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_connection_t  *c;

    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http upstream send request handler");

    //卤铆脢戮脧貌脡脧脫脦路镁脦帽脝梅路垄脣脥碌脛脟毛脟贸脪脩戮颅鲁卢脢卤
    if (c->write->timedout) { //赂脙露篓脢卤脝梅脭脷ngx_http_upstream_send_request脤铆录脫碌脛
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_TIMEOUT);
        return;
    }

#if (NGX_HTTP_SSL)

    if (u->ssl && c->ssl == NULL) {
        ngx_http_upstream_ssl_init_connection(r, u, c);
        return;
    }

#endif
    //卤铆脢戮脡脧脫脦路镁脦帽脝梅碌脛脧矛脫娄脨猫脪陋脰卤陆脫脳陋路垄赂酶驴脥禄搂露脣拢卢虏垄脟脪麓脣脢卤脪脩戮颅掳脩脧矛脫娄脥路路垄脣脥赂酶驴脥禄搂露脣脕脣
    if (u->header_sent) { //露录脪脩戮颅脢脮碌陆潞贸露脣碌脛脢媒戮脻虏垄脟脪路垄脣脥赂酶驴脥禄搂露脣盲炉脌脌脝梅脕脣拢卢脣碌脙梅虏禄禄谩脭脵脧毛潞贸露脣脨麓脢媒戮脻拢卢
        u->write_event_handler = ngx_http_upstream_dummy_handler;

        (void) ngx_handle_write_event(c->write, 0, NGX_FUNC_LINE);

        return;
    }

    ngx_http_upstream_send_request(r, u, 1);
}


static void
ngx_http_upstream_read_request_handler(ngx_http_request_t *r)
{
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

    c = r->connection;
    u = r->upstream;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http upstream read request handler");

    if (c->read->timedout) {
        c->timedout = 1;
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    ngx_http_upstream_send_request(r, u, 0);
}

//ngx_http_upstream_handler脰脨脰麓脨脨
/*
潞贸露脣路垄脣脥鹿媒脌麓碌脛脥路虏驴脨脨掳眉脤氓赂帽脢陆: 8脳脰陆脷fastcgi脥路虏驴脨脨+ 脢媒戮脻(脥路虏驴脨脨脨脜脧垄+ 驴脮脨脨 + 脢碌录脢脨猫脪陋路垄脣脥碌脛掳眉脤氓脛脷脠脻) + 脤卯鲁盲脳脰露脦
*/
static void
ngx_http_upstream_process_header(ngx_http_request_t *r, ngx_http_upstream_t *u)
{//露脕脠隆FCGI脥路虏驴脢媒戮脻拢卢禄貌脮脽proxy脥路虏驴脢媒戮脻隆拢ngx_http_upstream_send_request路垄脣脥脥锚脢媒戮脻潞贸拢卢
//禄谩碌梅脫脙脮芒脌茂拢卢禄貌脮脽脫脨驴脡脨麓脢脗录镁碌脛脢卤潞貌禄谩碌梅脫脙脮芒脌茂隆拢
//ngx_http_upstream_connect潞炉脢媒脕卢陆脫fastcgi潞贸拢卢禄谩脡猫脰脙脮芒赂枚禄脴碌梅潞炉脢媒脦陋fcgi脕卢陆脫碌脛驴脡露脕脢脗录镁禄脴碌梅隆拢
    ssize_t            n;
    ngx_int_t          rc;
    ngx_connection_t  *c;

    c = u->peer.connection;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process header, fd:%d, buffer_size:%uz", c->fd, u->conf->buffer_size);

    c->log->action = "reading response header from upstream";

    if (c->read->timedout) {//露脕鲁卢脢卤脕脣拢卢脗脰脩炉脧脗脪禄赂枚隆拢 ngx_event_expire_timers鲁卢脢卤潞贸脳脽碌陆脮芒脌茂
        //赂脙露篓脢卤脝梅脤铆录脫碌脴路陆脭脷ngx_http_upstream_send_request
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_TIMEOUT);
        return;
    }

    if (!u->request_sent && ngx_http_upstream_test_connect(c) != NGX_OK) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
        return;
    }

    if (u->buffer.start == NULL) { //路脰脜盲脪禄驴茅禄潞麓忙拢卢脫脙脌麓麓忙路脜陆脫脢脺禄脴脌麓碌脛脢媒戮脻隆拢
        u->buffer.start = ngx_palloc(r->pool, u->conf->buffer_size); 
        //脥路虏驴脨脨虏驴路脰(脪虏戮脥脢脟碌脷脪禄赂枚fastcgi data卤锚脢露脨脜脧垄拢卢脌茂脙忙脪虏禄谩脨炉麓酶脪禄虏驴路脰脥酶脪鲁脢媒戮脻)碌脛fastcgi卤锚脢露脨脜脧垄驴陋卤脵碌脛驴脮录盲脫脙buffer_size脜盲脰脙脰赂露篓
        if (u->buffer.start == NULL) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

        u->buffer.pos = u->buffer.start;
        u->buffer.last = u->buffer.start;
        u->buffer.end = u->buffer.start + u->conf->buffer_size;
        u->buffer.temporary = 1;

        u->buffer.tag = u->output.tag;

        //鲁玫脢录禄炉headers_in麓忙路脜脥路虏驴脨脜脧垄拢卢潞贸露脣FCGI,proxy陆芒脦枚潞贸碌脛HTTP脥路虏驴陆芦路脜脠毛脮芒脌茂
        if (ngx_list_init(&u->headers_in.headers, r->pool, 8,
                          sizeof(ngx_table_elt_t))
            != NGX_OK)
        {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return;
        }

#if (NGX_HTTP_CACHE)
        /*
        pVpVZ"
        KEY: /test.php

        //脧脗脙忙脢脟潞贸露脣脢碌录脢路碌禄脴碌脛脛脷脠脻拢卢脡脧脙忙碌脛脢脟脭陇脕么碌脛脥路虏驴
        IX-Powered-By: PHP/5.2.13
        Content-type: text/html

        <Html> 
        <Head> 
        <title>Your page Subject and domain name</title>
          */
        if (r->cache) { //脳垄脪芒脮芒脌茂脤酶鹿媒脕脣脭陇脕么碌脛脥路虏驴脛脷麓忙拢卢脫脙脫脷麓忙麓垄cache脨麓脠毛脦脛录镁脢卤潞貌碌脛脥路虏驴虏驴路脰拢卢录没
            u->buffer.pos += r->cache->header_start;
            u->buffer.last = u->buffer.pos;
        }
#endif
    }

    for ( ;; ) {
        //recv 脦陋 ngx_unix_recv拢卢露脕脠隆脢媒戮脻路脜脭脷u->buffer.last碌脛脦禄脰脙拢卢路碌禄脴露脕碌陆碌脛麓贸脨隆隆拢
        n = c->recv(c, u->buffer.last, u->buffer.end - u->buffer.last);

        if (n == NGX_AGAIN) { //脛脷潞脣禄潞鲁氓脟酶脪脩戮颅脙禄脢媒戮脻脕脣
#if 0 
            ngx_add_timer(rev, u->read_timeout);
#endif

            if (ngx_handle_read_event(c->read, 0, NGX_FUNC_LINE) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;
            }

            return;
        }

        if (n == 0) {
            ngx_log_error(NGX_LOG_ERR, c->log, 0,
                          "upstream prematurely closed connection");
        }

        if (n == NGX_ERROR || n == 0) {
            ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_ERROR);
            return;
        }

        u->buffer.last += n;

#if 0
        u->valid_header_in = 0;

        u->peer.cached = 0;
#endif
        //ngx_http_xxx_process_header ngx_http_proxy_process_header
        rc = u->process_header(r);//ngx_http_fastcgi_process_header碌脠拢卢陆酶脨脨脢媒戮脻麓娄脌铆拢卢卤脠脠莽潞贸露脣路碌禄脴碌脛脢媒戮脻脥路虏驴陆芒脦枚拢卢body露脕脠隆碌脠隆拢

        if (rc == NGX_AGAIN) {
            ngx_log_debugall(c->log, 0,  " ngx_http_upstream_process_header u->process_header() return NGX_AGAIN");

            if (u->buffer.last == u->buffer.end) { //路脰脜盲碌脛脫脙脌麓麓忙麓垄fastcgi STDOUT脥路虏驴脨脨掳眉脤氓碌脛buf脪脩戮颅脫脙脥锚脕脣脥路虏驴脨脨露录禄鹿脙禄脫脨陆芒脦枚脥锚鲁脡拢卢
                ngx_log_error(NGX_LOG_ERR, c->log, 0,
                              "upstream sent too big header");

                ngx_http_upstream_next(r, u,
                                       NGX_HTTP_UPSTREAM_FT_INVALID_HEADER);
                return;
            }
            
            continue;
        }

        break;
    }

    if (rc == NGX_HTTP_UPSTREAM_INVALID_HEADER) {
        ngx_http_upstream_next(r, u, NGX_HTTP_UPSTREAM_FT_INVALID_HEADER);
        return;
    }

    if (rc == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    /* rc == NGX_OK */

    u->state->header_time = ngx_current_msec - u->state->response_time;

    if (u->headers_in.status_n >= NGX_HTTP_SPECIAL_RESPONSE) {

        if (ngx_http_upstream_test_next(r, u) == NGX_OK) {
            return;
        }

        if (ngx_http_upstream_intercept_errors(r, u) == NGX_OK) {
            return;
        }
    }

    //碌陆脮芒脌茂拢卢FCGI碌脠赂帽脢陆碌脛脢媒戮脻脪脩戮颅陆芒脦枚脦陋卤锚脳录HTTP碌脛卤铆脢戮脨脦脢陆脕脣(鲁媒脕脣BODY)拢卢脣霉脪脭驴脡脪脭陆酶脨脨upstream碌脛process_headers隆拢
	//脡脧脙忙碌脛 u->process_header(r)脪脩戮颅陆酶脨脨FCGI碌脠赂帽脢陆碌脛陆芒脦枚脕脣隆拢脧脗脙忙陆芦脥路虏驴脢媒戮脻驴陆卤麓碌陆headers_out.headers脢媒脳茅脰脨隆拢
    if (ngx_http_upstream_process_headers(r, u) != NGX_OK) {
        return;
    }
    
    if (!r->subrequest_in_memory) {//脠莽鹿没脙禄脫脨脳脫脟毛脟贸脕脣拢卢脛脟戮脥脰卤陆脫路垄脣脥脧矛脫娄赂酶驴脥禄搂露脣掳脡隆拢
        //buffering路陆脢陆潞脥路脟buffering路陆脢陆脭脷潞炉脢媒ngx_http_upstream_send_response路脰虏忙
        ngx_http_upstream_send_response(r, u);//赂酶驴脥禄搂露脣路垄脣脥脧矛脫娄拢卢脌茂脙忙禄谩麓娄脌铆header,body路脰驴陋路垄脣脥碌脛脟茅驴枚碌脛
        return;
    }

    /* subrequest content in memory */
    //脳脫脟毛脟贸拢卢虏垄脟脪潞贸露脣脢媒戮脻脨猫脪陋卤拢麓忙碌陆脛脷麓忙脭脷

    
    //脳垄脪芒脧脗脙忙脰禄脢脟掳脩潞贸露脣脢媒戮脻麓忙碌陆buf脰脨拢卢碌芦脢脟脙禄脫脨路垄脣脥碌陆驴脥禄搂露脣拢卢脢碌录脢路垄脣脥脪禄掳茫脢脟脫脡ngx_http_finalize_request->ngx_http_set_write_handler脢碌脧脰
    
    if (u->input_filter == NULL) {
        u->input_filter_init = ngx_http_upstream_non_buffered_filter_init;
        u->input_filter = ngx_http_upstream_non_buffered_filter;
        u->input_filter_ctx = r;
    }

    if (u->input_filter_init(u->input_filter_ctx) == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    n = u->buffer.last - u->buffer.pos;

    if (n) {
        u->buffer.last = u->buffer.pos;

        u->state->response_length += n;

        if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }

    if (u->length == 0) {
        ngx_http_upstream_finalize_request(r, u, 0);
        return;
    }

    u->read_event_handler = ngx_http_upstream_process_body_in_memory;//脡猫脰脙body虏驴路脰碌脛露脕脢脗录镁禄脴碌梅隆拢

    ngx_http_upstream_process_body_in_memory(r, u);
}


static ngx_int_t
ngx_http_upstream_test_next(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_uint_t                 status;
    ngx_http_upstream_next_t  *un;

    status = u->headers_in.status_n;

    for (un = ngx_http_upstream_next_errors; un->status; un++) {

        if (status != un->status) {
            continue;
        }

        if (u->peer.tries > 1 && (u->conf->next_upstream & un->mask)) {
            ngx_http_upstream_next(r, u, un->mask);
            return NGX_OK;
        }

#if (NGX_HTTP_CACHE)

        if (u->cache_status == NGX_HTTP_CACHE_EXPIRED
            && (u->conf->cache_use_stale & un->mask))
        {
            ngx_int_t  rc;

            rc = u->reinit_request(r);

            if (rc == NGX_OK) {
                u->cache_status = NGX_HTTP_CACHE_STALE;
                rc = ngx_http_upstream_cache_send(r, u);
            }

            ngx_http_upstream_finalize_request(r, u, rc);
            return NGX_OK;
        }

#endif
    }

#if (NGX_HTTP_CACHE)

    if (status == NGX_HTTP_NOT_MODIFIED
        && u->cache_status == NGX_HTTP_CACHE_EXPIRED
        && u->conf->cache_revalidate)
    {
        time_t     now, valid;
        ngx_int_t  rc;

        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http upstream not modified");

        now = ngx_time();
        valid = r->cache->valid_sec;

        rc = u->reinit_request(r);

        if (rc != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u, rc);
            return NGX_OK;
        }

        u->cache_status = NGX_HTTP_CACHE_REVALIDATED;
        rc = ngx_http_upstream_cache_send(r, u);

        if (valid == 0) {
            valid = r->cache->valid_sec;
        }

        if (valid == 0) {
            valid = ngx_http_file_cache_valid(u->conf->cache_valid,
                                              u->headers_in.status_n);
            if (valid) {
                valid = now + valid;
            }
        }

        if (valid) {
            r->cache->valid_sec = valid;
            r->cache->date = now;

            ngx_http_file_cache_update_header(r);
        }

        ngx_http_upstream_finalize_request(r, u, rc);
        return NGX_OK;
    }

#endif

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_upstream_intercept_errors(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_int_t                  status;
    ngx_uint_t                 i;
    ngx_table_elt_t           *h;
    ngx_http_err_page_t       *err_page;
    ngx_http_core_loc_conf_t  *clcf;

    status = u->headers_in.status_n;

    if (status == NGX_HTTP_NOT_FOUND && u->conf->intercept_404) {
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_NOT_FOUND);
        return NGX_OK;
    }

    if (!u->conf->intercept_errors) {
        return NGX_DECLINED;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (clcf->error_pages == NULL) {
        return NGX_DECLINED;
    }

    err_page = clcf->error_pages->elts;
    for (i = 0; i < clcf->error_pages->nelts; i++) {

        if (err_page[i].status == status) {

            if (status == NGX_HTTP_UNAUTHORIZED
                && u->headers_in.www_authenticate)
            {
                h = ngx_list_push(&r->headers_out.headers);

                if (h == NULL) {
                    ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                    return NGX_OK;
                }

                *h = *u->headers_in.www_authenticate;

                r->headers_out.www_authenticate = h;
            }

#if (NGX_HTTP_CACHE)

            if (r->cache) {
                time_t  valid;

                valid = ngx_http_file_cache_valid(u->conf->cache_valid, status);

                if (valid) {
                    r->cache->valid_sec = ngx_time() + valid;
                    r->cache->error = status;
                }

                ngx_http_file_cache_free(r->cache, u->pipe->temp_file);
            }
#endif
            ngx_http_upstream_finalize_request(r, u, status);

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}

//录矛虏茅潞脥c->fd碌脛tcp脕卢陆脫脢脟路帽脫脨脪矛鲁拢
static ngx_int_t
ngx_http_upstream_test_connect(ngx_connection_t *c)
{
    int        err;
    socklen_t  len;

#if (NGX_HAVE_KQUEUE)

    if (ngx_event_flags & NGX_USE_KQUEUE_EVENT)  {
        if (c->write->pending_eof || c->read->pending_eof) {
            if (c->write->pending_eof) {
                err = c->write->kq_errno;

            } else {
                err = c->read->kq_errno;
            }

            c->log->action = "connecting to upstream";
            (void) ngx_connection_error(c, err,
                                    "kevent() reported that connect() failed");
            return NGX_ERROR;
        }

    } else
#endif
    {
        err = 0;
        len = sizeof(int);

        /*
         * BSDs and Linux return 0 and set a pending error in err
         * Solaris returns -1 and sets errno
         */

        if (getsockopt(c->fd, SOL_SOCKET, SO_ERROR, (void *) &err, &len)
            == -1)
        {
            err = ngx_socket_errno;
        }

        if (err) {
            c->log->action = "connecting to upstream";
            (void) ngx_connection_error(c, err, "connect() failed");
            return NGX_ERROR;
        }
    }

    return NGX_OK;
}

/*
陆芒脦枚脟毛脟贸碌脛脥路虏驴脳脰露脦隆拢脙驴脨脨HEADER禄脴碌梅脝盲copy_handler拢卢脠禄潞贸驴陆卤麓脪禄脧脗脳麓脤卢脗毛碌脠隆拢驴陆卤麓脥路虏驴脳脰露脦碌陆headers_out
*/ //ngx_http_upstream_process_header潞脥ngx_http_upstream_process_headers潞脺脧帽脜露拢卢潞炉脢媒脙没拢卢脳垄脪芒
static ngx_int_t //掳脩麓脫潞贸露脣路碌禄脴鹿媒脌麓碌脛脥路虏驴脨脨脨脜脧垄驴陆卤麓碌陆r->headers_out脰脨拢卢脪脭卤赂脥霉驴脥禄搂露脣路垄脣脥脫脙
ngx_http_upstream_process_headers(ngx_http_request_t *r, ngx_http_upstream_t *u) 
{
    ngx_str_t                       uri, args;
    ngx_uint_t                      i, flags;
    ngx_list_part_t                *part;
    ngx_table_elt_t                *h;
    ngx_http_upstream_header_t     *hh;
    ngx_http_upstream_main_conf_t  *umcf;

    umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);
    
    if (u->headers_in.x_accel_redirect
        && !(u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_REDIRECT))
    {//脠莽鹿没脥路虏驴脰脨脢鹿脫脙脕脣X-Accel-Redirect脤脴脨脭拢卢脪虏戮脥脢脟脧脗脭脴脦脛录镁碌脛脤脴脨脭拢卢脭貌脭脷脮芒脌茂陆酶脨脨脦脛录镁脧脗脭脴隆拢拢卢脰脴露篓脧貌隆拢
    /*nginx X-Accel-Redirect脢碌脧脰脦脛录镁脧脗脭脴脠篓脧脼驴脴脰脝 
    露脭脦脛录镁脧脗脭脴碌脛脠篓脧脼陆酶脨脨戮芦脠路驴脴脰脝脭脷潞脺露脿碌脴路陆露录脨猫脪陋拢卢脌媒脠莽脫脨鲁楼碌脛脧脗脭脴路镁脦帽隆垄脥酶脗莽脫虏脜脤隆垄赂枚脠脣脧脿虏谩隆垄路脌脰鹿卤戮脮戮脛脷脠脻卤禄脥芒脮戮碌脕脕麓碌脠
    虏陆脰猫0拢卢client脟毛脟贸http://downloaddomain.com/download/my.iso拢卢麓脣脟毛脟贸卤禄CGI鲁脤脨貌陆芒脦枚拢篓露脭脫脷 nginx脫娄赂脙脢脟fastcgi拢漏隆拢
    虏陆脰猫1拢卢CGI鲁脤脨貌赂霉戮脻路脙脦脢脮脽碌脛脡铆路脻潞脥脣霉脟毛脟贸碌脛脳脢脭麓脝盲脢脟路帽脫脨脧脗脭脴脠篓脧脼脌麓脜脨露篓脢脟路帽脫脨麓貌驴陋碌脛脠篓脧脼隆拢脠莽鹿没脫脨拢卢脛脟脙麓赂霉戮脻麓脣脟毛脟贸碌脙碌陆露脭脫娄脦脛录镁碌脛麓脜脜脤麓忙路脜脗路戮露拢卢脌媒脠莽脢脟 /var/data/my.iso隆拢
        脛脟脙麓鲁脤脨貌路碌禄脴脢卤脭脷HTTP header录脫脠毛X-Accel-Redirect: /protectfile/data/my.iso拢卢虏垄录脫脡脧head Content-Type:application/octet-stream隆拢
    虏陆脰猫2拢卢nginx碌脙碌陆cgi鲁脤脨貌碌脛禄脴脫娄潞贸路垄脧脰麓酶脫脨X-Accel-Redirect碌脛header拢卢脛脟脙麓赂霉戮脻脮芒赂枚脥路录脟脗录碌脛脗路戮露脨脜脧垄麓貌驴陋麓脜脜脤脦脛录镁隆拢
    虏陆脰猫3拢卢nginx掳脩麓貌驴陋脦脛录镁碌脛脛脷脠脻路碌禄脴赂酶client露脣隆拢
    脮芒脩霉脣霉脫脨碌脛脠篓脧脼录矛虏茅露录驴脡脪脭脭脷虏陆脰猫1脛脷脥锚鲁脡拢卢露酶脟脪cgi路碌禄脴麓酶X-Accel-Redirect碌脛脥路潞贸拢卢脝盲脰麓脨脨脪脩戮颅脰脮脰鹿拢卢脢拢脧脗碌脛麓芦脢盲脦脛录镁碌脛鹿陇脳梅脫脡nginx 脌麓陆脫鹿脺拢卢
        脥卢脢卤X-Accel-Redirect脥路碌脛脨脜脧垄卤禄nginx脡戮鲁媒拢卢脪虏脪镁虏脴脕脣脦脛录镁脢碌录脢麓忙麓垄脛驴脗录拢卢虏垄脟脪脫脡脫脷nginx脭脷麓貌驴陋戮虏脤卢脦脛录镁脡脧脢鹿脫脙脕脣 sendfile(2)拢卢脝盲IO脨搂脗脢路脟鲁拢赂脽隆拢
    */
        ngx_http_upstream_finalize_request(r, u, NGX_DECLINED);

        part = &u->headers_in.headers.part; //潞贸露脣路镁脦帽脝梅脫娄麓冒碌脛脥路虏驴脨脨脨脜脧垄脠芦虏驴脭脷赂脙headers脕麓卤铆脢媒脳茅脰脨
        h = part->elts;

        for (i = 0; /* void */; i++) {

            if (i >= part->nelts) { //headers脡脧脙忙碌脛脧脗脪禄赂枚脕麓卤铆
                if (part->next == NULL) {
                    break;
                }

                part = part->next;
                h = part->elts;
                i = 0;
            }

            hh = ngx_hash_find(&umcf->headers_in_hash, h[i].hash,
                               h[i].lowcase_key, h[i].key.len);  

            if (hh && hh->redirect) { 
            //脠莽鹿没潞贸露脣路镁脦帽脝梅脫脨路碌禄脴ngx_http_upstream_headers_in脰脨碌脛脥路虏驴脨脨脳脰露脦拢卢脠莽鹿没赂脙脢媒脳茅脰脨碌脛鲁脡脭卤redirect脦陋1拢卢脭貌脰麓脨脨鲁脡脭卤碌脛露脭脫娄碌脛copy_handler
                if (hh->copy_handler(r, &h[i], hh->conf) != NGX_OK) {
                    ngx_http_finalize_request(r,
                                              NGX_HTTP_INTERNAL_SERVER_ERROR);
                    return NGX_DONE;
                }
            }
        }

        uri = u->headers_in.x_accel_redirect->value; //脨猫脪陋脛脷虏驴脰脴露篓脧貌碌脛脨脗碌脛uri拢卢脥篓鹿媒潞贸脙忙碌脛ngx_http_internal_redirect麓脫脨脗脳脽13 phase陆脳露脦脕梅鲁脤

        if (uri.data[0] == '@') {
            ngx_http_named_location(r, &uri);

        } else {
            ngx_str_null(&args);
            flags = NGX_HTTP_LOG_UNSAFE;

            if (ngx_http_parse_unsafe_uri(r, &uri, &args, &flags) != NGX_OK) {
                ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
                return NGX_DONE;
            }

            if (r->method != NGX_HTTP_HEAD) {
                r->method = NGX_HTTP_GET;
            }

            ngx_http_internal_redirect(r, &uri, &args);//脢鹿脫脙脛脷虏驴脰脴露篓脧貌拢卢脟脡脙卯碌脛脧脗脭脴隆拢脌茂脙忙脫脰禄谩脳脽碌陆赂梅脰脰脟毛脟贸麓娄脌铆陆脳露脦隆拢
        }

        ngx_http_finalize_request(r, NGX_DONE);
        return NGX_DONE;
    }

    part = &u->headers_in.headers.part;
    h = part->elts;

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        if (ngx_hash_find(&u->conf->hide_headers_hash, h[i].hash,
                          h[i].lowcase_key, h[i].key.len)) //脮芒脨漏脥路虏驴虏禄脨猫脪陋路垄脣脥赂酶驴脥禄搂露脣拢卢脪镁虏脴
        {
            continue;
        }

        hh = ngx_hash_find(&umcf->headers_in_hash, h[i].hash,
                           h[i].lowcase_key, h[i].key.len);

        if (hh) {//脪禄赂枚赂枚驴陆卤麓碌陆脟毛脟贸碌脛headers_out脌茂脙忙
            if (hh->copy_handler(r, &h[i], hh->conf) != NGX_OK) { //麓脫u->headers_in.headers赂麓脰脝碌陆r->headers_out.headers脫脙脫脷路垄脣脥
                ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
                return NGX_DONE;
            }

            continue; 
        }

        //脠莽鹿没脙禄脫脨脳垄虏谩戮盲卤煤(脭脷ngx_http_upstream_headers_in脮脪虏禄碌陆赂脙鲁脡脭卤)拢卢驴陆卤麓潞贸露脣路镁脦帽脝梅路碌禄脴碌脛脪禄脨脨脪禄脨脨碌脛脥路虏驴脨脜脧垄(u->headers_in.headers脰脨碌脛脥路虏驴脨脨赂鲁脰碌赂酶r->headers_out.headers)
        if (ngx_http_upstream_copy_header_line(r, &h[i], 0) != NGX_OK) {
            ngx_http_upstream_finalize_request(r, u,
                                               NGX_HTTP_INTERNAL_SERVER_ERROR);
            return NGX_DONE;
        }
    }

    if (r->headers_out.server && r->headers_out.server->value.data == NULL) {
        r->headers_out.server->hash = 0;
    }

    if (r->headers_out.date && r->headers_out.date->value.data == NULL) {
        r->headers_out.date->hash = 0;
    }

    //驴陆卤麓脳麓脤卢脨脨拢卢脪貌脦陋脮芒赂枚虏禄脢脟麓忙脭脷headers_in脌茂脙忙碌脛隆拢
    r->headers_out.status = u->headers_in.status_n;
    r->headers_out.status_line = u->headers_in.status_line;

    r->headers_out.content_length_n = u->headers_in.content_length_n;

    r->disable_not_modified = !u->cacheable;

    if (u->conf->force_ranges) {
        r->allow_ranges = 1;
        r->single_range = 1;

#if (NGX_HTTP_CACHE)
        if (r->cached) {
            r->single_range = 0;
        }
#endif
    }

    u->length = -1;

    return NGX_OK;
}


static void
ngx_http_upstream_process_body_in_memory(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    size_t             size;
    ssize_t            n;
    ngx_buf_t         *b;
    ngx_event_t       *rev;
    ngx_connection_t  *c;

    c = u->peer.connection;
    rev = c->read;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process body on memory");

    if (rev->timedout) { //脭脷路垄脣脥脟毛脟贸碌陆潞贸露脣碌脛脢卤潞貌拢卢脦脪脙脟脨猫脪陋碌脠麓媒露脭路陆脫娄麓冒拢卢脪貌麓脣脡猫脰脙脕脣露脕鲁卢脢卤露篓脢卤脝梅拢卢录没ngx_http_upstream_send_request
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }

    b = &u->buffer;

    for ( ;; ) {

        size = b->end - b->last;

        if (size == 0) {
            ngx_log_error(NGX_LOG_ALERT, c->log, 0,
                          "upstream buffer is too small to read response");
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

        n = c->recv(c, b->last, size);

        if (n == NGX_AGAIN) {
            break;
        }

        if (n == 0 || n == NGX_ERROR) {
            ngx_http_upstream_finalize_request(r, u, n);
            return;
        }

        u->state->response_length += n;

        if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

        if (!rev->ready) {
            break;
        }
    }

    if (u->length == 0) {
        ngx_http_upstream_finalize_request(r, u, 0);
        return;
    }

    if (ngx_handle_read_event(rev, 0, NGX_FUNC_LINE) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (rev->active) {
        ngx_add_timer(rev, u->conf->read_timeout, NGX_FUNC_LINE);

    } else if (rev->timer_set) {
        ngx_del_timer(rev, NGX_FUNC_LINE);
    }
}

//路垄脣脥潞贸露脣路碌禄脴禄脴脌麓碌脛脢媒戮脻赂酶驴脥禄搂露脣隆拢脌茂脙忙禄谩麓娄脌铆header,body路脰驴陋路垄脣脥碌脛脟茅驴枚碌脛 
static void 
ngx_http_upstream_send_response(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    int                        tcp_nodelay;
    ssize_t                    n;
    ngx_int_t                  rc;
    ngx_event_pipe_t          *p;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;
    int flag;
    time_t  now, valid;

    rc = ngx_http_send_header(r);//脧脠路垄header拢卢脭脵路垄body //碌梅脫脙脙驴脪禄赂枚filter鹿媒脗脣拢卢麓娄脌铆脥路虏驴脢媒戮脻隆拢脳卯潞贸陆芦脢媒戮脻路垄脣脥赂酶驴脥禄搂露脣隆拢碌梅脫脙ngx_http_top_header_filter

    if (rc == NGX_ERROR || rc > NGX_OK || r->post_action) {
        ngx_http_upstream_finalize_request(r, u, rc);
        return;
    }

    u->header_sent = 1;//卤锚录脟脪脩戮颅路垄脣脥脕脣脥路虏驴脳脰露脦拢卢脰脕脡脵脢脟脪脩戮颅鹿脪脭脴鲁枚脠楼拢卢戮颅鹿媒脕脣filter脕脣隆拢

    if (u->upgrade) {
        ngx_http_upstream_upgrade(r, u);
        return;
    }

    c = r->connection;

    if (r->header_only) {//脠莽鹿没脰禄脨猫脪陋路垄脣脥脥路虏驴脢媒戮脻拢卢卤脠脠莽驴脥禄搂露脣脫脙curl -I 路脙脦脢碌脛隆拢路碌禄脴204脳麓脤卢脗毛录麓驴脡隆拢

        if (!u->buffering) { //脜盲脰脙虏禄脨猫脪陋禄潞麓忙掳眉脤氓拢卢禄貌脮脽潞贸露脣脪陋脟贸虏禄脜盲脰脙禄潞麓忙掳眉脤氓拢卢脰卤陆脫陆谩脢酶
            ngx_http_upstream_finalize_request(r, u, rc);
            return;
        }

        if (!u->cacheable && !u->store) { //脠莽鹿没露篓脪氓脕脣#if (NGX_HTTP_CACHE)脭貌驴脡脛脺脰脙1
            ngx_http_upstream_finalize_request(r, u, rc);
            return;
        }

        u->pipe->downstream_error = 1; //脙眉脙没驴脥禄搂露脣脰禄脟毛脟贸脥路虏驴脨脨拢卢碌芦脢脟脡脧脫脦脠赂脜盲脰脙禄貌脮脽脪陋脟贸禄潞麓忙禄貌脮脽麓忙麓垄掳眉脤氓
    }

    if (r->request_body && r->request_body->temp_file) { //驴脥禄搂露脣路垄脣脥鹿媒脌麓碌脛掳眉脤氓麓忙麓垄脭脷脕脵脢卤脦脛录镁脰脨拢卢脭貌脨猫脪陋掳脩麓忙麓垄脕脵脢卤脦脛录镁脡戮鲁媒
        ngx_pool_run_cleanup_file(r->pool, r->request_body->temp_file->file.fd); 
        //脰庐脟掳脕脵脢卤脦脛录镁脛脷脠脻脪脩戮颅虏禄脨猫脪陋脕脣拢卢脪貌脦陋脭脷ngx_http_fastcgi_create_request(ngx_http_xxx_create_request)脰脨脪脩戮颅掳脩脕脵脢卤脦脛录镁脰脨碌脛脛脷脠脻
        //赂鲁脰碌赂酶u->request_bufs虏垄脥篓鹿媒路垄脣脥碌陆脕脣潞贸露脣路镁脦帽脝梅拢卢脧脰脭脷脨猫脪陋路垄脥霉驴脥禄搂露脣碌脛脛脷脠脻脦陋脡脧脫脦脫娄麓冒禄脴脌麓碌脛掳眉脤氓拢卢脪貌麓脣麓脣脕脵脢卤脦脛录镁脛脷脠脻脪脩戮颅脙禄脫脙脕脣
        r->request_body->temp_file->file.fd = NGX_INVALID_FILE;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    /*
     脠莽鹿没驴陋脝么禄潞鲁氓拢卢脛脟脙麓Nginx陆芦戮隆驴脡脛脺露脿碌脴露脕脠隆潞贸露脣路镁脦帽脝梅碌脛脧矛脫娄脢媒戮脻拢卢碌脠麓茂碌陆脪禄露篓脕驴拢篓卤脠脠莽buffer脗煤拢漏脭脵麓芦脣脥赂酶脳卯脰脮驴脥禄搂露脣隆拢脠莽鹿没鹿脴卤脮拢卢
     脛脟脙麓Nginx露脭脢媒戮脻碌脛脰脨脳陋戮脥脢脟脪禄赂枚脥卢虏陆碌脛鹿媒鲁脤拢卢录麓麓脫潞贸露脣路镁脦帽脝梅陆脫脢脮碌陆脧矛脫娄脢媒戮脻戮脥脕垄录麓陆芦脝盲路垄脣脥赂酶驴脥禄搂露脣隆拢
     */
    flag = u->buffering;
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "ngx_http_upstream_send_response, buffering flag:%d", flag);
    if (!u->buffering) { 
    //buffering脦陋1拢卢卤铆脢戮脡脧脫脦脌麓碌脛掳眉脤氓脧脠禄潞麓忙脡脧脫脦路垄脣脥脌麓碌脛掳眉脤氓拢卢脠禄潞贸脭脷路垄脣脥碌陆脧脗脫脦拢卢脠莽鹿没赂脙脰碌脦陋0拢卢脭貌陆脫脢脮露脿脡脵脡脧脫脦掳眉脤氓戮脥脧貌脧脗脫脦脳陋路垄露脿脡脵掳眉脤氓

        if (u->input_filter == NULL) { //脠莽鹿没input_filter脦陋驴脮拢卢脭貌脡猫脰脙脛卢脠脧碌脛filter拢卢脠禄潞贸脳录卤赂路垄脣脥脢媒戮脻碌陆驴脥禄搂露脣隆拢脠禄潞贸脢脭脳脜露脕露脕FCGI
            u->input_filter_init = ngx_http_upstream_non_buffered_filter_init;
            //ngx_http_upstream_non_buffered_filter陆芦u->buffer.last - u->buffer.pos脰庐录盲碌脛脢媒戮脻路脜碌陆u->out_bufs路垄脣脥禄潞鲁氓脠楼脕麓卤铆脌茂脙忙隆拢
            //赂霉戮脻戮脽脤氓碌脛碌陆脡脧脫脦脳陋路垄碌脛路陆脢陆拢卢脩隆脭帽脢鹿脫脙fastcgi memcached碌脠拢卢ngx_http_xxx_filter
            u->input_filter = ngx_http_upstream_non_buffered_filter; //脪禄掳茫戮脥脡猫脰脙脦陋脮芒赂枚脛卢脠脧碌脛拢卢memcache脦陋ngx_http_memcached_filter
            u->input_filter_ctx = r;
        }

        //脡猫脰脙upstream碌脛露脕脢脗录镁禄脴碌梅拢卢脡猫脰脙驴脥禄搂露脣脕卢陆脫碌脛脨麓脢脗录镁禄脴碌梅隆拢
        u->read_event_handler = ngx_http_upstream_process_non_buffered_upstream;
        r->write_event_handler =
                             ngx_http_upstream_process_non_buffered_downstream;//碌梅脫脙鹿媒脗脣脛拢驴茅脪禄赂枚赂枚鹿媒脗脣body拢卢脳卯脰脮路垄脣脥鲁枚脠楼隆拢

        r->limit_rate = 0;
        //ngx_http_XXX_input_filter_init(脠莽ngx_http_fastcgi_input_filter_init ngx_http_proxy_input_filter_init ngx_http_proxy_input_filter_init)  
        //脰禄脫脨memcached禄谩脰麓脨脨ngx_http_memcached_filter_init拢卢脝盲脣没路陆脢陆脢虏脙麓脪虏脙禄脳枚 
        if (u->input_filter_init(u->input_filter_ctx) == NGX_ERROR) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

        if (clcf->tcp_nodelay && c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            tcp_nodelay = 1;

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                               (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }

        n = u->buffer.last - u->buffer.pos;

        /* 
          虏禄脢脟禄鹿脙禄陆脫脢脮掳眉脤氓脗茂拢卢脦陋脢虏脙麓戮脥驴陋脢录路垄脣脥脕脣脛脴驴
              脮芒脢脟脪貌脦陋脭脷脟掳脙忙碌脛ngx_http_upstream_process_header陆脫脢脮fastcgi脥路虏驴脨脨卤锚脢露掳眉脤氓麓娄脌铆碌脛脢卤潞貌拢卢脫脨驴脡脛脺禄谩掳脩脪禄虏驴路脰fastcgi掳眉脤氓卤锚脢露脪虏脢脮鹿媒脕脣拢卢
          脪貌麓脣脨猫脪陋麓娄脌铆
          */
        
        if (n) {//碌脙碌陆陆芦脪陋路垄脣脥碌脛脢媒戮脻碌脛麓贸脨隆拢卢脙驴麓脦脫脨露脿脡脵戮脥路垄脣脥露脿脡脵隆拢虏禄碌脠麓媒upstream脕脣  脪貌脦陋脮芒脢脟虏禄禄潞麓忙路陆脢陆路垄脣脥掳眉脤氓碌陆驴脥禄搂露脣
            u->buffer.last = u->buffer.pos;

            u->state->response_length += n;//脥鲁录脝脟毛脟贸碌脛路碌禄脴掳眉脤氓脢媒戮脻(虏禄掳眉脌篓脟毛脟贸脨脨)鲁陇露脠隆拢

            //脧脗脙忙input_filter脰禄脢脟录貌碌楼碌脛驴陆卤麓buffer脡脧脙忙碌脛脢媒戮脻脳脺鹿虏n鲁陇露脠碌脛拢卢碌陆u->out_bufs脌茂脙忙脠楼拢卢脪脭麓媒路垄脣脥隆拢
            //ngx_http_xxx_non_buffered_filter(脠莽ngx_http_fastcgi_non_buffered_filter)
            if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            ngx_http_upstream_process_non_buffered_downstream(r);

        } else {
            u->buffer.pos = u->buffer.start;
            u->buffer.last = u->buffer.start;

            if (ngx_http_send_special(r, NGX_HTTP_FLUSH) == NGX_ERROR) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            if (u->peer.connection->read->ready || u->length == 0) {
                ngx_http_upstream_process_non_buffered_upstream(r, u);
            }
        }

        return; //脮芒脌茂禄谩路碌禄脴禄脴脠楼
    }

    /* TODO: preallocate event_pipe bufs, look "Content-Length" */

#if (NGX_HTTP_CACHE)

    /* 脳垄脪芒脮芒脢卤潞貌禄鹿脢脟脭脷露脕脠隆碌脷脪禄赂枚脥路虏驴脨脨碌脛鹿媒鲁脤脰脨(驴脡脛脺禄谩脨炉麓酶虏驴路脰禄貌脮脽脠芦虏驴掳眉脤氓脢媒戮脻脭脷脌茂脙忙)  */

    if (r->cache && r->cache->file.fd != NGX_INVALID_FILE) {
        ngx_pool_run_cleanup_file(r->pool, r->cache->file.fd);
        r->cache->file.fd = NGX_INVALID_FILE;
    }

    /*   
     fastcgi_no_cache 脜盲脰脙脰赂脕卯驴脡脪脭脢鹿 upstream 脛拢驴茅虏禄脭脵禄潞麓忙脗煤脳茫录脠露篓脤玫录镁碌脛脟毛脟贸碌脙 
     碌陆碌脛脧矛脫娄隆拢脫脡脡脧脙忙 ngx_http_test_predicates 潞炉脢媒录掳脧脿鹿脴麓煤脗毛脥锚鲁脡隆拢 
     */
    switch (ngx_http_test_predicates(r, u->conf->no_cache)) {

    case NGX_ERROR:
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;

    case NGX_DECLINED:
        u->cacheable = 0;
        break;

    default: /* NGX_OK */
        //脭脷驴脥禄搂露脣脟毛脟贸潞贸露脣碌脛脢卤潞貌拢卢脠莽鹿没脙禄脫脨脙眉脰脨拢卢脭貌禄谩掳脩cache_status脰脙脦陋NGX_HTTP_CACHE_BYPASS
        if (u->cache_status == NGX_HTTP_CACHE_BYPASS) {//脣碌脙梅脢脟脪貌脦陋脜盲脰脙脕脣xxx_cache_bypass鹿娄脛脺拢卢麓脫露酶脰卤陆脫麓脫潞贸露脣脠隆脢媒戮脻

            /* create cache if previously bypassed */
            /*
               fastcgi_cache_bypass 脜盲脰脙脰赂脕卯驴脡脪脭脢鹿脗煤脳茫录脠露篓脤玫录镁碌脛脟毛脟贸脠脝鹿媒禄潞麓忙脢媒戮脻拢卢碌芦脢脟脮芒脨漏脟毛脟贸碌脛脧矛脫娄脢媒戮脻脪脌脠禄驴脡脪脭卤禄 upstream 脛拢驴茅禄潞麓忙隆拢 
               */
            if (ngx_http_file_cache_create(r) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }
        }

        break;
    }

    /*
     u->cacheable 脫脙脫脷驴脴脰脝脢脟路帽露脭脧矛脫娄陆酶脨脨禄潞麓忙虏脵脳梅隆拢脝盲脛卢脠脧脰碌脦陋 1拢卢脭脷禄潞麓忙露脕脠隆鹿媒鲁脤脰脨 驴脡脪貌脛鲁脨漏脤玫录镁陆芦脝盲脡猫脰脙脦陋 0拢卢录麓虏禄脭脷禄潞麓忙赂脙脟毛脟贸碌脛脧矛脫娄脢媒戮脻隆拢 
     */
    if (u->cacheable) {
        now = ngx_time();

        /*
           禄潞麓忙脛脷脠脻碌脛脫脨脨搂脢卤录盲脫脡 fastcgi_cache_valid  proxy_cache_valid脜盲脰脙脰赂脕卯脡猫脰脙拢卢虏垄脟脪脦麓戮颅赂脙脰赂脕卯脡猫脰脙碌脛脧矛脫娄脢媒戮脻脢脟虏禄禄谩卤禄 upstream 脛拢驴茅禄潞麓忙碌脛隆拢
          */
        valid = r->cache->valid_sec;

        if (valid == 0) { //赂鲁脰碌proxy_cache_valid xxx 4m;脰脨碌脛4m
            valid = ngx_http_file_cache_valid(u->conf->cache_valid,
                                              u->headers_in.status_n);
            if (valid) {
                r->cache->valid_sec = now + valid;
            }
        }

        if (valid) {
            r->cache->date = now;
            //脭脷赂脙潞炉脢媒脟掳ngx_http_upstream_process_header->p->process_header()潞炉脢媒脰脨脪脩戮颅陆芒脦枚鲁枚掳眉脤氓脥路虏驴脨脨 
            r->cache->body_start = (u_short) (u->buffer.pos - u->buffer.start); //潞贸露脣路碌禄脴碌脛脥酶脪鲁掳眉脤氓虏驴路脰脭脷buffer脰脨碌脛麓忙麓垄脦禄脰脙

            if (u->headers_in.status_n == NGX_HTTP_OK
                || u->headers_in.status_n == NGX_HTTP_PARTIAL_CONTENT)
            {
                //潞贸露脣脨炉麓酶碌脛脥路虏驴脨脨"Last-Modified:XXX"赂鲁脰碌拢卢录没ngx_http_upstream_process_last_modified
                r->cache->last_modified = u->headers_in.last_modified_time;

                if (u->headers_in.etag) {
                    r->cache->etag = u->headers_in.etag->value;

                } else {
                    ngx_str_null(&r->cache->etag);
                }

            } else {
                r->cache->last_modified = -1;
                ngx_str_null(&r->cache->etag);
            }

            /* 
               脳垄脪芒脮芒脢卤潞貌禄鹿脢脟脭脷露脕脠隆碌脷脪禄赂枚脥路虏驴脨脨碌脛鹿媒鲁脤脰脨(驴脡脛脺禄谩脨炉麓酶虏驴路脰禄貌脮脽脠芦虏驴掳眉脤氓脢媒戮脻脭脷脌茂脙忙)  
                 
               upstream 脛拢驴茅脭脷脡锚脟毛 u->buffer 驴脮录盲脢卤拢卢脪脩戮颅脭陇脧脠脦陋禄潞麓忙脦脛录镁掳眉脥路路脰脜盲脕脣驴脮录盲拢卢脣霉脪脭驴脡脪脭脰卤陆脫碌梅脫脙 ngx_http_file_cache_set_header 
               脭脷麓脣驴脮录盲脰脨鲁玫脢录禄炉禄潞麓忙脦脛录镁掳眉脥路拢潞 
               */
            if (ngx_http_file_cache_set_header(r, u->buffer.start) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } else {
            u->cacheable = 0;
        }
    }

    now = ngx_time();
    if(r->cache) {
        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http cacheable: %d, r->cache->valid_sec:%T, now:%T", u->cacheable, r->cache->valid_sec, now);
    } else {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http cacheable: %d", u->cacheable);
    }               
    if (u->cacheable == 0 && r->cache) {
        ngx_http_file_cache_free(r->cache, u->pipe->temp_file);
    }

#endif
    //buffering路陆脢陆禄谩脳脽碌陆脮芒脌茂拢卢脥篓鹿媒pipe路垄脣脥拢卢脠莽鹿没脦陋0拢卢脭貌脡脧脙忙碌脛鲁脤脨貌禄谩return
    
    p = u->pipe;

    //脡猫脰脙filter拢卢驴脡脪脭驴麓碌陆戮脥脢脟http碌脛脢盲鲁枚filter
    p->output_filter = (ngx_event_pipe_output_filter_pt) ngx_http_output_filter;
    p->output_ctx = r;
    p->tag = u->output.tag;
    p->bufs = u->conf->bufs;//脡猫脰脙bufs拢卢脣眉戮脥脢脟upstream脰脨脡猫脰脙碌脛bufs.u == &flcf->upstream;
    p->busy_size = u->conf->busy_buffers_size; //脛卢脠脧
    p->upstream = u->peer.connection;//赂鲁脰碌赂煤潞贸露脣upstream碌脛脕卢陆脫隆拢
    p->downstream = c;//赂鲁脰碌赂煤驴脥禄搂露脣碌脛脕卢陆脫隆拢
    p->pool = r->pool;
    p->log = c->log;
    p->limit_rate = u->conf->limit_rate;
    p->start_sec = ngx_time();

    p->cacheable = u->cacheable || u->store;

    p->temp_file = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
    if (p->temp_file == NULL) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    p->temp_file->file.fd = NGX_INVALID_FILE;
    p->temp_file->file.log = c->log;
    p->temp_file->path = u->conf->temp_path;
    p->temp_file->pool = r->pool;

    ngx_int_t cacheable = p->cacheable;

   if (r->cache && r->cache->file_cache->temp_path && r->cache->file_cache->path) {
        ngx_log_debugall(p->log, 0, "ngx_http_upstream_send_response, "
            "p->cacheable:%i, tempfile:%V, pathfile:%V", cacheable, &r->cache->file_cache->temp_path->name, &r->cache->file_cache->path->name);
    }
    
    if (p->cacheable) {
        p->temp_file->persistent = 1;
        /*
脛卢脠脧脟茅驴枚脧脗p->temp_file->path = u->conf->temp_path; 脪虏戮脥脢脟脫脡ngx_http_fastcgi_temp_path脰赂露篓脗路戮露拢卢碌芦脢脟脠莽鹿没脢脟禄潞麓忙路陆脢陆(p->cacheable=1)虏垄脟脪脜盲脰脙
proxy_cache_path(fastcgi_cache_path) /a/b碌脛脢卤潞貌麓酶脫脨use_temp_path=off(卤铆脢戮虏禄脢鹿脫脙ngx_http_fastcgi_temp_path脜盲脰脙碌脛path)拢卢
脭貌p->temp_file->path = r->cache->file_cache->temp_path; 脪虏戮脥脢脟脕脵脢卤脦脛录镁/a/b/temp隆拢use_temp_path=off卤铆脢戮虏禄脢鹿脫脙ngx_http_fastcgi_temp_path
脜盲脰脙碌脛脗路戮露拢卢露酶脢鹿脫脙脰赂露篓碌脛脕脵脢卤脗路戮露/a/b/temp   录没ngx_http_upstream_send_response 
*/
#if (NGX_HTTP_CACHE)
        if (r->cache && r->cache->file_cache->temp_path) {
            p->temp_file->path = r->cache->file_cache->temp_path;
        }
#endif

    } else {
        p->temp_file->log_level = NGX_LOG_WARN;
        p->temp_file->warn = "an upstream response is buffered "
                             "to a temporary file";
    }

    p->max_temp_file_size = u->conf->max_temp_file_size;
    p->temp_file_write_size = u->conf->temp_file_write_size;

    p->preread_bufs = ngx_alloc_chain_link(r->pool);
    if (p->preread_bufs == NULL) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    p->preread_bufs->buf = &u->buffer; //掳脩掳眉脤氓虏驴路脰碌脛pos潞脥last麓忙麓垄碌陆p->preread_bufs->buf
    p->preread_bufs->next = NULL;
    u->buffer.recycled = 1;

    //脰庐脟掳露脕脠隆潞贸露脣脥路虏驴脨脨脨脜脧垄碌脛脢卤潞貌碌脛buf禄鹿脫脨脢拢脫脿脢媒戮脻拢卢脮芒虏驴路脰脢媒戮脻戮脥脢脟掳眉脤氓脢媒戮脻拢卢脪虏戮脥脢脟露脕脠隆脥路虏驴脨脨fastcgi卤锚脢露脨脜脧垄碌脛脢卤潞貌掳脩虏驴路脰掳眉脤氓脢媒戮脻露脕脠隆脕脣
    p->preread_size = u->buffer.last - u->buffer.pos; 
    
    if (u->cacheable) { //脳垄脪芒脳脽碌陆脮芒脌茂碌脛脢卤潞貌拢卢脟掳脙忙脪脩戮颅掳脩潞贸露脣脥路虏驴脨脨脨脜脧垄陆芒脦枚鲁枚脌麓脕脣拢卢u->buffer.pos脰赂脧貌碌脛脢脟脢碌录脢脢媒戮脻虏驴路脰

        p->buf_to_file = ngx_calloc_buf(r->pool);
        if (p->buf_to_file == NULL) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }

        //脰赂脧貌碌脛脢脟脦陋禄帽脠隆潞贸露脣脥路虏驴脨脨碌脛脢卤潞貌路脰脜盲碌脛碌脷脪禄赂枚禄潞鲁氓脟酶拢卢buf麓贸脨隆脫脡xxx_buffer_size(fastcgi_buffer_size proxy_buffer_size memcached_buffer_size)脰赂露篓
        /*
            脮芒脌茂脙忙脰禄麓忙麓垄脕脣脥路虏驴脨脨buffer脰脨脥路虏驴脨脨碌脛脛脷脠脻虏驴路脰拢卢脪貌脦陋潞贸脙忙脨麓脕脵脢卤脦脛录镁碌脛脢卤潞貌拢卢脨猫脪陋掳脩潞贸露脣脥路虏驴脨脨脪虏脨麓陆酶脌麓拢卢脫脡脫脷脟掳脙忙露脕脠隆脥路虏驴脨脨潞贸脰赂脮毛脪脩戮颅脰赂脧貌脕脣脢媒戮脻虏驴路脰
            脪貌麓脣脨猫脪陋脕脵脢卤脫脙buf_to_file->start脰赂脧貌脥路虏驴脨脨虏驴路脰驴陋脢录拢卢pos脰赂脧貌脢媒戮脻虏驴路脰驴陋脢录拢卢脪虏戮脥脢脟脥路虏驴脨脨虏驴路脰陆谩脦虏
          */
        p->buf_to_file->start = u->buffer.start; 
        p->buf_to_file->pos = u->buffer.start;
        p->buf_to_file->last = u->buffer.pos;
        p->buf_to_file->temporary = 1;
    }

    if (ngx_event_flags & NGX_USE_IOCP_EVENT) {
        /* the posted aio operation may corrupt a shadow buffer */
        p->single_buf = 1;
    }

    /* TODO: p->free_bufs = 0 if use ngx_create_chain_of_bufs() */
    p->free_bufs = 1;

    /*
     * event_pipe would do u->buffer.last += p->preread_size
     * as though these bytes were read
     */
    u->buffer.last = u->buffer.pos; //掳眉脤氓脢媒戮脻脰赂脧貌脭脷脟掳脙忙脪脩戮颅麓忙麓垄碌陆脕脣p->preread_bufs->buf

    if (u->conf->cyclic_temp_file) {

        /*
         * we need to disable the use of sendfile() if we use cyclic temp file
         * because the writing a new data may interfere with sendfile()
         * that uses the same kernel file pages (at least on FreeBSD)
         */

        p->cyclic_temp_file = 1;
        c->sendfile = 0;

    } else {
        p->cyclic_temp_file = 0;
    }

    p->read_timeout = u->conf->read_timeout;
    p->send_timeout = clcf->send_timeout;
    p->send_lowat = clcf->send_lowat;

    p->length = -1;

    if (u->input_filter_init
        && u->input_filter_init(p->input_ctx) != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    //buffering路陆脢陆拢卢潞贸露脣脥路虏驴脨脜脧垄脪脩戮颅露脕脠隆脥锚卤脧脕脣拢卢脠莽鹿没潞贸露脣禄鹿脫脨掳眉脤氓脨猫脪陋路垄脣脥拢卢脭貌卤戮露脣脥篓鹿媒赂脙路陆脢陆露脕脠隆
    u->read_event_handler = ngx_http_upstream_process_upstream;
    r->write_event_handler = ngx_http_upstream_process_downstream; //碌卤驴脡脨麓脢脗录镁麓脵路垄碌脛脢卤潞貌拢卢脥篓鹿媒赂脙潞炉脢媒录脤脨酶脨麓脢媒戮脻

    ngx_http_upstream_process_upstream(r, u);
}


static void
ngx_http_upstream_upgrade(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    int                        tcp_nodelay;
    ngx_connection_t          *c;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    /* TODO: prevent upgrade if not requested or not possible */

    r->keepalive = 0;
    c->log->action = "proxying upgraded connection";

    u->read_event_handler = ngx_http_upstream_upgraded_read_upstream;
    u->write_event_handler = ngx_http_upstream_upgraded_write_upstream;
    r->read_event_handler = ngx_http_upstream_upgraded_read_downstream;
    r->write_event_handler = ngx_http_upstream_upgraded_write_downstream;

    if (clcf->tcp_nodelay) {
        tcp_nodelay = 1;

        if (c->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "tcp_nodelay");

            if (setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY,
                           (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(c, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            c->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }

        if (u->peer.connection->tcp_nodelay == NGX_TCP_NODELAY_UNSET) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, u->peer.connection->log, 0,
                           "tcp_nodelay");

            if (setsockopt(u->peer.connection->fd, IPPROTO_TCP, TCP_NODELAY,
                           (const void *) &tcp_nodelay, sizeof(int)) == -1)
            {
                ngx_connection_error(u->peer.connection, ngx_socket_errno,
                                     "setsockopt(TCP_NODELAY) failed");
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            u->peer.connection->tcp_nodelay = NGX_TCP_NODELAY_SET;
        }
    }

    if (ngx_http_send_special(r, NGX_HTTP_FLUSH) == NGX_ERROR) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (u->peer.connection->read->ready
        || u->buffer.pos != u->buffer.last)
    {
        ngx_post_event(c->read, &ngx_posted_events);
        ngx_http_upstream_process_upgraded(r, 1, 1);
        return;
    }

    ngx_http_upstream_process_upgraded(r, 0, 1);
}


static void
ngx_http_upstream_upgraded_read_downstream(ngx_http_request_t *r)
{
    ngx_http_upstream_process_upgraded(r, 0, 0);
}


static void
ngx_http_upstream_upgraded_write_downstream(ngx_http_request_t *r)
{
    ngx_http_upstream_process_upgraded(r, 1, 1);
}


static void
ngx_http_upstream_upgraded_read_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_http_upstream_process_upgraded(r, 1, 0);
}


static void
ngx_http_upstream_upgraded_write_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_http_upstream_process_upgraded(r, 0, 1);
}


static void
ngx_http_upstream_process_upgraded(ngx_http_request_t *r,
    ngx_uint_t from_upstream, ngx_uint_t do_write)
{
    size_t                     size;
    ssize_t                    n;
    ngx_buf_t                 *b;
    ngx_connection_t          *c, *downstream, *upstream, *dst, *src;
    ngx_http_upstream_t       *u;
    ngx_http_core_loc_conf_t  *clcf;

    c = r->connection;
    u = r->upstream;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process upgraded, fu:%ui", from_upstream);

    downstream = c;
    upstream = u->peer.connection;

    if (downstream->write->timedout) {
        c->timedout = 1;
        ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    if (upstream->read->timedout || upstream->write->timedout) { //脭脷路垄脣脥脟毛脟贸碌陆潞贸露脣碌脛脢卤潞貌拢卢脦脪脙脟脨猫脪陋碌脠麓媒露脭路陆脫娄麓冒拢卢脪貌麓脣脡猫脰脙脕脣露脕鲁卢脢卤露篓脢卤脝梅拢卢录没ngx_http_upstream_send_request
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }

    if (from_upstream) {
        src = upstream;
        dst = downstream;
        b = &u->buffer;

    } else {
        src = downstream;
        dst = upstream;
        b = &u->from_client;

        if (r->header_in->last > r->header_in->pos) {
            b = r->header_in;
            b->end = b->last;
            do_write = 1;
        }

        if (b->start == NULL) {
            b->start = ngx_palloc(r->pool, u->conf->buffer_size);
            if (b->start == NULL) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

            b->pos = b->start;
            b->last = b->start;
            b->end = b->start + u->conf->buffer_size;
            b->temporary = 1;
            b->tag = u->output.tag;
        }
    }

    for ( ;; ) {

        if (do_write) {

            size = b->last - b->pos;

            if (size && dst->write->ready) {

                n = dst->send(dst, b->pos, size);

                if (n == NGX_ERROR) {
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }

                if (n > 0) {
                    b->pos += n;

                    if (b->pos == b->last) {
                        b->pos = b->start;
                        b->last = b->start;
                    }
                }
            }
        }

        size = b->end - b->last;

        if (size && src->read->ready) {

            n = src->recv(src, b->last, size);

            if (n == NGX_AGAIN || n == 0) {
                break;
            }

            if (n > 0) {
                do_write = 1;
                b->last += n;

                continue;
            }

            if (n == NGX_ERROR) {
                src->read->eof = 1;
            }
        }

        break;
    }

    if ((upstream->read->eof && u->buffer.pos == u->buffer.last)
        || (downstream->read->eof && u->from_client.pos == u->from_client.last)
        || (downstream->read->eof && upstream->read->eof))
    {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                       "http upstream upgraded done");
        ngx_http_upstream_finalize_request(r, u, 0);
        return;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (ngx_handle_write_event(upstream->write, u->conf->send_lowat, NGX_FUNC_LINE)
        != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (upstream->write->active && !upstream->write->ready) {
        ngx_add_timer(upstream->write, u->conf->send_timeout, NGX_FUNC_LINE);

    } else if (upstream->write->timer_set) {
        ngx_del_timer(upstream->write, NGX_FUNC_LINE);
    }

    if (ngx_handle_read_event(upstream->read, 0, NGX_FUNC_LINE) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (upstream->read->active && !upstream->read->ready) {
        ngx_add_timer(upstream->read, u->conf->read_timeout, NGX_FUNC_LINE);

    } else if (upstream->read->timer_set) {
        ngx_del_timer(upstream->read, NGX_FUNC_LINE);
    }

    if (ngx_handle_write_event(downstream->write, clcf->send_lowat, NGX_FUNC_LINE)
        != NGX_OK)
    {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (ngx_handle_read_event(downstream->read, 0, NGX_FUNC_LINE) != NGX_OK) {
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (downstream->write->active && !downstream->write->ready) {
        ngx_add_timer(downstream->write, clcf->send_timeout, NGX_FUNC_LINE);

    } else if (downstream->write->timer_set) {
        ngx_del_timer(downstream->write, NGX_FUNC_LINE);
    }
}

/*
ngx_http_upstream_send_response路垄脣脥脥锚HERDER潞贸拢卢脠莽鹿没脢脟路脟禄潞鲁氓脛拢脢陆拢卢禄谩碌梅脫脙脮芒脌茂陆芦脢媒戮脻路垄脣脥鲁枚脠楼碌脛隆拢
脮芒赂枚潞炉脢媒脢碌录脢脡脧脜脨露脧脪禄脧脗鲁卢脢卤潞贸拢卢戮脥碌梅脫脙ngx_http_upstream_process_non_buffered_request脕脣隆拢nginx脌脧路陆路篓隆拢
*/
static void 
//buffring脛拢脢陆脥篓鹿媒ngx_http_upstream_process_upstream赂脙潞炉脢媒麓娄脌铆拢卢路脟buffring脛拢脢陆脥篓鹿媒ngx_http_upstream_process_non_buffered_downstream麓娄脌铆
ngx_http_upstream_process_non_buffered_downstream(ngx_http_request_t *r)
{
    ngx_event_t          *wev;
    ngx_connection_t     *c;
    ngx_http_upstream_t  *u;

    c = r->connection;
    u = r->upstream;
    wev = c->write;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process non buffered downstream");

    c->log->action = "sending to client";

    if (wev->timedout) {
        c->timedout = 1;
        ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_REQUEST_TIME_OUT);
        return;
    }

    //脧脗脙忙驴陋脢录陆芦out_bufs脌茂脙忙碌脛脢媒戮脻路垄脣脥鲁枚脠楼拢卢脠禄潞贸露脕脠隆脢媒戮脻拢卢脠禄潞贸路垄脣脥拢卢脠莽麓脣脩颅禄路隆拢
    ngx_http_upstream_process_non_buffered_request(r, 1);
}

//ngx_http_upstream_send_response脡猫脰脙潞脥碌梅脫脙脮芒脌茂拢卢碌卤脡脧脫脦碌脛PROXY脫脨脢媒戮脻碌陆脌麓拢卢驴脡脪脭露脕脠隆碌脛脢卤潞貌碌梅脫脙脮芒脌茂隆拢
//buffering路陆脢陆拢卢脦陋ngx_http_fastcgi_input_filter  路脟buffering路陆脢陆脦陋ngx_http_upstream_non_buffered_filter
static void
ngx_http_upstream_process_non_buffered_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{
    ngx_connection_t  *c;

    c = u->peer.connection;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process non buffered upstream");

    c->log->action = "reading upstream";

    if (c->read->timedout) { //脭脷路垄脣脥脟毛脟贸碌陆潞贸露脣碌脛脢卤潞貌拢卢脦脪脙脟脨猫脪陋碌脠麓媒露脭路陆脫娄麓冒拢卢脪貌麓脣脡猫脰脙脕脣露脕鲁卢脢卤露篓脢卤脝梅拢卢录没ngx_http_upstream_send_request
        ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        ngx_http_upstream_finalize_request(r, u, NGX_HTTP_GATEWAY_TIME_OUT);
        return;
    }
    
    //脮芒脌茂赂煤ngx_http_upstream_process_non_buffered_downstream脝盲脢碌戮脥脪禄赂枚脟酶卤冒: 虏脦脢媒脦陋0拢卢卤铆脢戮虏禄脫脙脕垄录麓路垄脣脥脢媒戮脻拢卢脪貌脦陋脙禄脫脨脢媒戮脻驴脡脪脭路垄脣脥拢卢碌脙脧脠露脕脠隆虏脜脨脨隆拢
    ngx_http_upstream_process_non_buffered_request(r, 0);
}

/*
碌梅脫脙鹿媒脗脣脛拢驴茅拢卢陆芦脢媒戮脻路垄脣脥鲁枚脠楼拢卢do_write脦陋脢脟路帽脪陋赂酶驴脥禄搂露脣路垄脣脥脢媒戮脻隆拢
1.脠莽鹿没脪陋路垄脣脥拢卢戮脥碌梅脫脙ngx_http_output_filter陆芦脢媒戮脻路垄脣脥鲁枚脠楼隆拢
2.脠禄潞贸ngx_unix_recv露脕脠隆脢媒戮脻拢卢路脜脠毛out_bufs脌茂脙忙脠楼隆拢脠莽麓脣脩颅禄路
*/
static void
ngx_http_upstream_process_non_buffered_request(ngx_http_request_t *r,
    ngx_uint_t do_write)
{
    size_t                     size;
    ssize_t                    n;
    ngx_buf_t                 *b;
    ngx_int_t                  rc;
    ngx_connection_t          *downstream, *upstream;
    ngx_http_upstream_t       *u;
    ngx_http_core_loc_conf_t  *clcf;

    u = r->upstream;
    downstream = r->connection;//脮脪碌陆脮芒赂枚脟毛脟贸碌脛驴脥禄搂露脣脕卢陆脫
    upstream = u->peer.connection;//脮脪碌陆脡脧脫脦碌脛脕卢陆脫

    b = &u->buffer; //脮脪碌陆脮芒脹莽脪陋路垄脣脥碌脛脢媒戮脻拢卢虏禄鹿媒麓贸虏驴路脰露录卤禄input filter路脜碌陆out_bufs脌茂脙忙脠楼脕脣隆拢

    do_write = do_write || u->length == 0; //do_write脦陋1脢卤卤铆脢戮脪陋脕垄录麓路垄脣脥赂酶驴脥禄搂露脣隆拢

    for ( ;; ) {

        if (do_write) { //脪陋脕垄录麓路垄脣脥隆拢
            //out_bufs脰脨碌脛脢媒戮脻脢脟麓脫ngx_http_fastcgi_non_buffered_filter禄帽脠隆
            if (u->out_bufs || u->busy_bufs) {
                //脠莽鹿没u->out_bufs虏禄脦陋NULL脭貌脣碌脙梅脫脨脨猫脪陋路垄脣脥碌脛脢媒戮脻拢卢脮芒脢脟u->input_filter_init(u->input_filter_ctx)(ngx_http_upstream_non_buffered_filter)驴陆卤麓碌陆脮芒脌茂碌脛隆拢
				//u->busy_bufs麓煤卤铆脢脟脭脷露脕脠隆fastcgi脟毛脟贸脥路碌脛脢卤潞貌拢卢驴脡脛脺脌茂脙忙禄谩麓酶脫脨掳眉脤氓脢媒戮脻拢卢戮脥脢脟脥篓鹿媒脮芒脌茂路垄脣脥
                rc = ngx_http_output_filter(r, u->out_bufs);

                if (rc == NGX_ERROR) {
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }

                //戮脥脢脟掳脩ngx_http_output_filter碌梅脫脙潞贸脦麓路垄脣脥脥锚卤脧碌脛脢媒戮脻buf脤铆录脫碌陆busy_bufs脰脨拢卢脠莽鹿没脧脗麓脦脭脵麓脦碌梅脫脙ngx_http_output_filter潞贸掳脩busy_bufs脰脨脡脧脪禄麓脦脙禄脫脨路垄脣脥脥锚碌脛路垄脣脥鲁枚脠楼脕脣拢卢脭貌掳脩露脭脫娄碌脛buf脪脝鲁媒脤铆录脫碌陆free脰脨
                //脧脗脙忙陆芦out_bufs碌脛脭陋脣脴脪脝露炉碌陆busy_bufs碌脛潞贸脙忙拢禄陆芦脪脩戮颅路垄脣脥脥锚卤脧碌脛busy_bufs脕麓卤铆脭陋脣脴脪脝露炉碌陆free_bufs脌茂脙忙
                ngx_chain_update_chains(r->pool, &u->free_bufs, &u->busy_bufs,
                                        &u->out_bufs, u->output.tag);
            }

            if (u->busy_bufs == NULL) {//busy_bufs脙禄脫脨脕脣拢卢露录路垄脥锚脕脣隆拢脧毛脪陋路垄脣脥碌脛脢媒戮脻露录脪脩戮颅路垄脣脥脥锚卤脧

                if (u->length == 0
                    || (upstream->read->eof && u->length == -1)) //掳眉脤氓脢媒戮脻脪脩戮颅露脕脥锚脕脣
                {
                    ngx_http_upstream_finalize_request(r, u, 0);
                    return;
                }

                if (upstream->read->eof) {
                    ngx_log_error(NGX_LOG_ERR, upstream->log, 0,
                                  "upstream prematurely closed connection");

                    ngx_http_upstream_finalize_request(r, u,
                                                       NGX_HTTP_BAD_GATEWAY);
                    return;
                }

                if (upstream->read->error) {
                    ngx_http_upstream_finalize_request(r, u,
                                                       NGX_HTTP_BAD_GATEWAY);
                    return;
                }

                b->pos = b->start;//脰脴脰脙u->buffer,脪脭卤茫脫毛脧脗麓脦脢鹿脫脙拢卢麓脫驴陋脢录脝冒隆拢b脰赂脧貌碌脛驴脮录盲驴脡脪脭录脤脨酶露脕脢媒戮脻脕脣
                b->last = b->start;
            }
        }

        size = b->end - b->last;//碌脙碌陆碌卤脟掳buf碌脛脢拢脫脿驴脮录盲

        if (size && upstream->read->ready) { 
        //脦陋脢虏脙麓驴脡脛脺脳脽碌陆脮芒脌茂?脪貌脦陋脭脷ngx_http_upstream_process_header脰脨露脕脠隆潞贸露脣脢媒戮脻碌脛脢卤潞貌拢卢buf麓贸脨隆脛卢脠脧脦陋脪鲁脙忙麓贸脨隆ngx_pagesize
        //碌楼脫脨驴脡脛脺潞贸露脣路垄脣脥鹿媒脌麓碌脛脢媒戮脻卤脠ngx_pagesize麓贸拢卢脪貌麓脣戮脥脙禄脫脨露脕脥锚拢卢脪虏戮脥脢脟recv脰脨虏禄禄谩掳脡ready脰脙0拢卢脣霉脪脭脮芒脌茂驴脡脪脭录脤脨酶露脕

            n = upstream->recv(upstream, b->last, size);

            if (n == NGX_AGAIN) { //脣碌脙梅脪脩戮颅脛脷潞脣禄潞鲁氓脟酶脢媒戮脻脪脩戮颅露脕脥锚拢卢脥脣鲁枚脩颅禄路拢卢脠禄潞贸赂霉戮脻epoll脢脗录镁脌麓录脤脨酶麓楼路垄露脕脠隆潞贸露脣脢媒戮脻
                break;
            }

            if (n > 0) {
                u->state->response_length += n;

                if (u->input_filter(u->input_filter_ctx, n) == NGX_ERROR) {
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                    return;
                }
            }

            do_write = 1;//脪貌脦陋赂脮赂脮脦脼脗脹脠莽潞脦n麓贸脫脷0拢卢脣霉脪脭露脕脠隆脕脣脢媒戮脻拢卢脛脟脙麓脧脗脪禄赂枚脩颅禄路禄谩陆芦out_bufs碌脛脢媒戮脻路垄脣脥鲁枚脠楼碌脛隆拢

            continue;
        }

        break;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    if (downstream->data == r) {
        if (ngx_handle_write_event(downstream->write, clcf->send_lowat, NGX_FUNC_LINE)
            != NGX_OK)
        {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }

    if (downstream->write->active && !downstream->write->ready) { 
    //脌媒脠莽脦脪掳脩脢媒戮脻掳脩脢媒戮脻脨麓碌陆脛脷潞脣脨颅脪茅脮禄碌陆脨麓脗煤脨颅脪茅脮禄禄潞麓忙拢卢碌芦脢脟露脭露脣脪禄脰卤虏禄露脕脠隆碌脛脢卤潞貌拢卢脢媒戮脻脪禄脰卤路垄虏禄鲁枚脠楼脕脣拢卢脪虏虏禄禄谩麓楼路垄epoll_wait脨麓脢脗录镁拢卢
    //脮芒脌茂录脫赂枚露篓脢卤脝梅戮脥脢脟脦陋脕脣卤脺脙芒脮芒脰脰脟茅驴枚路垄脡煤
        ngx_add_timer(downstream->write, clcf->send_timeout, NGX_FUNC_LINE);

    } else if (downstream->write->timer_set) {
        ngx_del_timer(downstream->write, NGX_FUNC_LINE);
    }

    if (ngx_handle_read_event(upstream->read, 0, NGX_FUNC_LINE) != NGX_OK) { //epoll脭脷accept碌脛脢卤潞貌露脕脨麓脪脩戮颅录脫脠毛epoll脰脨拢卢脪貌麓脣露脭epoll脌麓脣碌脙禄脫脙
        ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        return;
    }

    if (upstream->read->active && !upstream->read->ready) {
        ngx_add_timer(upstream->read, u->conf->read_timeout, NGX_FUNC_LINE);

    } else if (upstream->read->timer_set) {
        ngx_del_timer(upstream->read, NGX_FUNC_LINE);
    }
}


static ngx_int_t
ngx_http_upstream_non_buffered_filter_init(void *data)
{
    return NGX_OK;
}

/*
陆芦u->buffer.last - u->buffer.pos脰庐录盲碌脛脢媒戮脻路脜碌陆u->out_bufs路垄脣脥禄潞鲁氓脠楼脕麓卤铆脌茂脙忙隆拢脮芒脩霉驴脡脨麓碌脛脢卤潞貌戮脥禄谩路垄脣脥赂酶驴脥禄搂露脣隆拢
ngx_http_upstream_process_non_buffered_request潞炉脢媒禄谩露脕脠隆out_bufs脌茂脙忙碌脛脢媒戮脻拢卢脠禄潞贸碌梅脫脙脢盲鲁枚鹿媒脗脣脕麓陆脫陆酶脨脨路垄脣脥碌脛隆拢
*/ //buffering路陆脢陆拢卢脦陋ngx_http_fastcgi_input_filter  路脟buffering路陆脢陆脦陋ngx_http_upstream_non_buffered_filter
static ngx_int_t
ngx_http_upstream_non_buffered_filter(void *data, ssize_t bytes)
{
    ngx_http_request_t  *r = data;

    ngx_buf_t            *b;
    ngx_chain_t          *cl, **ll;
    ngx_http_upstream_t  *u;

    u = r->upstream;

    for (cl = u->out_bufs, ll = &u->out_bufs; cl; cl = cl->next) { //卤茅脌煤u->out_bufs
        ll = &cl->next;
    }

    cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs);//路脰脜盲脪禄赂枚驴脮脧脨碌脛chain buff
    if (cl == NULL) {
        return NGX_ERROR;
    }

    *ll = cl; //陆芦脨脗脡锚脟毛碌脛禄潞麓忙脕麓陆脫陆酶脌麓隆拢

    cl->buf->flush = 1;
    cl->buf->memory = 1;

    b = &u->buffer; //脠楼鲁媒陆芦脪陋路垄脣脥碌脛脮芒赂枚脢媒戮脻拢卢脫娄赂脙脢脟驴脥禄搂露脣碌脛路碌禄脴脢媒戮脻脤氓隆拢陆芦脝盲路脜脠毛

    cl->buf->pos = b->last;
    b->last += bytes;
    cl->buf->last = b->last;
    cl->buf->tag = u->output.tag;

    if (u->length == -1) {//u->length卤铆脢戮陆芦脪陋路垄脣脥碌脛脢媒戮脻麓贸脨隆脠莽鹿没脦陋-1,脭貌脣碌脙梅潞贸露脣脨颅脪茅虏垄脙禄脫脨脰赂露篓脨猫脪陋路垄脣脥碌脛麓贸脨隆(脌媒脠莽chunk路陆脢陆)拢卢麓脣脢卤脦脪脙脟脰禄脨猫脪陋路垄脣脥脦脪脙脟陆脫脢脮碌陆碌脛.
        return NGX_OK;
    }

    u->length -= bytes;//赂眉脨脗陆芦脪陋路垄脣脥碌脛脢媒戮脻麓贸脨隆
 
    return NGX_OK;
}


static void
ngx_http_upstream_process_downstream(ngx_http_request_t *r)
{
    ngx_event_t          *wev;
    ngx_connection_t     *c;
    ngx_event_pipe_t     *p;
    ngx_http_upstream_t  *u;

    c = r->connection;
    u = r->upstream;
    p = u->pipe;
    wev = c->write;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process downstream");

    c->log->action = "sending to client";

    if (wev->timedout) {

        if (wev->delayed) {

            wev->timedout = 0;
            wev->delayed = 0;

            if (!wev->ready) {
                ngx_add_timer(wev, p->send_timeout, NGX_FUNC_LINE);

                if (ngx_handle_write_event(wev, p->send_lowat, NGX_FUNC_LINE) != NGX_OK) {
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                }

                return;
            }

            if (ngx_event_pipe(p, wev->write) == NGX_ABORT) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } else {
            p->downstream_error = 1;
            c->timedout = 1;
            ngx_connection_error(c, NGX_ETIMEDOUT, "client timed out");
        }

    } else {

        if (wev->delayed) {

            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                           "http downstream delayed");

            if (ngx_handle_write_event(wev, p->send_lowat, NGX_FUNC_LINE) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            }

            return;
        }

        if (ngx_event_pipe(p, 1) == NGX_ABORT) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }

    ngx_http_upstream_process_request(r, u);
}

/*
脮芒脢脟脭脷脫脨buffering碌脛脟茅驴枚脧脗脢鹿脫脙碌脛潞炉脢媒隆拢
ngx_http_upstream_send_response碌梅脫脙脮芒脌茂路垄露炉脪禄脧脗脢媒戮脻露脕脠隆隆拢脪脭潞贸脫脨脢媒戮脻驴脡露脕碌脛脢卤潞貌脪虏禄谩碌梅脫脙脮芒脌茂碌脛露脕脠隆潞贸露脣脢媒戮脻隆拢脡猫脰脙碌陆脕脣u->read_event_handler脕脣隆拢
*/
static void
ngx_http_upstream_process_upstream(ngx_http_request_t *r,
    ngx_http_upstream_t *u) 
//buffring脛拢脢陆脥篓鹿媒ngx_http_upstream_process_upstream赂脙潞炉脢媒麓娄脌铆拢卢路脟buffring脛拢脢陆脥篓鹿媒ngx_http_upstream_process_non_buffered_downstream麓娄脌铆
{ //脳垄脪芒脳脽碌陆脮芒脌茂碌脛脢卤潞貌拢卢潞贸露脣路垄脣脥碌脛脥路虏驴脨脨脨脜脧垄脪脩戮颅脭脷脟掳脙忙碌脛ngx_http_upstream_send_response->ngx_http_send_header脪脩戮颅掳脩脥路虏驴脨脨虏驴路脰路垄脣脥赂酶驴脥禄搂露脣脕脣
    ngx_event_t       *rev;
    ngx_event_pipe_t  *p;
    ngx_connection_t  *c;

    c = u->peer.connection;
    p = u->pipe;
    rev = c->read;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http upstream process upstream");

    c->log->action = "reading upstream";

    if (rev->timedout) { //脭脷路垄脣脥脟毛脟贸碌陆潞贸露脣碌脛脢卤潞貌拢卢脦脪脙脟脨猫脪陋碌脠麓媒露脭路陆脫娄麓冒拢卢脪貌麓脣脡猫脰脙脕脣露脕鲁卢脢卤露篓脢卤脝梅拢卢录没ngx_http_upstream_send_request

        if (rev->delayed) {

            rev->timedout = 0;
            rev->delayed = 0;

            if (!rev->ready) { 
                ngx_add_timer(rev, p->read_timeout, NGX_FUNC_LINE);

                if (ngx_handle_read_event(rev, 0, NGX_FUNC_LINE) != NGX_OK) {
                    ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                }

                return;
            }

            if (ngx_event_pipe(p, 0) == NGX_ABORT) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
                return;
            }

        } else {
            p->upstream_error = 1;
            ngx_connection_error(c, NGX_ETIMEDOUT, "upstream timed out");
        }

    } else {//脟毛脟贸脙禄脫脨鲁卢脢卤拢卢脛脟脙麓露脭潞贸露脣拢卢麓娄脌铆脪禄脧脗露脕脢脗录镁隆拢ngx_event_pipe驴陋脢录麓娄脌铆

        if (rev->delayed) {

            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                           "http upstream delayed");

            if (ngx_handle_read_event(rev, 0, NGX_FUNC_LINE) != NGX_OK) {
                ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            }

            return;
        }

        if (ngx_event_pipe(p, 0) == NGX_ABORT) { //脳垄脪芒脮芒脌茂碌脛do_write脦陋0
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
            return;
        }
    }
    //脳垄脪芒脳脽碌陆脮芒脌茂碌脛脢卤潞貌拢卢潞贸露脣路垄脣脥碌脛脥路虏驴脨脨脨脜脧垄脪脩戮颅脭脷脟掳脙忙碌脛ngx_http_upstream_send_response->ngx_http_send_header脪脩戮颅掳脩脥路虏驴脨脨虏驴路脰路垄脣脥赂酶驴脥禄搂露脣脕脣
    //赂脙潞炉脢媒麓娄脌铆碌脛脰禄脢脟潞贸露脣路脜禄脴鹿媒脌麓碌脛脥酶脪鲁掳眉脤氓虏驴路脰

    ngx_http_upstream_process_request(r, u);
}

//ngx_http_upstream_init_request->ngx_http_upstream_cache 驴脥禄搂露脣禄帽脠隆禄潞麓忙 潞贸露脣脫娄麓冒禄脴脌麓脢媒戮脻潞贸脭脷ngx_http_file_cache_create脰脨麓麓陆篓脕脵脢卤脦脛录镁
//潞贸露脣禄潞麓忙脦脛录镁麓麓陆篓脭脷ngx_http_upstream_send_response拢卢潞贸露脣脫娄麓冒脢媒戮脻脭脷ngx_http_upstream_send_response->ngx_http_upstream_process_request->ngx_http_file_cache_update脰脨陆酶脨脨禄潞麓忙
static void
ngx_http_upstream_process_request(ngx_http_request_t *r,
    ngx_http_upstream_t *u)
{//脳垄脪芒脳脽碌陆脮芒脌茂碌脛脢卤潞貌拢卢潞贸露脣路垄脣脥碌脛脥路虏驴脨脨脨脜脧垄脪脩戮颅脭脷脟掳脙忙碌脛ngx_http_upstream_send_response->ngx_http_send_header脪脩戮颅掳脩脥路虏驴脨脨虏驴路脰路垄脣脥赂酶驴脥禄搂露脣脕脣
//赂脙潞炉脢媒麓娄脌铆碌脛脰禄脢脟潞贸露脣路脜禄脴鹿媒脌麓碌脛脥酶脪鲁掳眉脤氓虏驴路脰
    ngx_temp_file_t   *tf;
    ngx_event_pipe_t  *p;

    p = u->pipe;

    if (u->peer.connection) {

        if (u->store) {

            if (p->upstream_eof || p->upstream_done) { //卤戮麓脦脛脷潞脣禄潞鲁氓脟酶脢媒戮脻露脕脠隆脥锚卤脧拢卢禄貌脮脽潞贸露脣脣霉脫脨脢媒戮脻露脕脠隆脥锚卤脧

                tf = p->temp_file;

                if (u->headers_in.status_n == NGX_HTTP_OK
                    && (p->upstream_done || p->length == -1)
                    && (u->headers_in.content_length_n == -1
                        || u->headers_in.content_length_n == tf->offset))
                {
                    ngx_http_upstream_store(r, u);
                }
            }
        }

#if (NGX_HTTP_CACHE)
        tf = p->temp_file;

        if(r->cache) {
        ngx_log_debugall(r->connection->log, 0, "ngx http cache, p->length:%O, u->headers_in.content_length_n:%O, "
            "tf->offset:%O, r->cache->body_start:%ui", p->length, u->headers_in.content_length_n,
                tf->offset, r->cache->body_start);
        } else {
            ngx_log_debugall(r->connection->log, 0, "ngx http cache, p->length:%O, u->headers_in.content_length_n:%O, "
            "tf->offset:%O", p->length, u->headers_in.content_length_n,
                tf->offset);
        }
        
        /*
          脭脷Nginx脢脮碌陆潞贸露脣路镁脦帽脝梅碌脛脧矛脫娄脰庐潞贸拢卢禄谩掳脩脮芒赂枚脧矛脫娄路垄禄脴赂酶脫脙禄搂隆拢露酶脠莽鹿没禄潞麓忙鹿娄脛脺脝么脫脙碌脛禄掳拢卢Nginx戮脥禄谩掳脩脧矛脫娄麓忙脠毛麓脜脜脤脌茂隆拢
          */ //潞贸露脣脫娄麓冒脢媒戮脻脭脷ngx_http_upstream_process_request->ngx_http_file_cache_update脰脨陆酶脨脨禄潞麓忙
        if (u->cacheable) { //脢脟路帽脪陋禄潞麓忙拢卢录麓proxy_no_cache脰赂脕卯  

            if (p->upstream_done) { //潞贸露脣脢媒戮脻脪脩戮颅露脕脠隆脥锚卤脧,脨麓脠毛禄潞麓忙
                ngx_http_file_cache_update(r, p->temp_file);

            } else if (p->upstream_eof) { //p->upstream->recv_chain(p->upstream, chain, limit);路碌禄脴0碌脛脢卤潞貌脰脙1
                        
                if (p->length == -1
                    && (u->headers_in.content_length_n == -1
                        || u->headers_in.content_length_n
                           == tf->offset - (off_t) r->cache->body_start))
                {
                    ngx_http_file_cache_update(r, tf);

                } else {
                    ngx_http_file_cache_free(r->cache, tf);
                }

            } else if (p->upstream_error) {
                ngx_http_file_cache_free(r->cache, p->temp_file);
            }
        }

#endif
        size_t upstream_done = p->upstream_done;
        size_t upstream_eof = p->upstream_eof;
        size_t upstream_error = p->upstream_error;
        size_t downstream_error = p->downstream_error;
          
        ngx_log_debugall(r->connection->log, 0, "ngx http cache, upstream_done:%z, upstream_eof:%z, "
            "upstream_error:%z, downstream_error:%z", upstream_done, upstream_eof, 
            upstream_error, downstream_error);
            
        if (p->upstream_done || p->upstream_eof || p->upstream_error) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http upstream exit: %p", p->out);

            if (p->upstream_done
                || (p->upstream_eof && p->length == -1))
            {
                ngx_http_upstream_finalize_request(r, u, 0);
                return;
            }

            if (p->upstream_eof) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "upstream prematurely closed connection");
            }

            ngx_http_upstream_finalize_request(r, u, NGX_HTTP_BAD_GATEWAY);
            return;
        }
    }

    if (p->downstream_error) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http upstream downstream error");

        if (!u->cacheable && !u->store && u->peer.connection) {
            ngx_http_upstream_finalize_request(r, u, NGX_ERROR);
        }
    }
}


static void
ngx_http_upstream_store(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    size_t                  root;
    time_t                  lm;
    ngx_str_t               path;
    ngx_temp_file_t        *tf;
    ngx_ext_rename_file_t   ext;

    tf = u->pipe->temp_file;

    if (tf->file.fd == NGX_INVALID_FILE) {

        /* create file for empty 200 response */

        tf = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
        if (tf == NULL) {
            return;
        }

        tf->file.fd = NGX_INVALID_FILE;
        tf->file.log = r->connection->log;
        tf->path = u->conf->temp_path;
        tf->pool = r->pool;
        tf->persistent = 1;

        if (ngx_create_temp_file(&tf->file, tf->path, tf->pool,
                                 tf->persistent, tf->clean, tf->access)
            != NGX_OK)
        {
            return;
        }

        u->pipe->temp_file = tf;
    }

    ext.access = u->conf->store_access;
    ext.path_access = u->conf->store_access;
    ext.time = -1;
    ext.create_path = 1;
    ext.delete_file = 1;
    ext.log = r->connection->log;

    if (u->headers_in.last_modified) {

        lm = ngx_parse_http_time(u->headers_in.last_modified->value.data,
                                 u->headers_in.last_modified->value.len);

        if (lm != NGX_ERROR) {
            ext.time = lm;
            ext.fd = tf->file.fd;
        }
    }

    if (u->conf->store_lengths == NULL) {

        if (ngx_http_map_uri_to_path(r, &path, &root, 0) == NULL) {
            return;
        }

    } else {
        if (ngx_http_script_run(r, &path, u->conf->store_lengths->elts, 0,
                                u->conf->store_values->elts)
            == NULL)
        {
            return;
        }
    }

    path.len--;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "upstream stores \"%s\" to \"%s\"",
                   tf->file.name.data, path.data);

    (void) ngx_ext_rename_file(&tf->file.name, &path, &ext);

    u->store = 0;
}


static void
ngx_http_upstream_dummy_handler(ngx_http_request_t *r, ngx_http_upstream_t *u)
{
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http upstream dummy handler");
}

//脠莽鹿没虏芒脢脭脢搂掳脺拢卢碌梅脫脙ngx_http_upstream_next潞炉脢媒拢卢脮芒赂枚潞炉脢媒驴脡脛脺脭脵麓脦碌梅脫脙peer.get碌梅脫脙卤冒碌脛潞贸露脣路镁脦帽脝梅陆酶脨脨脕卢陆脫隆拢
static void // ngx_http_upstream_next 路陆路篓鲁垄脢脭脫毛脝盲脣没脡脧脫脦路镁脦帽脝梅陆篓脕垄脕卢陆脫  脢脳脧脠脨猫脪陋赂霉戮脻潞贸露脣路碌禄脴碌脛status潞脥鲁卢脢卤碌脠脨脜脧垄脌麓脜脨露脧脢脟路帽脨猫脪陋脰脴脨脗脕卢陆脫脧脗脪禄赂枚潞贸露脣路镁脦帽脝梅
ngx_http_upstream_next(ngx_http_request_t *r, ngx_http_upstream_t *u,
    ngx_uint_t ft_type) //潞脥潞贸露脣脛鲁赂枚路镁脦帽脝梅陆禄禄楼鲁枚麓铆(脌媒脠莽connect)拢卢脭貌脩隆脭帽脧脗脪禄赂枚潞贸露脣路镁脦帽脝梅拢卢脥卢脢卤卤锚录脟赂脙路镁脦帽脝梅鲁枚麓铆
{
    ngx_msec_t  timeout;
    ngx_uint_t  status, state;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http next upstream, %xi", ft_type);

    if (u->peer.sockaddr) {

        if (ft_type == NGX_HTTP_UPSTREAM_FT_HTTP_403
            || ft_type == NGX_HTTP_UPSTREAM_FT_HTTP_404) //潞贸露脣路镁脦帽脝梅戮脺戮酶路镁脦帽拢卢卤铆脢戮禄鹿脢脟驴脡脫脙碌脛拢卢脰禄脢脟戮脺戮酶脕脣碌卤脟掳脟毛脟贸
        {
            state = NGX_PEER_NEXT;

        } else {
            state = NGX_PEER_FAILED;
        }

        u->peer.free(&u->peer, u->peer.data, state);
        u->peer.sockaddr = NULL;
    }

    if (ft_type == NGX_HTTP_UPSTREAM_FT_TIMEOUT) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, NGX_ETIMEDOUT,
                      "upstream timed out");
    }

    if (u->peer.cached && ft_type == NGX_HTTP_UPSTREAM_FT_ERROR
        && (!u->request_sent || !r->request_body_no_buffering))
    {
        status = 0;

        /* TODO: inform balancer instead */

        u->peer.tries++;

    } else {
        switch (ft_type) {

        case NGX_HTTP_UPSTREAM_FT_TIMEOUT:
            status = NGX_HTTP_GATEWAY_TIME_OUT;
            break;

        case NGX_HTTP_UPSTREAM_FT_HTTP_500:
            status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            break;

        case NGX_HTTP_UPSTREAM_FT_HTTP_403:
            status = NGX_HTTP_FORBIDDEN;
            break;

        case NGX_HTTP_UPSTREAM_FT_HTTP_404:
            status = NGX_HTTP_NOT_FOUND;
            break;

        /*
         * NGX_HTTP_UPSTREAM_FT_BUSY_LOCK and NGX_HTTP_UPSTREAM_FT_MAX_WAITING
         * never reach here
         */

        default:
            status = NGX_HTTP_BAD_GATEWAY;
        }
    }

    if (r->connection->error) {
        ngx_http_upstream_finalize_request(r, u,
                                           NGX_HTTP_CLIENT_CLOSED_REQUEST);
        return;
    }

    if (status) {
        u->state->status = status;
        timeout = u->conf->next_upstream_timeout;

        if (u->peer.tries == 0
            || !(u->conf->next_upstream & ft_type)
            || (u->request_sent && r->request_body_no_buffering)
            || (timeout && ngx_current_msec - u->peer.start_time >= timeout))  //脜脨露脧脢脟路帽脨猫脪陋脰脴脨脗脕卢陆脫脧脗脪禄赂枚潞贸露脣路镁脦帽脝梅拢卢虏禄脨猫脪陋脭貌脰卤陆脫路碌禄脴麓铆脦贸赂酶驴脥禄搂露脣
        {
#if (NGX_HTTP_CACHE)

            if (u->cache_status == NGX_HTTP_CACHE_EXPIRED
                && (u->conf->cache_use_stale & ft_type))
            {
                ngx_int_t  rc;

                rc = u->reinit_request(r);

                if (rc == NGX_OK) {
                    u->cache_status = NGX_HTTP_CACHE_STALE;
                    rc = ngx_http_upstream_cache_send(r, u);
                }

                ngx_http_upstream_finalize_request(r, u, rc);
                return;
            }
#endif

            ngx_http_upstream_finalize_request(r, u, status);
            return;
        }
    }

    if (u->peer.connection) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "close http upstream connection: %d",
                       u->peer.connection->fd);
#if (NGX_HTTP_SSL)

        if (u->peer.connection->ssl) {
            u->peer.connection->ssl->no_wait_shutdown = 1;
            u->peer.connection->ssl->no_send_shutdown = 1;

            (void) ngx_ssl_shutdown(u->peer.connection);
        }
#endif

        if (u->peer.connection->pool) {
            ngx_destroy_pool(u->peer.connection->pool);
        }

        ngx_close_connection(u->peer.connection);
        u->peer.connection = NULL;
    }

    ngx_http_upstream_connect(r, u);//脭脵麓脦路垄脝冒脕卢陆脫
}


static void
ngx_http_upstream_cleanup(void *data)
{
    ngx_http_request_t *r = data;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "cleanup http upstream request: \"%V\"", &r->uri);

    ngx_http_upstream_finalize_request(r, r->upstream, NGX_DONE);
}

//ngx_http_upstream_create麓麓陆篓ngx_http_upstream_t拢卢脳脢脭麓禄脴脢脮脫脙ngx_http_upstream_finalize_request
static void
ngx_http_upstream_finalize_request(ngx_http_request_t *r,
    ngx_http_upstream_t *u, ngx_int_t rc)
{
    ngx_uint_t  flush;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "finalize http upstream request rc: %i", rc);

    if (u->cleanup == NULL) {
        /* the request was already finalized */
        ngx_http_finalize_request(r, NGX_DONE);
        return;
    }

    *u->cleanup = NULL;
    u->cleanup = NULL;

    if (u->resolved && u->resolved->ctx) {
        ngx_resolve_name_done(u->resolved->ctx);
        u->resolved->ctx = NULL;
    }

    if (u->state && u->state->response_time) {
        u->state->response_time = ngx_current_msec - u->state->response_time;

        if (u->pipe && u->pipe->read_length) {
            u->state->response_length = u->pipe->read_length;
        }
    }

    u->finalize_request(r, rc);

    if (u->peer.free && u->peer.sockaddr) {
        u->peer.free(&u->peer, u->peer.data, 0);
        u->peer.sockaddr = NULL;
    }

    if (u->peer.connection) { //脠莽鹿没脢脟脡猫脰脙脕脣keepalive num脜盲脰脙拢卢脭貌脭脷ngx_http_upstream_free_keepalive_peer脰脨禄谩掳脩u->peer.connection脰脙脦陋NULL,卤脺脙芒鹿脴卤脮脕卢陆脫拢卢禄潞麓忙脝冒脌麓卤脺脙芒脰脴赂麓陆篓脕垄潞脥鹿脴卤脮脕卢陆脫

#if (NGX_HTTP_SSL)

        /* TODO: do not shutdown persistent connection */

        if (u->peer.connection->ssl) {

            /*
             * We send the "close notify" shutdown alert to the upstream only
             * and do not wait its "close notify" shutdown alert.
             * It is acceptable according to the TLS standard.
             */

            u->peer.connection->ssl->no_wait_shutdown = 1;

            (void) ngx_ssl_shutdown(u->peer.connection);
        }
#endif

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "close http upstream connection: %d",
                       u->peer.connection->fd);

        if (u->peer.connection->pool) {
            ngx_destroy_pool(u->peer.connection->pool);
        }

        ngx_close_connection(u->peer.connection);
    }

    u->peer.connection = NULL;

    if (u->pipe && u->pipe->temp_file) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http upstream temp fd: %d",
                       u->pipe->temp_file->file.fd);
    }

    if (u->store && u->pipe && u->pipe->temp_file
        && u->pipe->temp_file->file.fd != NGX_INVALID_FILE)
    {
        if (ngx_delete_file(u->pipe->temp_file->file.name.data)
            == NGX_FILE_ERROR)
        {
            ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno,
                          ngx_delete_file_n " \"%s\" failed",
                          u->pipe->temp_file->file.name.data);
        }
    }

#if (NGX_HTTP_CACHE)

    if (r->cache) {

        if (u->cacheable) {

            if (rc == NGX_HTTP_BAD_GATEWAY || rc == NGX_HTTP_GATEWAY_TIME_OUT) {
                time_t  valid;

                valid = ngx_http_file_cache_valid(u->conf->cache_valid, rc);

                if (valid) {
                    r->cache->valid_sec = ngx_time() + valid;
                    r->cache->error = rc;
                }
            }
        }
        
        ngx_http_file_cache_free(r->cache, u->pipe->temp_file);
    }

#endif

    if (r->subrequest_in_memory
        && u->headers_in.status_n >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        u->buffer.last = u->buffer.pos;
    }

    if (rc == NGX_DECLINED) {
        return;
    }

    r->connection->log->action = "sending to client";

    if (!u->header_sent
        || rc == NGX_HTTP_REQUEST_TIME_OUT
        || rc == NGX_HTTP_CLIENT_CLOSED_REQUEST)
    {
        ngx_http_finalize_request(r, rc);
        return;
    }

    flush = 0;

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        rc = NGX_ERROR;
        flush = 1;
    }

    if (r->header_only) { //脰禄路垄脣脥脥路虏驴脨脨拢卢脟毛脟贸潞贸露脣碌脛脥路虏驴脨脨脭脷ngx_http_upstream_send_response->ngx_http_send_header脪脩戮颅路垄脣脥
        ngx_http_finalize_request(r, rc);
        return;
    }

    if (rc == 0) { //脣碌脙梅脢脟NGX_OK
        rc = ngx_http_send_special(r, NGX_HTTP_LAST);

    } else if (flush) {
        r->keepalive = 0;
        rc = ngx_http_send_special(r, NGX_HTTP_FLUSH);
    }

    ngx_http_finalize_request(r, rc); //ngx_http_upstream_finalize_request->ngx_http_finalize_request
}


static ngx_int_t
ngx_http_upstream_process_header_line(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_table_elt_t  **ph;

    ph = (ngx_table_elt_t **) ((char *) &r->upstream->headers_in + offset);

    if (*ph == NULL) { //脮芒赂枚戮脥脢脟录谩鲁脰r->upstream->headers_in脰脨碌脛offset鲁脡脭卤麓娄脢脟路帽脫脨麓忙路脜赂脙ngx_table_elt_t碌脛驴脮录盲
        *ph = h;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_ignore_header_line(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_content_length(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

    u->headers_in.content_length = h;
    u->headers_in.content_length_n = ngx_atoof(h->value.data, h->value.len);

    return NGX_OK;
}

//掳脩脳脰路没麓庐脢卤录盲"2014-12-22 12:03:44"脳陋禄禄脦陋time_t脢卤录盲麓锚
static ngx_int_t
ngx_http_upstream_process_last_modified(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;

    u->headers_in.last_modified = h;

#if (NGX_HTTP_CACHE)

    if (u->cacheable) {
        u->headers_in.last_modified_time = ngx_parse_http_time(h->value.data,
                                                               h->value.len);
    }

#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_set_cookie(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_array_t           *pa;
    ngx_table_elt_t      **ph;
    ngx_http_upstream_t   *u;

    u = r->upstream;
    pa = &u->headers_in.cookies;

    if (pa->elts == NULL) {
        if (ngx_array_init(pa, r->pool, 1, sizeof(ngx_table_elt_t *)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    ph = ngx_array_push(pa);
    if (ph == NULL) {
        return NGX_ERROR;
    }

    *ph = h;

#if (NGX_HTTP_CACHE)
    if (!(u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_SET_COOKIE)) {
        u->cacheable = 0;
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_array_t          *pa;
    ngx_table_elt_t     **ph;
    ngx_http_upstream_t  *u;

    u = r->upstream;
    pa = &u->headers_in.cache_control;

    if (pa->elts == NULL) {
       if (ngx_array_init(pa, r->pool, 2, sizeof(ngx_table_elt_t *)) != NGX_OK)
       {
           return NGX_ERROR;
       }
    }

    ph = ngx_array_push(pa);
    if (ph == NULL) {
        return NGX_ERROR;
    }

    *ph = h;

#if (NGX_HTTP_CACHE)
    {
    u_char     *p, *start, *last;
    ngx_int_t   n;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_CACHE_CONTROL) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    if (r->cache->valid_sec != 0 && u->headers_in.x_accel_expires != NULL) {
        return NGX_OK;
    }

    start = h->value.data;
    last = start + h->value.len;

    //脠莽鹿没Cache-Control虏脦脢媒脰碌脦陋no-cache隆垄no-store隆垄private脰脨脠脦脪芒脪禄赂枚脢卤拢卢脭貌虏禄禄潞麓忙...虏禄禄潞麓忙...
    if (ngx_strlcasestrn(start, last, (u_char *) "no-cache", 8 - 1) != NULL
        || ngx_strlcasestrn(start, last, (u_char *) "no-store", 8 - 1) != NULL
        || ngx_strlcasestrn(start, last, (u_char *) "private", 7 - 1) != NULL)
    {
        u->cacheable = 0;
        return NGX_OK;
    }

    
    //脠莽鹿没Cache-Control虏脦脢媒脰碌脦陋max-age脢卤拢卢禄谩卤禄禄潞麓忙拢卢脟脪nginx脡猫脰脙碌脛cache碌脛鹿媒脝脷脢卤录盲拢卢戮脥脢脟脧碌脥鲁碌卤脟掳脢卤录盲 + mag-age碌脛脰碌
    p = ngx_strlcasestrn(start, last, (u_char *) "s-maxage=", 9 - 1);
    offset = 9;

    if (p == NULL) {
        p = ngx_strlcasestrn(start, last, (u_char *) "max-age=", 8 - 1);
        offset = 8;
    }

    if (p == NULL) {
        return NGX_OK;
    }

    n = 0;

    for (p += offset; p < last; p++) {
        if (*p == ',' || *p == ';' || *p == ' ') {
            break;
        }

        if (*p >= '0' && *p <= '9') {
            n = n * 10 + *p - '0';
            continue;
        }

        u->cacheable = 0;
        return NGX_OK;
    }

    if (n == 0) {
        u->cacheable = 0;
        return NGX_OK;
    }

    r->cache->valid_sec = ngx_time() + n;
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_expires(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.expires = h;

#if (NGX_HTTP_CACHE)
    {
    time_t  expires;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_EXPIRES) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    if (r->cache->valid_sec != 0) {
        return NGX_OK;
    }

    expires = ngx_parse_http_time(h->value.data, h->value.len);

    if (expires == NGX_ERROR || expires < ngx_time()) {
        u->cacheable = 0;
        return NGX_OK;
    }

    r->cache->valid_sec = expires;
    }
#endif

    return NGX_OK;
}

//虏脦驴录http://blog.csdn.net/clh604/article/details/9064641
static ngx_int_t
ngx_http_upstream_process_accel_expires(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.x_accel_expires = h; //潞贸露脣脨炉麓酶脫脨"x_accel_expires"脥路虏驴脨脨

#if (NGX_HTTP_CACHE)
    {
    u_char     *p;
    size_t      len;
    ngx_int_t   n;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_EXPIRES) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    len = h->value.len;
    p = h->value.data;

    if (p[0] != '@') {
        n = ngx_atoi(p, len);

        switch (n) {
        case 0:
            u->cacheable = 0;
            /* fall through */

        case NGX_ERROR:
            return NGX_OK;

        default:
            r->cache->valid_sec = ngx_time() + n;
            return NGX_OK;
        }
    }

    p++;
    len--;

    n = ngx_atoi(p, len);

    if (n != NGX_ERROR) {
        r->cache->valid_sec = n;
    }
    }
#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_limit_rate(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_int_t             n;
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.x_accel_limit_rate = h;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_LIMIT_RATE) {
        return NGX_OK;
    }

    n = ngx_atoi(h->value.data, h->value.len);

    if (n != NGX_ERROR) {
        r->limit_rate = (size_t) n;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_buffering(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    u_char                c0, c1, c2;
    ngx_http_upstream_t  *u;

    u = r->upstream;

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_BUFFERING) {
        return NGX_OK;
    }

    if (u->conf->change_buffering) {

        if (h->value.len == 2) {
            c0 = ngx_tolower(h->value.data[0]);
            c1 = ngx_tolower(h->value.data[1]);

            if (c0 == 'n' && c1 == 'o') {
                u->buffering = 0;
            }

        } else if (h->value.len == 3) {
            c0 = ngx_tolower(h->value.data[0]);
            c1 = ngx_tolower(h->value.data[1]);
            c2 = ngx_tolower(h->value.data[2]);

            if (c0 == 'y' && c1 == 'e' && c2 == 's') {
                u->buffering = 1;
            }
        }
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_charset(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    if (r->upstream->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_XA_CHARSET) {
        return NGX_OK;
    }

    r->headers_out.override_charset = &h->value;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_connection(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    r->upstream->headers_in.connection = h;

    if (ngx_strlcasestrn(h->value.data, h->value.data + h->value.len,
                         (u_char *) "close", 5 - 1)
        != NULL)
    {
        r->upstream->headers_in.connection_close = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_process_transfer_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    r->upstream->headers_in.transfer_encoding = h;

    if (ngx_strlcasestrn(h->value.data, h->value.data + h->value.len,
                         (u_char *) "chunked", 7 - 1)
        != NULL)
    {
        r->upstream->headers_in.chunked = 1;
    }

    return NGX_OK;
}

//潞贸露脣路碌禄脴碌脛脥路虏驴脨脨麓酶脫脨vary:xxx  ngx_http_upstream_process_vary
static ngx_int_t
ngx_http_upstream_process_vary(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_http_upstream_t  *u;

    u = r->upstream;
    u->headers_in.vary = h;

#if (NGX_HTTP_CACHE)

    if (u->conf->ignore_headers & NGX_HTTP_UPSTREAM_IGN_VARY) {
        return NGX_OK;
    }

    if (r->cache == NULL) {
        return NGX_OK;
    }

    if (h->value.len > NGX_HTTP_CACHE_VARY_LEN
        || (h->value.len == 1 && h->value.data[0] == '*'))
    {
        u->cacheable = 0;
    }

    r->cache->vary = h->value;

#endif

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_header_line(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_table_elt_t  *ho, **ph;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (offset) {
        ph = (ngx_table_elt_t **) ((char *) &r->headers_out + offset);
        *ph = ho;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_multi_header_lines(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_array_t      *pa;
    ngx_table_elt_t  *ho, **ph;

    pa = (ngx_array_t *) ((char *) &r->headers_out + offset);

    if (pa->elts == NULL) {
        if (ngx_array_init(pa, r->pool, 2, sizeof(ngx_table_elt_t *)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    ph = ngx_array_push(pa);
    if (ph == NULL) {
        return NGX_ERROR;
    }

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;
    *ph = ho;

    return NGX_OK;
}

//Content-Type:text/html;charset=ISO-8859-1陆芒脦枚卤脿脗毛路陆脢陆麓忙碌陆r->headers_out.charset
static ngx_int_t
ngx_http_upstream_copy_content_type(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    u_char  *p, *last;

    r->headers_out.content_type_len = h->value.len;
    r->headers_out.content_type = h->value;
    r->headers_out.content_type_lowcase = NULL;

    for (p = h->value.data; *p; p++) {

        if (*p != ';') {
            continue;
        }

        last = p;

        while (*++p == ' ') { /* void */ }

        if (*p == '\0') {
            return NGX_OK;
        }

        if (ngx_strncasecmp(p, (u_char *) "charset=", 8) != 0) {
            continue;
        }

        p += 8;

        r->headers_out.content_type_len = last - h->value.data;

        if (*p == '"') {
            p++;
        }

        last = h->value.data + h->value.len;

        if (*(last - 1) == '"') {
            last--;
        }

        r->headers_out.charset.len = last - p;
        r->headers_out.charset.data = p;

        return NGX_OK;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_last_modified(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    r->headers_out.last_modified = ho;

#if (NGX_HTTP_CACHE)

    if (r->upstream->cacheable) {
        r->headers_out.last_modified_time =
                                    r->upstream->headers_in.last_modified_time;
    }

#endif

    return NGX_OK;
}

//脠莽鹿没陆脫脢脺碌陆碌脛潞贸露脣脥路虏驴脨脨脰脨脰赂露篓脫脨location:xxx脥路虏驴脨脨拢卢脭貌脨猫脪陋陆酶脨脨脰脴露篓脧貌拢卢虏脦驴录proxy_redirect
/*
location /proxy1/ {			
    proxy_pass  http://10.10.0.103:8080/; 		
}

脠莽鹿没url脦陋http://10.2.13.167/proxy1/拢卢脭貌ngx_http_upstream_rewrite_location麓娄脌铆潞贸拢卢
潞贸露脣路碌禄脴Location: http://10.10.0.103:8080/secure/MyJiraHome.jspa
脭貌脢碌录脢路垄脣脥赂酶盲炉脌脌脝梅驴脥禄搂露脣碌脛headers_out.headers.location脦陋http://10.2.13.167/proxy1/secure/MyJiraHome.jspa
*/
static ngx_int_t
ngx_http_upstream_rewrite_location(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset) //ngx_http_upstream_headers_in脰脨碌脛鲁脡脭卤copy_handler
{
    ngx_int_t         rc;
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (r->upstream->rewrite_redirect) {
        rc = r->upstream->rewrite_redirect(r, ho, 0); //ngx_http_proxy_rewrite_redirect

        if (rc == NGX_DECLINED) {
            return NGX_OK;
        }

        if (rc == NGX_OK) {
            r->headers_out.location = ho;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rewritten location: \"%V\"", &ho->value);
        }

        return rc;
    }

    if (ho->value.data[0] != '/') {
        r->headers_out.location = ho;
    }

    /*
     * we do not set r->headers_out.location here to avoid the handling
     * the local redirects without a host name by ngx_http_header_filter()
     */

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_rewrite_refresh(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    u_char           *p;
    ngx_int_t         rc;
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (r->upstream->rewrite_redirect) {

        p = ngx_strcasestrn(ho->value.data, "url=", 4 - 1);

        if (p) {
            rc = r->upstream->rewrite_redirect(r, ho, p + 4 - ho->value.data);

        } else {
            return NGX_OK;
        }

        if (rc == NGX_DECLINED) {
            return NGX_OK;
        }

        if (rc == NGX_OK) {
            r->headers_out.refresh = ho;

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rewritten refresh: \"%V\"", &ho->value);
        }

        return rc;
    }

    r->headers_out.refresh = ho;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_rewrite_set_cookie(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    ngx_int_t         rc;
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    if (r->upstream->rewrite_cookie) {
        rc = r->upstream->rewrite_cookie(r, ho);

        if (rc == NGX_DECLINED) {
            return NGX_OK;
        }

#if (NGX_DEBUG)
        if (rc == NGX_OK) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "rewritten cookie: \"%V\"", &ho->value);
        }
#endif

        return rc;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_copy_allow_ranges(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_table_elt_t  *ho;

    if (r->upstream->conf->force_ranges) {
        return NGX_OK;
    }

#if (NGX_HTTP_CACHE)

    if (r->cached) {
        r->allow_ranges = 1;
        return NGX_OK;
    }

    if (r->upstream->cacheable) {
        r->allow_ranges = 1;
        r->single_range = 1;
        return NGX_OK;
    }

#endif

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    r->headers_out.accept_ranges = ho;

    return NGX_OK;
}


#if (NGX_HTTP_GZIP)

static ngx_int_t
ngx_http_upstream_copy_content_encoding(ngx_http_request_t *r,
    ngx_table_elt_t *h, ngx_uint_t offset)
{
    ngx_table_elt_t  *ho;

    ho = ngx_list_push(&r->headers_out.headers);
    if (ho == NULL) {
        return NGX_ERROR;
    }

    *ho = *h;

    r->headers_out.content_encoding = ho;

    return NGX_OK;
}

#endif


static ngx_int_t
ngx_http_upstream_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_upstream_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_addr_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = 0;
    state = r->upstream_states->elts;

    for (i = 0; i < r->upstream_states->nelts; i++) {
        if (state[i].peer) {
            len += state[i].peer->len + 2;

        } else {
            len += 3;
        }
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;

    for ( ;; ) {
        if (state[i].peer) {
            p = ngx_cpymem(p, state[i].peer->data, state[i].peer->len);
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_status_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (3 + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        if (state[i].status) {
            p = ngx_sprintf(p, "%ui", state[i].status);

        } else {
            *p++ = '-';
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_response_time_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_msec_int_t              ms;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (NGX_TIME_T_LEN + 4 + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        if (state[i].status) {

            if (data == 1 && state[i].header_time != (ngx_msec_t) -1) {
                ms = state[i].header_time;

            } else if (data == 2 && state[i].connect_time != (ngx_msec_t) -1) {
                ms = state[i].connect_time;

            } else {
                ms = state[i].response_time;
            }

            ms = ngx_max(ms, 0);
            p = ngx_sprintf(p, "%T.%03M", (time_t) ms / 1000, ms % 1000);

        } else {
            *p++ = '-';
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_response_length_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (NGX_OFF_T_LEN + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        p = ngx_sprintf(p, "%O", state[i].response_length);

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;

    return NGX_OK;
}


ngx_int_t
ngx_http_upstream_header_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->upstream == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    return ngx_http_variable_unknown_header(v, (ngx_str_t *) data,
                                         &r->upstream->headers_in.headers.part,
                                         sizeof("upstream_http_") - 1);
}


ngx_int_t
ngx_http_upstream_cookie_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  *name = (ngx_str_t *) data;

    ngx_str_t   cookie, s;

    if (r->upstream == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    s.len = name->len - (sizeof("upstream_cookie_") - 1);
    s.data = name->data + sizeof("upstream_cookie_") - 1;

    if (ngx_http_parse_set_cookie_lines(&r->upstream->headers_in.cookies,
                                        &s, &cookie)
        == NGX_DECLINED)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    v->len = cookie.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = cookie.data;

    return NGX_OK;
}


#if (NGX_HTTP_CACHE)

ngx_int_t
ngx_http_upstream_cache_status(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t  n;

    if (r->upstream == NULL || r->upstream->cache_status == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    n = r->upstream->cache_status - 1;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = ngx_http_cache_status[n].len;
    v->data = ngx_http_cache_status[n].data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_cache_last_modified(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p;

    if (r->upstream == NULL
        || !r->upstream->conf->cache_revalidate
        || r->upstream->cache_status != NGX_HTTP_CACHE_EXPIRED
        || r->cache->last_modified == -1)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    p = ngx_pnalloc(r->pool, sizeof("Mon, 28 Sep 1970 06:00:00 GMT") - 1);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = ngx_http_time(p, r->cache->last_modified) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_cache_etag(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->upstream == NULL
        || !r->upstream->conf->cache_revalidate
        || r->upstream->cache_status != NGX_HTTP_CACHE_EXPIRED
        || r->cache->etag.len == 0)
    {
        v->not_found = 1;
        return NGX_OK;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = r->cache->etag.len;
    v->data = r->cache->etag.data;

    return NGX_OK;
}

#endif

/*
upstream backend {
    server backend1.example.com weight=5;
    server backend2.example.com:8080;
    server unix:/tmp/backend3;
}

server {
    location / {
        proxy_pass http://backend;
    }
}

max_fails=number

  脡猫脰脙脭脷fail_timeout虏脦脢媒脡猫脰脙碌脛脢卤录盲脛脷脳卯麓贸脢搂掳脺麓脦脢媒拢卢脠莽鹿没脭脷脮芒赂枚脢卤录盲脛脷拢卢脣霉脫脨脮毛露脭赂脙路镁脦帽脝梅碌脛脟毛脟贸
  露录脢搂掳脺脕脣拢卢脛脟脙麓脠脧脦陋赂脙路镁脦帽脝梅禄谩卤禄脠脧脦陋脢脟脥拢禄煤脕脣拢卢脥拢禄煤脢卤录盲脢脟fail_timeout脡猫脰脙碌脛脢卤录盲隆拢脛卢脠脧脟茅驴枚脧脗拢卢
  虏禄鲁脡鹿娄脕卢陆脫脢媒卤禄脡猫脰脙脦陋1隆拢卤禄脡猫脰脙脦陋脕茫脭貌卤铆脢戮虏禄陆酶脨脨脕麓陆脫脢媒脥鲁录脝隆拢脛脟脨漏脕卢陆脫卤禄脠脧脦陋脢脟虏禄鲁脡鹿娄碌脛驴脡脪脭脥篓鹿媒
  proxy_next_upstream, fastcgi_next_upstream拢卢潞脥memcached_next_upstream脰赂脕卯脜盲脰脙隆拢http_404
  脳麓脤卢虏禄禄谩卤禄脠脧脦陋脢脟虏禄鲁脡鹿娄碌脛鲁垄脢脭隆拢

fail_time=time
  脡猫脰脙 露脿鲁陇脢卤录盲脛脷脢搂掳脺麓脦脢媒麓茂碌陆脳卯麓贸脢搂掳脺麓脦脢媒禄谩卤禄脠脧脦陋路镁脦帽脝梅脥拢禄煤脕脣路镁脦帽脝梅禄谩卤禄脠脧脦陋脥拢禄煤碌脛脢卤录盲鲁陇露脠 脛卢脠脧脟茅驴枚脧脗拢卢鲁卢脢卤脢卤录盲卤禄脡猫脰脙脦陋10S

*/
static char *
ngx_http_upstream(ngx_conf_t *cf, ngx_command_t *cmd, void *dummy)
{//碌卤脜枚碌陆upstream{}脰赂脕卯碌脛脢卤潞貌碌梅脫脙脮芒脌茂隆拢
    char                          *rv;
    void                          *mconf;
    ngx_str_t                     *value;
    ngx_url_t                      u;
    ngx_uint_t                     m;
    ngx_conf_t                     pcf;
    ngx_http_module_t             *module;
    ngx_http_conf_ctx_t           *ctx, *http_ctx;
    ngx_http_upstream_srv_conf_t  *uscf;

    ngx_memzero(&u, sizeof(ngx_url_t));

    value = cf->args->elts;
    u.host = value[1]; //upstream backend { }脰脨碌脛backend
    u.no_resolve = 1;
    u.no_port = 1;

    //脧脗脙忙陆芦u麓煤卤铆碌脛脢媒戮脻脡猫脰脙碌陆umcf->upstreams脌茂脙忙脠楼隆拢脠禄潞贸路碌禄脴露脭脫娄碌脛upstream{}陆谩鹿鹿脢媒戮脻脰赂脮毛隆拢
    uscf = ngx_http_upstream_add(cf, &u, NGX_HTTP_UPSTREAM_CREATE
                                         |NGX_HTTP_UPSTREAM_WEIGHT
                                         |NGX_HTTP_UPSTREAM_MAX_FAILS
                                         |NGX_HTTP_UPSTREAM_FAIL_TIMEOUT
                                         |NGX_HTTP_UPSTREAM_DOWN
                                         |NGX_HTTP_UPSTREAM_BACKUP);
    if (uscf == NULL) {
        return NGX_CONF_ERROR;
    }


    ctx = ngx_pcalloc(cf->pool, sizeof(ngx_http_conf_ctx_t));
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    http_ctx = cf->ctx;
    ctx->main_conf = http_ctx->main_conf; //禄帽脠隆赂脙upstream xxx{}脣霉麓娄碌脛http{}

    /* the upstream{}'s srv_conf */

    ctx->srv_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->srv_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    ctx->srv_conf[ngx_http_upstream_module.ctx_index] = uscf;

    uscf->srv_conf = ctx->srv_conf;


    /* the upstream{}'s loc_conf */
    ctx->loc_conf = ngx_pcalloc(cf->pool, sizeof(void *) * ngx_http_max_module);
    if (ctx->loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    //赂脙upstream{}脰脨驴脡脪脭脜盲脰脙脣霉脫脨碌脛loc录露卤冒脛拢驴茅碌脛脜盲脰脙脨脜脧垄拢卢脪貌麓脣脦陋脙驴赂枚脛拢驴茅麓麓陆篓露脭脫娄碌脛麓忙麓垄驴脮录盲
    for (m = 0; ngx_modules[m]; m++) {
        if (ngx_modules[m]->type != NGX_HTTP_MODULE) {
            continue;
        }

        module = ngx_modules[m]->ctx;

        if (module->create_srv_conf) {
            mconf = module->create_srv_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->srv_conf[ngx_modules[m]->ctx_index] = mconf;
        }

        if (module->create_loc_conf) {
            mconf = module->create_loc_conf(cf);
            if (mconf == NULL) {
                return NGX_CONF_ERROR;
            }

            ctx->loc_conf[ngx_modules[m]->ctx_index] = mconf;
        }
    }

    uscf->servers = ngx_array_create(cf->pool, 4,
                                     sizeof(ngx_http_upstream_server_t));
    if (uscf->servers == NULL) {
        return NGX_CONF_ERROR;
    }


    /* parse inside upstream{} */

    pcf = *cf;   //卤拢麓忙upstream{}脣霉麓娄碌脛ctx
    cf->ctx = ctx;//脕脵脢卤脟脨禄禄ctx拢卢陆酶脠毛upstream{}驴茅脰脨陆酶脨脨陆芒脦枚隆拢
    cf->cmd_type = NGX_HTTP_UPS_CONF;

    rv = ngx_conf_parse(cf, NULL);

    *cf = pcf; //upstream{}脛脷虏驴脜盲脰脙陆芒脦枚脥锚卤脧潞贸拢卢禄脰赂麓碌陆脰庐脟掳碌脛cf

    if (rv != NGX_CONF_OK) {
        return rv;
    }

    if (uscf->servers->nelts == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "no servers are inside upstream");
        return NGX_CONF_ERROR;
    }

    return rv;
}

//server   127.0.0.1:8080          max_fails=3  fail_timeout=30s;
static char *
ngx_http_upstream_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf = conf;

    time_t                       fail_timeout;
    ngx_str_t                   *value, s;
    ngx_url_t                    u;
    ngx_int_t                    weight, max_fails;
    ngx_uint_t                   i;
    ngx_http_upstream_server_t  *us;

    us = ngx_array_push(uscf->servers);
    if (us == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(us, sizeof(ngx_http_upstream_server_t));

    value = cf->args->elts;

    weight = 1;
    max_fails = 1;
    fail_timeout = 10;

    for (i = 2; i < cf->args->nelts; i++) {

        if (ngx_strncmp(value[i].data, "weight=", 7) == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_WEIGHT)) {
                goto not_supported;
            }

            weight = ngx_atoi(&value[i].data[7], value[i].len - 7);

            if (weight == NGX_ERROR || weight == 0) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "max_fails=", 10) == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_MAX_FAILS)) {
                goto not_supported;
            }

            max_fails = ngx_atoi(&value[i].data[10], value[i].len - 10);

            if (max_fails == NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "fail_timeout=", 13) == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_FAIL_TIMEOUT)) {
                goto not_supported;
            }

            s.len = value[i].len - 13;
            s.data = &value[i].data[13];

            fail_timeout = ngx_parse_time(&s, 1);

            if (fail_timeout == (time_t) NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strcmp(value[i].data, "backup") == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_BACKUP)) {
                goto not_supported;
            }

            us->backup = 1;

            continue;
        }

        if (ngx_strcmp(value[i].data, "down") == 0) {

            if (!(uscf->flags & NGX_HTTP_UPSTREAM_DOWN)) {
                goto not_supported;
            }

            us->down = 1;

            continue;
        }

        goto invalid;
    }

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.default_port = 80;

    if (ngx_parse_url(cf->pool, &u) != NGX_OK) {
        if (u.err) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "%s in upstream \"%V\"", u.err, &u.url);
        }

        return NGX_CONF_ERROR;
    }

    us->name = u.url;
    us->addrs = u.addrs;
    us->naddrs = u.naddrs;
    us->weight = weight;
    us->max_fails = max_fails;
    us->fail_timeout = fail_timeout;

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid parameter \"%V\"", &value[i]);

    return NGX_CONF_ERROR;

not_supported:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "balancing method does not support parameter \"%V\"",
                       &value[i]);

    return NGX_CONF_ERROR;
}


ngx_http_upstream_srv_conf_t *
ngx_http_upstream_add(ngx_conf_t *cf, ngx_url_t *u, ngx_uint_t flags)
{
    ngx_uint_t                      i;
    ngx_http_upstream_server_t     *us;
    ngx_http_upstream_srv_conf_t   *uscf, **uscfp;
    ngx_http_upstream_main_conf_t  *umcf;

    if (!(flags & NGX_HTTP_UPSTREAM_CREATE)) {

        if (ngx_parse_url(cf->pool, u) != NGX_OK) { //陆芒脦枚uri拢卢脠莽鹿没uri脢脟IP:PORT脨脦脢陆脭貌禄帽脠隆脣没脙脟拢卢脠莽鹿没脢脟脫貌脙没www.xxx.com脨脦脢陆拢卢脭貌陆芒脦枚脫貌脙没
            if (u->err) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "%s in upstream \"%V\"", u->err, &u->url);
            }

            return NULL;
        }
    }

    umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_upstream_module);

    uscfp = umcf->upstreams.elts;

    //卤茅脌煤碌卤脟掳碌脛upstream拢卢脠莽鹿没脫脨脰脴赂麓碌脛拢卢脭貌卤脠陆脧脝盲脧脿鹿脴碌脛脳脰露脦拢卢虏垄麓貌脫隆脠脮脰戮隆拢脠莽鹿没脮脪碌陆脧脿脥卢碌脛拢卢脭貌路碌禄脴露脭脫娄脰赂脮毛隆拢脙禄脮脪碌陆脭貌脭脷潞贸脙忙麓麓陆篓
    for (i = 0; i < umcf->upstreams.nelts; i++) {

        if (uscfp[i]->host.len != u->host.len
            || ngx_strncasecmp(uscfp[i]->host.data, u->host.data, u->host.len)
               != 0)
        {
            continue;
        }

        if ((flags & NGX_HTTP_UPSTREAM_CREATE)
             && (uscfp[i]->flags & NGX_HTTP_UPSTREAM_CREATE))
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "duplicate upstream \"%V\"", &u->host);
            return NULL;
        }

        if ((uscfp[i]->flags & NGX_HTTP_UPSTREAM_CREATE) && !u->no_port) {
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                               "upstream \"%V\" may not have port %d",
                               &u->host, u->port);
            return NULL;
        }

        if ((flags & NGX_HTTP_UPSTREAM_CREATE) && !uscfp[i]->no_port) {
            ngx_log_error(NGX_LOG_WARN, cf->log, 0,
                          "upstream \"%V\" may not have port %d in %s:%ui",
                          &u->host, uscfp[i]->port,
                          uscfp[i]->file_name, uscfp[i]->line);
            return NULL;
        }

        if (uscfp[i]->port && u->port
            && uscfp[i]->port != u->port)
        {
            continue;
        }

        if (uscfp[i]->default_port && u->default_port
            && uscfp[i]->default_port != u->default_port)
        {
            continue;
        }

        if (flags & NGX_HTTP_UPSTREAM_CREATE) {
            uscfp[i]->flags = flags;
        }

        return uscfp[i];//脮脪碌陆脧脿脥卢碌脛脜盲脰脙脢媒戮脻脕脣拢卢脰卤陆脫路碌禄脴脣眉碌脛脰赂脮毛隆拢
    }

    uscf = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_srv_conf_t));
    if (uscf == NULL) {
        return NULL;
    }

    uscf->flags = flags;
    uscf->host = u->host;
    uscf->file_name = cf->conf_file->file.name.data; //脜盲脰脙脦脛录镁脙没鲁脝
    uscf->line = cf->conf_file->line;
    uscf->port = u->port;
    uscf->default_port = u->default_port;
    uscf->no_port = u->no_port;

    //卤脠脠莽: server xx.xx.xx.xx:xx weight=2 max_fails=3;  赂脮驴陋脢录拢卢ngx_http_upstream禄谩碌梅脫脙卤戮潞炉脢媒隆拢碌芦脢脟脝盲naddres=0.
    if (u->naddrs == 1 && (u->port || u->family == AF_UNIX)) {
        uscf->servers = ngx_array_create(cf->pool, 1,
                                         sizeof(ngx_http_upstream_server_t));
        if (uscf->servers == NULL) {
            return NULL;
        }

        us = ngx_array_push(uscf->servers);//录脟脗录卤戮upstream{}驴茅碌脛脣霉脫脨server脰赂脕卯隆拢
        if (us == NULL) {
            return NULL;
        }

        ngx_memzero(us, sizeof(ngx_http_upstream_server_t));

        us->addrs = u->addrs;
        us->naddrs = 1;
    }

    uscfp = ngx_array_push(&umcf->upstreams);
    if (uscfp == NULL) {
        return NULL;
    }

    *uscfp = uscf;

    return uscf;
}

//proxy_bind  fastcgi_bind
char *
ngx_http_upstream_bind_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
{
    char  *p = conf;

    ngx_int_t                           rc;
    ngx_str_t                          *value;
    ngx_http_complex_value_t            cv;
    ngx_http_upstream_local_t         **plocal, *local;
    ngx_http_compile_complex_value_t    ccv;

    plocal = (ngx_http_upstream_local_t **) (p + cmd->offset);

    if (*plocal != NGX_CONF_UNSET_PTR) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        *plocal = NULL;
        return NGX_CONF_OK;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &cv;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    local = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_local_t));
    if (local == NULL) {
        return NGX_CONF_ERROR;
    }

    *plocal = local;

    if (cv.lengths) {
        local->value = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
        if (local->value == NULL) {
            return NGX_CONF_ERROR;
        }

        *local->value = cv;

        return NGX_CONF_OK;
    }

    local->addr = ngx_palloc(cf->pool, sizeof(ngx_addr_t));
    if (local->addr == NULL) {
        return NGX_CONF_ERROR;
    }

    rc = ngx_parse_addr(cf->pool, local->addr, value[1].data, value[1].len);

    switch (rc) {
    case NGX_OK:
        local->addr->name = value[1];
        return NGX_CONF_OK;

    case NGX_DECLINED:
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid address \"%V\"", &value[1]);
        /* fall through */

    default:
        return NGX_CONF_ERROR;
    }
}


static ngx_addr_t *
ngx_http_upstream_get_local(ngx_http_request_t *r,
    ngx_http_upstream_local_t *local)
{
    ngx_int_t    rc;
    ngx_str_t    val;
    ngx_addr_t  *addr;

    if (local == NULL) {
        return NULL;
    }

    if (local->value == NULL) {
        return local->addr;
    }

    if (ngx_http_complex_value(r, local->value, &val) != NGX_OK) {
        return NULL;
    }

    if (val.len == 0) {
        return NULL;
    }

    addr = ngx_palloc(r->pool, sizeof(ngx_addr_t));
    if (addr == NULL) {
        return NULL;
    }

    rc = ngx_parse_addr(r->pool, addr, val.data, val.len);

    switch (rc) {
    case NGX_OK:
        addr->name = val;
        return addr;

    case NGX_DECLINED:
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "invalid local address \"%V\"", &val);
        /* fall through */

    default:
        return NULL;
    }
}

//fastcgi_param  Params脢媒戮脻掳眉拢卢脫脙脫脷麓芦碌脻脰麓脨脨脪鲁脙忙脣霉脨猫脪陋碌脛虏脦脢媒潞脥禄路戮鲁卤盲脕驴隆拢
char *
ngx_http_upstream_param_set_slot(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf) //uboot.din
{
    char  *p = conf;

    ngx_str_t                   *value;
    ngx_array_t                **a;
    ngx_http_upstream_param_t   *param;

    //fastcgi_param脡猫脰脙碌脛麓芦脣脥碌陆FastCGI路镁脦帽脝梅碌脛脧脿鹿脴虏脦脢媒露录脤铆录脫碌陆赂脙脢媒脳茅脰脨拢卢录没ngx_http_upstream_param_set_slot
    a = (ngx_array_t **) (p + cmd->offset);//ngx_http_fastcgi_loc_conf_t->params_source

    if (*a == NULL) {
        *a = ngx_array_create(cf->pool, 4, sizeof(ngx_http_upstream_param_t));
        if (*a == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    param = ngx_array_push(*a);
    if (param == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    param->key = value[1];
    param->value = value[2];
    param->skip_empty = 0;

    if (cf->args->nelts == 4) {
        if (ngx_strcmp(value[3].data, "if_not_empty") != 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "invalid parameter \"%V\"", &value[3]);
            return NGX_CONF_ERROR;
        }

        param->skip_empty = 1; //潞脥ngx_http_fastcgi_init_params脜盲潞脧脭脛露脕拢卢脠莽鹿没脡猫脰脙脕脣赂脙脰碌拢卢虏垄脟脪value虏驴路脰脦陋0拢卢脭貌脰卤陆脫虏禄脢鹿脫脙麓脣卤盲脕驴
    }

    return NGX_CONF_OK;
}


ngx_int_t
ngx_http_upstream_hide_headers_hash(ngx_conf_t *cf,
    ngx_http_upstream_conf_t *conf, ngx_http_upstream_conf_t *prev,
    ngx_str_t *default_hide_headers, ngx_hash_init_t *hash)
{
    ngx_str_t       *h;
    ngx_uint_t       i, j;
    ngx_array_t      hide_headers;
    ngx_hash_key_t  *hk;

    if (conf->hide_headers == NGX_CONF_UNSET_PTR
        && conf->pass_headers == NGX_CONF_UNSET_PTR)
    {
        conf->hide_headers = prev->hide_headers;
        conf->pass_headers = prev->pass_headers;

        conf->hide_headers_hash = prev->hide_headers_hash;

        if (conf->hide_headers_hash.buckets
#if (NGX_HTTP_CACHE)
            && ((conf->cache == 0) == (prev->cache == 0)) //脪脩戮颅脳枚鹿媒hash脭脣脣茫脕脣
#endif
           )
        {
            return NGX_OK;
        }

    } else {
        if (conf->hide_headers == NGX_CONF_UNSET_PTR) {
            conf->hide_headers = prev->hide_headers;
        }

        if (conf->pass_headers == NGX_CONF_UNSET_PTR) {
            conf->pass_headers = prev->pass_headers;
        }
    }

    if (ngx_array_init(&hide_headers, cf->temp_pool, 4, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (h = default_hide_headers; h->len; h++) { //掳脩default_hide_headers脰脨碌脛脭陋脣脴赂鲁脰碌赂酶hide_headers脢媒脳茅脰脨
        hk = ngx_array_push(&hide_headers);
        if (hk == NULL) {
            return NGX_ERROR;
        }

        hk->key = *h;
        hk->key_hash = ngx_hash_key_lc(h->data, h->len);
        hk->value = (void *) 1;
    }

    if (conf->hide_headers != NGX_CONF_UNSET_PTR) { //proxy_hide_header  fastcgi_hide_header脜盲脰脙碌脛脧脿鹿脴脨脜脧垄脪虏脪陋脤铆录脫碌陆hide_headers脢媒脳茅

        h = conf->hide_headers->elts;

        for (i = 0; i < conf->hide_headers->nelts; i++) {

            hk = hide_headers.elts;

            for (j = 0; j < hide_headers.nelts; j++) {
                if (ngx_strcasecmp(h[i].data, hk[j].key.data) == 0) {
                    goto exist;
                }
            }

            hk = ngx_array_push(&hide_headers);
            if (hk == NULL) {
                return NGX_ERROR;
            }

            hk->key = h[i];
            hk->key_hash = ngx_hash_key_lc(h[i].data, h[i].len);
            hk->value = (void *) 1;

        exist:

            continue;
        }
    }

    //脠莽鹿没hide_headers脫脨脧脿鹿脴脨脜脧垄拢卢卤铆脢戮脨猫脪陋脫掳虏脴拢卢碌楼xxx_pass_header脰脨脫脨脡猫脰脙脕脣虏禄脪镁虏脴拢卢脭貌脛卢脠脧赂脙脨脜脧垄禄鹿脢脟脫掳虏脴拢卢掳脩pass_header脰脨碌脛赂脙脧卯脠楼碌么
    if (conf->pass_headers != NGX_CONF_UNSET_PTR) { //proxy_pass_headers  fastcgi_pass_headers脜盲脰脙碌脛脧脿鹿脴脨脜脧垄麓脫hide_headers脢媒脳茅

        h = conf->pass_headers->elts;
        hk = hide_headers.elts;

        for (i = 0; i < conf->pass_headers->nelts; i++) {
            for (j = 0; j < hide_headers.nelts; j++) {

                if (hk[j].key.data == NULL) {
                    continue;
                }

                if (ngx_strcasecmp(h[i].data, hk[j].key.data) == 0) {
                    hk[j].key.data = NULL;
                    break;
                }
            }
        }
    }

    //掳脩default_hide_headers(ngx_http_proxy_hide_headers  ngx_http_fastcgi_hide_headers)脰脨碌脛鲁脡脭卤脳枚hash卤拢麓忙碌陆conf->hide_headers_hash
    hash->hash = &conf->hide_headers_hash; //掳脩脛卢脠脧碌脛default_hide_headers  xxx_pass_headers脜盲脰脙碌脛 
    hash->key = ngx_hash_key_lc;
    hash->pool = cf->pool;
    hash->temp_pool = NULL;

    return ngx_hash_init(hash, hide_headers.elts, hide_headers.nelts);
}


static void *
ngx_http_upstream_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_upstream_main_conf_t  *umcf;

    umcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_main_conf_t));
    if (umcf == NULL) {
        return NULL;
    }

    if (ngx_array_init(&umcf->upstreams, cf->pool, 4,
                       sizeof(ngx_http_upstream_srv_conf_t *))
        != NGX_OK)
    {
        return NULL;
    }

    return umcf;
}


static char *
ngx_http_upstream_init_main_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_upstream_main_conf_t  *umcf = conf;

    ngx_uint_t                      i;
    ngx_array_t                     headers_in;
    ngx_hash_key_t                 *hk;
    ngx_hash_init_t                 hash;
    ngx_http_upstream_init_pt       init;
    ngx_http_upstream_header_t     *header;
    ngx_http_upstream_srv_conf_t  **uscfp;

    uscfp = umcf->upstreams.elts;

    for (i = 0; i < umcf->upstreams.nelts; i++) {

        init = uscfp[i]->peer.init_upstream ? uscfp[i]->peer.init_upstream:
                                            ngx_http_upstream_init_round_robin;

        if (init(cf, uscfp[i]) != NGX_OK) { //脰麓脨脨init_upstream
            return NGX_CONF_ERROR;
        }
    }


    /* upstream_headers_in_hash */

    if (ngx_array_init(&headers_in, cf->temp_pool, 32, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    for (header = ngx_http_upstream_headers_in; header->name.len; header++) {
        hk = ngx_array_push(&headers_in);
        if (hk == NULL) {
            return NGX_CONF_ERROR;
        }

        hk->key = header->name;
        hk->key_hash = ngx_hash_key_lc(header->name.data, header->name.len);
        hk->value = header;
    }

    hash.hash = &umcf->headers_in_hash;
    hash.key = ngx_hash_key_lc;
    hash.max_size = 512;
    hash.bucket_size = ngx_align(64, ngx_cacheline_size);
    hash.name = "upstream_headers_in_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    if (ngx_hash_init(&hash, headers_in.elts, headers_in.nelts) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

