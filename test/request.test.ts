import { HttpClient } from '../src';

const HTTP_URL = 'http://testfront.net:8091';
const HTTPS_URL = 'https://testfront.net';

const SOCKS_PROXY = process.env.SOCKS_PROXY || 'socks4a://localhost:9050';
const HTTP_PROXY = process.env.HTTP_PROXY;
const HTTPS_PROXY = process.env.HTTPS_PROXY || HTTP_PROXY;

let ourIp: string;

beforeAll(async () => {
  const client = new HttpClient();
  const response = await client.get(HTTPS_URL);
  ourIp = response.json().address;
});

test('get', async () => {
  const client = new HttpClient();
  const res = await client.get(HTTPS_URL);
  expect(res.statusCode).toBe(200);
});

test('inflate', async () => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/test/gzip`;
  const res = await client.get(url);
  expect(res.body.toString()).toBe('gzip');
  const res2 = await client.get(url, { inflate: false });
  expect(res2.body.toString()).not.toBe('gzip');
});

test('post', async () => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/echo`;
  const data = 'hello';
  const res = await client.post(url, data);
  expect(res.body.toString()).toBe(data);
});

test('post - json', async () => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/echo`;
  const options = {
    headers: { 'content-type': 'application/json' }
  };
  const res = await client.post(url, { hello: 'world' }, options);
  expect(res.headers['content-type']).toBe('application/json');
  expect(res.json().hello).toBe('world');
});

test('redirect', async () => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/redirect`;
  const res = await client.get(url);
  expect(res.statusCode).toBe(301);
});

test('follow location', async () => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/redirect`;
  const res = await client.get(url, { followLocation: true });
  expect(res.statusCode).toBe(200);
  expect(res.body.toString()).toBe('info');
}, 10000);

test('max redirect', async done => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/redirect`;
  try {
    await client.get(url, { followLocation: true, maxRedirect: 2 });
  } catch (error) {
    expect(/maximum.+?redirects/i.test(error.message)).toBe(true);
    done();
  }
});

test('timeout', async done => {
  const client = new HttpClient();
  const url = `${HTTPS_URL}/timeout`;
  try {
    await client.get(url, { timeout: 2000 });
  } catch (error) {
    expect(/time.*?out/i.test(error.message)).toBe(true);
    done();
  }
});

test('cookie', async () => {
  const client = new HttpClient();
  await client.get(HTTPS_URL);
  const res = await client.get(HTTPS_URL);
  expect(!!res.json().headers.cookie).toBe(true);
});

test('proxy (http)', async () => {
  const client = new HttpClient();
  const res = await client.get(HTTP_URL, { proxy: HTTP_PROXY });
  expect(res.json().address).not.toBe(ourIp);
});

test('proxy (https)', async () => {
  const client = new HttpClient();
  const res = await client.get(HTTPS_URL, { proxy: HTTPS_PROXY });
  expect(res.json().address).not.toBe(ourIp);
});

test('socks proxy (http)', async () => {
  const client = new HttpClient();
  const res = await client.get(HTTP_URL, { proxy: SOCKS_PROXY });
  expect(res.json().address).not.toBe(ourIp);
}, 10000);

test('socks proxy (https)', async () => {
  const client = new HttpClient();
  const res = await client.get(HTTPS_URL, { proxy: SOCKS_PROXY });
  expect(res.json().address).not.toBe(ourIp);
}, 10000);
