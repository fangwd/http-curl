# Installation

`$ npm install http-curl`

# Usage

TBA

# NOTE

Customising the order of http headers sent in cURL seems not possible; see `lib/http.c`

```
CURLcode Curl_http(struct connectdata *conn, bool *done)
{
  ...
  result =
    Curl_add_bufferf(req_buffer,
                     "%s" /* ftp typecode (;type=x) */
                     " HTTP/%s\r\n" /* HTTP version */
                     "%s" /* host */
                     "%s" /* proxyuserpwd */
                     "%s" /* userpwd */
                     "%s" /* range */
                     "%s" /* user agent */
                     "%s" /* accept */
                     "%s" /* TE: */
                     "%s" /* accept-encoding */
                     "%s" /* referer */
                     "%s" /* Proxy-Connection */
                     "%s",/* transfer-encoding */
  ...
}
```

There was also some discussion around this issue here:

https://curl.haxx.se/mail/lib-2005-10/0047.html

