\
    #include <gtk/gtk.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>

    typedef struct {
        GtkWidget *window;
        GtkWidget *text;
        GtkWidget *entry;
        GtkWidget *run_btn;
        GtkWidget *open_btn;
        gchar *current_path;
    } App;

    static void append_text(GtkWidget *text, const char *s) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buf, &end);
        gtk_text_buffer_insert(buf, &end, s, -1);
        gtk_text_buffer_insert(buf, &end, "\n", 1);
    }

    static void run_file(App *app, const char *path) {
        if (!path || !g_file_test(path, G_FILE_TEST_EXISTS)) {
            append_text(app->text, "⚠ Aucun fichier à exécuter.");
            return;
        }
        gchar *cmd = g_strdup_printf("vitte run \"%s\" 2>&1", path);
        GError *err = NULL;
        gchar *out = NULL;
        gsize len = 0;

        gint64 t0 = g_get_monotonic_time();
        gboolean ok = g_spawn_command_line_sync(cmd, &out, NULL, NULL, &err);
        gint64 t1 = g_get_monotonic_time();
        double ms = (t1 - t0) / 1000.0;

        if (!ok) {
            append_text(app->text, err ? err->message : "Erreur inconnue");
            if (err) g_error_free(err);
        } else {
            append_text(app->text, out ? out : "(no output)");
            gchar *msg = g_strdup_printf("✔ Terminé en %.2f ms", ms);
            append_text(app->text, msg);
            g_free(msg);
        }
        g_free(out);
        g_free(cmd);
    }

    static void on_run(GtkButton *btn, gpointer user_data) {
        (void)btn;
        App *app = (App*)user_data;
        const char *p = gtk_editable_get_text(GTK_EDITABLE(app->entry));
        run_file(app, p);
    }

    static void on_open(GtkButton *btn, gpointer user_data) {
        (void)btn;
        App *app = (App*)user_data;
        GtkFileDialog *dlg = gtk_file_dialog_new();
    #if GTK_CHECK_VERSION(4,0,0)
        gtk_file_dialog_open(dlg, GTK_WINDOW(app->window), NULL, NULL, NULL, NULL);
        g_signal_connect(dlg, "open-response", G_CALLBACK(+[](
            GtkFileDialog *d, GAsyncResult *res, gpointer data){
            App *app = (App*)data;
            GFile *file = gtk_file_dialog_open_finish(d, res, NULL);
            if (file) {
                char *path = g_file_get_path(file);
                gtk_editable_set_text(GTK_EDITABLE(app->entry), path);
                g_object_unref(file);
                g_free(path);
            }
        }), app);
    #else
        GtkWidget *chooser = gtk_file_chooser_dialog_new("Open file",
            GTK_WINDOW(app->window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT, NULL);
        if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
            gtk_entry_set_text(GTK_ENTRY(app->entry), filename);
            g_free(filename);
        }
        gtk_widget_destroy(chooser);
    #endif
    }

    int main(int argc, char **argv) {
        gtk_init();
        App app = {0};

    #if GTK_CHECK_VERSION(4,0,0)
        app.window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(app.window), "Vitte Desktop (GTK)");
        gtk_window_set_default_size(GTK_WINDOW(app.window), 900, 600);

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_window_set_child(GTK_WINDOW(app.window), box);

        GtkWidget *h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        app.entry = gtk_entry_new();
        app.run_btn = gtk_button_new_with_label("Run");
        app.open_btn = gtk_button_new_with_label("Open");

        gtk_box_append(GTK_BOX(h), app.open_btn);
        gtk_box_append(GTK_BOX(h), app.entry);
        gtk_box_append(GTK_BOX(h), app.run_btn);

        app.text = gtk_text_view_new();
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.text), TRUE);
        GtkWidget *scroll = gtk_scrolled_window_new();
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), app.text);

        gtk_box_append(GTK_BOX(box), h);
        gtk_box_append(GTK_BOX(box), scroll);
    #else
        app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(app.window), "Vitte Desktop (GTK)");
        gtk_window_set_default_size(GTK_WINDOW(app.window), 900, 600);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_add(GTK_CONTAINER(app.window), vbox);

        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        app.open_btn = gtk_button_new_with_label("Open");
        app.entry = gtk_entry_new();
        app.run_btn = gtk_button_new_with_label("Run");

        gtk_box_pack_start(GTK_BOX(hbox), app.open_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), app.entry, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), app.run_btn, FALSE, FALSE, 0);

        app.text = gtk_text_view_new();
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.text), TRUE);
        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), app.text);
        gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

        g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    #endif

        g_signal_connect(app.run_btn, "clicked", G_CALLBACK(on_run), &app);
        g_signal_connect(app.open_btn, "clicked", G_CALLBACK(on_open), &app);

        gtk_widget_show(app.window);
    #if GTK_CHECK_VERSION(4,0,0)
        // GTK4 uses the default main loop started by showing the window
        while (g_list_model_get_n_items(G_LIST_MODEL(gtk_window_get_toplevels())) > 0)
            g_main_context_iteration(NULL, TRUE);
    #else
        gtk_main();
    #endif
        return 0;
    }
