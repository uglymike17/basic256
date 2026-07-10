#!/usr/bin/env bash
set -euo pipefail


        mkdir -p Basic256
        mkdir -p Basic256/lib
        mkdir -p Basic256/plugins/texttospeech
        mkdir -p Basic256/plugins/multimedia
        mkdir -p Basic256/plugins/imageformats
        
        cp build/basic256 Basic256/
        cp -r Examples Basic256/
        cp -r TestSuite Basic256/ || true
        cp README.md Basic256/ || true
        cp Basic256-IDE.png Basic256-CLI.png Basic256-Web.png Basic256/ || true

        # Point at the aqtinstall-installed Qt6 tree (build_Linux_x86.sh exports
        # QT_DIR via $GITHUB_ENV; this is a fresh step so read it from there).
        export QT_PLUGIN_DIR="${QT_DIR:?QT_DIR not set - build_Linux_x86.sh must run first}/plugins"

        # Qt's official builds bundle a sqldrivers plugin for every backend
        # (Mimer, DB2, Firebird/ibase, MySQL, ODBC, PostgreSQL, SQLite)
        # regardless of whether that backend's proprietary/optional client
        # library is installed on this runner. BASIC256 only uses SQLite,
        # but linuxdeployqt's ldd trace aborts the *entire* build on any
        # plugin with an unresolved dependency (hit libqsqlmimer.so ->
        # libmimerapi.so first) -- so strip every driver but SQLite from
        # the Qt tree before deploying, instead of allowlisting failures
        # one linuxdeployqt run at a time.
        find "$QT_PLUGIN_DIR/sqldrivers" -name '*.so' ! -name 'libqsqlite.so' -delete 2>/dev/null || true

        # Manually grab the dynamic plugins that linuxdeployqt misses statically.
        # NOTE: Qt6 Multimedia plugins live under a "multimedia" plugin dir
        # (e.g. libffmpegmediaplugin.so) -- Qt5's "mediaservice"/"audio"
        # categories no longer exist, confirmed via linuxdeployqt's own
        # "plugin could not be found" warnings in the CI log.
        cp -r $QT_PLUGIN_DIR/texttospeech/*.so Basic256/plugins/texttospeech/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/multimedia/*.so Basic256/plugins/multimedia/ 2>/dev/null || true
        cp -r $QT_PLUGIN_DIR/imageformats/*.so Basic256/plugins/imageformats/ 2>/dev/null || true
        cp /usr/lib/x86_64-linux-gnu/libespeak-ng.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libspeechd.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libasound.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse-simple.so.*  Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpulse-mainloop-glib.so.* Basic256/lib/ || true
        cp /usr/lib/x86_64-linux-gnu/libpipewire-0.3.so.* Basic256/lib/ || true

        # GStreamer runtime libs
        GLIB="/usr/lib/x86_64-linux-gnu"
        cp $GLIB/libgstreamer-1.0.so.*        Basic256/lib/ || true
        cp $GLIB/libgstbase-1.0.so.*          Basic256/lib/ || true
        cp $GLIB/libgstaudio-1.0.so.*         Basic256/lib/ || true
        cp $GLIB/libgstpbutils-1.0.so.*       Basic256/lib/ || true
        cp $GLIB/libglib-2.0.so.*             Basic256/lib/ || true
        cp $GLIB/libgobject-2.0.so.*          Basic256/lib/ || true
        cp $GLIB/libgio-2.0.so.*              Basic256/lib/ || true
        mkdir -p Basic256/gstreamer-1.0
        GSTPLUG="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
        for p in libgstautodetect libgstaudioconvert libgstaudioresample \
          libgstplayback libgstalsa libgstpulseaudio libgstpipewire \
          libgstvolume libgstcoreelements \
          libgstwavparse libgstaudioparsers libgstmpegaudioparse \
          libgstmpg123 libgstlibav libgstogg libgstvorbis libgstisomp4; do
          cp "$GSTPLUG/${p}.so" Basic256/gstreamer-1.0/ 2>/dev/null || true
        done

        # Download linuxdeployqt asset into /tmp — NOT the workspace root.
        # This is a build tool, not a deliverable; downloading it to the
        # workspace would cause it to be swept up by the CI upload step's
        # "*.AppImage" glob alongside the actual BASIC256 AppImage.
        LDQT="/tmp/linuxdeployqt-continuous-x86_64.AppImage"
        wget -c -nv -O "${LDQT}" "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
        chmod a+x "${LDQT}"
        
        # Force linuxdeployqt to trace dependencies for our manually copied plugins
        EXTRA_ARGS=""
        for plugin in Basic256/plugins/*/*.so; do
          if [ -f "$plugin" ]; then
            EXTRA_ARGS="$EXTRA_ARGS -executable=$plugin"
          fi
        done

        # Force linuxdeployqt to look at the aqtinstall-installed Qt6 tree
        export PATH="${QT_DIR}/bin:$PATH"
        
        # Run linuxdeployqt with the plugin arguments
        "${LDQT}" Basic256/basic256 \
          -bundle-non-qt-libs \
          -extra-plugins=texttospeech,multimedia,imageformats \
          --appimage-extract-and-run

        # Ensure the binary actually lives in Basic256/ alongside the libraries
        if [ ! -f "Basic256/basic256" ] && [ -f "build/basic256" ]; then
            cp build/basic256 Basic256/
        fi

        #Launcher script
        cat > Basic256/run.sh << 'EOF'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$DIR/plugins"
export GST_PLUGIN_PATH="$DIR/gstreamer-1.0"
export GST_PLUGIN_SYSTEM_PATH=""
export GST_REGISTRY="$DIR/gstreamer-1.0/registry.bin"
exec "$DIR/basic256" "$@"
EOF
        chmod +x Basic256/run.sh

        # Compress everything into your final delivery asset
        tar -czf "${ARTIFACT_NAME}.tar.gz" Basic256/