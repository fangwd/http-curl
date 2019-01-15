# Installation

`$ npm install http-curl`

# Usage

```js
const { HttpClient } = require('http-curl');

(async function(proxy) {
  const client = new HttpClient();
  const response = await client.get('https://www.google.com/', { proxy });
  console.log(response.statusCode); // 200
})('socks5://localhost:9050');
```

