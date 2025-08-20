    # Vitte Desktop Makefile
    .PHONY: all gtk qt clean dist

    all: gtk qt

    build:
    	mkdir -p build

    # ---- GTK ----
    GTK_PKG := $(shell pkg-config --exists gtk4 && echo gtk4 || (pkg-config --exists gtk+-3.0 && echo gtk+-3.0 || echo none))
    GTK_CFLAGS := $(shell pkg-config --cflags $(GTK_PKG))
    GTK_LIBS := $(shell pkg-config --libs $(GTK_PKG))

    gtk: build
    	@if [ "$(GTK_PKG)" = "none" ]; then \
		echo 'GTK non trouvé, compilation du stub...'; \
		$(CC) -O2 -o build/desktop-gtk gtk_stub.c; \
	else \
		echo 'Build GTK avec' $(GTK_PKG); \
		$(CC) -O2 $(GTK_CFLAGS) -o build/desktop-gtk gtk_real.c $(GTK_LIBS); \
	fi

    # ---- Qt via CMake ----
    qt:
    	cmake -S qt -B build-qt -G Ninja || cmake -S qt -B build-qt
    	cmake --build build-qt

    clean:
    	rm -rf build build-qt

    dist:
    	mkdir -p dist
    	cp -r assets dist/
    	cp packaging/linux/vitte-studio.desktop dist/
    	cp build/desktop-gtk dist/ || true
    	cp build-qt/desktop-qt dist/ || true
    	echo 'OK → dist/'
