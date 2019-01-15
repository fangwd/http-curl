import { Response } from './response';

var curl: any = null;

if (process.platform == 'win32' && process.arch == 'x64') {
  curl = require('../bin/winx64/http-curl');
} else if (process.platform == 'win32' && process.arch == 'ia32') {
  curl = require('../bin/winx86/http-curl');
} else {
  curl = require('../build/Release/http-curl');
}

export interface WithUrl {
  [key: string]: any;
  url: string;
}

export interface Options {
  [key: string]: any;
  data: any;
  followLocation: boolean;
  headers: { [key: string]: string | number };
  maxRedirect: number;
  proxy: string;
  timeout: number | [number, number]; // Low speed time/limit
  userAgent: string;
}

export type RequestOptions = { [key in keyof Options]?: Options[key] };

interface ResponseData {
  url: string;
  httpVersion: string;
  inflate: boolean;
  status: number;
  headers: [string, string][];
  body: Buffer;
}

export class HttpClient {
  client: any;

  constructor(options?: RequestOptions) {
    this.client = new curl.Client(options);
    if (options) {
      this.setOptions(options);
    }
  }

  setOptions(options: RequestOptions) {
    for (const key in options) {
      const value = options[key];
      switch (key) {
        case 'followLocation':
          this.client.setFollowLocation(value);
          break;
        case 'inflate':
          this.client.setAcceptEncoding(value ? '' : null);
          break;
        case 'maxRedirect':
          this.client.setMaxRedirects(value);
          break;
        case 'proxy':
          this.client.setProxy(value);
          break;
        case 'userAgent':
          this.client.setUserAgent(value);
          break;
        case 'timeout':
          if (Array.isArray(value)) {
            this.client.setTimeout(value[0], value[1]);
          } else {
            this.client.setTimeout(value);
          }
          break;
        case 'headers':
          for (const name in value) {
            this.client.setHeader(name, value[name]);
          }
          break;
        default:
          throw Error(`Unknown options: ${key}`);
      }
    }
  }

  get(url: string | WithUrl, options?: RequestOptions): Promise<Response> {
    if (options) this.setOptions(options);
    return new Promise((resolve, reject) => {
      const fetchStart = new Date();
      const href = typeof url === 'string' ? url : url.url;
      this.client.get(href, (error: Error, data: ResponseData) => {
        if (error) return reject(error);
        const response = new Response(url, data, data.body);
        response.fetchStart = fetchStart;
        response.fetchEnd = new Date();
        resolve(response);
      });
    });
  }

  post(
    url: string | WithUrl,
    data: any,
    options?: RequestOptions
  ): Promise<Response> {
    if (options) this.setOptions(options);
    return new Promise(resolve => {
      const fetchStart = new Date();
      const href = typeof url === 'string' ? url : url.url;
      if (typeof data !== 'string') data = JSON.stringify(data);
      this.client.post(href, data, (error: Error, data: ResponseData) => {
        if (error) throw error;
        const response = new Response(url, data, data.body);
        response.fetchStart = fetchStart;
        response.fetchEnd = new Date();
        resolve(response);
      });
    });
  }
}

export { Response };
