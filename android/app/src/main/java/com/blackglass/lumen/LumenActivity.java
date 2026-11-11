package com.blackglass.lumen;

import org.libsdl.app.SDLActivity;

/**
 * Entry activity. SDL3's SDLActivity handles the Android lifecycle, surface and input;
 * we only tell it which native libraries to load (SDL3 first, then the Lumen player).
 */
public class LumenActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", "lumen_player"};
    }
}
