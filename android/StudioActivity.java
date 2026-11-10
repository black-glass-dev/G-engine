package dev.gengine.demo;

import android.app.Activity;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.webkit.RenderProcessGoneDetail;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;

/**
 * G-Engine Studio — the game engine itself on a phone: draw a level, write the rules,
 * play and save games. The engine is the same C++ code as everywhere else, compiled to
 * WebAssembly and shown in a WebView.
 *
 * The files of assets/web/ are served through shouldInterceptRequest on the virtual host
 * https://appassets.androidplatform.net/ — the same technique as Jetpack's WebViewAssetLoader.
 * That means: no web server, no ports, no cleartext, nothing that can fail to connect.
 * ES modules, fetch() and localStorage all work there, and everything runs offline.
 */
public class StudioActivity extends Activity {
    private static final String TAG = "GEngine";
    private static final String HOST = "appassets.androidplatform.net";
    private static final String START = "https://" + HOST + "/index.html";

    private WebView web;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        web = new WebView(this);
        web.setBackgroundColor(0xFF0B0D12);  // the studio's dark background — never a white flash
        web.setLayerType(View.LAYER_TYPE_HARDWARE, null);  // WebGL 2 needs this on some devices
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);      // saved games
        settings.setAllowFileAccess(false);
        settings.setMediaPlaybackRequiresUserGesture(false);
        web.setWebViewClient(new LocalClient(getAssets()));
        setContentView(web);
        web.loadUrl(START);
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
        super.onDestroy();
    }

    /** Answers every request for the virtual host from assets/web — and never shows a blank screen. */
    static final class LocalClient extends WebViewClient {
        private final AssetManager assets;

        LocalClient(AssetManager assets) {
            this.assets = assets;
        }

        @Override
        public WebResourceResponse shouldInterceptRequest(WebView view, WebResourceRequest request) {
            Uri uri = request.getUrl();
            if (uri == null || !HOST.equals(uri.getHost())) return null;
            String path = uri.getPath();
            if (path == null || path.equals("/") || path.isEmpty()) path = "/index.html";
            path = path.substring(1);
            if (path.contains("..")) return notFound();
            try {
                InputStream in = assets.open("web/" + path);
                return new WebResourceResponse(mime(path), isText(path) ? "UTF-8" : null, in);
            } catch (IOException missing) {
                return notFound();
            }
        }

        @Override
        public void onReceivedError(WebView view, WebResourceRequest request,
                                    android.webkit.WebResourceError error) {
            if (request != null && !request.isForMainFrame()) return;
            String what = error == null ? "unknown" : String.valueOf(error.getDescription());
            Log.e(TAG, "page error: " + what);
            if (view != null) view.loadDataWithBaseURL("https://" + HOST + "/",
                    errorPage(what), "text/html", "UTF-8", null);
        }

        @Override
        public boolean onRenderProcessGone(WebView view, RenderProcessGoneDetail detail) {
            Log.e(TAG, "renderer process gone — restarting the studio");
            Activity activity = view == null ? null
                    : (view.getContext() instanceof Activity ? (Activity) view.getContext() : null);
            if (view != null) view.destroy();
            if (activity != null) activity.recreate();
            return true;
        }

        private static WebResourceResponse notFound() {
            byte[] body = "not found".getBytes(StandardCharsets.UTF_8);
            return new WebResourceResponse("text/plain", "UTF-8", 404, "Not Found",
                    new HashMap<String, String>(), new ByteArrayInputStream(body));
        }

        private static boolean isText(String path) {
            return path.endsWith(".html") || path.endsWith(".js") || path.endsWith(".css")
                    || path.endsWith(".json");
        }

        private static String mime(String path) {
            if (path.endsWith(".html")) return "text/html";
            if (path.endsWith(".js")) return "text/javascript";
            if (path.endsWith(".css")) return "text/css";
            if (path.endsWith(".wasm")) return "application/wasm";
            if (path.endsWith(".json")) return "application/json";
            if (path.endsWith(".woff2")) return "font/woff2";
            if (path.endsWith(".png")) return "image/png";
            if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
            return "application/octet-stream";
        }

        /** A visible, self-explanatory page in Bengali and English — never a blank white screen. */
        private static String errorPage(String what) {
            return "<!doctype html><html><head><meta charset=\"utf-8\">"
                    + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                    + "<style>body{background:#0b0d12;color:#ece7dc;font-family:system-ui,sans-serif;"
                    + "padding:24px;line-height:1.6}h1{color:#ffb347;font-size:20px}"
                    + "code{background:#1b212d;padding:2px 6px;border-radius:6px}</style></head><body>"
                    + "<h1>G-Engine — কিছু একটা সমস্যা হয়েছে</h1>"
                    + "<p>পেজটা লোড করা যায়নি: <code>" + what + "</code><br>"
                    + "অ্যাপটা বন্ধ করে আবার খুলুন। সমস্যা থাকলে স্ক্রিনশটসহ জানান।</p>"
                    + "<h1>Something went wrong</h1>"
                    + "<p>The studio could not load: <code>" + what + "</code><br>"
                    + "Close the app and open it again. If it keeps happening, please send a screenshot.</p>"
                    + "</body></html>";
        }
    }
}
