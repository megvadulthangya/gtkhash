#!/usr/bin/env bash
set -e

DEST="msys2/dist"
rm -rf "$DEST"
mkdir -p "$DEST/bin"
mkdir -p "$DEST/share/glib-2.0/schemas"
mkdir -p "$DEST/share/icons"
mkdir -p "$DEST/lib"

echo "=== 1. Fő bináris másolása ==="
cp "${MINGW_PREFIX}/bin/gtkhash.exe" "$DEST/bin/"

echo "=== 2. DLL függőségek automatikus kigyűjtése ==="
mapfile -t DLLS < <(ldd "$DEST/bin/gtkhash.exe" | grep -i "${MINGW_PREFIX}/bin" | awk '{print $3}')
for dll in "${DLLS[@]}"; do
    if [ -f "$dll" ]; then
        cp -n "$dll" "$DEST/bin/"
    fi
done

echo "=== 3. GSettings sémák másolása és befordítása ==="
cp "${MINGW_PREFIX}"/share/glib-2.0/schemas/*.xml "$DEST/share/glib-2.0/schemas/"
glib-compile-schemas "$DEST/share/glib-2.0/schemas/"

echo "=== 4. GTK Ikonok és Erőforrások másolása ==="
if [ -d "${MINGW_PREFIX}/share/icons/hicolor" ]; then
    cp -r "${MINGW_PREFIX}/share/icons/hicolor" "$DEST/share/icons/"
fi
if [ -d "${MINGW_PREFIX}/share/icons/Adwaita" ]; then
    cp -r "${MINGW_PREFIX}/share/icons/Adwaita" "$DEST/share/icons/"
fi

if [ -d "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0" ]; then
    cp -r "${MINGW_PREFIX}/lib/gdk-pixbuf-2.0" "$DEST/lib/"
fi

echo "=== A futtatókörnyezet összeállítása sikeres! ==="