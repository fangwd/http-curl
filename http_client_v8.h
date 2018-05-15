#ifndef HTTP_CLIENT_V8_H_
#define HTTP_CLIENT_V8_H_

#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

#include <vector>

#include "http_client.h"

namespace http_v8 {

class Client: public http::Client, public node::ObjectWrap {
public:
    static void Init(v8::Local<v8::Object> exports);

private:
    static v8::Persistent<v8::Function> client_constructor;

    static void New(const v8::FunctionCallbackInfo<v8::Value> &);

    static void Get(const v8::FunctionCallbackInfo<v8::Value> &);
    static void Post(const v8::FunctionCallbackInfo<v8::Value> &);
    static void RunRequest(uv_work_t *req);
    static void RequestAfter(uv_work_t *req, int status);

    static void GetProxy(const v8::FunctionCallbackInfo<v8::Value> &);
    static void SetProxy(const v8::FunctionCallbackInfo<v8::Value> &);

    static void SetUserAgent(const v8::FunctionCallbackInfo<v8::Value> &);
    static void SetVerbose(const v8::FunctionCallbackInfo<v8::Value> &);

    static void SetHeader(const v8::FunctionCallbackInfo<v8::Value> &);
    static void SetTimeout(const v8::FunctionCallbackInfo<v8::Value> &);

    static void SetFollowLocation(const v8::FunctionCallbackInfo<v8::Value> &);
};

}  // namespace http_v8

#endif  // HTTP_CLIENT_V8_H_
