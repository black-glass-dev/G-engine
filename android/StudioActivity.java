package dev.gengine.demo;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * G-Engine Studio — the game engine itself on a phone: draw a level, write the rules,
 * play and save games. The engine is the same C++ code as everywhere else, compiled to
 * WebAssembly and shown in a WebView. A tiny localhost server serves the files so that
 * JavaScript modules, fetch() and saved games all work exactly like in a browser.
 */
public class StudioActivity extends Activity {
    private WebView web;
    private Server server;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        server = new Server(getAssets());
        server.start();
        web = new WebView(this);
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        web.setWebViewClient(new WebViewClient());
        setContentView(web);
        web.loadUrl("http://127.0.0.1:8377/index.html");
    }

    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        if (web != null && web.canGoBack()) {
            web.goBack();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onDestroy() {
        if (web != null) web.destroy();
        if (server != null) server.stopServer();
        super.onDestroy();
    }

    /** Serves the assets/web folder on http://127.0.0.1:8377 (loopback only). */
    static final class Server extends Thread {
        private final AssetManager assets;
        private volatile boolean running = true;
        private ServerSocket socket;

        Server(AssetManager assets) {
            this.assets = assets;
        }

        void stopServer() {
            running = false;
            try {
                if (socket != null) socket.close();
            } catch (IOException ignored) {
            }
        }

        @Override
        public void run() {
            try {
                socket = new ServerSocket(8377, 16, InetAddress.getByName("127.0.0.1"));
                while (running) {
                    final Socket client = socket.accept();
                    new Thread(() -> serve(client)).start();
                }
            } catch (IOException ignored) {
            }
        }

        private void serve(Socket client) {
            try {
                InputStream in = client.getInputStream();
                String line = readLine(in);
                String path = "index.html";
                if (line != null && line.startsWith("GET ")) {
                    String p = line.split(" ")[1];
                    int q = p.indexOf('?');
                    if (q >= 0) p = p.substring(0, q);
                    if (p.startsWith("/")) p = p.substring(1);
                    if (p.length() > 0 && !p.contains("..")) path = p;
                }
                OutputStream out = client.getOutputStream();
                byte[] body;
                try (InputStream file = new BufferedInputStream(assets.open("web/" + path))) {
                    body = readAll(file);
                } catch (IOException missing) {
                    body = "not found".getBytes("UTF-8");
                    out.write(("HTTP/1.0 404 Not Found\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: "
                            + body.length + "\r\nConnection: close\r\n\r\n").getBytes("UTF-8"));
                    out.write(body);
                    client.close();
                    return;
                }
                out.write(("HTTP/1.0 200 OK\r\nContent-Type: " + mime(path)
                        + "\r\nContent-Length: " + body.length
                        + "\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n").getBytes("UTF-8"));
                out.write(body);
                out.flush();
                client.close();
            } catch (IOException ignored) {
                try {
                    client.close();
                } catch (IOException alsoIgnored) {
                }
            }
        }

        private static String readLine(InputStream in) throws IOException {
            StringBuilder sb = new StringBuilder();
            int c = in.read();
            while (c >= 0 && c != '\n') {
                if (c != '\r') sb.append((char) c);
                if (sb.length() > 4096) break;
                c = in.read();
            }
            return sb.length() == 0 && c < 0 ? null : sb.toString();
        }

        private static byte[] readAll(InputStream in) throws IOException {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[16384];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
            return out.toByteArray();
        }

        private static String mime(String path) {
            if (path.endsWith(".html")) return "text/html; charset=utf-8";
            if (path.endsWith(".js")) return "text/javascript; charset=utf-8";
            if (path.endsWith(".css")) return "text/css; charset=utf-8";
            if (path.endsWith(".wasm")) return "application/wasm";
            if (path.endsWith(".json")) return "application/json";
            if (path.endsWith(".woff2")) return "font/woff2";
            if (path.endsWith(".png")) return "image/png";
            if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
            return "application/octet-stream";
        }
    }
}
