#!/usr/bin/env bash
set -e

DEST="dist"
rm -rf "$DEST"
mkdir -p "$DEST/bin"
mkdir -p "$DEST/share/glib-2.0/schemas"
mkdir -p "$DEST/share/icons"
mkdir -p "$DEST/share/locale"
mkdir -p "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders"
mkdir -p "$DEST/lib/gio/modules"

# 1. Main executable placed in the bin/ subfolder (Untouched, console-enabled)
cp "${MINGW_PREFIX}/bin/gtkhash.exe" "$DEST/bin/"

# 2. Create the GUI launcher inside the bin/ folder next to its DLLs
cp "${MINGW_PREFIX}/bin/gtkhash.exe" "$DEST/bin/org.gtkhash.gtkhash.exe"
if command -v objcopy >/dev/null 2>&1; then
    objcopy --subsystem windows "$DEST/bin/org.gtkhash.gtkhash.exe"
fi

# 3. Generate a native Windows .ico file strictly. Fails CI if imagemagick is missing.
ICON_SRC="${MINGW_PREFIX}/share/icons/hicolor/256x256/apps/org.gtkhash.gtkhash.png"
ICON_DST="$DEST/bin/gtkhash.ico"
if [ -f "$ICON_SRC" ]; then
    if command -v magick >/dev/null 2>&1; then
        magick "$ICON_SRC" "$ICON_DST"
    elif command -v convert >/dev/null 2>&1; then
        convert "$ICON_SRC" "$ICON_DST"
    else
        echo "FATAL: ImageMagick not found. Install mingw-w64-x86_64-imagemagick in GitHub Actions."
        exit 1
    fi
else
    echo "FATAL: Source icon not found at $ICON_SRC"
    exit 1
fi

# 4. Copy GTK loaders and modules into their required subdirectories
if [ -d "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders" ]; then
    cp "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders"/*.dll "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders/" 2>/dev/null || true
    cp "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" "$DEST/lib/gdk-pixbuf-2.0/2.10.0/" 2>/dev/null || true
    sed -i 's|"[^"]*/lib/gdk-pixbuf-2.0/|\\lib\\gdk-pixbuf-2.0\\|g' "$DEST/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" 2>/dev/null || true
fi

if [ -d "${MINGW_PREFIX}/lib/gio/modules" ]; then
    cp "${MINGW_PREFIX}/lib/gio/modules"/*.dll "$DEST/lib/gio/modules/" 2>/dev/null || true
fi

# 5. Recursively resolve and copy all dynamically linked DLLs to the bin/ directory
resolve_deps() {
    local added=1
    while [ $added -eq 1 ]; do
        added=0
        mapfile -t files < <(find "$DEST" -type f \( -name "*.exe" -o -name "*.dll" \))
        for f in "${files[@]}"; do
            mapfile -t deps < <(ldd "$f" 2>/dev/null | grep -i "${MINGW_PREFIX}/bin" | awk '{print $3}')
            for d in "${deps[@]}"; do
                if [ -n "$d" ] && [ "$d" != "not" ]; then
                    posix_path=$(cygpath -u "$d" 2>/dev/null || echo "$d")
                    basename_d=$(basename "$posix_path")
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

# 6. Copy GTK schemas, asset icons, and target locales
cp "${MINGW_PREFIX}"/share/glib-2.0/schemas/*.xml "$DEST/share/glib-2.0/schemas/" 2>/dev/null || true
glib-compile-schemas "$DEST/share/glib-2.0/schemas/"

if [ -d "${MINGW_PREFIX}/share/icons" ]; then
    cp -r "${MINGW_PREFIX}/share/icons"/* "$DEST/share/icons/" 2>/dev/null || true
fi

if [ -d "${MINGW_PREFIX}/share/locale" ]; then
    find "${MINGW_PREFIX}/share/locale" -type f -name "gtkhash.mo" | while read -r mo_file; do
        relative_path="${mo_file#${MINGW_PREFIX}/share/locale/}"
        dest_dir="$DEST/share/locale/$(dirname "$relative_path")"
        mkdir -p "$dest_dir"
        cp "$mo_file" "$dest_dir/"
    done
fi

# 7. Copy project license & readme (as .txt for Windows) from repository root
if [ -f "../COPYING" ]; then
    cp "../COPYING" "$DEST/COPYING.txt"
else
    echo "WARNING: ../COPYING not found, license will be omitted from installer"
fi

if [ -f "../README.md" ]; then
    cp "../README.md" "$DEST/README.txt"
else
    echo "WARNING: ../README.md not found, readme will be omitted from installer"
fi

# 8. Build native C launcher for root directory (Resolves portable path issues)
cat << 'EOF' > launcher.c
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "open";
    sei.lpFile = "bin\\org.gtkhash.gtkhash.exe";
    sei.lpParameters = lpCmdLine;
    sei.lpDirectory = "bin";
    sei.nShow = nCmdShow;

    if (ShellExecuteEx(&sei)) {
        return 0;
    }
    return 1;
}
EOF

# Embed the .ico as the default application icon (resource ID 1)
echo '1 ICON "dist/bin/gtkhash.ico"' > launcher.rc

windres launcher.rc -O coff -o launcher.res
gcc -mwindows launcher.c launcher.res -o "$DEST/GtkHash.exe"

# Clean up build artifacts
rm launcher.c launcher.rc launcher.res