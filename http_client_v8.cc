#include "http_client_v8.h"
#include <node_buffer.h>
#include <nan.h>

namespace http_v8 {

using namespace v8;

void InitAll(Local<Object> exports) {
    Client::Init(exports);
}

NODE_MODULE(http_curl, InitAll)

Persistent<Function> Client::client_constructor;

void Client::Init(Local<Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  HandleScope handle_scope(isolate);

  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Client"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(tpl, "post", Post);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getProxy", GetProxy);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setProxy", SetProxy);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setUserAgent", SetUserAgent);
  NODE_SET_PROTOTYPE_METHOD(tpl, "setVerbose", SetVerbose);

  NODE_SET_PROTOTYPE_METHOD(tpl, "setHeader", SetHeader);

  exports->Set(String::NewFromUtf8(isolate, "Client"), tpl->GetFunction());

  curl_global_init(CURL_GLOBAL_ALL);
}

static std::string ToString(Local<Value> value) {
  String::Utf8Value utf8_value(value);
  return std::string(utf8_value.length() ? *utf8_value : "");
}

void Client::New(const FunctionCallbackInfo<Value>& args) {
  Isolate *isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (args.IsConstructCall()) {
    Client *client = new Client();
    client->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    isolate->ThrowException(
        String::NewFromUtf8(isolate, "Cannot call constructor as function",
            NewStringType::kNormal).ToLocalChecked());
  }
}

struct RequestData {
    Isolate *isolate;
    Persistent<Function> callback;
    Client *client;
    std::string url;
    std::string *body;
    http::Response *response;

    RequestData(Isolate *isolate, Client *client, std::string url) {
        this->isolate = isolate;
        this->client = client;
        this->url = url;

        body = NULL;
        response = NULL;
    }

    ~RequestData() {
        delete body;
        delete response;
    }
};

void Client::RunRequest(uv_work_t *req) {
    RequestData *data = reinterpret_cast<RequestData *>(req->data);
    const char *url = data->url.c_str();

    if (!data->body) {
        data->response = data->client->request(url);
    } else {
        data->response = data->client->request(url, http::POST,
                data->body->c_str(), data->body->size());

    }
}

// https://github.com/nodejs/nan/blob/master/test/cpp/buffer.cpp
static void free_body(char* data, void* hint) {
    free(data);
}

static Local<Object> ResponseToObject(Isolate *isolate,
        http::Response* response) {
    EscapableHandleScope scope(isolate);

    Local<Object> result = Object::New(isolate);

    result->Set(String::NewFromUtf8(isolate, "url"),
            String::NewFromUtf8(isolate, response->url().c_str()));

    result->Set(String::NewFromUtf8(isolate, "status"),
            Integer::New(isolate, response->status_code()));

    const std::vector<std::string>& headers = response->headers();
    Local<Array> array = Array::New(isolate, headers.size() / 2);

    for (size_t i = 0; i < headers.size() / 2; i++) {
        Local<Array> header = Array::New(isolate, 2);
        header->Set(0,
                String::NewFromUtf8(isolate, headers[2 * i].c_str(),
                        NewStringType::kNormal).ToLocalChecked());
        header->Set(1,
                String::NewFromUtf8(isolate, headers[2 * i + 1].c_str(),
                        NewStringType::kNormal).ToLocalChecked());
        array->Set(i, header);
    }

    result->Set(String::NewFromUtf8(isolate, "headers"), array);

    char *data = static_cast<char *>(malloc(response->body().size()));
    memcpy(data, response->body().c_str(), response->body().size());

    MaybeLocal<Object> body = Nan::NewBuffer(data, response->body().size(),
            free_body, 0);
    result->Set(String::NewFromUtf8(isolate, "body"), body.ToLocalChecked());

    return scope.Escape(result);
}

void Client::RequestAfter(uv_work_t *req, int status) {
    RequestData *data = reinterpret_cast<RequestData *>(req->data);
    Isolate *isolate = data->isolate;
    HandleScope handle_scope(isolate);

    Local<Context> context = isolate->GetCurrentContext();
    Context::Scope context_scope(context);

    const int argc = 2;
    Local<Value> argv[argc];

    if (!data->response) {
        Local<Object> error = Object::New(isolate);
        error->Set(String::NewFromUtf8(isolate, "code"),
                Integer::New(isolate, data->client->error_code()));
        error->Set(String::NewFromUtf8(isolate, "message"),
                String::NewFromUtf8(isolate, data->client->error(),
                        NewStringType::kNormal).ToLocalChecked());
        argv[0] = error;
        argv[1] = Undefined(isolate);
    } else {
        argv[0] = Undefined(isolate);
        Local<Object> value = ResponseToObject(isolate, data->response);
        argv[1] = value;
    }

    if (!data->callback.IsEmpty()) {
        Local<Function> callback = Local<Function>::New(isolate,
                data->callback);
        Local<Value> result;
        if (!callback->Call(context, context->Global(), argc, argv).ToLocal(
                &result)) {
            ;
        }
        data->callback.Reset();
    }

    delete data;
    delete req;
}

void Client::Get(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();
  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  RequestData *data = new RequestData(isolate, client, ToString(args[0]));

  if (args[1]->IsFunction()) {
    Local<Function> callback = Local<Function>::Cast(args[1]);
    data->callback.Reset(isolate, callback);
  }

  uv_work_t *req = new uv_work_t;
  req->data = data;

  uv_queue_work(uv_default_loop(), req, RunRequest, RequestAfter);
}

void Client::Post(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();
  Client *client = ObjectWrap::Unwrap<Client>(args.Holder());

  RequestData *data = new RequestData(isolate, client, ToString(args[0]));

  data->body = new std::string(ToString(args[1]));

  if (args[2]->IsFunction()) {
    Local<Function> callback = Local<Function>::Cast(args[2]);
    data->callback.Reset(isolate, callback);
  }

  uv_work_t *req = new uv_work_t;
  req->data = data;

  uv_queue_work(uv_default_loop(), req, RunRequest, RequestAfter);
}

void Client::GetProxy(const FunctionCallbackInfo<Value> &args) {
    Isolate* isolate = args.GetIsolate();
    Client *client = ObjectWrap::Unwrap<Client>(args.Holder());
    Local<String> proxy = String::NewFromUtf8(isolate, client->proxy());
    args.GetReturnValue().Set(proxy);
}

void Client::SetProxy(const FunctionCallbackInfo<Value> &args) {
    Client *client = ObjectWrap::Unwrap<Client>(args.Holder());
    client->set_proxy(ToString(args[0]).c_str());
}

void Client::SetUserAgent(const FunctionCallbackInfo<Value> &args) {
    Client *client = ObjectWrap::Unwrap<Client>(args.Holder());
    client->set_user_agent(ToString(args[0]).c_str());
}

void Client::SetVerbose(const FunctionCallbackInfo<Value> &args) {
    Client *client = ObjectWrap::Unwrap<Client>(args.Holder());
    client->set_verbose(args[0]->BooleanValue());
}

void Client::SetHeader(const FunctionCallbackInfo<Value> &args) {
    Client *client = ObjectWrap::Unwrap<Client>(args.Holder());
    std::string key = ToString(args[0]);
    std::string value = args.Length() > 1 ? ToString(args[1]) : "";
    client->set_header(key, value);
}

}  // namespace http_v8
