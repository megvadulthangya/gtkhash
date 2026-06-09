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

echo "=== 2. GLib helper binárisok másolása ==="
# A GLib/GTK-nak szüksége van ezekre a folyamatkezeléshez Windows alatt
cp "${MINGW_PREFIX}"/bin/gspawn-*-helper*.exe "$DEST/bin/" 2>/dev/null || true

echo "=== 3. DLL függőségek automatikus kigyűjtése ==="
# Az ldd kimenetét soronként olvassuk, és cygpath-al konvertáljuk tiszta MSYS2 POSIX úttá.
# Ez megoldja a Windows vs. Unix útvonal keveredési problémát.
ldd "$DEST/bin/gtkhash.exe" | while read -r line; do
    if [[ "$line" == *"=>"* ]]; then
        raw_path=$(echo "$line" | sed -e 's/.*=> //' -e 's/ (0x.*)//' | tr -d '\r' | xargs)
        if [ -n "$raw_path" ] && [ "$raw_path" != "not found" ]; then
            # Uniformizálás (pl. /mingw64/bin/libgtk-3-0.dll formátumra)
            posix_path=$(cygpath -u "$raw_path")
            
            # Csak a saját MinGW/UCRT prefixünkből származó függőségeket gyűjtjük ki
            if [[ "$posix_path" == "${MINGW_PREFIX}"/* ]] && [ -f "$posix_path" ]; then
                cp -n "$posix_path" "$DEST/bin/"
            fi
        fi
    fi
done

echo "=== 4. GSettings sémák másolása és befordítása ==="
cp "${MINGW_PREFIX}"/share/glib-2.0/schemas/*.xml "$DEST/share/glib-2.0/schemas/"
glib-compile-schemas "$DEST/share/glib-2.0/schemas/"

echo "=== 5. GTK Ikonok és Erőforrások másolása ==="
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