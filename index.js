var curl = null;

if (process.platform == 'win32' && process.arch == 'x64') {
  curl = require('./bin/winx64/http-curl');
} else if (process.platform == 'win32' && process.arch == 'ia32') {
  curl = require('./bin/winx86/http-curl');
} else {
  curl = require('./build/Release/http-curl');
}

function setOptions(client, options) {
  for (const key in options) {
    const value = options[key];
    switch (key) {
      case 'proxy':
        client.setProxy(value);
        break;
      case 'user-agent':
      case 'User-Agent':
        client.setUserAgent(value);
      case 'headers':
        for (const name in value) {
          client.setHeader(name, value[name]);
        }
        break;
      default:
        throw Error(`Unknown options: ${key}`);
    }
  }
}

function get(options, url) {
  if (!url) {
    url = options;
    options = {};
  }
  return new Promise(resolve => {
    const client = new curl.Client(options);
    setOptions(client, options);
    client.get(url, (err, res) => {
      if (err) throw err;
      resolve(res);
    });
  });
}

module.exports = { get, Client: curl.Client };
