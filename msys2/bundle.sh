#!/usr/bin/env bash
set -e

DEST="msys2/dist"
rm -rf "$DEST"
mkdir -p "$DEST/bin"
mkdir -p "$DEST/share/glib-2.0/schemas"
mkdir -p "$DEST/share/icons"
mkdir -p "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders"
mkdir -p "$DEST/lib/gio/modules"

# 1. Main executable placed in the bin/ subfolder
cp "${MINGW_PREFIX}/bin/gtkhash.exe" "$DEST/bin/"

# 2. Patch PE subsystem to "windows" (GUI) – suppresses the background console window
if command -v objcopy >/dev/null 2>&1; then
    objcopy --subsystem windows "$DEST/bin/gtkhash.exe" || true
fi

# 3. Copy GTK loaders and modules into their required subdirectories
if [ -d "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders" ]; then
    cp "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders"/*.dll "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders/" 2>/dev/null || true
    cp "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" "$DEST/lib/gdk-pixbuf-2.0/2.10.0/" 2>/dev/null || true
    # Make the cache paths relative so the loaders are found at runtime
    sed -i 's|"[^"]*/lib/gdk-pixbuf-2.0/|"lib/gdk-pixbuf-2.0/|g' "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" 2>/dev/null || true
fi

if [ -d "${MINGW_PREFIX}/lib/gio/modules" ]; then
    cp "${MINGW_PREFIX}/lib/gio/modules"/*.dll "$DEST/lib/gio/modules/" 2>/dev/null || true
fi

# 4. Recursively resolve and copy all dynamically linked DLLs to the bin/ directory
resolve_deps() {
    local added=1
    while [ $added -eq 1 ]; do
        added=0
        # Scan the ENTIRE $DEST tree to catch dependencies of GTK modules/loaders in lib/
        mapfile -t files < <(find "$DEST" -type f \( -name "*.exe" -o -name "*.dll" \))
        for f in "${files[@]}"; do
            mapfile -t deps < <(ldd "$f" 2>/dev/null | grep -i "${MINGW_PREFIX}/bin" | awk '{print $3}')
            for d in "${deps[@]}"; do
                if [ -n "$d" ] && [ "$d" != "not" ]; then
                    posix_path=$(cygpath -u "$d" 2>/dev/null || echo "$d")
                    basename_d=$(basename "$posix_path")
                    # Place all resolved dependencies strictly into the bin/ directory
                    if [ -f "$posix_path" ] && [ ! -f "$DEST/bin/$basename_d" ]; then
                        cp "$posix_path" "$DEST/bin/"
                        added=1
                    fi
                fi
            done
        done
    done
}

resolve_deps

# 5. GTK schemas and icon theme resources remain correctly mapped
cp "${MINGW_PREFIX}"/share/glib-2.0/schemas/*.xml "$DEST/share/glib-2.0/schemas/" 2>/dev/null || true
glib-compile-schemas "$DEST/share/glib-2.0/schemas/"

if [ -d "${MINGW_PREFIX}/share/icons/hicolor" ]; then
    cp -r "${MINGW_PREFIX}/share/icons/hicolor" "$DEST/share/icons/"
fi
if [ -d "${MINGW_PREFIX}/share/icons/Adwaita" ]; then
    cp -r "${MINGW_PREFIX}/share/icons/Adwaita" "$DEST/share/icons/"
fi